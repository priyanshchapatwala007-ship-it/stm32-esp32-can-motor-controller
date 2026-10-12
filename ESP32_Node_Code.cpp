/* 
 * ESP32_Node_Code.cpp
 * ESP32 Gateway Node (CAN to MQTT)
 * Recommended Framework: Arduino
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "driver/twai.h" // Two-Wire Automotive Interface (ESP32 CAN driver)

// --- WiFi & MQTT Configuration ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "broker.hivemq.com"; // Free public broker for testing

WiFiClient espClient;
PubSubClient client(espClient);

// --- CAN (TWAI) Configuration ---
#define RX_PIN 4
#define TX_PIN 5
#define CAN_ID_SETPOINT  0x100
#define CAN_ID_TELEMETRY 0x101
#define CAN_ID_HEARTBEAT 0x102

unsigned long last_heartbeat = 0;

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
}

// --- MQTT Callback ---
// Fires when a new message arrives from the dashboard
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    if (strcmp(topic, "robot/motor/setpoint") == 0) {
        String msg = "";
        for (int i = 0; i < length; i++) {
            msg += (char)payload[i];
        }
        float target_rpm = msg.toFloat();
        
        Serial.print("Received MQTT Setpoint: ");
        Serial.println(target_rpm);
        
        // Forward setpoint via CAN to STM32
        twai_message_t tx_msg = {.identifier = CAN_ID_SETPOINT, .extd = 0, .rtr = 0, .data_length_code = 4};
        memcpy(tx_msg.data, &target_rpm, 4);
        
        if (twai_transmit(&tx_msg, pdMS_TO_TICKS(10)) == ESP_OK) {
            Serial.println("Sent Setpoint to CAN Bus");
        } else {
            Serial.println("Failed to send CAN message");
        }
    }
}

void reconnect() {
    // Loop until we're reconnected to MQTT
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP32Gateway-";
        clientId += String(random(0xffff), HEX);
        
        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
            // Subscribe to the setpoint topic
            client.subscribe("robot/motor/setpoint");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void setup_twai() {
    // Initialize configuration structures using macro initializers
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)TX_PIN, (gpio_num_t)RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); // Match STM32 500kbps
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        Serial.println("TWAI Driver installed");
    } else {
        Serial.println("Failed to install TWAI driver");
        return;
    }
    
    // Start TWAI driver
    if (twai_start() == ESP_OK) {
        Serial.println("TWAI Driver started");
    } else {
        Serial.println("Failed to start TWAI driver");
    }
}

void setup() {
    Serial.begin(115200);
    
    setup_wifi();
    
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqtt_callback);
    
    setup_twai();
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop(); // Handle MQTT keep-alives and incoming messages

    // --- 1. Read incoming CAN Telemetry from STM32 ---
    twai_message_t rx_msg;
    // Check if message is in RX queue (non-blocking)
    if (twai_receive(&rx_msg, 0) == ESP_OK) {
        if (rx_msg.identifier == CAN_ID_TELEMETRY) {
            float current_rpm = 0, target_rpm = 0;
            // Unpack 8 bytes into two floats
            memcpy(&current_rpm, rx_msg.data, 4);
            memcpy(&target_rpm, rx_msg.data + 4, 4);
            
            // Publish Telemetry to MQTT Dashboard
            char payload[128];
            snprintf(payload, sizeof(payload), "{\"rpm\": %.2f, \"target\": %.2f}", current_rpm, target_rpm);
            client.publish("robot/motor/telemetry", payload);
            
            Serial.print("Telemetry Sent: ");
            Serial.println(payload);
        }
    }

    // --- 2. Send CAN Heartbeat (1Hz) ---
    // Tells the STM32 that the gateway is alive and MQTT is connected
    if (millis() - last_heartbeat > 1000) {
        last_heartbeat = millis();
        
        twai_message_t tx_msg = {.identifier = CAN_ID_HEARTBEAT, .extd = 0, .rtr = 0, .data_length_code = 1};
        tx_msg.data[0] = 0xAA; // dummy heartbeat byte
        
        twai_transmit(&tx_msg, pdMS_TO_TICKS(10));
    }
}

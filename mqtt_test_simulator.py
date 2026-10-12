"""
mqtt_test_simulator.py
Simulates the ESP32 publishing motor telemetry over MQTT.
Use this to test & develop the dashboard WITHOUT any hardware connected.

Usage:
    pip install paho-mqtt
    python mqtt_test_simulator.py
"""

import paho.mqtt.client as mqtt
import json
import time
import math
import random

BROKER = "broker.hivemq.com"
PORT = 1883
TOPIC_TELEMETRY = "robot/motor/telemetry"
TOPIC_SETPOINT  = "robot/motor/setpoint"

target_rpm = 0.0
current_rpm = 0.0

def on_connect(client, userdata, flags, rc):
    print(f"[MQTT] Connected (rc={rc})")
    client.subscribe(TOPIC_SETPOINT)
    print(f"[MQTT] Subscribed to {TOPIC_SETPOINT}")

def on_message(client, userdata, msg):
    global target_rpm
    try:
        target_rpm = float(msg.payload.decode())
        print(f"[RX] New setpoint: {target_rpm} RPM")
    except ValueError:
        print(f"[RX] Bad payload: {msg.payload}")

def main():
    global current_rpm, target_rpm

    client = mqtt.Client(client_id="simulator_" + str(random.randint(1000, 9999)))
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"Connecting to {BROKER}:{PORT}...")
    client.connect(BROKER, PORT, 60)
    client.loop_start()

    print("Simulator running. Open dashboard.html in a browser.")
    print("Use the slider on the dashboard to send setpoints.\n")

    t = 0
    while True:
        # Simulate PID response: current_rpm slowly chases target_rpm
        error = target_rpm - current_rpm
        current_rpm += error * 0.08  # simple first-order response
        # Add realistic noise
        noise = random.gauss(0, 0.5)
        simulated_rpm = current_rpm + noise

        payload = json.dumps({
            "rpm": round(simulated_rpm, 2),
            "target": round(target_rpm, 2)
        })

        client.publish(TOPIC_TELEMETRY, payload)
        print(f"[TX] {payload}")

        time.sleep(0.1)  # 10 Hz, matches ESP32 firmware
        t += 0.1

if __name__ == "__main__":
    main()

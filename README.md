# Two-Node CAN Motor Controller (STM32 + ESP32)

A miniature robot-fleet motor control system demonstrating CAN bus communication, closed-loop PID speed control with FreeRTOS, and IoT telemetry via MQTT.

## 📁 Project Structure

```
stm-project/
├── STM32_Node_Code.c        # STM32 firmware (FreeRTOS + PID + CAN)
├── ESP32_Node_Code.cpp       # ESP32 gateway (CAN ↔ MQTT bridge)
├── dashboard.html            # Live web dashboard (open in browser)
├── nodered_flow.json         # Alternative Node-RED dashboard flow
├── mqtt_test_simulator.py    # Python simulator for testing without hardware
└── README.md                 # This file
```

## 🏗️ System Architecture

```
┌──────────────────────┐       CAN Bus (500 kbps)       ┌──────────────────────┐
│     STM32 Node       │◄──────────────────────────────►│     ESP32 Node       │
│                      │  ID 0x100: Speed Setpoint       │                      │
│  • FreeRTOS (3 Tasks)│  ID 0x101: Telemetry (RPM)     │  • TWAI (CAN) Driver │
│  • PID Speed Control │  ID 0x102: Heartbeat            │  • WiFi + MQTT       │
│  • Encoder Reading   │                                 │                      │
│  • PWM Motor Driver  │       SN65HVD230 Transceivers   │                      │
│  • Watchdog (IWDG)   │       120Ω termination each end │                      │
└──────────────────────┘                                 └──────────┬───────────┘
                                                                    │ WiFi
                                                                    ▼
                                                         ┌──────────────────────┐
                                                         │    MQTT Broker       │
                                                         │  (HiveMQ / Mosquitto)│
                                                         └──────────┬───────────┘
                                                                    │ WebSocket
                                                                    ▼
                                                         ┌──────────────────────┐
                                                         │   Web Dashboard      │
                                                         │  (dashboard.html)    │
                                                         │  • Live RPM Gauge    │
                                                         │  • Realtime Chart    │
                                                         │  • Speed Controls    │
                                                         └──────────────────────┘
```

## 📡 CAN Message Map

| CAN ID  | Name       | Direction       | DLC | Payload                          | Rate  |
|---------|------------|-----------------|-----|----------------------------------|-------|
| `0x100` | Setpoint   | ESP32 → STM32   | 4   | `[float target_rpm]`             | On change |
| `0x101` | Telemetry  | STM32 → ESP32   | 8   | `[float current_rpm, float target_rpm]` | 10 Hz |
| `0x102` | Heartbeat  | ESP32 → STM32   | 1   | `[0xAA]`                         | 1 Hz  |

## 🚀 Quick Start (No Hardware Needed)

Test the full dashboard using the Python simulator:

```bash
# 1. Install MQTT library
pip install paho-mqtt

# 2. Start the simulator (simulates motor + ESP32)
python mqtt_test_simulator.py

# 3. Open the dashboard in your browser
#    Just double-click dashboard.html

# 4. Use the slider to send speed commands and watch live telemetry
```

## 🔧 Hardware Setup

### Parts List
- STM32 Nucleo-F446RE (or G431RB / Blue Pill)
- ESP32 DevKit v1
- 2× SN65HVD230 CAN transceiver modules
- 2× 120Ω resistors (bus termination)
- N20 DC gear motor with encoder
- TB6612 or DRV8833 motor driver
- 7-12V power supply
- Breadboard + jumper wires

### Wiring Summary

**STM32 → Motor Driver (TB6612):**
| STM32 Pin | Function    | Connects To   |
|-----------|-------------|---------------|
| PA6       | TIM3_CH1 (PWM) | TB6612 AIN1 |
| PA7       | TIM3_CH2 (PWM) | TB6612 AIN2 |
| PA5       | GPIO (LED)  | Status LED    |

**STM32 → Encoder:**
| STM32 Pin | Function        | Connects To     |
|-----------|-----------------|-----------------|
| PA0       | TIM2_CH1 (Enc A)| Encoder Ch A    |
| PA1       | TIM2_CH2 (Enc B)| Encoder Ch B    |

**STM32 → CAN Transceiver:**
| STM32 Pin | Function | Connects To       |
|-----------|----------|-------------------|
| PA11      | CAN1_RX  | SN65HVD230 RXD   |
| PA12      | CAN1_TX  | SN65HVD230 TXD   |

**ESP32 → CAN Transceiver:**
| ESP32 Pin | Function | Connects To       |
|-----------|----------|-------------------|
| GPIO 4    | TWAI_RX  | SN65HVD230 RXD   |
| GPIO 5    | TWAI_TX  | SN65HVD230 TXD   |

> ⚠️ Both CAN transceivers' CANH and CANL pins connect to the shared bus. Place a 120Ω resistor between CANH and CANL at **each** end.

## 🧠 Key Concepts Demonstrated

- **FreeRTOS** — 3 tasks with priority-based preemptive scheduling
- **PID Control** — Closed-loop speed control with anti-windup
- **CAN Bus** — Multi-node communication with defined message protocol
- **MQTT / IoT** — Cloud-connected telemetry and remote control
- **Watchdog Timer** — Hardware fault recovery (IWDG)
- **Failsafe Logic** — Motor stops if heartbeat is lost
- **DMA-ready UART** — Debug logging without CPU overhead

## 📊 Dashboard Features

The `dashboard.html` file is a fully self-contained web page:
- **RPM Gauge** — Animated circular gauge with color zones
- **Live Chart** — Scrolling plot of Actual vs Target RPM
- **Speed Slider** — Send setpoints directly to the motor
- **Emergency Stop** — One-click motor kill switch
- **Reverse** — Send negative RPM for reverse rotation
- **Event Log** — Timestamped MQTT message history
- **Connection Status** — Visual indicators for MQTT and CAN health

## 📝 License

MIT

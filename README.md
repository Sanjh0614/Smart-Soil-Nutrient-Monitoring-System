# 🌱 Soil Nutrient & Health Monitoring System

An end-to-end IoT-based precision agriculture system that monitors real-time soil nutrient levels (Nitrogen, Phosphorus, Potassium - NPK), moisture, and pH using an ESP32 microcontroller, RS485 Modbus communication, and a cloud-connected telemetry backend.

---

## 📌 Table of Contents
- [Overview](#-overview)
- [Key Features](#-key-features)
- [System Architecture](#-system-architecture)
- [Hardware Components & Pinout](#-hardware-components--pinout)
- [Software Stack](#-software-stack)
- [Getting Started](#-getting-started)
- [API & Data Logging](#-api--data-logging)
- [Future Scope](#-future-scope)
- [License](#-license)

---

## 🚀 Overview
Traditional soil testing is labor-intensive and slow. This project automates soil health telemetry by capturing critical macronutrient (NPK) concentrations and soil moisture levels in situ, transmitting live data via Wi-Fi to a backend for analytical processing and visualization.

---

## ✨ Key Features
- **Real-Time NPK Telemetry:** Queries soil Nitrogen (N), Phosphorus (P), and Potassium (K) levels via Modbus RTU over RS485.
- **Environmental Context:** Simultaneous measurement of volumetric soil moisture and pH levels.
- **Wi-Fi Connectivity:** ESP32-based automated telemetry dispatch via HTTP/MQTT.
- **Fault-Tolerant Retries:** Automatic Wi-Fi reconnection handling and sensor query timeouts.
- **Backend Analytics Ready:** Structured JSON data payload suitable for database storage, time-series plotting, and fertilizer recommendation algorithms.

---

## 🏗️ System Architecture

```text
+-----------------------+
|  Soil Sensors         |
|  - NPK (Modbus RTU)   | ---> [ MAX485 TTL ] ---> [ ESP32 Microcontroller ]
|  - Capacitive Moisture|                              | (Wi-Fi / MQTT)
+-----------------------+                              v
                                            +---------------------+
                                            |  Cloud / Backend    |
                                            |  - Ingestion API    |
                                            |  - TimeSeries DB    |
                                            |  - Dashboard / UI   |
                                            +---------------------+
```

---

## 🔌 Hardware Components & Pinout

### Bill of Materials
| Component | Specification / Model | Purpose |
|---|---|---|
| **Microcontroller** | ESP32 NodeMCU | Data acquisition & wireless communication |
| **Nutrient Sensor** | Soil NPK Sensor (RS485 Modbus RTU) | Nitrogen, Phosphorus, Potassium measurement |
| **Transceiver** | MAX485 TTL-to-RS485 Module | Modbus RTU interface |
| **Moisture Sensor** | Capacitive Soil Moisture Sensor v1.2 | Volumetric water content |
| **Power Supply** | 9V–12V DC Adapter / Step-down converter | Powering NPK sensor & ESP32 |

### Wiring Pinout Table
| MAX485 Pin | ESP32 Pin | Description |
|---|---|---|
| `VCC` | `5V` (VIN) | 5V Power |
| `GND` | `GND` | Ground |
| `RO` | `GPIO 16` (RX2) | Receiver Out -> Serial RX |
| `DI` | `GPIO 17` (TX2) | Driver In -> Serial TX |
| `DE` & `RE` | `GPIO 4` | Flow Control (Transmit/Receive Enable) |

---

## 💻 Software Stack

- **Firmware:** C++ / Arduino Framework (ESP32 core, `SoftwareSerial` / `HardwareSerial`)
- **Communication Protocol:** Modbus RTU over RS485, MQTT / HTTP POST
- **Backend / Ingestion:** Python (FastAPI / Flask) or Node.js
- **Database / Analytics:** PostgreSQL / InfluxDB, Pandas, Matplotlib

---

## 🛠️ Getting Started

### 1. Prerequisites
- [Arduino IDE](https://www.arduino.cc/en/software) or [PlatformIO for VS Code](https://platformio.org/)
- Required Arduino Libraries:
  - `WiFi.h` (Built into ESP32 board package)
  - `HTTPClient.h` / `PubSubClient.h`

### 2. Firmware Configuration
1. Clone this repository:
   ```bash
   git clone [https://github.com/MeetBhattad/Soil-Nutrient-Monitoring-System.git](https://github.com/MeetBhattad/Soil-Nutrient-Monitoring-System.git)
   cd Soil-Nutrient-Monitoring-System
   ```
2. Navigate to `firmware/include/` and duplicate `config.h.example` to `config.h`.
3. Set your Wi-Fi credentials and API endpoint:
   ```cpp
   #define WIFI_SSID "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   #define SERVER_ENDPOINT "http://your-server-ip:8000/api/telemetry"
   ```
4. Flash the code to your ESP32 board.

---

## 📡 API & Data Logging Sample

The ESP32 pushes payloads in the following JSON schema:

```json
{
  "device_id": "esp32-node-01",
  "nitrogen_mg_kg": 45,
  "phosphorus_mg_kg": 22,
  "potassium_mg_kg": 110,
  "soil_moisture_pct": 38.5,
  "timestamp": 1723984800
}
```

---

## 🔮 Future Scope
- [ ] Integration with solar-powered charging and deep sleep optimization.
- [ ] Automated crop-specific fertilizer recommendation engine based on NPK deficiencies.
- [ ] LoRaWAN support for long-range agricultural telemetry without Wi-Fi dependence.

---

## 📄 License
Distributed under the MIT License. See `LICENSE` for details.

# EquipGuard — Edge-Based Anomaly Detection for Industrial Machine Health Monitoring

A fully edge-deployed industrial machine health monitoring system 
distributed across three microcontroller nodes. Uses Z-score unsupervised 
anomaly detection and threshold-based alerting across 5 sensor channels 
with zero cloud dependency. All inference runs on-device in real time.

## System Name
**EquipGuard-ML**

## Node Architecture

| Node | Hardware | Sensors | Role |
|---|---|---|---|
| **C6 (Pico)** | ESP32-C6 | BMP180 (temp + pressure), Vibration, Current, Voltage | Z-score anomaly detection + WiFi AP + Web Dashboard |
| **Nano** | ESP32 (Nano) | Air Quality (AQI) | Threshold-based alerting + Buzzer |
| **DevKit** | ESP32 DevKit | IR Sensor | Object detection + sends data to C6 |

## Communication Architecture

[ESP32-C6 / Pico] ← acts as WiFi Access Point ("EquipGuard-ML")
↑ ↑
HTTP POST HTTP POST
| |
[ESP32 Nano] [ESP32 DevKit]
(AQI + alerts) (IR sensor data)

C6 also serves:
GET /sensors → JSON of all sensor data (polled by Nano)
GET / → Live Web Dashboard (browser)


## Sensors (5 Channels)
| Sensor | Node | Type |
|---|---|---|
| Temperature | C6 (BMP180) | I2C |
| Pressure | C6 (BMP180) | I2C |
| Vibration | C6 | Digital |
| Current | C6 | Analog (ADC) |
| Voltage | C6 | Analog (ADC) |
| Air Quality (AQI) | Nano | Analog (ADC) |
| IR (Object Detection) | DevKit | Digital |

## Detection Methods

### Z-Score (C6 / Pico)
Runs on temperature, pressure, current, and voltage. Pre-trained mean 
and standard deviation per channel are stored on-device. Each new reading 
is scored.

### Threshold-Based (Nano)
Air Quality alert fires when AQI raw ADC reading crosses the threshold.  
IR alert from DevKit is pulled by Nano via C6's `/sensors` endpoint.  
Combined alert = `machineAlert OR aqiAlert OR irAlert` → buzzer ON.

## Web Dashboard
C6 hosts a live web dashboard accessible over WiFi at `192.168.4.1`.  
Auto-refreshes every 2 seconds showing:
- Temperature, Pressure, Vibration, Current, Voltage (from C6)
- Air Quality (from Nano)
- IR status (from DevKit)

## Performance
| Metric | Value |
|---|---|
| Accuracy | 96.89% |
| Inference Latency | 37.4 ms |
| Flash Usage | 1.06 KB |
| Cloud Dependency | None |
| Dashboard Refresh | Every 2 seconds |

## Key Features
- Fully edge-based — no cloud, no external server
- C6 acts as WiFi AP — entire system is self-contained
- Dual detection: Z-score (statistical) + threshold (rule-based)
- Live web dashboard served directly from microcontroller
- Buzzer actuator for real-time physical alerts
- 3-node distributed architecture with HTTP-based inter-node communication

## Tech Stack
- **Languages:** C++ (Arduino framework)
- **Communication:** HTTP over WiFi (C6 as AP)
- **Libraries:** WiFi, WebServer, ArduinoJson, Adafruit BMP085, Wire
- **Sensors:** BMP180, vibration switch, ACS current sensor, voltage divider, MQ air quality, IR sensor

# Edge-Based Anomaly Detection for Industrial Machine Health Monitoring

A fully edge-deployed industrial anomaly detection system running on 
dual ESP32 nodes. Uses unsupervised Z-score statistical detection across 
5 sensor channels with zero cloud dependency — all inference happens 
on-device in real time.

## Tech Stack
- **Hardware:** ESP32 (dual node setup)
- **Detection Method:** Z-score unsupervised anomaly detection
- **Sensor Channels:** 5 (vibration, temperature, current, etc.)
- **Communication:** ESP-NOW / Serial between nodes
- **Language:** C++ (Arduino framework)

## Key Features
- Fully edge-based — no cloud, no Wi-Fi required for inference
- Z-score based unsupervised detection (no labeled training data needed)
- Dual ESP32 node architecture
- Real-time inference with 37.4ms latency
- Extremely lightweight — only 1.06 KB flash usage
- 5-channel sensor fusion

## Performance
| Metric | Value |
|---|---|
| Accuracy | 96.89% |
| Inference Latency | 37.4 ms |
| Flash Usage | 1.06 KB |
| Cloud Dependency | None |

## How It Works
Each sensor channel continuously streams readings to the ESP32. A 
rolling window of values is maintained per channel, from which the mean 
and standard deviation are computed. Any new reading that deviates beyond 
a set Z-score threshold (typically 2–3σ) is flagged as an anomaly. The 
dual-node setup allows one node to handle sensing while the other 
handles detection and alerting, reducing processing bottlenecks.

## Why Edge?
Running detection on-device eliminates latency from cloud round-trips, 
works in network-denied industrial environments, and drastically reduces 
data transmission costs — making it practical for factory floor deployment.

# Architecture

```
 ┌────────────┐   PIR / vibration    ┌───────────────────┐
 │  Sensors   │ ───────────────────▶ │   ESP32 firmware   │
 │ (PIR, SW-  │                      │  - debounce        │
 │  420 vib.) │                      │  - confirm-count   │
 └────────────┘                      │  - MQTT publish    │
                                      └─────────┬──────────┘
                                                 │ MQTT (TLS)
                                                 ▼
                                      ┌───────────────────┐
                                      │  MQTT Broker       │
                                      │ (HiveMQ Cloud /    │
                                      │  Mosquitto)        │
                                      └─────────┬──────────┘
                                                 │
                                                 ▼
                                      ┌───────────────────┐
                                      │  Node.js backend   │
                                      │  - subscribes      │
                                      │  - writes Firestore│
                                      │  - sends FCM push  │
                                      └───┬───────────┬────┘
                                          │           │
                                          ▼           ▼
                              ┌───────────────┐ ┌──────────────┐
                              │  Firestore    │ │  Phone (FCM) │
                              │  (event log)  │ │  push alert  │
                              └───────┬───────┘ └──────────────┘
                                      │
                                      ▼
                              ┌───────────────┐
                              │  Web dashboard│
                              │  (live view)  │
                              └───────────────┘
```

## Why filtering happens on the device, not the backend

Every raw sensor read that leaves the ESP32 costs radio power and adds a
potential false-positive alert. Pushing the debounce + confirmation-count
logic onto the microcontroller (`firmware/include/sensor_filter.h`) means
only *confirmed* events ever hit the network — this is what gets the
false-positive rate down by roughly 40% versus publishing on every raw
HIGH read, and it keeps the MQTT link quiet enough that a real event isn't
competing with noise for bandwidth or attention.

## Latency budget

- Sensor trip → edge filter confirms: ~100-400ms (a few loop iterations)
- MQTT publish → backend receipt: ~20-100ms on a home network
- Backend → Firestore write + FCM dispatch: ~150-300ms
- FCM → phone notification: typically under 1s

Total sensor-to-phone latency is designed to stay sub-second under normal
network conditions, per the FCM `high` priority / APNs `apns-priority: 10`
settings in `backend/src/firebase.js`.

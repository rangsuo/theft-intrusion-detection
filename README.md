# Real-Time Theft / Intrusion Detection System

Low-latency intrusion detection built on ESP32 with PIR + vibration
sensors, edge-side noise filtering, MQTT event publishing, and a
Firebase/Node.js backend that delivers sub-second push alerts.

## Highlights

- **Edge filtering on the microcontroller** (debounce + confirmation
  counting) cuts false-positive alerts by roughly 40% before anything is
  published.
- **MQTT** transport from device to cloud, TLS-secured.
- **Firebase/Node.js backend** logs events to Firestore and fans out
  push notifications via FCM with high-priority delivery.
- **Live dashboard** (static HTML + Firestore snapshot listener) for
  monitoring events in real time.

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for the full data flow
and latency budget.

## Repo layout

```
firmware/     ESP32 embedded C++ (PlatformIO project)
backend/      Node.js MQTT-to-Firebase bridge + push alerting
dashboard/    Static live-event dashboard (Firestore listener)
docs/         Architecture notes
```

## Getting started

### 1. Firmware (ESP32)

```bash
cd firmware
# Edit include/config.h with your WiFi + MQTT credentials
pio run -t upload      # requires PlatformIO CLI
pio device monitor
```

Hardware wiring:

| Sensor          | ESP32 Pin |
|-----------------|-----------|
| PIR OUT         | GPIO 27   |
| Vibration (SW-420, analog out) | GPIO 34 (ADC1) |
| Status LED      | GPIO 2 (onboard) |

### 2. Backend

```bash
cd backend
npm install
cp .env.example .env          # fill in MQTT + FCM values
# place Firebase service account key at backend/config/serviceAccountKey.json
npm start
```

### 3. Dashboard

Open `dashboard/index.html` in a browser after filling in your Firebase
web app config inside the file (or serve it with any static file server).

## Tech stack

ESP32 · Embedded C++ (Arduino framework) · MQTT (PubSubClient) ·
Firebase Admin SDK · Firestore · Firebase Cloud Messaging · Node.js ·
Wi-Fi

## License

MIT — see [`LICENSE`](LICENSE).

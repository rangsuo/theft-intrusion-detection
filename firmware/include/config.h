/**
 * config.h
 *
 * Copy this file's values into a local config.h (gitignored) or override
 * via platformio.ini build_flags. Never commit real credentials.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ---------- WiFi ----------
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// ---------- MQTT ----------
#define MQTT_BROKER              "your-broker.hivemq.cloud"
#define MQTT_PORT                8883
#define MQTT_USERNAME            "mqtt_user"
#define MQTT_PASSWORD            "mqtt_pass"
#define MQTT_CLIENT_ID_PREFIX    "esp32-intrusion-"
#define MQTT_TOPIC_EVENTS        "home/security/events"
#define MQTT_TOPIC_HEARTBEAT     "home/security/heartbeat"

// ---------- Device ----------
#define DEVICE_ID   "node-01-front-door"

// ---------- Sensor tuning ----------
// PIR: physically can't re-trigger faster than ~2s; require 3 consecutive
// confirmed reads at the loop interval before declaring motion.
#define PIR_DEBOUNCE_MS         2000
#define PIR_CONFIRM_COUNT       3

// Vibration: raw ADC threshold (0-4095) above which a read counts as "hit";
// require 4 consecutive hits to reject single-sample spikes.
#define VIBRATION_THRESHOLD     1800
#define VIBRATION_DEBOUNCE_MS   150
#define VIBRATION_CONFIRM_COUNT 4

#define LOOP_INTERVAL_MS        100

#endif // CONFIG_H

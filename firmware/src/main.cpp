/**
 * Real-Time Theft/Intrusion Detection System
 * ESP32 firmware — PIR + vibration sensing, edge-side noise filtering,
 * MQTT event publishing.
 *
 * Board: ESP32 DevKit (any variant with 2+ GPIO inputs)
 * Sensors:
 *   - PIR motion sensor (digital HIGH/LOW)
 *   - SW-420 / analog vibration sensor
 *
 * Build: PlatformIO (see platformio.ini) or Arduino IDE (rename to .ino)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "sensor_filter.h"

// ---------- Pin configuration ----------
static const uint8_t PIR_PIN = 27;
static const uint8_t VIBRATION_PIN = 34;   // ADC1 channel
static const uint8_t STATUS_LED_PIN = 2;

// ---------- Globals ----------
WiFiClient espClient;
PubSubClient mqttClient(espClient);

EdgeFilter pirFilter(PIR_DEBOUNCE_MS, PIR_CONFIRM_COUNT);
EdgeFilter vibrationFilter(VIBRATION_DEBOUNCE_MS, VIBRATION_CONFIRM_COUNT);

unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL_MS = 30000;

// ---------- Forward declarations ----------
void connectWiFi();
void connectMQTT();
void publishEvent(const char *eventType, int rawValue, int confidence);
void publishHeartbeat();
int readVibration();

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  analogReadResolution(12); // 0-4095

  connectWiFi();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

  Serial.println("[BOOT] Intrusion detection node online");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();

  // --- PIR motion path ---
  bool pirRaw = digitalRead(PIR_PIN) == HIGH;
  FilterResult pirResult = pirFilter.update(pirRaw);
  if (pirResult.triggered) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    publishEvent("motion", 1, pirResult.confidence);
    Serial.printf("[EVENT] motion confirmed, confidence=%d%%\n", pirResult.confidence);
  } else {
    digitalWrite(STATUS_LED_PIN, LOW);
  }

  // --- Vibration path ---
  int vibrationRaw = readVibration();
  bool vibrationAboveThreshold = vibrationRaw > VIBRATION_THRESHOLD;
  FilterResult vibResult = vibrationFilter.update(vibrationAboveThreshold);
  if (vibResult.triggered) {
    publishEvent("vibration", vibrationRaw, vibResult.confidence);
    Serial.printf("[EVENT] vibration confirmed, raw=%d, confidence=%d%%\n",
                  vibrationRaw, vibResult.confidence);
  }

  // --- Heartbeat so the backend knows the node is alive ---
  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL_MS) {
    publishHeartbeat();
    lastHeartbeat = millis();
  }

  delay(LOOP_INTERVAL_MS);
}

int readVibration() {
  // Simple oversampling to reduce ADC jitter before it ever reaches the filter
  const int SAMPLES = 4;
  long sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(VIBRATION_PIN);
    delayMicroseconds(200);
  }
  return sum / SAMPLES;
}

void connectWiFi() {
  Serial.printf("[WIFI] Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(400);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WIFI] Connected, IP=%s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WIFI] Failed to connect, will retry in loop()");
  }
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Connecting...");
    String clientId = String(MQTT_CLIENT_ID_PREFIX) + String((uint32_t)ESP.getEfuseMac(), HEX);

    if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
    } else {
      Serial.printf("failed, rc=%d, retrying in 3s\n", mqttClient.state());
      delay(3000);
    }
  }
}

void publishEvent(const char *eventType, int rawValue, int confidence) {
  char payload[192];
  snprintf(payload, sizeof(payload),
           "{\"device\":\"%s\",\"event\":\"%s\",\"raw\":%d,\"confidence\":%d,\"ts\":%lu}",
           DEVICE_ID, eventType, rawValue, confidence, millis());
  mqttClient.publish(MQTT_TOPIC_EVENTS, payload);
}

void publishHeartbeat() {
  char payload[96];
  snprintf(payload, sizeof(payload),
           "{\"device\":\"%s\",\"status\":\"alive\",\"rssi\":%d,\"uptime_ms\":%lu}",
           DEVICE_ID, WiFi.RSSI(), millis());
  mqttClient.publish(MQTT_TOPIC_HEARTBEAT, payload);
}

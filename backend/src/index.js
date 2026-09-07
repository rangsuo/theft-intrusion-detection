/**
 * Intrusion Detection Backend
 *
 * Subscribes to MQTT events published by the ESP32 node, persists them to
 * Firestore for the dashboard, and pushes an FCM notification to the
 * registered device(s) — targeting sub-second delivery from sensor trip
 * to phone alert.
 */

import 'dotenv/config';
import mqtt from 'mqtt';
import { initFirebase, logEvent, logHeartbeat, sendPushAlert } from './firebase.js';
import { config } from './config.js';

initFirebase();

const client = mqtt.connect(config.mqttUrl, {
  username: config.mqttUsername,
  password: config.mqttPassword,
  clientId: `backend-${Math.random().toString(16).slice(2, 8)}`,
  reconnectPeriod: 2000,
});

client.on('connect', () => {
  console.log('[MQTT] connected to broker');
  client.subscribe([config.topics.events, config.topics.heartbeat], (err) => {
    if (err) console.error('[MQTT] subscribe error', err);
    else console.log('[MQTT] subscribed to', config.topics.events, config.topics.heartbeat);
  });
});

client.on('reconnect', () => console.log('[MQTT] reconnecting...'));
client.on('error', (err) => console.error('[MQTT] error', err));

client.on('message', async (topic, payloadBuf) => {
  const receivedAt = Date.now();
  let payload;
  try {
    payload = JSON.parse(payloadBuf.toString());
  } catch (err) {
    console.error('[MQTT] bad payload, ignoring:', payloadBuf.toString());
    return;
  }

  if (topic === config.topics.events) {
    await handleEvent(payload, receivedAt);
  } else if (topic === config.topics.heartbeat) {
    await handleHeartbeat(payload, receivedAt);
  }
});

async function handleEvent(payload, receivedAt) {
  const { device, event, raw, confidence, ts } = payload;
  console.log(`[EVENT] ${device} :: ${event} (confidence=${confidence}%, raw=${raw})`);

  const latencyMs = receivedAt - Number(ts || receivedAt); // approximate, device clock != server clock but useful in local networks

  const record = {
    device,
    event,
    raw,
    confidence,
    deviceTs: ts,
    receivedAt,
  };

  await logEvent(record);

  // Alert on anything above the confidence floor — avoids paging the user
  // for a low-confidence single-sample blip that slipped past the edge filter.
  if (confidence >= config.alertConfidenceThreshold) {
    await sendPushAlert({
      title: '⚠️ Possible intrusion detected',
      body: `${event.toUpperCase()} detected on ${device} (confidence ${confidence}%)`,
      data: { device, event, confidence: String(confidence) },
    });
    console.log(`[ALERT] push sent (approx end-to-end ${latencyMs}ms)`);
  }
}

async function handleHeartbeat(payload, receivedAt) {
  const { device, status, rssi, uptime_ms } = payload;
  await logHeartbeat({ device, status, rssi, uptimeMs: uptime_ms, receivedAt });
  console.log(`[HEARTBEAT] ${device} alive, rssi=${rssi}dBm, uptime=${uptime_ms}ms`);
}

process.on('SIGINT', () => {
  console.log('\n[SHUTDOWN] closing MQTT connection');
  client.end();
  process.exit(0);
});

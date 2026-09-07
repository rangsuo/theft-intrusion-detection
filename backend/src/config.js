export const config = {
  mqttUrl: process.env.MQTT_URL || 'mqtts://your-broker.hivemq.cloud:8883',
  mqttUsername: process.env.MQTT_USERNAME || 'mqtt_user',
  mqttPassword: process.env.MQTT_PASSWORD || 'mqtt_pass',

  topics: {
    events: process.env.MQTT_TOPIC_EVENTS || 'home/security/events',
    heartbeat: process.env.MQTT_TOPIC_HEARTBEAT || 'home/security/heartbeat',
  },

  // Firestore collections
  firestore: {
    eventsCollection: 'intrusion_events',
    heartbeatCollection: 'device_heartbeats',
  },

  // FCM tokens to notify — in production, load these from Firestore
  // (a `device_tokens` collection keyed by user) instead of env vars.
  fcmTokens: (process.env.FCM_TOKENS || '').split(',').filter(Boolean),

  // Minimum edge-reported confidence (0-100) required to trigger a push alert.
  alertConfidenceThreshold: Number(process.env.ALERT_CONFIDENCE_THRESHOLD || 80),
};

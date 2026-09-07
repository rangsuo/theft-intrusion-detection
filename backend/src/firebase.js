import admin from 'firebase-admin';
import { readFileSync } from 'fs';
import { config } from './config.js';

let db;

export function initFirebase() {
  if (admin.apps.length) return;

  const serviceAccountPath = process.env.FIREBASE_SERVICE_ACCOUNT_PATH
    || './config/serviceAccountKey.json';

  const serviceAccount = JSON.parse(readFileSync(serviceAccountPath, 'utf8'));

  admin.initializeApp({
    credential: admin.credential.cert(serviceAccount),
  });

  db = admin.firestore();
  console.log('[FIREBASE] initialized');
}

export async function logEvent(record) {
  await db.collection(config.firestore.eventsCollection).add({
    ...record,
    createdAt: admin.firestore.FieldValue.serverTimestamp(),
  });
}

export async function logHeartbeat(record) {
  // One doc per device, overwritten each heartbeat — keeps "last seen" cheap to query
  await db.collection(config.firestore.heartbeatCollection)
    .doc(record.device)
    .set({
      ...record,
      updatedAt: admin.firestore.FieldValue.serverTimestamp(),
    }, { merge: true });
}

export async function sendPushAlert({ title, body, data }) {
  if (!config.fcmTokens.length) {
    console.warn('[FCM] no registered tokens, skipping push');
    return;
  }

  const message = {
    notification: { title, body },
    data,
    tokens: config.fcmTokens,
    android: { priority: 'high' },
    apns: { headers: { 'apns-priority': '10' } },
  };

  try {
    const response = await admin.messaging().sendEachForMulticast(message);
    console.log(`[FCM] sent: ${response.successCount} ok, ${response.failureCount} failed`);
  } catch (err) {
    console.error('[FCM] send error', err);
  }
}

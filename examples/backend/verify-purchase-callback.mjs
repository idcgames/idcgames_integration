import { createHmac, timingSafeEqual } from 'node:crypto';

/** Verify only. The game must still implement durable idempotent grants/refunds. */
export function verifyPurchaseCallback({ rawBody, headers, paymentSecret, now = Math.floor(Date.now()/1000) }) {
  if (!Buffer.isBuffer(rawBody) || !rawBody.length || rawBody.length > 1048576 || typeof paymentSecret !== 'string' || !paymentSecret) throw new Error('invalid_callback_input');
  const get = name => typeof headers.get === 'function' ? headers.get(name) : headers[name.toLowerCase()];
  const timestamp = get('X-IDC-Timestamp');
  const signature = get('X-IDC-Signature');
  const notificationId = get('X-IDC-Notification-Id');
  if (typeof timestamp !== 'string' || !/^\d+$/.test(timestamp) || !Number.isSafeInteger(Number(timestamp)) || Math.abs(now-Number(timestamp)) > 300) throw new Error('invalid_callback_timestamp');
  if (typeof notificationId !== 'string' || !notificationId || typeof signature !== 'string' || !/^sha256=[a-f0-9]{64}$/.test(signature)) throw new Error('invalid_callback_headers');
  const expected = createHmac('sha256', paymentSecret).update(timestamp+'.').update(rawBody).digest();
  if (!timingSafeEqual(expected, Buffer.from(signature.slice(7), 'hex'))) throw new Error('invalid_callback_signature');
  let data;
  try { data = JSON.parse(rawBody.toString('utf8')); } catch { throw new Error('invalid_callback_json'); }
  if (!data || typeof data !== 'object' || Array.isArray(data) || data.action !== 'idcpayment'
    || !/^\d+$/.test(String(data.useridc)) || Number(data.useridc) <= 0
    || typeof data.transactionid !== 'string' || !data.transactionid || !Array.isArray(data.bought_data)) throw new Error('invalid_purchase_payload');
  for (const flag of ['sandbox', 'refund']) if (![0,1,'0','1',false,true].includes(data[flag])) throw new Error('invalid_purchase_flag');
  return { notificationId, transactionId: data.transactionid, userId: String(data.useridc),
    sandbox: [1,'1',true].includes(data.sandbox), refund: [1,'1',true].includes(data.refund), lines: data.bought_data };
}

if (process.argv.includes('--self-test')) {
  const assert = await import('node:assert/strict');
  const secret = 'inert-fixture-not-a-production-key';
  const body = Buffer.from(JSON.stringify({ action: 'idcpayment', useridc: '42', transactionid: 'inert-order', sandbox: '1', refund: '0', bought_data: [{ game_item_id: 'credits', number_of_items: 100, article_count: 1 }] }));
  const signed = raw => ({ 'x-idc-notification-id': 'IDCNOTGAME_inert', 'x-idc-timestamp': '1000',
    'x-idc-signature': 'sha256='+createHmac('sha256', secret).update('1000.').update(raw).digest('hex') });
  const args = { rawBody: body, headers: signed(body), paymentSecret: secret, now: 1000 };
  assert.equal(verifyPurchaseCallback(args).sandbox, true);
  assert.throws(() => verifyPurchaseCallback({ ...args, rawBody: Buffer.concat([body, Buffer.from(' ')]) }));
  assert.throws(() => verifyPurchaseCallback({ ...args, paymentSecret: 'wrong-inert-key' }));
  assert.throws(() => verifyPurchaseCallback({ ...args, now: 1400 }));
  assert.throws(() => verifyPurchaseCallback({ ...args, headers: {} }));
  const refund = Buffer.from(JSON.stringify({ action: 'idcpayment', useridc: 42, transactionid: 'inert-order', sandbox: 1, refund: 1, bought_data: [] }));
  assert.equal(verifyPurchaseCallback({ ...args, rawBody: refund, headers: signed(refund) }).refund, true);
  console.log('Payment verifier example: 6 offline cases passed; no delivery, HTTP request or real purchase.');
}

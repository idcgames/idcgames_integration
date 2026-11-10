/** Backend example. Keep the existing Launcher login; receive its credential by HTTPS. */
export async function verifyNativeSession({ token, gameId, expectedUserId, now = Math.floor(Date.now()/1000), fetchImpl = fetch }) {
  if (typeof token !== 'string' || !token || token.length > 4096 || !Number.isSafeInteger(gameId) || gameId <= 0) throw new Error('invalid_login_input');
  let response;
  try {
    response = await fetchImpl('https://auth.idcgames.com/api/launcher/get-access-data', {
      method: 'POST', redirect: 'error', signal: AbortSignal.timeout(10000),
      headers: { 'Content-Type': 'application/json', Accept: 'application/json' },
      body: JSON.stringify({ token, gameId }),
    });
  } catch { throw new Error('auth_unavailable'); }
  if (!response.ok) throw new Error(`auth_http_${response.status}`);
  let data;
  try { data = await response.json(); } catch { throw new Error('invalid_auth_response'); }
  if (data?.result !== 0 || data.can_play !== true || data.game_id !== gameId) throw new Error('game_access_denied_or_legacy_projection');
  const userId = Number(data.user?.userid);
  if (!Number.isSafeInteger(userId) || userId <= 0 || (expectedUserId !== undefined && String(userId) !== String(expectedUserId))) throw new Error('player_identity_mismatch');
  if (!Number.isSafeInteger(data.expires_at) || data.expires_at <= now) throw new Error('player_session_expired');
  if (data.catalog_synced !== true || !Array.isArray(data.dlcs) || data.dlcs.some(id => !Number.isSafeInteger(id) || id <= 0)) throw new Error('catalog_not_verified');
  return { gameId, userId, expiresAt: data.expires_at, dlcs: [...new Set(data.dlcs)] };
}

if (process.argv.includes('--self-test')) {
  const assert = await import('node:assert/strict');
  const args = { token: 'inert-issued-session-fixture', gameId: 1614, expectedUserId: 42, now: 1000 };
  const accepted = { result: 0, can_play: true, game_id: 1614, user: { userid: 42 }, expires_at: 1100, catalog_synced: true, dlcs: [7001] };
  const reply = data => async (url, options) => {
    assert.equal(url, 'https://auth.idcgames.com/api/launcher/get-access-data');
    assert.equal(options.redirect, 'error');
    assert.deepEqual(JSON.parse(options.body), { token: args.token, gameId: 1614 });
    assert.equal(options.headers['X-Internal-Key'], undefined);
    return { ok: true, json: async () => data };
  };
  assert.deepEqual(await verifyNativeSession({ ...args, fetchImpl: reply(accepted) }), { gameId: 1614, userId: 42, expiresAt: 1100, dlcs: [7001] });
  for (const changed of [{ ...accepted, game_id: 1611 }, { ...accepted, can_play: false }, { ...accepted, expires_at: 999 },
    { ...accepted, user: { userid: 43 } }, { ...accepted, catalog_synced: false }, { ...accepted, dlcs: ['7001'] },
    { result: 0, user: { userid: 42 } }]) {
    await assert.rejects(verifyNativeSession({ ...args, fetchImpl: reply(changed) }));
  }
  console.log('Native backend example: 8 offline cases passed; no real Auth request or credentials.');
}

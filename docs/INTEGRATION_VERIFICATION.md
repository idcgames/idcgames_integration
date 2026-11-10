# Integration verification — 2026-11-10

This record distinguishes observed behavior from contracts, fixtures and work
still requiring acceptance. No player credential, account data or signed URL is
included. Only the AdminPanel chat performed the final game1599 live checks;
the other chats were asked to stop overlapping Launcher/UI probes.

## Actual existing Launcher + SDK, game1599

Used the already running signed-in Launcher7.1.27, without restarting it, and
the native DLL supplied in `CSharp_Test_1599`, matching the official SDK's DLL
SHA256 `0f80a973254a21d0a08950b0b0ecf39da24c2cf7b9a24da64ba2c664f266af51`.
A separate sandbox-only SDK probe suppressed native logs, kept player data in
memory and never called `_ClosePurchase`, checkout or a reward grant.

| Check | Observed result |
|---|---|
| InitLib and UserData | Init0, UserData status0, credential received |
| Native credential → backend Auth verification | POST check-user-game-token returned HTTP200/statusOK |
| Optional get-access-data projection | HTTP200/result0, matching verified user; **legacy response lacks can_play/game_id/expires_at/catalog_synced/dlcs** |
| `_OpenShop` sandbox, empty article | Actual `IDC Games — Tienda` window opened with Defuse Gold packs/prices, starting at3000 for EUR1.50 |
| Shop close | Window closed; DLL callback PurchaseClosed0x6C1CCF80 received |
| UTF-8 videoAd JSON to IDCGamesPipe | Write succeeded; actual sandbox video window opened for1599/defuse |
| Video callbacks | VideoAdResult0xB1DE0A7D STARTED (twice), then SKIPPED after the Skip ad control; matching game and session present |
| Financial actions | Zero checkout actions, acknowledgements and rewards; **no payment or item delivery claimed** |

The UI's embedded pages were observed at Admin `/shop/frontend/index` and Auth
`/pages/video/1599/defuse`, with all query/session values redacted. These are
Launcher-managed destinations, not a supported template for manually inventing
authenticated deep links. Seeing a return0 alone is not proof of UI success.
The repeated STARTED event also demonstrates why callbacks must tolerate
duplicates and why STARTED must never count as an earned reward.

The legacy1599 projection is not the modern opaque-v1 entitlement response.
Do not claim DLC/can_play authorization from missing fields, and do not switch
a live game's Auth mode merely to make a test pass.

## Other, explicitly separate evidence

- Auth's isolated PlayerGameSessions suite:24 tests/410 assertions for issued
  opaque sessions, identity binding, wrong-game rejection, revocation and
  operational DLC ownership. A dedicated1614 fixture and modern token/gameId
  alias test passed. These are **not** a real1614 player login or a1599 mode change.
- StarFetcher's earlier1611 SDK probe obtained matching real Auth identity,
  can_play, expiry and synced catalog. Its Shop window reported no Shop
  configured. Pulse Zero's published Rust EXE implements native login and an
  Auth HTTPS check, **not an independent remote S2S backend, GamePanel adapter
  or Shop/Ads UI**. Do not present the separate SDK probe as those missing features.
- SDK offline verification:16 tests, inert demo, native DLL exports and pinned
  hash passed. These tests do not prove live payment or rewarded-ad revenue.
- Admin PHPUnit:1311 tests,15438 assertions,1 expected Windows skip. Focused
  private payment key/signature tests also passed after the final URL guard.
- MCP full suite:140 tests passed, including a fresh unconfigured-client
  guide/resource retrieval test. Publisher and Admin catalogs remain separate.
- Frontend Vite and legacy builds passed; no hand-edited compiled files.

Follow-up1.11.0: full PHPUnit1314 tests/15484 assertions with1 Windows skip;
new-game public/private generation and compatibility-copy migration passed
through Quick Setup and publisher API. Native/payments backend examples passed
8 and6 inert cases. A fresh MCP connection can read the entire publishing
workflow and example resources without a key. The actual AdminLTE navigation
regression was reproduced before the fix and passed after preserving the
original treeview root across repeated iframe navigation. Existing public
tokens were not regenerated; payment-only rotation was tested on disposable data.

## Remaining acceptance, never implied by the above

Actual sandbox checkout through IDCPayment → Admin → game backend and durable
idempotent delivery; multi-pack quantities and nested bonuses; refund/reversal
and consumed-balance policies; private payment key deployment to an actual
receiver; complete/NO_FILL/error ad callbacks and verified backend rewards;
standalone authenticated web Shop deep-link contract; a real external backend's
full S2S entitlement flow and session renewal. These need their own scoped
acceptance. No real charges, entitlement changes or credential rotations were
performed to fill these gaps.

See [the usable integration contract](GAME_INTEGRATION.md), available directly
through MCP `get_integration_guide` and `idcgames://publisher/integration`.

# IDC Games integration: native login, Shop, purchases and ads

Read this guide before writing an integration. Available directly through the
publisher MCP tool `get_integration_guide` and resource
`idcgames://publisher/integration`, as well as `dist/INTEGRATION.md` in its package.
The MCP configures the publisher's game; it is not a player login/payment API.
Read the [dated verification record](https://github.com/victorlamas/idcgames_panel_ia/blob/main/docs/mcp/INTEGRATION_VERIFICATION.md)
for actual observed outcomes and the remaining acceptance work.

## 1. Identify the system and the credentials

| Value | Source and use | May ship in the EXE? |
|---|---|---|
| Panel `game_id` | `get_game`; selects publisher tools and Admin records | Yes; identification only |
| IDC `id_game` / `id_idcgame` | Game's operational identifier in Auth/Launcher; do not substitute the Panel ID | Yes |
| `game_token` | `get_game`; public Launcher application identifier, historical `gameSecret` wire alias | Yes; never authorizes a payment/player by itself |
| Publisher MCP key | Studio Configuration or Game Configuration → MCP keys; scoped `X-Agent-Key` for Admin | No |
| Native `tokenUserGameId` | Live Launcher UserData; short-lived bearer credential for the player/game | Runtime only; never logs, URLs, repository or support messages |
| Private `payment_secret` | `prepare_game_payment_callback`; shared only by Admin and the game's backend | **No** |
| Auth `X-Internal-Key` | Internal IDC web-account verification; not a publisher credential | **No; never delegate to a studio** |
| GamePanel internal game ID | Receiver configuration, if GamePanel is used | Separate from both IDC and Panel IDs |

The public token and private payment secret are independent. Do not rename a
public token to "secret" and then use it as an HMAC key. Payment keys do not
authenticate users. A studio-scoped MCP key manages its games; a game-scoped
one manages that game only. Neither can administer Auth or issue MCP keys.

## 2. Existing EXE login and optional backend verification

Keep a working StarFetcher/Launcher login unchanged. Server-to-server validation
is the next optional integration when the game has a backend: forward the
already received credential to that backend, which verifies it with Auth. It
does not ask the player to log in again or replace the Pipe/DLL protocol.

Get the official integration SDK and its verified native DLL from
[idcgames/idcgames_integration](https://github.com/idcgames/idcgames_integration).
Read its `docs/launcher-protocol.md`, `docs/quickstart-csharp.md` and
`docs/authentication-idc.md`. Do not recreate native structs or encryption from
memory. Native Windows x64 ABI: `_InitIDCLib`, `_PullMessage`, `_EndIDCLib`;
StdCall, native bool one byte, JSON callback UTF-16LE, length in bytes. Copy
callback bytes before returning, retain delegates, poll on the game thread,
dispose on exit. A C# .NET8 wrapper is an example, not a requirement for C++ games.

1. Approve the studio, create the game and DLC catalog, then
   `sync_development_game`. Wait for its Auth delivery acknowledgement before
   testing login. A human assigns the tester's separate IDC player account via
   Game Configuration → Developer access (`user_developer`, numeric `id_user`).
2. Download the official IDC Launcher, sign in and leave it open. Initialize
   IDC's DLL with the operational game ID, public client token and actual process
   ID. Test the existing encrypted InitLib Pipe flow. Init returning success is
   not authenticated UserData. An old provisional JSON login path is not an
   interchangeable implementation.
3. On UserData, send its credential to **your backend by HTTPS**. Do not trust
   `userId`, nick, email or DLC claims supplied by the client without validation.
   Do not try `/api/web/verify-token` for an opaque native credential.
4. For games configured in Auth with `client_session_protocol="opaque-v1"`,
   the current issued-session verifier reuses this historical URL with a
   **new game-scoped session implementation**, not the old calculable MD5 check:

   ```http
   POST https://auth.idcgames.com/api/launcher/get-access-data
   Content-Type: application/json

   {"token":"CREDENTIAL_FROM_LIVE_PIPE","gameId":1614}
   ```

   The backend fixes `1614` from its own game configuration. No global S2S key
   is required by this route: the issued opaque credential is the bearer proof.
   An invented 32-hex string, public game token or web JWT is not such proof.
   Prefer POST; never put a player credential in a query string.
   `idctk` and `idcgameid` remain historical aliases. The supported `token` and
   `gameId` names above avoid confusing this with the old implementation. Do not implement MD5.
5. Require HTTP success **and** `result===0`, `can_play===true`, expected
   `game_id`, positive `user.userid`, future `expires_at` in Unix seconds and
   `catalog_synced===true` before unlocking catalog content. Bind the internal
   game session to the **verified** `(game_id,user.userid)`. Reject a conflicting
   client user ID; never bind by email/nickname or `usercrc`.
6. `dlcs` contains purchased/authorized **operational DLC IDs**. It is not the
   catalog's `panel_id`, a SKU, a slug or an array of all available DLCs. `[]`
   unlocks nothing. Developer access to the base game does not grant DLCs.
   Refresh authorization for protected actions and revocations; an expiry is
   an upper bound, not a guarantee of access until then. Session TTL is at most
   30 minutes and cannot exceed its parent/access right. Renew through the
   supported InitLib lifecycle; do not silently prolong offline access.
7. Handle invalid/expired/revoked/wrong-game credentials (401), maintenance,
   bans or access denial (403), unavailable dependency (503), timeout and
   incomplete JSON by denying access and showing the supplied error code and
   localized message. Do not fall back to an unverified cached identity.

`/api/launcher/check-user-game-token` is also retained, taking `userId`, `gameId`
and `tokenUserGameId`, but an OK alone is not the full can_play/DLC projection.
`/api/web/verify-token` belongs to web/account tokens and internal IDC access;
it is not an alternative verifier when an opaque native session is rejected.

**Compatibility boundary:** Auth dispatches to the new implementation only for
an enabled game. Existing GamePanel adapters/documentation may still describe
launcher-check followed by web-token fallback and call the credential MD5.
Inspect the installed adapter and confirm its mode; do not silently mix these
flows. The current Auth implementation and isolated game1614 tests establish
the opaque contract. An invalid-token production probe demonstrates rejection,
not a successful real-player EXE test. SDK session renewal and the real EXE still
need acceptance with the signed-in Launcher. No credentials are included here.

## 3. Build a Shop with the publisher MCP

Free-to-play is the base-game price model; it does not make Shop articles or
DLCs free. Unlock DLCs are separate catalog entitlements (`save_dlc_draft`), not
coins or consumable Shop articles. Do not upload another EXE for bundled levels.

| Identifier | Meaning and source |
|---|---|
| `game_item_id` | Stable string chosen by the game backend, e.g. `credits`; natural key of `save_shop_item` |
| item `id` | Admin row returned by that tool; use as article `item_id` |
| `game_article_id` | Stable game pack/SKU string, e.g. `credits_100`; natural key of `save_shop_article` |
| article `id` | Admin row for price, placement, offer and coupon relations |
| `idc_article_token` | IDC checkout/callback article token; read back from the saved catalog; do not substitute a row ID or invent one |
| level/tab/category/country/currency IDs | Discover with `get_commerce` / `get_commerce_catalogs`; never guess |
| `transactionid` | Payment transaction ID; durable delivery/refund idempotency key scoped to the game |

Use these steps, reading back each saved object's IDs before constructing its
dependants. Send every required field on edits; inspect each tool's schema.

1. `get_commerce_catalogs` → `get_commerce` → `setup_shop` (initially inactive).
2. Create item categories and translated item definitions (`save_shop_category`,
   `save_shop_item`). Set `game_item_id` to the backend's actual grant mapping.
3. `save_shop_article`: its own `game_article_id`, returned `item_id`, pack
   `number_of_items`, payment mode, region, text and image. Creating an article
   does not itself define every regional checkout price or visible placement.
4. `save_shop_level` and `save_shop_tab`, then `save_shop_price` for each required
   level/country/article/currency and `save_shop_placement` for visible articles.
5. Optional offers/coupons (`save_shop_offer`, `save_shop_coupon`,
   `save_coupon_code`); relations must belong to the same game. Define how bonus
   articles map to backend grants. Do not expect the reference receiver to
   automatically deliver nested bonus arrays.
6. Read back the complete shop, configure the private signed callback below,
   perform sandbox acceptance, then explicitly activate commerce with the
   corresponding activation tool and verify the live destination's result.

The historical `bought_notify` flag appears on an item; the current producer
does not use it to suppress the game callback. Do not rely on false to prevent
notification or rely on it as a security control.

## 4. Private backend payment configuration

`signing_mode=private_payment_secret` means Admin uses the separate private key;
it does not prove an actual checkout or item delivery has succeeded.

Set MCP `IDCGAMES_PAYMENT_SECRETS_DIR` to a protected **absolute** directory
outside the repository, public uploads and game distribution. Restrict Windows
ACLs to the service/owner, or Unix directory700/files600. Never paste the file
contents into a chat. Back it up as a backend credential.

1. `get_game_payment_callback` reports signing mode/fingerprint only.
2. `prepare_game_payment_callback` creates/reuses a random private key in that
   directory, bound to Panel URL and Panel game ID. The response contains only
   a local path and SHA256 fingerprint. It does not activate the key.
3. Install that file in the receiver's server-side secret store through an
   authorized server deployment. No secret goes into client assets or an EXE.
4. Once the receiver uses it, call `configure_game_payment_callback` with the
   Panel game ID, HTTPS `callback_url` and `backend_ready:true`. MCP reads the
   private file itself; the secret is never a tool argument visible to the AI.
   Admin stores it encrypted in a dedicated table, outside public game props.
5. Compare fingerprints; repeat with the same key safely. A different existing
   key returns409: coordinate rotation with IDC and the receiver, not automatic
   overwriting. Lost local files require recovery of the original backend key.

For an HTTP API integration the write-only route is
`PUT /api/agent/games/{panel_game_id}/payment-callback`, publisher profile,
`commerce:write`, approved studio, JSON `payment_secret` (64 lowercase hex),
`callback_url`, `backend_ready:true`. GET uses `commerce:read` and returns no key.
This is a game webhook secret, not issuance/delegation of an Agent API key.

Existing receivers retain **temporary** legacy public-token signing until
migrated: `signing_mode=legacy_public_game_token`, `migration_required=true`,
`production_ready=false`. `GAME_PAYMENT_ALLOW_LEGACY_GAME_TOKEN=false` makes
unmigrated notifications fail closed. A configured private key always wins;
there is never a fallback after private configuration. Do not disable legacy
globally before coordinating existing games. Changing this is not automatic
proof that a backend has received or delivered a payment.

## 5. Signed Admin → game payment delivery

The complete wire contract is
[GAME_PURCHASE_CALLBACK.md](https://github.com/victorlamas/idcgames_panel_ia/blob/main/docs/GAME_PURCHASE_CALLBACK.md).
IDCPayment confirms to Admin. For ARTICLE purchases, Admin independently sends
one notification to the game's configured callback to deliver items, and one
to IDC/Auth for accounting. Auth receiving its copy does **not** deliver the
game's coins or turn an article into a base/DLC library entitlement.

```http
POST https://backend.example/your-payment-callback
Content-Type: application/json
X-IDC-Notification-Id: IDCNOTGAME_example
X-IDC-Timestamp: 1794312000
X-IDC-Signature: sha256=HEX_HMAC
```

```json
{"action":"idcpayment","useridc":"42","transactionid":"example-payment","sandbox":"1","refund":"0","currency":"EUR","gross":2.99,"bought_data":[{"game_item_id":"credits","game_article_id":"credits_100","idc_article_token":"example-article-token","number_of_items":100,"article_count":1,"extra_articles":[],"offer_extra_articles":[]}]}
```

`HMAC-SHA256(payment_secret, timestamp + "." + exact_raw_body)` → lowercase
hex, prefixed `sha256=`. Compare constant-time **before parsing/delivering**.
Validate timestamp within300 seconds; reject altered JSON bytes, missing headers,
wrong game/key and replayed processing. TLS verification is enabled; timeouts
are10s connect/30s total. Returns any2xx are acknowledged by Admin, so respond
2xx only after durably storing the accepted event/delivery. A retry carries the
same notification ID/body, fresh timestamp/signature. Automatic retries are
hourly, up to5; an operator can resend pending notifications.

In one database transaction: verify buyer's stable IDC identity and game
mapping, require recognized active SKUs, validate quantities/currency/amounts,
record unique `(game,transactionid)` purchase state and ledger grants, commit.
Duplicate callbacks must return success without a second grant. An outbox may
handle downstream rewards only if the acceptance is durable and retryable.

`number_of_items` and `article_count` are separate wire fields. Admin currently
forwards them rather than multiplying. GamePanel's reference receiver consumes
`number_of_items` directly (fallback `article_count`); do not multiply twice.
Before accepting multi-pack orders, test quantity1 and quantity>1 end to end
and define the agreed total-grant semantics. Test bonuses separately.

For refunds, keep a separate idempotent reversal state for the same transaction,
including refund-before-purchase and partially consumed currency policy.
Sandbox must not create production grants/revenue. Invalid signatures return401;
transient failures must not masquerade as successful delivery.

### GamePanel receiver reference and its limits

GamePanel exposes `POST /api/v1/webhooks/idcpayment/games/{game}`; `{game}` is
its internal ID, not the IDC game ID. Its service resolves products by
`game_item_id`, then `idc_article_token`, then `game_article_id`, scoped to the
game, and requires an existing IDC-linked user. Missing user/product is not a
reason to guess a different account or grant an arbitrary item.

The reference currently documents HMAC and transaction idempotency, but does
**not** automatically reverse previously delivered inventory/currency on refund,
and does not automatically expand Admin's nested bonus arrays. Add those game
policies and tests before claiming a complete paid consumables integration.
Its old IDC credential field has also been used for web-auth verification:
do not reuse a global Auth key as the new per-game payment secret. Configure a
dedicated receiver payment key and coordinate its adapter/version explicitly.

## 6. Open checkout from the game/Launcher or web

With verified local login and supported SDK wrapper:

```csharp
idc.OpenShop(Guid.NewGuid().ToString(), article: "", sandbox: true);
// Empty article opens the shop. Use the actual IDC checkout article identifier
// from the catalog to target a pack; never guess it from the Panel row ID.
```

Native `_OpenShop(transactionId,extra,userLevel,sandbox,article)` is not a payment
verification API. `extra`/`userLevel` are client hints, never authorization.
Use a unique order/correlation ID and keep backend mapping to buyer and product.
The included DLL can return0 even when its notifier is not running: that return
value alone does not prove a window opened or a Shop/catalog loaded. Require
the actual UI and callback evidence for that claim.
Local purchase events carry `dllTransaction` and `idcpaymentTransaction`;
they only prompt the game to check its backend's verified delivery status.
Call `AcknowledgePurchase` / `_ClosePurchase` only after durable verified
delivery, never grant on PurchaseOk or close alone.

### Concrete C# F2P example, game1599

The current `CSharp_Test_1599/Program.cs` example supplied with the Launcher
initializes login first, then `DoShop(null, sandbox:true)` calls:

```csharp
IDCLib._OpenShop(uniqueTransactionId, "", 0, true, "");
```

This opens the F2P game's **article shop**, not purchase of its free base game.
An empty article opens the full shop; a configured article targets that pack.
Do not copy that example's actual identifiers/credentials into a different game.
Its interactive `[6]` is sandbox shop; `[3]` and `[4]` use live mode. `[8s]`
requests sandbox video, `[8]` live video. In the test application login is `[1]`.
The example's `[p]` separately forwards UserData to GamePanel's
`POST /api/v1/auth/third-party/login` with `platform:"idc"`, credential `token`,
`user_id`, internal GamePanel `game_id`, operational `idc_game_id` and that
installation's `gamepanel_secret_token`. These are not the Admin MCP key or
payment signing key. A successful GamePanel login issues its own bearer
access_token; `/auth/validate` validates that new game session. GamePanel is
optional; another backend can call Auth directly as above.

Do not copy the diagnostic example's raw UserData/DLL logging or automatic
`_ClosePurchase` from PurchaseNotification into a production game. Replace
automatic acknowledgement with the verified backend delivery sequence above.

For web, use the actual published game Store/Shop and its existing authenticated
checkout flow. A Store URL, an in-game article checkout and a base-game/DLC cart
item are different products. The publisher MCP is not an endpoint for charging
a player. **No generic signed web checkout/deep-link URL is promised by this
contract:** do not manufacture `?token=`, iframe authentication or checkout
query parameters. Discover the published URL via `get_game_review.public_url`;
if an embedded web-shop URL is required, resolve it with the receiving web's
current authenticated route before exposing a button. Missing route support is
a pending integration requirement, not a reason to substitute the Launcher URL.

## 7. Launcher video advertisements

The current SDK wrapper provides:

```csharp
await idc.OpenVideoAdAsync(sandbox: true);
```

There is no `_OpenVideoAd` export in the included native DLL. The wrapper uses
the local `IDCGamesPipe` JSON videoAd request and receives `VideoAdResult`.
Layouts: `fullscreen_monitor`, `fullscreen_game`, `windowed`. Connect timeout3s;
one active request only. Statuses: STARTED (not completion), OK, SKIPPED,
NO_FILL, ERROR; the current Launcher can report BUSY (older guides call it
ALREADY_ACTIVE). Handle both and late/duplicate events. Current results
contain `gameId`, `sessionId`, `errorCode`, but do **not** echo requestId, so do
not promise guaranteed correlation. Never loop endlessly on NO_FILL.

The Launcher manages its session and provider bridge; do not manually construct
an Auth video-page URL as a substitute. Sandbox/preview does not prove revenue.
A local OK is not a trusted reward receipt: valuable rewards require a separate
verified backend receipt and once-per-session ledger. This guide does not claim
that a public rewarded-ad verification endpoint already exists. Do not invent
one or award production currency from a spoofable local callback.

The C# example sends this actual message as UTF-8 bytes to `IDCGamesPipe`,
then Flush; retain and poll the native callback for the result:

```json
{"videoAd":true,"gameId":"1599","sandbox":true,"requestId":"unique-request-id","layout":{"mode":"fullscreen_monitor"}}
```

It also supports `layout:{"mode":"rect","x":100,"y":80,"width":800,"height":450}`;
the simpler SDK wrapper intentionally exposes only the first three modes.
Do not confuse UTF-8 request encoding with native UTF-16LE JSON callback
encoding. Video compatibility callbacks may be UTF-8 text
`VIDEO_AD|STATUS|gameId|sessionId|errorCode`.

## 8. Acceptance checklist the AI must report honestly

| Test | Required result |
|---|---|
| Real EXE + signed-in Launcher | Verified backend identity for the expected IDC game |
| Missing Launcher, malformed/wrong-game/expired token, maintenance | Denied with useful error; no cached authorization fallback |
| DLC none/owned/refunded | Only the verified operational IDs unlock levels; revocation observed |
| Session renewal | No access beyond expiry; supported InitLib refresh tested in the real SDK |
| Shop sandbox pack1 and pack>1 | Correct backend quantity exactly once; all IDs mapped explicitly |
| Invalid HMAC/altered body/stale timestamp | Rejected, no inventory changes |
| Duplicate, restart, delayed callback | Durable eventual delivery once, not repeated grants |
| Refund, refund-first, consumed balance, bonus articles | Explicit tested game policy; reference gaps fixed or disclosed |
| Web and Launcher checkout | Real configured route opens correct game/product; cancellation handled |
| Ads complete/skip/no-fill/error/late/duplicate | Correct UI; no unverified valuable reward |

Offline tests and sandbox payments are not real purchases. Do not charge a
player, replay a production payment, rotate keys or grant entitlements to make
a test pass without the corresponding authorization. Report configured,
deployed, simulated and real verified outcomes separately.

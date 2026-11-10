# Publisher E2E: studio, local integration, release and game page

Public guide: https://admin.idcgames.com/publisher/get-started

For executable login, verified user/DLC access, Shop identifiers, private payment
callbacks, Launcher/web checkout and video ads, first call MCP
`get_integration_guide` or read [Game integration](https://github.com/victorlamas/idcgames_panel_ia/blob/main/docs/mcp/GAME_INTEGRATION.md).
It is also available as resource `idcgames://publisher/integration` and packaged
`dist/INTEGRATION.md`. Do not infer these runtime contracts from publishing tools.

This is the canonical publisher workflow. A studio approval, developer access,
a build upload, a public page approval and a playable phase are separate decisions.
Do not skip the real Launcher test or describe a queued synchronization as delivered.

## 1. Create and approve the studio

Connect the publisher MCP from source or install its downloadable `.tgz` (Node
20.19+). Start without a saved key for a new studio. Use
`request_company_signup`; ask the owner for the OTP received in their own email
and explicit acceptance of the publisher terms. Call `confirm_company_signup`.
The MCP saves its scoped publisher credential privately. Never ask for passwords
or paste API keys, device codes or player tokens into a conversation.

**Registration is not IDC approval.** `confirm_company_signup` returns
`status:registered`, not an approved studio. Read `get_company_profile.onboarding`
immediately; never tell the owner to wait without checking what is missing.

| Studio status / response | Required next action | Should the AI wait? |
| --- | --- | --- |
| `pending`, `missing_fields` nonempty | Ask the owner for exactly those fields, using `required_inputs`; call `update_company_profile`. | **No. Waiting cannot complete missing data.** |
| `pending`, complete profile | Existing registrations may need `submit_company_review`, or save the complete profile again. | **No. It has not been submitted.** |
| `pending_review`, complete profile | Poll `get_company_profile` after 60 seconds. Auto review normally runs every ten minutes when enabled; otherwise IDC must review manually. | Yes, with a clear explanation to the owner. |
| `approved` | `onboarding.ready_for_uploads=true`; studio onboarding is complete. Game approval remains separate. | No. Continue preparing games. |
| `rejected` | Read the feedback, ask the owner for corrections, and save the corrected profile. | **No. It requires corrections.** |

Required legal inputs are **`legal_name`** (owner-supplied legal entity name,
1–191 characters) and **`country`** (the studio's legal country, two ISO alpha-2
letters, e.g. `ES`). The public studio name and the owner's account country do
not prove those values. Ask if unknown; do not invent legal or billing data.
Contact name, phone, website and tax ID are additional profile fields. A missing
contact address is flagged to IDC separately; the API does not change the owner
email or grant the agent authority over account credentials.

Saving a complete pending/rejected profile through the API now **automatically
submits it**, matching the panel. It does not approve it. The older explicit
`submit_company_review` remains safe to repeat and never demotes an approved
studio. Read `missing_fields`, `required_inputs`, `next_action`, `should_wait`,
`poll_after_seconds` and `automatic_review.enabled` in every response.
If review is still pending after 20 minutes, stop the polling loop, tell the
owner it is delayed and ask IDC to check the scheduler/manual queue. Never
re-register the studio or repeatedly submit it to make progress.

Example: ask “What is your studio's legal entity name and legal country?”;
call `update_company_profile({legal_name:"<owner answer>",country:"<ISO code>"})`;
expect `pending_review` and `should_wait:true`; poll until `approved`. Only then
say “Your studio is approved.” Draft games can be prepared earlier; uploads,
sales and publication still require approval and permissions. See
[the signup contract](https://github.com/victorlamas/idcgames_panel_ia/blob/main/docs/mcp/PUBLISHER_OTP_FLOW.md).

In the panel, studio-wide credentials are managed under **Studio → Studio
configuration → Studio MCP Keys**. Game-restricted keys have their own screen.
Each key belongs to one studio; choose all studio games for a studio-wide agent,
or restrict it to one game for a project-specific agent. **Studio MCP Keys** lists
only credentials for the whole studio. For separate game credentials, open
**Game Configuration → Game MCP keys**; the selected game is fixed on issuance
and its screen lists only that game's keys. These are different secrets and
independent MCP configurations, using the same publisher package and PANEL_URL
but a different PANEL_API_KEY for each agent. Studio keys cover future games too;
a game key cannot create another game or edit the studio profile. Scopes still limit its
actions. IDC staff select **Studios → All studios → [studio] → Studio
configuration → Studio MCP Keys**. The staff studio page also links its
games, members, review, billing, invoices and balance, subject to permissions.
Admin MCP credentials remain separate from studio publisher credentials.

## 2. Prepare the game and its complete page

Discover IDs with `get_game_catalogs` and creator names with `search_creators`.
Create the game using `create_game`; read it back with `get_game`. Use its
`idc_game_id` and `game_public_token` (compatible alias `game_token`) for client configuration. The token is
an application identifier, not player authentication or an Admin credential.
The historical `game_secret` alias remains for compatible receivers.

Game Configuration shows `game_public_token` and masked `game_payment_secret`.
New games generate independent values; existing games copy their original value
into the encrypted payment field without changing it. A human can reveal/copy
or explicitly regenerate only the payment secret, then update the backend.
Never place that secret in the EXE. For AI integration, set the protected local
`IDCGAMES_PAYMENT_SECRETS_DIR` and call `prepare_game_payment_callback`; it exports
the existing value to a file, never into the conversation. Read topic `payments`
from `get_integration_guide` before configuring the callback.

Supply English short, full and SEO descriptions as the default, then localized
copy, commercial model, required Store Params and video. Upload assets through
the MCP, apply clean imports, set store content and generate/check artwork
renditions. Use `list_store_param_defs` and `validate_publication` for the
actual current requirements; do not guess that a few uploaded images make a page
complete. See [artwork requirements](https://github.com/victorlamas/idcgames_panel_ia/blob/main/docs/GAME_ARTWORK.md).

Short copy must contain **120–500 visible characters** per language; full/long
copy **1,000–10,000**. For `zh`, `ja` and `ko`, the minima are 60 and 400.
HTML does not count; Unicode is normalized to NFC. Read the canonical
`text_policy` from `list_store_param_defs`. Aim for a specific short pitch and a
structured 2,000–4,000-character long description covering setting, gameplay,
progression and confirmed features. Use natural equivalents in CJK, never filler
or invented modes. See [the description contract](https://github.com/victorlamas/idcgames_panel_ia/blob/main/docs/GAME_DESCRIPTIONS.md).

For DLC levels bundled in the base EXE use `save_dlc_draft` with
`delivery_mode: "unlock"`, English text, stored cover, price and
`request_publication: true`. There is no DLC ZIP or download URL. Distinct DLC
files are only for a game that actually distributes separate content. Use the
explicit operational `idc_dlc_id`, never a local label as proof of ownership.

Each DLC needs its own representative 16:9 cover: **1920 × 1080 recommended,
1280 × 720 suggested minimum**, PNG/JPEG/WebP. Show real pack-specific levels,
keep important details away from card crop edges, and describe the setting,
included mechanics, differences and base-game requirement in English and the
other supported locales. See [DLC artwork guidance](https://github.com/victorlamas/idcgames_panel_ia/blob/main/docs/GAME_ARTWORK.md#dlc-covers-and-descriptions).
For an already published unlock DLC, `update_dlc_presentation` stages cover and
descriptions only; submit a new game review to apply and synchronize them.
Prices, ownership IDs and files stay unchanged until separately managed through
their existing workflows. Never duplicate a DLC just to replace its cover.

WWW discovery distinguishes **Coming Soon** from **New releases**. New releases
are available Production games released within the last two calendar months;
Coming Soon games belong in their own section with their launch window. A page
approval or synchronization timestamp does not make a game a recent release.

## 3. Register Development in Auth and assign a test player

Call `sync_development_game` with the studio's authorized publisher key
(`games:write` and `releases:publish`). It chooses the existing `developing`
phase and sends the game only to operational Auth. Poll
`get_game_review.development_registration` until `state` is `sent`; an outbox
request alone is not registration. WWW remains unpublished and nobody receives
player rights through this operation.

The tester must create a normal IDC Games **player account**, then a person with
access to this game's configuration and `game_manage` assigns that account under
**Game Configuration → Developer access** (`/game/userdeveloper?game=<panel-id>`).
Search by nickname, select the actual account and assign its stable `id_user`.
The studio's panel owner account and its player account are separate identities.
Company approval, a creator credit named “developer”, and `user_developer` are
not equivalent. Do not assign global developer access for a game's tester.

The panel calls Auth server-to-server. There is currently **no publisher MCP
tool to assign developer rights**: the AI must ask the authorized human to do
that panel step. Never give an agent or EXE the Auth administration service
token. Read [User Developers](https://github.com/victorlamas/idcgames_panel_ia/blob/main/docs/USER_DEVELOPERS.md) for the real API and permissions.

## 4. Download the Launcher and test the local executable

Download/install the current IDC Games Launcher from the official IDC Games
website's Launcher download entry: https://idcgames.com/en/download (Spanish: https://idcgames.com/es/download). Do not invent a binary URL or use an old copy
from a third party. Sign in with the authorized test player and leave the
Launcher running. Refresh the Launcher login after new session-protocol changes.

Integrate IDC's Windows DLL/SDK, configure the same IDC game ID and game token,
and launch the freshly compiled EXE manually **while the Launcher is open**.
Initialize the SDK and read the authenticated session through its encrypted
Pipe; do not build a mock Launcher or log its account/game session tokens.
Validate the session with Auth through the supported integration contract. The
public game token alone never authorizes a player.

Check real identity, game binding, `can_play`, expiry and the purchased DLC list.
An empty DLC list means no DLC entitlement; it must not unlock levels. Developer
access to the base game does not mean ownership of every DLC. Test missing
Launcher, denied/expired sessions and revoked DLC fail closed. Keep DLC content
locked and refresh permissions according to the session contract. A local Auth
fixture can test purchased/refunded DLCs but is not a real purchase or production
login. Do not grant production DLCs merely to make a test pass.

## 5. Upload the validated release

Only after that real local integration passes, create a release with
`create_release`; its numeric version code increments automatically. Package a
ZIP with the documented `common/`, optional `uncommon/` and `redist/` layout and
the EXE path. Discover `get_release_delivery_options`; upload to an advertised trusted
provider (including enabled SwissTransfer) or the configured kDrive inbox.
Use `import_release_from_url`/`import_release_from_kdrive`, SHA-256 and an
idempotency key. Poll `get_import_status` until `ready`, inspect the report and
call `apply_import`. A clean import is not publication.

Call `publish_release` to synchronize the build to the CDN; verify completion,
then activate the intended validated release with `activate_release`. Never
activate an old or intermediate EXE. Unlock DLCs stay in the same base build.

## 6. Choose the launch phase and request the public page

Discover the phase IDs with `get_game_catalogs`. Choose the intended phase
(Development, Closed Beta, Open Beta, Production, etc.) according to the
[phase contract](https://github.com/victorlamas/idcgames_panel_ia/blob/main/docs/PUBLIC_GAME_PHASE_CONTRACT.md). Phase access restrictions
still apply: a beta is not blanket public access.

Use `set_game_launch_plan` for `coming_soon` or `available`, plus an exact or
approximate launch window if known. A Coming Soon **page can be reviewed and
published without an uploaded/active release**; it does not offer installation
or purchasing just because it has a date. This can happen before step 5 when
announcing the game. Available/playable publication requires its relevant
validated release and permissions. Do not infer `available` from a phase alone.

Run `validate_publication`, fix missing fields and call `submit_game_for_review`.
The scheduler reviews every ten minutes and approves automatically only if all
checks pass. Changing reviewed content requires resubmission. Approval queues
WWW/Auth and configured Forum delivery and the requested unlock DLC catalog;
poll `get_game_review` for approval **and successful delivery acknowledgements**.
Open its returned `public_url` and verify the page, images, video, languages,
phase and DLC prices. A review email alone does not prove WWW applied the catalog.

If the review has `attention` because its provider failed, it is not a content
rejection: IDC must resolve the technical failure. Publishers cannot bypass
moderation or change the server policy through the MCP. For the integration
test of 2026-11-09 IDC explicitly enabled temporary automatic approval without
AI (`GAME_REVIEWS_SKIP_AI_VALIDATION=true`). These decisions are reported as
`automatic_no_ai`, with `ai_checked=false`; they are not successful AI checks.
The scheduler still checks technical readiness and delivery requirements.
See [review policy and restoration steps](https://github.com/victorlamas/idcgames_panel_ia/blob/main/docs/PUBLISHER_REVIEW_FLOW.md).

## AI completion report

Report the studio/game IDs, selected phase, release code/version and verified
hash, real Launcher/Auth result, DLC test scope, review state, each delivery state
and actual public URL. Explicitly distinguish local fixtures, real player login,
CDN delivery and an actual purchase. Keep secrets and personal session data out.

## Required image compositions

Five originals are mandatory: a free-proportion logo, hero 16:9 (`game_home_bg_section_1`, minimum1280×720, recommended1920×1080), banner2:1 (`game_banner`, minimum1080×540), cover3:4 (`square_image`, minimum540×720) and launcher cover4:5 (`game_cover_4x5`, minimum576×720). Larger images with the same ratio are accepted. Upload each composition through MCP and assign its returned media_id. Processing only creates sizes of that composition, never automatic crops into other ratios. Section backgrounds and wallpapers are optional. Read `list_store_param_defs.artwork_policy` and publication preflight. Supply four real gameplay screenshots and a trailer.

## Store review versus App readiness

Store review approves the public page, descriptions, artwork and DLC presentation.
Its email is explicitly labelled Store; it does not certify any executable.
App readiness is a separate email after the imported ZIP and extracted build pass
antivirus, apply succeeds and CDN synchronization completes. The scheduler checks
every ten minutes and retries delivery boundedly; it never activates anything.
The owner may then activate through panel/MCP and select a pre-launch phase
(e.g. Closed Beta) or launch (Production), using the phase catalog, not hardcoded IDs.
`activate_release` is independent from Store publication. To launch: select the
production phase, choose `set_game_launch_plan` available with an exact launch_date,
check `validate_publication`, submit review and poll deliveries. Base free-to-play
pricing is independent of access phase and optional paid unlock DLCs. WWW Novedades
includes only available recent releases; a Coming Soon page never enters that list
solely because its page was approved.

## Activation keys for a beta

See [phases, rounds and key scopes](https://github.com/victorlamas/idcgames_panel_ia/blob/main/docs/mcp/GAME_PHASES_ROUNDS_KEYS.md). The live
`get_game_catalogs` explains phase access and key scopes; `list_game_rounds`,
`create_game_round` and `select_game_round` manage proposed rounds. Review and
delivery are required before treating a selection as live.

Use `create_activation_key_batch` only after the owner authorizes the quantity,
product and lifetime. Omit `dlc_id` (or use `0`) for the base game; a positive
`dlc_id` must belong to that game and unlocks only that DLC, never the base game.
Use `access_scope=round` with its exact `status_id` and owned `round_id` for one
round; `phase` covers all current/future rounds and re-entry to that phase.
`game` with `status_id=0` covers all phases. Omitted scope preserves legacy
phase-exit cutoffs. `valid_until` also limits code validity.
Keep a stable idempotency key and reconcile uncertain responses before reissuing.
Retrieve codes into a private local file, never a log or public asset. Creating
or retrieving a batch does not claim or redeem it and does not prove redemption
works; test those actions separately with an explicitly authorized player.


---

# Game phases, access rounds and keys

The panel owns phase/round identities; Auth owns player access and keys.
Store approval, App readiness, phase, round and price are independent.
**Open does not mean free.** Paid base games and optional DLCs keep their own
license requirements.

Call `get_game_catalogs`: `statuses` lists selectable real phases, `phases` explains each
phase's label, access rules, flags and round support, and `activation_key_scopes`
explains key behavior. Always discover IDs; never infer them from labels or tiers.

| Phase code | Intended access |
| --- | --- |
| `developing` | Prepare and test the IDC SDK with an authorized developer account. |
| `coming_soon` | Public Store and estimated date; no general play or sales. |
| `closed_alpha` | Valid base-game key or developer only. No subscription trials/general sales. The migration allocates its ID without renumbering existing phases. |
| `closed_beta`, `limited_beta` | Restricted testing: keys, developers and eligible Silver/Gold trials. |
| `early_access` | Historical non-playable announcement. |
| `play_early_access` | Playable Early Access: paid-game purchase or valid license/key, without Gold or new subscription trials bypassing purchase. Existing trial windows retain their expiry. Explicitly free games remain free. |
| `open_beta` | Open testing, still subject to the game's free/paid requirements. |
| `production` | Commercial launch, free or paid as configured. |
| `maintenance`, `maintenance_playable` | Operational maintenance without/with configured play. |
| `inactive` | No new sales; existing owners retain only the access Auth permits, not reinstall rights. |
| `not_in_idc` | Informational availability without IDC gameplay. |

Catalog flags are not a player's entitlement. Auth alone determines `can_play`.

## Demos are separate games

**Demo is not a phase.** Call `create_game(demo=true, id_commercial_game=...)`
with the full game's IDC ID owned by the same studio. The demo has its own game
ID, App/releases, keys and Store under `/demo/<full-game-slug>/` (with the normal
language prefix). Only one demo per full game is supported. Discover and select
its real phase separately, such as Development, Closed Alpha, Closed Beta or
Commercial release. A full-game key or DLC does not implicitly license its demo,
or vice versa. The historical `demo` status row and existing legacy keys remain
readable; do not use that status for new games, phase/round keys or rounds.

## Operational access errors

`maintenance` means gameplay is unavailable, including for developer accounts;
`maintenance_playable` intentionally permits configured gameplay. Auth must
enforce the operational block on login and revalidate existing game sessions.
Clients must show the public `game_maintenance` error as a localized maintenance
notice and offer retry later, rather than suggesting a purchase or a password
change. Licenses, DLC ownership and keys never override maintenance. A phase
selection remains a draft until publication and successful Auth delivery; verify
the delivery before assuming a block or reopening has taken effect.

## Prepare and publish rounds

Rounds are optional; migrations never enable them on existing games.
Each round belongs to one game/phase, has an immutable ID and an automatically
assigned number, and moves from `planned` to `open` to permanently `closed`.

1. `list_game_rounds(game_id)` shows history, selected draft and published round.
2. `create_game_round(game_id, phase_id, name)` prepares a round. The same
   phase/name returns the same identity, even if closed; a new round needs a new
   name. The limit is 250 retained round identities per game.
3. Select the phase with `update_game(status=phase_id)`, then call
   `select_game_round(rounds_enabled=true, round_id=returned_id)`.
4. Check Store/App prerequisites, call `submit_game_for_review`, and poll
   `get_game_review` until approval and required WWW/Auth deliveries are sent.
   Selection alone does not change published access.
5. Approval opens the selected round and permanently closes the previous one.
   Never reuse or reopen a closed ID.

Publishing `rounds_enabled=true, round_id=null` creates an interval without
player access (`playable=false`). Disabling rounds and publishing also closes
the old round; old round keys never become permanent. Panel users have the same
controls in **Game Configuration → Phases and rounds**. Staff MCP keeps its own
authorization/publication workflow; round management does not bypass approval.

## Key scopes

Choose explicitly when calling `create_activation_key_batch`:

| `access_scope` | `status_id` | `round_id` | New key's scope |
| --- | --- | --- | --- |
| `round` | Exact positive phase ID | Required owned round | Only that round; never revives after closure. |
| `phase` | Exact positive phase ID | Omitted | All present/future rounds of that phase, including later re-entry. |
| `game` | `0` | Omitted | This product in every phase and round. |
| Omitted | Historical type | Omitted | Legacy phase-exit cutoffs; existing grants are never expanded. |

Omit `dlc_id` or use `0` for the base game. A positive ID must belong to the
game. A DLC key licenses only that DLC and never grants the base game.
Expiry, revocation, bans, `playable=false` and intervals without an open round
still apply to every scope.

Keys can be prepared for a planned round. Auth registers an inactive reference
from authenticated issuance; only an approved signed snapshot can open it.
Issuance, claim and explicit redemption are separate. Never redeem, purchase
or grant rights automatically to test issuance. Store codes privately, never in
chat/logs/Git. Use stable idempotency keys and reconcile uncertain results with
`list_activation_key_batches` / `get_activation_operation` before reissuing.
`get_activation_keys` retrieves codes; `revoke_activation_key` revokes one.

## Receiver contract

Optional `game.data.access_policy` is `standard|keys_only|purchase`.
`rounds_enabled` is boolean. `access_round` is null or
`{id:string, phase_id:int, number:int, name:string}`. Only the approved round is
sent. Explicit null closes the current round; absence preserves legacy state.
No rounds array is sent. IDs cannot change game/phase or revive after closure.
See `docs/web-sync/content-v2.schema.json` for the normative shape. WWW displays
metadata; Auth enforces rights and invalidates affected sessions/caches.

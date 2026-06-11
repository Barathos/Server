# Advanced Loot (AutoLoot) Handoff — 2026-06-10

Continuation handoff for the all-features Advanced Loot system. The previous
handoff (`docs/autoloot-inline-ui-third-party-handoff-2026-06-09.md`) covers
the original blank-cell investigation; this doc supersedes it for current
state.

## Project Layout

| Thing | Where |
|---|---|
| Repo / branch | `D:\Codex\Apps\EQEmu-feature-all`, branch `codex/features-all` (NOT pushed to remote; ~35 local commits ahead) |
| Client DLL source | `client_files\native_autoloot\eq-core-dll\src\core_autoloot_native.h` (single header, ~11k lines) |
| Client UI XML | `client_files\native_autoloot\ui\EQUI_NativeAutoLootWnd.xml` (+ patched `EQUI_Animations.xml`, `EQUI_Templates.xml`, `nal_pieces07/11.tga` in the same folder) |
| Server autoloot | `zone\autoloot_manager.cpp/.h`, gate in `zone\corpse.cpp` (`IsManualLootLocked` call site) |
| Patcher manifest | `features\all-features\patcher.yml` (currently Build 79, auto-increments on publish) |
| Patcher client (C#) | `features\patcher\eqemupatcher\EQEmu Patcher\EQEmu Patcher\MainForm.cs` |
| Public feed | `http://47.181.1.223:8091/patcher/all-features/` (server is REMOTE at that IP; deploy scripts upload) |
| Local test client | `D:\Testbed 2 Equip test` (user often runs TWO instances two-boxing) |
| Secondary client | `D:\EQClients\EQClient-All-Features` (deploy target only) |
| Live EQ install (art source) | `D:\SteamLibrary\steamapps\common\Everquest F2P\uifiles\default` |
| Reference screenshots | `docs\live-advloot-reference\*.jpg` (Live's window, menus, filters) |
| Client diagnostics log | `D:\Testbed 2 Equip test\native_autoloot.log` |

## Workflows

Build client DLL:
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\Codex\Apps\EQEmu-feature-all\client_files\native_autoloot\eq-core-dll\eq-core-dll-visualstudio2022.sln' /p:Configuration=Release /p:Platform=Win32 /m /clp:ErrorsOnly
```

Build server: `cmake --build build\win-msvc --config Release --target zone -- /m` (and `world`).

Local client deploy loop (authorized by user): kill only eqgame whose Path
starts with `D:\Testbed 2 Equip test` (NEVER the THJ instance at
`D:\Codex\Apps\THJ Copy\client\THJ`), copy dinput8.dll + window XML into both
clients, then relaunch for the user:
`Start-Process 'D:\Testbed 2 Equip test\eqgame.exe' -ArgumentList 'patchme' -WorkingDirectory 'D:\Testbed 2 Equip test'`.

Publish (patcher only): `D:\Codex\Apps\EQEmu-feature-workspaces\publish-testbed-project.ps1 all-features -PatcherRepo D:\Codex\Apps\EQEmu-feature-all -SkipServerDeploy`.
Full deploy (server apply + RESTART — needs explicit user authorization every
time): add `-ApplyServer -ApproveServiceRestart`. After any publish, commit the
auto-bumped patcher.yml ("Bump all-features patcher to Build NN").

## Client Architecture (the important part)

Transport is chat-tunneled both ways: client sends `/say #advloot ...`
commands; server replies with `ADVLOOT|...` chat lines intercepted by a
`CEverQuest__dsp_chat` detour. Per-frame work runs from a
`ProcessGameEvents` detour ("the pulse") — **one unhandled fault in the pulse
permanently disables it for the session** (`gNativeAutoLootPulseFaulted`), so
every new engine call reachable from the pulse must be SEH-wrapped.

The loot window UI is built from **pooled XML controls** positioned over
CListWnd cells every pulse (`SyncInlinePool`): checkbox pools, Live-art icon
buttons (green check/red X/hand/dice/gear from `nal_pieces*.tga`), and
per-row item-icon buttons whose NormalDecal is a private XML copy of
A_DragItem (`NAL_DragItem_R*`, set via `SetCurCell`). Same pattern powers the
Edit Filters window pools and the right-click popup menu window.

### Engine facts (hard-won; do not rediscover)

- BROKEN in this client (fault on call): `pSidlMgr->FindAnimation`,
  `CXWnd::DrawColoredRect`, `CreateXWndFromTemplate` path (`SidlPiece` is
  null on instances). Never call them.
- PROVEN working: GetChildItem, GetItemRect, GetScreenRect/ClipRect, Show,
  SetCurSel/GetCurSel/GetItemData/SetItemText/AddString/DeleteAll,
  `CComboWnd::GetCurChoice`, `CTextureAnimation::SetCurCell`, direct struct
  writes (Location, Checked).
- `CListWnd` fixed row height lives at **+0x21C** (stock 14; we hold main
  lists at 36, menu 24, manage 30 — reapplied each pulse). `GetItemHeight()`
  is NOT the storage (returns 0).
- `CButtonWnd` NormalDecal animation instance pointer at **+0x240**
  (Normal +0x228, Pressed +0x22C, Flyby +0x230, PressedFlyby +0x238).
  Template animation instances are SHARED between buttons → per-row XML
  animation copies for item icons.
- Child `Location` is relative to the window CLIENT origin (≈ −3,−18 vs
  outer rect — self-calibrated at runtime), and the engine CACHES absolute
  screen rects: after writing Location, toggle `Show(0,1)/Show(1,1)` in the
  same pulse or nothing moves.
- Animations/ButtonDrawTemplates must be defined in `EQUI_Animations.xml` /
  `EQUI_Templates.xml` (window-XML definitions are silently ignored), every
  texture needs a `<TextureInfo>` declaration, and **active UI skins
  override those two files** — the patcher now deletes skin-local copies
  (see patcher notes).
- Combo/list selection notifies WndNotification with **message 33**
  (32 = dropdown open); the payload is NOT the index — read GetCurChoice.
- `<Font>N</Font>` works on any control; list text follows it but row
  height does not (hence +0x21C).
- Build gotchas: never ASSIGN `CXRect` (operator= declared but unlinkable —
  copy-initialize only); no locals with destructors (std::vector etc.) in
  functions containing `__try` (C2712) — use class members.

### Patcher

The C# patcher self-updates from the feed hash. It supports **wildcard
deletes** (single `/*/` segment, e.g. `uifiles/*/EQUI_Animations.xml`) that
expand client-side and never delete patcher-managed paths; the all-features
manifest uses them to clean skin-local `EQUI.xml` / `EQUI_Animations.xml` /
`EQUI_Templates.xml`. Project deletes are declared under `deletes:` in
patcher.yml and pass through `New-WorkspacePatcherDeployment.ps1` verbatim.

## Server Architecture Notes

`AutoLootManager` (zone): LootEntry keyed by entry_id with corpse_id,
loot_slot, item_id, icon_id, owner/master character ids, `group_id` (carries
the RAID id when `raid_party` is set), votes map, free_grab, state strings
(waiting/ask/rolling/freegrab/inventory_full/lore/failed).

- **Corpse locking**: `IsManualLootLocked(corpse, slot, looter, reason)` in
  the OP_LootItem path — personal entries only open for their owner, shared
  stay locked until distributed, Free Grab opens to party members who can
  see the entry; slot matching falls back to item-id when unambiguous. NO GM
  exemption (it used to hide the feature from the GM-flagged test chars).
- **Raids** (Build 78, NOT yet raid-tested in-game): killer's raid takes
  precedence over group; master order = designated → raid leader →
  is_looter members → any candidate; `#advloot master set/clear` raid-aware;
  coin splits via `Raid::SplitMoney` within the looter's raid group. Corpse
  loot rights (`/raidloot`) gate participation.
- **Auto-show**: `EntryNeedsDecisionForClient` — master always for waiting
  shared entries; other members when their saved filters don't cover the
  item ("Unfiltered Only" semantics).
- **Filter sharing** (Build 79): `#advloot filter share` (to target, 5-min
  offer in `m_pending_filter_shares`), `accept merge|replace`,
  `copyfrom <name>` (same-account, offline OK). Share button in filters
  window.
- Loot failures send red chat ("inventory is full" / lore / instance /
  partial-stack remaining).

## 2026-06-10 Tester Report — Root Causes (all traced, fixes built)

Three reports about the shared loot section; investigated with the code +
`native_autoloot.log` evidence. Fixes compiled clean (zone + DLL) and the
DLL/XML were copied to both local clients. **Server side needs a deploy +
restart and the patcher needs a publish (Build 80) — both pending user
go-ahead.**

1. **AG/AN filters never fired without the master's AR; entries never timed
   out.** Root cause: `QueueCorpseEntries` seeds member votes from saved
   filters but only ever called `ResolveSharedVote` when the MASTER's
   `auto_ask_roll` was set. Without it the entry stayed `waiting` with
   `vote_started_at = 0`, and `Process()` only times out entries with a
   started vote — so fully-filtered items sat until corpse rot, and any
   later single manual vote completed the (invisible) seeded set instantly.
   FIX (zone/autoloot_manager.cpp): after seeding, if every eligible
   member's vote is non-Unset AND at least one is Need/Greed, set state
   `ask` + `vote_started_at` and resolve at queue time. Partial filter
   coverage still waits for the master (Live-like); all-Pass (everyone NV)
   stays `waiting` so the master can still Give/Free Grab.

2. **"Set all" appearing to apply the wrong action — NOT a mapping bug.**
   Verified three ways: XML choice order == client mapping == server
   actions; the `DIAG set-all` log shows a clean 0,1,2,3,4 walk reading
   correctly and `choice=0` sending `action N need` to every row; personal
   `choice=3` correctly sent `alwaysgreed`. The scrambled RESULTS were
   emergent: the other two boxed chars' stale AG/AN filters had pre-voted
   invisibly (bug 1), so the first manual action resolved every roll using
   those hidden votes, and the "won with Need/Greed" message reflects the
   WINNER's pool, not the clicked action. Fixing 1 + 3 removes the
   confusion (rolls now resolve at kill time, and filters are manageable
   again so stale ones can be cleared).

3. **Filter list unusable past 12 entries ("last 3 rows have no buttons or
   pictures", "can no longer add").** Root cause: the Edit Filters window
   has `kAALRulePoolRows = 12` pooled XML control rows that were mapped to
   ABSOLUTE list rows 0–11 — rows 13+ never got checkboxes/icons/remove
   buttons, and scrolling didn't help. There is NO server or DB cap
   (composite PK table, unbounded `filter set`), and the per-line
   `ADVLOOT|filter|` sync has no truncation — adds always worked; they just
   rendered bare (the list sorts by decision then item_id, so new filters
   often landed in the broken zone → "can't add" perception). FIX (DLL):
   `NativeAutoLootRulesWnd::Layout` now assigns the 12 pool rows to VISIBLE
   list rows (scroll-aware cursor, same idea as `SyncInlinePool`) with a
   `RulePoolListRow[]` click-routing map used by `HandleRuleCell`;
   `RefreshRows` no longer caps per-row state at 12. The tester promptly
   hit the residual (window resized to show 15 rows > 12 pools), so the
   pool was expanded 12 → 24 rows (Build 82): `kAALRulePoolRows = 24`,
   generated `AALR_CB/RM/IB_R12..R23` buttons + Pieces in the window XML,
   `NAL_BDT_ItemF_R12..R23` templates, and `NAL_DragItem_F12..F23`
   animation copies (~2.8k lines each; EQUI_Animations.xml grew 2.0 →
   2.6 MB). 24 rows covers a ~864px-tall list (roughly fullscreen 1080p);
   beyond that rows go bare again.

   Also noted for later: shared-section "Leave All" is master-gated per row
   (`leave` action) — non-masters get a red error per row, which reads like
   the button "did something weird". Consider mapping non-master Leave All
   to a Pass vote or disabling the button for non-masters.

4. **Scrolling the filters window scattered the pooled checkboxes
   diagonally** (tester screenshot, same morning; "scroll back fixes it").
   Root cause: `CListWnd::GetItemRect` returns LIST-RELATIVE rects in this
   client, and every rect helper guessed relative-vs-absolute per cell with
   `cell.left/top < list origin - 4`. Right-hand columns (AN..Del start at
   relative X 324–508) of lower rows (relative Y > ~400) fail BOTH
   conditions, get misread as absolute, and land at garbage positions
   inside the list — also stealing pool slots since the bogus rect passes
   the clip test. Worse when scrolled (rows below the viewport flip modes)
   or when the window sits high on screen (small list-origin Y) — the
   latter is very likely the real mechanism behind the old "bottom-row
   dead clicks" mystery in the MAIN window, which shares the same helper
   pattern. FIX: `NativeAutoLootListRectIsRelative` decides the mode from
   COLUMN 0's left edge (relative mode reads ~0 there regardless of scroll
   position or column); applied to `NativeAutoLootCellControlRect`,
   `GetInlineCellDrawRect`, `GetIconCellDrawRect`, and the right-click
   menu anchor. Known residual: mode detection misreads if the window is
   dragged almost fully off the LEFT screen edge (list origin X < 4).

## 2026-06-10 Tester Report Batch 2 (evening) — Root Causes

Tester confirmed AG/AN rolls now work for group AND raid. New findings, all
server-side (zone), fixed together in one commit:

1. **Ask disabled the voting it was meant to start.** The `ask` action
   called `Corpse::Lock()`, and the snapshot computed `locked` from
   `corpse->IsLocked()` — which zeroes `canvote` for every member, so ND/GD
   stayed dead after Ask (AN/AG still worked because those buttons are not
   gated on canvote). Worse, the no-winner/ineligible-winner resolution
   paths never UnLock, leaving the corpse GM-locked until decay. FIX:
   removed the `Corpse::Lock()` from ask (`IsManualLootLocked` already
   gates the shared slots) and shared rows now ignore `IsLocked` in the
   snapshot's `locked` computation.
2. **Members could not pre-vote.** `canvote` required `roll_active`, but
   the server accepts votes any time (first vote starts the countdown) and
   saved filters already pre-vote. FIX: `canvote` now only needs
   shared+eligible+!freegrab. Client needs no change ("Need is available
   once Ask/Roll starts" branch keys off canvote).
3. **One member's AN/AG lit everyone's checkbox.** The snapshot sent the
   per-entry `rule` field, which `RecordSharedVote(set_always_rule)`
   overwrote with the last always-setter's filter. FIX: shared rows now
   serialize the VIEWER's own saved filter; the entry.rule overwrite was
   removed (NOT a boxer artifact — tester guess was wrong, real bug).
4. **Non-master could Ask/Give.** `HandleSharedLootAction`, the snapshot
   master flag, and SendManageInfo all had `Admin() >= GMAdmin` bypasses —
   the test characters are GM-flagged, same masking issue as the old
   corpse-lock GM exemption. FIX: removed all three bypasses.
5. **Dynamic (live/forged) items could not be filtered.** AG/AN/NV were
   deliberately skipped for `dynamic_instance` items everywhere. These
   per-instance rolls share a stable TEMPLATE item id, so filters now key
   on the template ("Always Greed any roll of this base item") — all
   dynamic_instance filter gates removed (queueing, vote seeding, action
   handlers, leave-never, auto-loot/leave, auto-show, rule refresh). The
   filter list shows the template's base name.
6. **Roll timeout 60s → 180s** (kNeedGreedSeconds) per tester feedback
   that 60s is too tight once a roll is underway.

Still open from this batch:
- **Gearscore lines missing from item displays since the 10:15 deploy**
  (tester report). The zone binary deployed at 10:15 was built from the
  working tree, which contained ~1.7k UNCOMMITTED lines of item_power
  operator-search work (search APIs + migration v25 adding item_power
  indexes + item_rarity table). Local analysis says the send path
  (`SendItemPowerTransport` → `TryBuildTransportMessage`) is untouched and
  the migration is additive — no smoking gun found from code alone.
  Diagnosis needs either read-only SSH to the testbed box (check world
  boot log for migration v25 errors + `rule_values`
  CustomFeatures:GearScoreEnabled + item_power table state) — SSH was
  DENIED by the permission layer without explicit user approval — or an
  in-game repro on the local testbed client (loot any item, then read
  native_autoloot.log for "ItemPower cached" lines).
- **Cosmetic: scrolling the filters list draws the partially-scrolled top
  row over the column headers** (engine-drawn row text AND our pooled
  controls). Likely cause: engine wheel-scroll offsets are not multiples
  of our forced 36px row height (+0x21C), so the top row renders shifted
  up into the header band, and CListWnd's clip includes the header area.
  Options: snap the scroll offset (offset unknown — unproven engine
  territory), or inset our pool-control clip by ~20px (fixes controls but
  engine text still overlaps). Deferred.

## Open Items

1. **Deploy the 2026-06-10 fixes**: server apply + restart (zone binary)
   and patcher publish Build 80 (dinput8.dll) — needs user authorization.
   In-game two-box verify: (a) item on both chars' AG/AN lists rolls
   instantly at kill with no AR set; (b) Edit Filters window with 15+
   filters shows working buttons on every visible row while scrolling.
2. **Bottom-row dead clicks** (tester report, Build 79 shipped mitigations,
   root cause UNVERIFIED): FitListColumns now only reapplies on width change
   (scrollbar-flicker oscillation theory) + identity-fallback click routing
   (`PoolListRow` maps) + logging — a recurrence writes
   `DIAG pool fallback ...` lines to native_autoloot.log naming what the
   click hit. Check the log before theorizing. NOTE: two strong suspects
   now: (a) the GetItemRect relative/absolute misread fixed in finding 4
   above (lower rows flip modes when the window sits high on screen) —
   likely RESOLVED by that fix, retest before more theorizing; (b) pool
   exhaustion — the personal list has only `kAALPoolPersonalRows = 8`
   pooled rows (shared has 14); a tall-resized window showing more rows
   than the pool leaves bare/dead rows.
3. **Raid mode needs an in-game test** (two-box `/raidinvite`).
4. **Branch not pushed** to the `barathos` remote.
5. Client status line shows raids as "grouped" (cosmetic).
6. Diagnostics still in the DLL (log-only: pulse counters, notify budget,
   set-all traces) — useful for tester reports; strip someday.
7. Live-parity stretch ideas: taller header band typography, AR column icon
   header (Header_AutoRoll), engine context menus (offsets unverified).

## Working Discipline (this is what made the project move)

Never stack unproven engine calls — probe one unknown at a time with SEH +
log evidence (`native_autoloot.log`), read the log before theorizing, and
prefer XML-declared controls + struct writes over engine functions. When a
report comes in: reproduce locally via the two-box testbed, read the DIAG
lines, then fix. Commit per verified milestone; deploys that restart the
server always need the user's explicit go-ahead.

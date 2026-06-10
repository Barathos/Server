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

## Open Items

1. **Bottom-row dead clicks** (tester report, Build 79 shipped mitigations,
   root cause UNVERIFIED): FitListColumns now only reapplies on width change
   (scrollbar-flicker oscillation theory) + identity-fallback click routing
   (`PoolListRow` maps) + logging — a recurrence writes
   `DIAG pool fallback ...` lines to native_autoloot.log naming what the
   click hit. Check the log before theorizing.
2. **Raid mode needs an in-game test** (two-box `/raidinvite`).
3. **Branch not pushed** to the `barathos` remote.
4. Client status line shows raids as "grouped" (cosmetic).
5. Diagnostics still in the DLL (log-only: pulse counters, notify budget,
   set-all traces) — useful for tester reports; strip someday.
6. Live-parity stretch ideas: taller header band typography, AR column icon
   header (Header_AutoRoll), engine context menus (offsets unverified).

## Working Discipline (this is what made the project move)

Never stack unproven engine calls — probe one unknown at a time with SEH +
log evidence (`native_autoloot.log`), read the log before theorizing, and
prefer XML-declared controls + struct writes over engine functions. When a
report comes in: reproduce locally via the two-box testbed, read the DIAG
lines, then fix. Commit per verified milestone; deploys that restart the
server always need the user's explicit go-ahead.

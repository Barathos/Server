# AutoLoot Inline UI Third-Party Handoff - 2026-06-09

## Context

This handoff is for reviewing the Advanced Loot / AutoLoot native client UI in the all-features project.

Repo:

```text
D:\Codex\Apps\EQEmu-feature-all
```

Branch:

```text
codex/features-all
```

Current local state when this handoff was written:

```text
branch is ahead of barathos/codex/features-all by 29 commits
worktree was clean before creating this handoff doc
```

Current patcher build:

```text
Build 69
```

Public tester feed:

```text
http://47.181.1.223:8091/patcher/all-features/
```

Matching local test client:

```text
D:\EQClients\EQClient-All-Features
```

## Problem To Review

The AutoLoot window is trying to match Live EverQuest's Advanced Loot row layout, where each row has clickable square choices inline with the loot item:

```text
Personal:
Item | Loot | Leave | AN | AG | Never | NPC Name

Shared:
Item | Status | Action | Mgr | AR | ND | GD | NO | AN | AG | NV | NPC Name
```

The bottom XML checkboxes, such as `Apply Filters` and `Group by NPC`, render correctly. The inline list row cells have been difficult:

- Text fallback showed `[ ]` / `[X]`, but the user wants actual checkbox/square visuals inline like Live.
- Build 68 blanked the row cells because it expected overlay controls to render, but they did not appear in-game.
- Build 69 now uses direct native drawing in `PostDraw`, but this still needs third-party review and in-game validation.

The user also reported that the shared section had a `Mana` column. That was actually the `Manage` column getting clipped. It was changed to `Mgr`.

## Most Relevant Files

Native AutoLoot implementation:

```text
D:\Codex\Apps\EQEmu-feature-all\client_files\native_autoloot\eq-core-dll\src\core_autoloot_native.h
```

Native DLL output bundled to clients:

```text
D:\Codex\Apps\EQEmu-feature-all\client_files\native_autoloot\eq-core-dll\bin\dinput8.dll
```

AutoLoot XML window:

```text
D:\Codex\Apps\EQEmu-feature-all\client_files\native_autoloot\ui\EQUI_NativeAutoLootWnd.xml
```

Patcher manifest:

```text
D:\Codex\Apps\EQEmu-feature-all\features\all-features\patcher.yml
```

Native EQ class/function declarations:

```text
D:\Codex\Apps\EQEmu-feature-all\client_files\native_autoloot\eq-core-dll\src\EQClasses.h
D:\Codex\Apps\EQEmu-feature-all\client_files\native_autoloot\eq-core-dll\src\EQClasses.cpp
D:\Codex\Apps\EQEmu-feature-all\client_files\native_autoloot\eq-core-dll\src\eqgame.h
D:\Codex\Apps\EQEmu-feature-all\client_files\native_autoloot\eq-core-dll\src\MQ2Globals.h
D:\Codex\Apps\EQEmu-feature-all\client_files\native_autoloot\eq-core-dll\src\MQ2Globals.cpp
```

## Current Key Code Locations

Column enums:

```text
core_autoloot_native.h:139-153
```

Inline cell spec and square-cell registration:

```text
core_autoloot_native.h:250-269
```

Shared list column sizing, including `Mgr`:

```text
core_autoloot_native.h:336-379
```

NativeAutoLootWnd constructor and vtable hooks:

```text
core_autoloot_native.h:384-425
```

Current `PostDraw` hook:

```text
core_autoloot_native.h:437-441
```

Draw helper declarations:

```text
core_autoloot_native.h:686-688
```

Current inline draw implementation:

```text
core_autoloot_native.h:7268-7361
```

List click routing:

```text
core_autoloot_native.h:7363-7492
```

Row population:

```text
core_autoloot_native.h:7499-7652
```

Personal row square registration:

```text
core_autoloot_native.h:7548-7552
```

Shared row square registration:

```text
core_autoloot_native.h:7599-7605
```

XML shared list:

```text
EQUI_NativeAutoLootWnd.xml:155-185
```

Patcher build version:

```text
patcher.yml:4
```

## Recent Commits Directly Related To This

```text
3ac8a0dce Bump all-features patcher to Build 69
aae85e5a0 Draw AutoLoot row choices directly
ca7dca347 Bump all-features patcher to Build 68
cdb049e8a Overlay AutoLoot row choices with checkbox controls
031d0d088 Bump all-features patcher build to 67
a7a4595c1 Render AutoLoot choices as checkbox icons
```

## Approaches Already Tried

### 1. `CListWnd::SetItemIcon` per subitem

Commit:

```text
a7a4595c1 Render AutoLoot choices as checkbox icons
```

Idea:

- Use `CListWnd::SetItemIcon(row, column, animation)` with `A_CheckBoxNormal`, `A_CheckBoxPressed`, or `A_CloseBtnNormal`.

Observed result:

- Did not visibly render the inline cells in-game.
- The code path was not enough for this list/subitem layout.

Relevant older declarations still exist:

```text
eqgame.h:445 CListWnd__SetItemIcon_x
MQ2Globals.h / MQ2Globals.cpp CListWnd__SetItemIcon
```

### 2. Clone real checkbox controls and overlay them over list cells

Commit:

```text
cdb049e8a Overlay AutoLoot row choices with checkbox controls
```

Idea:

- Clone the existing `AALW_ApplyFiltersCheck` XML checkbox with `CSidlManager::CreateXWndFromTemplate`.
- Move cloned `CButtonWnd` controls over the visible `CListWnd` cells every frame.

Observed result:

- Build succeeded, but in-game Build 68 still showed blank inline cells.
- Likely issue: cloned child controls were either behind the list, not accepted into the draw order, clipped, or otherwise not rendered by this old client UI path.

Cleanup note:

- The offset for `CSidlManager__CreateXWndFromTemplate` still exists in the branch from that attempt, but current `core_autoloot_native.h` no longer uses it.
- It can be removed if the third-party review confirms the clone approach is dead.

### 3. Current Build 69: direct draw in window `PostDraw`

Commit:

```text
aae85e5a0 Draw AutoLoot row choices directly
```

Idea:

- Store each row/cell needing a square in `gNativeAutoLootInlineCellSpecs`.
- Leave list cell text as a blank spacer.
- Hook `NativeAutoLootWnd::PostDraw` by setting vtable index `3`.
- For each registered cell:
  - call `CListWnd::GetItemRect(row, column)`
  - convert to screen coords if needed
  - clip to `list->GetScreenClipRect()`
  - draw `A_CheckBoxNormal` / `A_CheckBoxPressed` / `A_CloseBtnNormal` if animation lookup succeeds
  - otherwise draw a small colored rectangle fallback with `CXWnd::DrawColoredRect`

Review concerns:

- Confirm vtable index `3` is the correct place for `PostDraw` in this client and that returning `1` is okay.
- Confirm drawing in parent `PostDraw` happens after the list contents, not before/behind them.
- Confirm `CListWnd::GetItemRect(row, column)` coordinate space. The current code tries to detect relative coordinates and add `list_rect`.
- Confirm `CTextureAnimation::Draw(CXRect, CXRect, color, alpha)` parameters are correct for this client.
- Confirm `NativeAutoLootActionCellAnimation()` can find `A_CheckBoxNormal` / `A_CheckBoxPressed` in the actual client UI at runtime.
- Confirm `CXWnd::DrawColoredRect` works during `PostDraw`; if animation draw fails, fallback should still be visible.
- Confirm no list clipping or scroll offset issue hides the markers.
- If parent `PostDraw` is wrong, consider hooking `CListWnd::DrawLine`, a custom list subclass, or a later draw point instead.

## Important Behavioral Wiring

Clicking row cells is still handled through the list click path, not through overlay controls:

```text
NativeAutoLootWnd::WndNotification
NativeAutoLootWnd::HandleListColumnClick
NativeAutoLootWnd::HandleListColumnAction
```

The key action mapping is in:

```text
core_autoloot_native.h:7383-7492
```

Examples:

```text
Personal Loot column     -> #advloot action <id> loot
Personal Leave column    -> #advloot action <id> leave
Personal AN/AG/Never     -> #advloot action <id> alwaysneed / alwaysgreed / never

Shared Status            -> free grab / loot if free grab
Shared Action            -> ask or roll
Shared Mgr               -> opens manage window
Shared AR                -> toggles auto-roll filter
Shared ND/GD/NO          -> need / greed / no vote
Shared AN/AG/NV          -> saved filters
Shared NPC Name          -> target corpse, Alt-click links corpse loot
```

## XML Notes

The bottom checkboxes that render correctly are defined here:

```text
D:\Codex\Apps\EQEmu-feature-all\client_files\native_autoloot\ui\EQUI_NativeAutoLootWnd.xml
```

Working checkbox examples:

```text
AALW_ApplyFiltersCheck
AALW_GroupedByNpcCheck
```

They use:

```xml
<Style_Checkbox>true</Style_Checkbox>
<Template>BDT_CheckboxWithText</Template>
<DecalSize><CX>16</CX><CY>16</CY></DecalSize>
<TextOffsetX>20</TextOffsetX>
```

The list rows are not XML controls. They are native `CListWnd` rows populated from C++.

## Build Commands

Native DLL only:

```powershell
cd D:\Codex\Apps\EQEmu-feature-all
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'client_files\native_autoloot\eq-core-dll\eq-core-dll-visualstudio2022.sln' /p:Configuration=Release /p:Platform=Win32 /m /clp:ErrorsOnly
```

Server verification used by this project:

```powershell
cd D:\Codex\Apps\EQEmu-feature-all
cmake --preset win-msvc
cmake --build build\win-msvc --config Release --target zone -- /m
cmake --build build\win-msvc --config Release --target world -- /m
git diff --check
```

Publish to the all-features public testbed:

```powershell
cd D:\Codex\Apps\EQEmu-feature-workspaces
.\publish-testbed-project.ps1 all-features -PatcherRepo D:\Codex\Apps\EQEmu-feature-all -ApplyServer -ApproveServiceRestart -RunDatabaseUpdates
```

## Deployment / Patcher Notes

The public patcher feed is generated from:

```text
D:\Codex\Apps\EQEmu-feature-all\features\all-features\patcher.yml
```

When deployment runs, it auto-increments:

```text
patchVersion
```

Client-facing files must be sourced from this repo, not copied manually into the local EQ client as source of truth.

## Suggested Review Direction

Start in this order:

1. Inspect `NativeAutoLootWnd::PostDraw()` and confirm vtable index `3` is correct and useful for drawing over child list controls.
2. Confirm whether parent-window post-draw occurs after `AALW_PersonalList` and `AALW_SharedList`; if not, the markers may still draw behind the list.
3. Confirm the coordinate space returned by `CListWnd::GetItemRect(row, column)` in this client.
4. If direct drawing is still hidden, test drawing a bright fixed rectangle in the AutoLoot window `PostDraw` to prove the hook fires and is visible.
5. If the hook fires but list-cell drawings are clipped/hidden, test drawing into the list's own draw path instead of the parent window.
6. If `CListWnd` cannot do subitem icons reliably, consider a custom row renderer or replacing the list with row-native child controls created from XML at fixed row slots.

## Known Unrelated Noise / Avoid

This branch contains many all-features systems. For this review, avoid unrelated areas unless needed:

```text
Achievements
Multiclass
Item Forge / Spell Forge
Tribute-related work
Tradeskills
HP Fix
AI NPC Response
```

The target issue is specifically inline Advanced Loot row controls in:

```text
client_files\native_autoloot\eq-core-dll\src\core_autoloot_native.h
client_files\native_autoloot\ui\EQUI_NativeAutoLootWnd.xml
```

---

## Third-Party Review Findings + Decision (2026-06-09, review session)

### Verified — stop re-checking these

- **Vtable slots are correctly addressed.** From the ordered `VFTABLE` macro in `EQUIStructs.h:101`: offset `0x0C` -> slot 3 = `PostDraw`; offset `0x88` -> slot 34 = `WndNotification` (PROVEN working, clicks route); offset `0xC4` -> slot 49 = `OnProcessFrame`. Slot 34 working validates the offset math for the others.
- **The `SetvfTable` patch is memory-safe and window-local.** `CCustomWnd` ctor calls `ReplacevfTable()` (`MQ2Internal.h:383`), which deep-copies the vtable per object before patching slot 3. No global corruption.
- **All draw calls are ABI-correct:** `CTextureAnimation::Draw(CXRect,CXRect,ulong,ulong)`, `CXWnd::DrawColoredRect(CXRect,ulong,CXRect)`, `CListWnd::GetItemRect(int,int)`, `GetScreenClipRect()` all match declarations. No crash risk.
- **Conclusion:** the bug is NOT wrong vtable index, bad signature, or memory corruption. Those are ruled out.

### Root-cause analysis

- The header labels EIGHT slots `PostDraw`..`PostDraw8` -> "slot 3 = PostDraw" is a reverse-engineer's GUESS. Whether slot 3 fires *after* the list children paint is unproven.
- **KEY INSIGHT:** Build 68 (overlay clone controls) and Build 69 (direct draw) are unrelated mechanisms yet both produce IDENTICAL blank cells. When two unrelated approaches fail the same way, the culprit is their *shared assumption*: that anything placed in the cell rects survives on top of the list's own opaque per-cell rendering. That points to a z-order/draw-order (and/or hook-firing) problem, not a per-approach detail.
- Two live hypotheses, indistinguishable by reading code alone:
  - **(A) Draw-order / hook timing** — the per-frame hook doesn't fire, or fires before the opaque list (`Style_Transparent=false`, `WDT_Inner`) paints -> our content is hidden behind the list.
  - **(B) Geometry** — `GetItemRect` coordinate space differs from what the code assumes; the relative-vs-screen heuristic at `core_autoloot_native.h:7284` mislocates cells -> clip-rejected.
- **Lead hypothesis: (A)**, based on the identical-failure reasoning above.

### Why Build 68 (clones) likely failed — stacked unverified assumptions

1. `SyncInlineCellControls` ran from `OnProcessFrame` (slot 49) — hook firing never proven.
2. `CreateXWndFromTemplate(this, ...)` assumed to parent the clone into the window's draw list — never confirmed.
3. The clone inherits `ApplyFiltersCheck`'s `AutoStretch=true` + anchor offsets -> the next layout pass fights the per-frame `Move()` and snaps buttons back to the bottom-bar anchor region.
4. Same fragile `GetItemRect` coordinate heuristic.
5. Z-order vs the opaque list.

Any one of these yields blank cells; none were isolated.

### Decision

**Go to REAL CHILD CONTROLS:** re-do the inline cells as real `CButtonWnd` checkbox children like the working bottom checkboxes (`AALW_ApplyFiltersCheck`, `AALW_GroupedByNpcCheck`, which DO render), and instrument why the Build 68 clones never showed. Direct-draw (Build 69) and hooking the list's own `PostDraw` were considered and set aside.

### RESUME HERE next session

Deferred open question — pick the path to working child controls:

1. **Diagnostic build first (recommended).** One tiny build: a single real checkbox pinned at a known fixed spot over the personal list + a hook-fire counter + `GetItemRect` logging to the status label. One in-game run proves (a) the hook fires, (b) a child control renders ON TOP of the list, (c) `GetItemRect`'s coordinate space. Then implement the full pool with zero guessing. Smallest total cost given 4 failures.
2. **Implement directly with baked-in diagnostics.** Build the full child-control implementation now (best guess) with counter/logging included.

Design ideas to weigh during implementation:

- **Parent the per-row controls to the LIST** (inherits the list clip and draws after list content -> fixes z-order) vs. to the window.
- **Strip `AutoStretch`/anchors** so `Move()` actually sticks.
- Consider a **fixed XML-declared checkbox pool** (guaranteed to render — same mechanism as the working bottom checkboxes) vs. runtime cloning.

Status: brainstorming/design NOT yet approved; NO code written in this review session. Resume by resolving the (1)/(2) choice, then design -> approve -> implement. Re-verify any cited file/line is still current before acting.

---

## UPDATE - 2026-06-09 evening: diagnostic run results + pool implementation

The diagnostic build ran in `D:\Testbed 2 Equip test` (user's actual test client; they copy dinput8.dll there manually). Log: `D:\Testbed 2 Equip test\native_autoloot.log`. Findings supersede the hypotheses above:

### Proven by the diagnostic log

- **Vtable slot 3 (PostDraw) FIRES every frame** (2,250 probe entries). Slot-49 status still unknown.
- **`pSidlMgr->FindAnimation` FAULTS on every call** in this client (access violation). This single broken call killed: the original SetItemIcon build (faulted during cell population, aborting RefreshList), Build 69's PostDraw markers (first call in DrawInlineCellMarker -> 59,100 faults), and the pulse diagnostics (unguarded call -> `gNativeAutoLootPulseFaulted` permanently disabled the pulse for the session).
- **`CXWnd::DrawColoredRect` also faults independently** (probe used no FindAnimation; 2,250 faults). So the colored-rect fallback can never render either.
- **`((PCSIDLWND)button)->SidlPiece` is NULL for XML controls** -> Build 68's CreateXWndFromTemplate clone path silently created zero controls. Build 68 mystery solved.
- Geometry was NEVER the problem: `GetItemRect`, `GetScreenRect`, `GetScreenClipRect` all work (zero rect faults).
- **WARNING:** the ProcessGameEvents pulse detour sets `gNativeAutoLootPulseFaulted` on the FIRST unhandled fault and never runs again that session. Any unproven engine call in pulse-driven code must be SEH-wrapped.

### Fix implemented (this working tree, not yet committed)

XML-declared checkbox pool positioned over list cells per pulse — uses ONLY proven primitives (GetChildItem, GetItemRect, Show, direct `Location`/`Checked` struct writes, XWM_LCLICK notification):

- `EQUI_NativeAutoLootWnd.xml`: 138 generated `<Button>` pieces — `AALW_CBP_R{0-7}C{0-4}` (personal: Loot/Leave/AN/AG/Never) and `AALW_CBS_R{0-13}C{0-6}` (shared: AR/ND/GD/NO/AN/AG/NV), `BDT_CheckboxWithText`, 16x16, declared AFTER the lists in `<Pieces>` so they draw on top.
- `core_autoloot_native.h`: pool constants/struct + `NativeAutoLootPoolSlotForColumn` (near the spec struct); constructor fetches+hides pool; `SyncInlinePool()` (replaces the draw-marker functions) maps visible spec rows -> pool rows via per-list cursor, writes parent-relative `Location` rects derived from `GetInlineCellDrawRect`, sets `Checked`, toggles `Show` on transitions; `DiagnosticPulse()` calls it from the pulse and reports `DIAG P/PD/OPF/specs/pool` to status label + log every 2s; WndNotification XWM_LCLICK routes pool-button clicks through `HandleListColumnAction` (list-click path retained as fallback).
- Removed: all FindAnimation/DrawColoredRect call sites in the draw path, the SetItemIcon probe, the clone-checkbox probe.
- Gotcha hit during build: `CXRect::operator=` is declared but not linkable — never assign CXRect; copy-initialize only.

Deployed to `D:\Testbed 2 Equip test` and `D:\EQClients\EQClient-All-Features` (dll + uifiles\default XML). Awaiting in-game verification.

### In-game verification round 2 (same evening): pool renders + clicks work

Confirmed in-game: pool checkboxes render inline, clicks route correctly (`#advloot action N loot/alwaysneed/alwaysgreed` all observed in log), counters healthy, zero faults. Two more engine facts discovered:

- **Child `Location` is interpreted relative to the window's client origin (below titlebar/border), not its outer screen rect.** Measured delta on this window: (-3,-18). Fixed with self-calibration: place first button, next pulse measure its actual `GetScreenRect`, latch the delta (`PoolCalibrated`/`PoolCorrectionX/Y` members), apply to all Location writes.
- **The engine caches each control's absolute screen position.** Direct `Location` writes do NOT take visual effect until the cache is invalidated — e.g. by a window move or a hide→show transition (user observed boxes "refresh" into place only after dragging the window). Fix: when a button's Location actually changes, toggle `Show(0,1)`/`Show(1,1)` within the same pulse (between frames, no visible flicker) — `PoolRefresh` vector in `SyncInlinePool`.

### Future refinement (after pool confirmed)

Live-style green-check/red-X art: pure XML — define custom button draw templates / decal animations in the XML and use them on pool buttons. No new C++ paths needed. SetItemIcon could be revisited only with a trustworthy CTextureAnimation* source (FindAnimation is broken; offset repair would need verified RoF2-era offsets).


# AutoLoot Handoff

Last updated: 2026-06-05

This handoff is for a fresh AutoLoot-focused chat in the all-features project:

```text
D:\Codex\Apps\EQEmu-feature-all
```

The goal is to keep improving the source-backed native AutoLoot system in the all-features bundle. Do not revive the old Perl/MQ/Lua AutoLoot script path. The intended direction is the native EverQuest AutoLoot-style window plus server-authoritative source code for every loot decision and item transfer.

## Current Shape

- Feature gate: `CustomFeatures:AutoLootEnabled`, defined in `common/ruletypes.h`.
- Server command entry points are thin wrappers in `zone/gm_commands/autoloot.cpp`.
- Main server implementation is `zone/autoloot_manager.cpp` and `zone/autoloot_manager.h`.
- DB schema is custom migration version `1`, with repair migration version `9`, in `common/database/database_update_manifest_custom.h`.
- Standalone feature SQL mirror lives at `features/autoloot/sql/001_source_backed_autoloot.sql`.
- Native client runtime is monolithic and lives under `client_files/native_autoloot/eq-core-dll/`.
- AutoLoot native XML is `client_files/native_autoloot/ui/EQUI_NativeAutoLootWnd.xml`.
- All-features patcher mapping is `features/all-features/patcher.yml`.
- AutoLoot-only feature notes are in `features/autoloot/README.md`.

The custom tables expected on the server are:

```text
custom_autoloot_settings
custom_autoloot_filters
custom_autoloot_autosell_exclusions
custom_autoloot_group_settings
custom_autoloot_audit
```

## Implemented Behavior

- Character settings: enabled/off, filter mode, debug flag, log flag.
- Filters: Keep and Ignore mapped to include/exclude rows.
- Nearby corpse scan with clamped radius, currently default `75`, max `250`.
- Server-owned queue rows for lootable corpse slots.
- Native transport lines using `AUTOLOOT|...`.
- Native window commands from buttons:
  - refresh/status
  - nearby scan
  - filters window
  - loot all / leave all
  - loot / leave / always loot / never
  - need / greed / pass / always need / always greed
- Group settings:
  - `none`
  - `solo`
  - `master`
  - `robin`
  - `killer`
  - `assigned`
  - Need/Greed on/off
  - force process/recover votes
- AutoSell preview/confirm/cancel and exclusions exist server-side.
- Native UI cleanup hooks exist for logout, UI reload, and leaving game state.

## Useful Commands

These still exist for debugging and for players while the UI is being finished:

```text
#autoloot status
#autoloot native show
#autoloot native status
#autoloot on [both|include|exclude]
#autoloot off
#autoloot mode [both|include|exclude]
#autoloot nearby [radius]
#autoloot group status
#autoloot group help
#autoloot group [none|solo|master|robin|killer]
#autoloot group assign [Character Name]
#autoloot group needgreed [on|off]
#autoloot group forceprocess
#autoloot group recover
#lootfilter keep [item_id]
#lootfilter ignore [item_id]
#lootfilter unset [item_id]
#lootfilter list [both|include|exclude]
#lootfilter mode [both|include|exclude]
#autosell preview
#autosell confirm
#autosell cancel
#autosell exclude [item_id]
#autosell include [item_id]
#autosell list
#needgreed vote [vote_id] [need|greed|pass]
```

The long-term UI goal is to reduce player reliance on commands. Keep commands as diagnostic/admin fallback, but make the native window the normal workflow.

## Recent Context And Known Pain Points

- Players saw SQL failures when `custom_autoloot_*` tables were missing. This was repaired with the custom schema repair migration, but any future branch/chat should still verify the DB version and table presence before chasing UI bugs.
- The UI was reported not to clear cleanly on logout to character select. The native runtime now has session reset and UI reset hooks, but this should be retested hard with repeated logout/re-enter and `/loadskin` or UI reload flows.
- The current native window is our custom SIDL implementation, not the old script UI. It was expanded using live-style concepts, but it still may not match the real EQ AutoLoot window closely enough.
- Nearby scan now reports scanned corpse count separately from queued loot rows. If testers still see "nothing happened," add clearer UI status rows/messages rather than only chat lines.
- The server remains authoritative for corpse eligibility. The client must not be trusted for item movement or loot rules.
- The native DLL is monolithic and shared by many all-features windows. Be careful when changing hooks or lifecycle code because it can affect Live Items, Live Spells, Achievements, Multiclass, HP Fix, etc.

## High-Value Next Work

1. Re-audit the UI against the live RoF2 AutoLoot XML blueprint.
   - User pointed to: `D:\Codex\Apps\THJ Copy\client\thj-rof2\uifiles\default`
   - Look for the live `EQUI_*.xml` AutoLoot window and compare controls/layout against `client_files/native_autoloot/ui/EQUI_NativeAutoLootWnd.xml`.
   - Keep our behavior source-backed, but borrow the expected live layout, naming, and row flow where sensible.

2. Make the native UI less command-shaped.
   - Main window should make the normal loop obvious: status, queued items, rule/action buttons, nearby scan.
   - Filters/rules window should clearly show Keep/Ignore/Unset and refresh without manual command knowledge.
   - Expose AutoSell in the UI if it is meant to be part of the player-facing AutoLoot feature.
   - Prefer server-sent status/snapshot rows over relying on chat feedback.

3. Harden logout/re-entry cleanup.
   - Confirm `NativeAutoLootDestroyRuntimeWindows`, `NativeAutoLootResetSessionRequests`, and `NativeAutoLootResetClientUiSession` actually release/hide AutoLoot and filter windows on character select, zoning, UI reload, and return to game.
   - Watch `native_autoloot.log` in the client root during repeated logout/re-enter tests.
   - If windows survive incorrectly, fix lifecycle in `client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h`.

4. Improve empty-state/debug output.
   - If nearby scan finds corpses but queues zero rows, the UI/status should say why where possible: not lootable, already queued, corpse locked, filter skipped, no item data, settings disabled, feature disabled.
   - Add server-side diagnostic strings only when `debug_enabled` is set, so normal players are not spammed.
   - Consider sending a structured native diagnostic transport instead of raw red chat.

5. Validate actual looting semantics.
   - Confirm `LootEntryForClient` uses safe source corpse loot paths and does not duplicate or bypass no-drop/group ownership checks.
   - Confirm coin loot/final corpse cleanup does not prematurely destroy a corpse while visible rows or pending votes still exist.
   - Confirm grouped modes behave correctly with split-zone or missing members.

6. Align DB/schema ownership.
   - If new AutoLoot tables/columns are needed, add them to `common/database/database_update_manifest_custom.h` and bump `CUSTOM_BINARY_DATABASE_VERSION` in `common/version.h`.
   - Mirror standalone AutoLoot-only SQL in `features/autoloot/sql/001_source_backed_autoloot.sql` when appropriate.
   - Use idempotent SQL and avoid touching tester state outside the AutoLoot custom tables.

7. Keep patcher ownership clean.
   - All-features external client files must be mapped in `features/all-features/patcher.yml`.
   - If native XML or DLL changes, rebuild `dinput8.dll`, update the repo copy, and regenerate/deploy the patcher feed.
   - Do not copy UI files directly into `D:\EQClients\EQClient-All-Features` as source of truth.

## Key Code Pointers

Server:

```text
zone/autoloot_manager.h
zone/autoloot_manager.cpp
zone/gm_commands/autoloot.cpp
zone/zone.cpp
common/ruletypes.h
common/database/database_update_manifest_custom.h
common/version.h
features/autoloot/sql/001_source_backed_autoloot.sql
```

Native client:

```text
client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h
client_files/native_autoloot/eq-core-dll/src/native_interface.cpp
client_files/native_autoloot/eq-core-dll/eq-core-dll-visualstudio2022.sln
client_files/native_autoloot/eq-core-dll/bin/dinput8.dll
client_files/native_autoloot/ui/EQUI_NativeAutoLootWnd.xml
client_files/native_autoloot/config/native_interface.ini
```

Patcher/docs:

```text
features/all-features/patcher.yml
features/autoloot/README.md
features/autoloot/patcher.yml
docs/features-all.md
docs/native-client-runtime.md
```

## Build And Verification

Server:

```powershell
cd D:\Codex\Apps\EQEmu-feature-all
cmake --preset win-msvc
cmake --build build\win-msvc --config Release --target zone -- /m
cmake --build build\win-msvc --config Release --target world -- /m
git diff --check
```

Native DLL:

```powershell
cd D:\Codex\Apps\EQEmu-feature-all
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe' client_files\native_autoloot\eq-core-dll\eq-core-dll-visualstudio2022.sln /p:Configuration=Release /p:Platform=Win32 /m
```

Local DB update smoke:

```powershell
cd D:\EQServers\EQServer-All-Features
D:\Codex\Apps\EQEmu-feature-all\build\win-msvc\bin\Release\world.exe database:updates
```

Manual client smoke:

```text
1. Patch a test RoF2 client from the all-features feed.
2. Log in and run #autoloot native show.
3. Toggle AutoLoot on from command or UI, then kill tutorialb mobs with loot.
4. Click nearby scan and confirm rows appear or an accurate empty-state is shown.
5. Try Loot, Leave, Always Loot, Never, Need, Greed, Pass.
6. Add Keep/Ignore filters and confirm the next matching drop follows the rule.
7. Test grouped modes with at least two clients if possible.
8. Log out to character select and re-enter several times; verify AutoLoot windows and cached rows do not survive incorrectly.
```

## Deploy Notes

All-features is the maintained source of truth for this combined package. For public testbed deploys, use the workspace publisher:

```powershell
cd D:\Codex\Apps\EQEmu-feature-workspaces
.\publish-testbed-project.ps1 all-features -ApplyServer -ApproveServiceRestart -RunDatabaseUpdates
```

Routine server deploys now use lightweight server backups by default. Use `-ServerBackupMode full` only before risky deploys. If the change is client-only, consider skipping server deploy and regenerating/uploading the patcher feed instead.

If client-facing files change, regenerate the patcher feed with the all-features public login host explicitly:

```powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project all-features -BaseUrl http://47.181.1.223:8091/patcher/ -LoginHost 47.181.1.223 -LoginPort 5999
.\Test-WorkspacePatcherDeployment.ps1 -Project all-features -BaseUrl http://47.181.1.223:8091/patcher/
```

## Suggested First Prompt For The New Chat

```text
We need to continue AutoLoot work in D:\Codex\Apps\EQEmu-feature-all. Read docs/autoloot-handoff.md first. Focus on making the native AutoLoot UI closer to live EQ, less command-driven, and more stable on logout/re-entry. Compare our XML/runtime against the live UI files under D:\Codex\Apps\THJ Copy\client\thj-rof2\uifiles\default. Implement fixes in the all-features repo, rebuild server/native DLL as needed, and verify with local tests before deploy.
```

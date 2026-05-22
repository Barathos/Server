# Start Here

Last updated: 2026-05-21

This is the short handoff file for new chats working on this EQEmu custom server. Read this first, then follow the links at the bottom only when a task needs more detail.

## Local Workspace

Main EQEmu source checkout for this prototype:

`D:\Codex\Apps\Everquest-autoloot-native-ui`

Isolated server runtime:

`D:\server-autoloot-native-ui`

Isolated EQ client:

`D:\Codex\Apps\Everquest-EQ-autoloot-native-ui`

Server binaries used by the local launcher:

`D:\server-autoloot-native-ui\bin`

## Project Rule

Source code owns reusable mechanics, validation, loot transfer, combat/progression math, and anything exploitable.

Database tables and rules own persistent state and inspectable configuration.

Quest scripts own zone/NPC flavor, one-off content, dialogue, and event scripting.

The native client DLL owns only the AutoLoot window presentation and sends server commands through the chat bridge.

Do not route this branch through any older external overlay AutoLoot UI code.

## Current AutoLoot State

AutoLoot is source-backed now. The old Perl `autoloot.pl` is reference only and should not be called by global quest scripts.

Important files:

- `D:\Codex\Apps\Everquest\zone\autoloot_manager.cpp`
- `D:\Codex\Apps\Everquest\zone\autoloot_manager.h`
- `D:\Codex\Apps\Everquest\zone\gm_commands\autoloot.cpp`
- `D:\Codex\Apps\Everquest\zone\corpse.cpp`
- `D:\Codex\Apps\Everquest-autoloot-native-ui\client_files\native_autoloot\eq-core-dll\src\core_autoloot_native.h`
- `D:\Codex\Apps\Everquest-autoloot-native-ui\client_files\native_autoloot\ui\EQUI_AoTAutoLootWnd.xml`

Live local quest cleanup already done:

- `D:\server\quests\global\global_npc.pl` no longer calls `plugin::auto_loot`.
- `D:\server\quests\global\global_player.pl` had no old AutoLoot call.
- `plugin::double_loot` was left alone because it belongs to `plugins\easter_event.pl`.

Actual item transfer must stay server-authoritative. Use `Corpse::AutoLootItem` or an equally safe extracted source path so lore checks, bag contents, augments, attuned/evolving state, task credit, discoverability, adventure credit, dynamic-zone restrictions, `EVENT_LOOT`, logs, and corpse item removal remain correct.

The native EQ client window is only the display/control surface. It sends server commands through the `dinput8.dll` bridge, but the server still validates every action.

## AutoLoot UI

Primary UI in this branch is the native C++/SIDL window loaded by the isolated EQ client folder, not the older overlay prototype.

Player-facing commands:

- `#autoloot native show` opens the native main window if it was closed.
- `#autoloot native status` refreshes the native snapshot.
- `#lootfilter native list both` refreshes the native filters window.
- `/lootfilter keep <item_id>` sets an item to Keep.
- `/lootfilter ignore <item_id>` sets an item to Ignore.
- `/lootfilter unset <item_id>` removes the saved rule.
- `/autosell` opens AutoSell preview behavior.
- `/needgreed vote <id> <need|greed|pass>` handles shared loot votes.

Player-facing rule language is:

- `Keep`: auto-loot this item.
- `Ignore`: leave this item alone.
- `Unset`: no saved rule, so show it in the loot window for a decision.

Implementation detail: the current DB table still stores these as `include` and `exclude` for compatibility. Do not expose those words in the UI unless working on internals.

The filters window is separate because Keep/Never lists can grow large. It currently supports item names, item IDs, selected-row `Keep / Never / Unset` changes, immediate refresh after changes, and resize through SIDL `AutoStretch` anchors.

Item icons use the EQ item icon atlas `A_DragItem`; the server emits item icon IDs through `AUTOLOOT|...` protocol lines.

## AutoLoot Persistence And Safety

Persistent state lives in custom AutoLoot tables such as:

- character settings
- item rules
- autosell exclusions
- group loot settings
- optional audit/debug rows

Transient loot rows live in zone memory. Real items remain on corpses until a server-approved transfer succeeds.

There is no stack buffer anymore. That was a Perl workaround. Source inventory insertion can stack normally; failed or blocked transfers should leave loot on the corpse.

## Build And Deploy

Build the native DLL from `D:\Codex\Apps\Everquest-autoloot-native-ui\client_files\native_autoloot\eq-core-dll` when client code changes, then deploy `dinput8.dll` and `EQUI_AoTAutoLootWnd.xml` to the isolated client folder `D:\Codex\Apps\Everquest-EQ-autoloot-native-ui`.

Build zone:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build 'D:\Codex\Apps\Everquest\build\win-msvc' --config Debug --target zone -- /m
```

Deploy zone to the isolated local server:

```powershell
Copy-Item -LiteralPath 'D:\Codex\Apps\Everquest-autoloot-native-ui\build\win-msvc\bin\Debug\zone.exe' -Destination 'D:\server-autoloot-native-ui\bin\zone.exe' -Force
Copy-Item -LiteralPath 'D:\Codex\Apps\Everquest-autoloot-native-ui\build\win-msvc\bin\Debug\zone.pdb' -Destination 'D:\server-autoloot-native-ui\bin\zone.pdb' -Force
```

The user has said EQ and server sessions can be closed when needed for future deploys. Still check running processes before copying binaries.

## Test Loop

Start the isolated local server from `D:\server-autoloot-native-ui`, usually through:

```powershell
spire.exe eqemu-server:launcher start
```

In game:

1. Launch the isolated EQ client folder with the native `dinput8.dll`.
2. Run `#autoloot native show`.
3. Kill a low-level mob.
4. Confirm the item appears in the Personal Loot table.
5. Set `Keep`, kill another matching mob, and confirm the source backend auto-loots it.
6. Set `Never`, kill another matching mob, and confirm the item remains on the corpse.
7. Open the filters window and verify resize plus `Keep / Never / Unset`.

## Broader Server Direction

The old server had good ideas but too much global script interaction and power creep. The new direction is to keep the creative feature set while making reusable systems source-owned, DB-backed, inspectable, and easier to balance.

High-priority future systems:

- Rebirth and perk trees
- DDO-style Normal/Hard/Insane zone difficulty
- Attunement/permanent gear mastery
- Voltron epics and class mastery rewards
- Solo/box-friendly progression
- Better task-window and native-window surfaces
- Safer power-budget and gear-score systems

## Useful Docs

Read next as needed:

- `D:\Codex\Apps\Everquest\docs\custom-server-vision.md`
- `D:\Codex\Apps\Everquest\docs\project-reference-map.md`
- `D:\Codex\Apps\Everquest\docs\autoloot-ui.md`
- `D:\Codex\Apps\Everquest\docs\autoloot-queue-v2.md`
- `D:\Codex\Apps\Everquest\docs\live-items.md`

External references are cataloged in `project-reference-map.md`. Use them for guidance, not direct code copying, unless we intentionally vendor or fork something.

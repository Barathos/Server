# Dynamic Quests Manifest

Feature id: `dynamic-quests`

## Owned Files

- `zone/task_manager.h`
- `zone/task_manager.cpp`
- `zone/task_client_state.cpp`
- `zone/worldserver.cpp`
- `world/zonelist.cpp`
- `features/dynamic-quests/sql/001_dynamic_quests_task_seed.sql`
- `features/dynamic-quests/quests/tutorialb/910001.lua`
- `client_files/native_autoloot/ui/EQUI_NativeDynamicQuestsWnd.xml`
- `features/dynamic-quests/patcher.yml`
- `docs/dynamic-quests.md`

## Behavior

- Builds on EQEmu `tasks` and `task_activities` for the first prototype.
- Adds lazy single-task loading on task selector/accept cache miss.
- Fixes targeted task reload so `#task reload task <id>` refreshes one task definition instead of forcing a full task reload.
- Keeps full reload through `#reload tasks` / `#task reloadall`.
- Keeps task progress and completion in existing character task tables.

## Sample Quest

- Task id: `910001`
- Title: `First Steps in Gloomingdeep`
- NPC id: `910001`
- Zone: `tutorialb`
- Objective types: `Kill`, `SpeakWith`

## Client Files

The native tracker XML is tracked and listed in `patcher.yml`, but the prototype does not require a native DLL. The built-in task journal is authoritative until the custom tracker transport is implemented.

## Test Target

- Server: `D:\EQServers\EQServer-Dynamic-Quests`
- Client: `D:\EQClients\EQClient-Dynamic-Quests`
- Database: `eqemu_dynamic_quests`

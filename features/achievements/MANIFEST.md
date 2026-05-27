# Achievements Manifest

This manifest lists the files and hook points that make up the standalone Achievements feature.

## Added Server Files

- `zone/achievement_manager.cpp`
- `zone/achievement_manager.h`
- `zone/gm_commands/achievements.cpp`
- `docs/achievements.md`

## Existing Server Files To Patch

| File | Purpose |
| --- | --- |
| `common/database/database_update_manifest_custom.h` | Add achievements-only custom DB migrations and seed data. |
| `common/version.h` | Set `CUSTOM_BINARY_DATABASE_VERSION` to `4`. |
| `zone/CMakeLists.txt` | Build achievement manager and command source. |
| `zone/command.cpp` | Register achievement command aliases. |
| `zone/command.h` | Declare the achievement command handler. |
| `zone/attack.cpp` | Update kill-based objectives for solo, group, and raid kills. |
| `zone/client.cpp` | Update skill objectives when a skill is set. |
| `zone/client_packet.cpp` | Update login-level and zone-visit objectives. |
| `zone/exp.cpp` | Update level objectives after level changes. |
| `zone/task_client_state.cpp` | Update task-completion objectives. |

## Database Objects

The custom migration manifest creates and seeds:

- `custom_achievement_categories`
- `custom_achievements`
- `custom_achievement_objectives`
- `custom_character_achievement_progress`
- `custom_character_achievements`
- `custom_achievement_audit`
- `custom_achievement_rewards`
- `custom_character_achievement_rewards`
- `custom_achievement_live_item_requests`
- `custom_account_achievement_unlocks`

## Native Client Assets

| File | Purpose |
| --- | --- |
| `client_files/native_autoloot/ui/EQUI_NativeAchievementWnd.xml` | Native SIDL window layout for Achievement browsing. |
| `client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h` | Not included in this proof branch. The current implementation is still shared runtime code and needs a `native-client-base` split. |

## Commands

- `#ach`
- `#ach window`
- `#ach status`
- `#ach categories`
- `#ach category [category_id]`
- `#ach detail [achievement_id]`
- `#ach rewards`
- `#ach claim [reward_id|all]`
- `#ach check`

## Native Transport

- `ACH|window|...`
- `ACH|category|...`
- `ACH|achievement|...`
- `ACH|objective|...`
- `ACH|reward|...`

The server owns progress, completion, and validation. The native window is only a display/input surface.

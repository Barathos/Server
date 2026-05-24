# Live Items Manifest

This manifest lists the files and hook points that make up the standalone Live Items feature.

## Added Server Files

- `zone/gm_commands/itemedit.cpp`
- `zone/gm_commands/liveitem.cpp`
- `docs/live-items.md`

## Existing Server Files To Patch

| File | Purpose |
| --- | --- |
| `common/item_instance.cpp` | Add dynamic item data storage and application. |
| `common/item_instance.h` | Declare dynamic item APIs. |
| `common/shareddb.cpp` | Add live item lookup, cache, and DB fallback. |
| `common/shareddb.h` | Declare live item cache APIs. |
| `common/ruletypes.h` | Add Live Item range and polling rules. |
| `world/world_boot.cpp` | Preserve shared item count before live fallback is enabled. |
| `zone/CMakeLists.txt` | Build `itemedit.cpp` and `liveitem.cpp`. |
| `zone/command.cpp` | Register `#liveitem`, `#itemedit`, and `#itemforge`. |
| `zone/command.h` | Declare Live Item command handlers. |
| `zone/client.cpp` | Refresh live items during relevant client flows. |
| `zone/client.h` | Declare live item refresh helpers. |
| `zone/inventory.cpp` | Refresh held/inventory live item trees. |
| `zone/lua_iteminst.cpp` | Expose dynamic item helpers to Lua. |
| `zone/lua_iteminst.h` | Declare Lua dynamic item bindings. |
| `zone/perl_client.cpp` | Expose client refresh helper to Perl. |
| `zone/perl_questitem.cpp` | Expose dynamic item helpers to Perl quest items. |
| `tests/CMakeLists.txt` | Add dynamic item tests. |
| `tests/dynamic_item_test.h` | Test dynamic item behavior. |
| `tests/main.cpp` | Include dynamic item tests. |

## Database Objects

No custom tables are required. Live items use existing `items`, `rule_values`, and `data_buckets` tables.

Optional rules SQL lives in:

- `features/live-items/sql/001_live_items_rules.sql`

## Native Client Assets

| File | Purpose |
| --- | --- |
| `client_files/native_autoloot/ui/EQUI_NativeItemForgeWnd.xml` | Native SIDL window layout for Item Forge input. |
| `client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h` | Not included in this proof branch. The current implementation is still shared runtime code and needs a `native-client-base` split. |

## Commands

- `#liveitem status [item_id]`
- `#liveitem clear [item_id|all]`
- `#liveitem clone [new_item_id] [source_item_id]`
- `#liveitem summon [item_id] [charges]`
- `#liveitem bump [item_id] [hp] [mana] [damage]`
- `#itemedit`
- `#itemforge dialog`
- `#itemforge craft ...`

## Native Transport

- `LIVEITEM|ui|open|...`

The server owns item creation and validation. The native window is only an input surface.

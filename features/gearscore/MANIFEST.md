# Gearscore Manifest

Feature id: `gearscore`

This manifest tracks the portable implementation pieces for the standalone
Gearscore feature.

## Test Target

- Server: `D:\EQServers\EQServer-Gearscore`
- Client: `D:\EQClients\EQClient-Gearscore`
- Database: `eqemu_gearscore`

## Source Files

- `common/item_power.h`
- `common/item_power.cpp`
- `zone/gm_commands/itemscore.cpp`

## Integration Points

- `common/CMakeLists.txt`: builds `item_power.*`.
- `zone/CMakeLists.txt`: builds `gm_commands/itemscore.cpp`.
- `zone/command.h` and `zone/command.cpp`: registers `#itemscore`.
- `zone/inventory.cpp`: emits hidden `ITEMPOWER|set|...` transport on item
  packet sends and calculates missing scores on demand.
- `common/database/database_update_manifest_custom.h`: custom DB version `1`
  creates `item_power`, `item_power_override`, and `item_power_breakdown`.
- `common/version.h`: `CUSTOM_BINARY_DATABASE_VERSION` is `1`.

## Client Files

- `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll`: Gearscore-local
  native client DLL.
- `client_files/native_autoloot/eq-core-dll/src/gearscore_native.cpp`:
  native ItemDisplay/chat hook source.

## Commands

- `#itemscore init`
- `#itemscore show <item_id>`
- `#itemscore recalc <item_id|all>`
- `#itemscore explain <item_id>`
- `#itemscore audit [limit]`
- `#itemscore override <item_id> level|multiplier|bonus|notes <value>`
- `#itemscore clearoverride <item_id>`
- `#itemscore view <item_id>`

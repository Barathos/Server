# Item Rarity Manifest

Feature id: `item-rarity`

This manifest tracks the feature-owned source, schema, and operator notes for
the standalone item rarity checkout.

## Test Target

- Server: `D:\EQServers\EQServer-Item-Rarity`
- Client: `D:\EQClients\EQClient-Item-Rarity`
- Database: `eqemu_item_rarity`

## Feature Files

- `zone/item_rarity_manager.h`
- `zone/item_rarity_manager.cpp`
- `zone/gm_commands/itemrarity.cpp`
- `client_files/item_rarity/README.md`
- `client_files/item_rarity/eq-core-dll/item-rarity-dll.sln`
- `client_files/item_rarity/eq-core-dll/bin/dinput8.dll`
- `client_files/item_rarity/eq-core-dll/src/item-rarity-dll.vcxproj`
- `client_files/item_rarity/eq-core-dll/src/item_rarity_native.cpp`
- `client_files/item_rarity/eq-core-dll/src/dinput8.def`
- `features/item-rarity/sql/001_item_rarity.sql`
- `docs/item-rarity.md`
- `features/item-rarity/README.md`

## Current Scope

The testable slice is server plus item-rarity-owned native client support:

- Stores item rarity tags outside the stock `items` table.
- Adds a GM command for setting, linking, viewing, and injecting test loot.
- Adds a rarity-colored chat line when a tagged item is looted.
- Sends `ITEMRARITY|...` transport lines so this feature's `dinput8.dll` can
  color tagged item names in item inspect/link windows.

The native DLL is feature-owned and must deploy only to
`D:\EQClients\EQClient-Item-Rarity`.

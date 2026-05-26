# Item Rarity

Design and operator documentation for the standalone `item-rarity` feature.

## Overview

Item Rarity adds explicit rarity tags for item IDs without altering the stock
`items` table. The first implementation proves the server behavior:

- GM tooling can tag items as Common, Uncommon, Rare, Legendary, or Unique.
- Tagged items can be linked in chat with rarity-colored text.
- Tagged items can be injected into targeted NPC or corpse loot for testing.
- When a tagged item is looted, the normal EQ loot message is preserved and an
  additional rarity-colored loot line is sent to the looter.
- The feature-owned native DLL caches rarity transports and adds a colored
  rarity header when tagged items are inspected from item links.

The server uses normal EQ item say links and chat colors. The native client
slice is owned under `client_files/item_rarity` and must be installed only to
`D:\EQClients\EQClient-Item-Rarity`.

## Schema

Apply `features/item-rarity/sql/001_item_rarity.sql`, or run `#itemrarity init`
as a GM after the server binary is installed:

```sql
CREATE TABLE IF NOT EXISTS `item_rarity` (
  `item_id` INT UNSIGNED NOT NULL,
  `rarity` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`item_id`),
  CONSTRAINT `item_rarity_rarity_chk` CHECK (`rarity` BETWEEN 0 AND 4)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

Rarity values:

- `0`: Common
- `1`: Uncommon
- `2`: Rare
- `3`: Legendary
- `4`: Unique

## Commands

- `#itemrarity init`
- `#itemrarity legend`
- `#itemrarity set <item_id> <common|uncommon|rare|legendary|unique>`
- `#itemrarity clear <item_id>`
- `#itemrarity show <item_id>`
- `#itemrarity link <item_id>`
- `#itemrarity view <item_id>`
- `#itemrarity loot <item_id> <rarity> [charges]`

Quick test flow:

```text
#itemrarity init
#itemrarity legend
#itemrarity set 1001 rare
#itemrarity link 1001
#itemrarity view 1001
```

For loot testing, target an NPC or corpse:

```text
#itemrarity loot 1001 rare 1
```

If the target is an NPC, kill it and loot the item. If the target is a corpse,
open the corpse after adding the item. Tagged loot should show the normal loot
message plus a rarity-colored line such as `Rare loot: <item link>`. With the
item-rarity `dinput8.dll` installed, clicking a tagged link should also show a
colored rarity header in the item inspect window.

## Native Client

Build the feature-owned DLL:

```text
client_files/item_rarity/eq-core-dll/item-rarity-dll.sln
```

Use `Release|Win32`. The output is:

```text
client_files/item_rarity/eq-core-dll/bin/dinput8.dll
```

Install only this DLL to:

```text
D:\EQClients\EQClient-Item-Rarity\dinput8.dll
```

## Local Verification

- Build: `.\verify-feature.ps1 item-rarity`
- Build native DLL: `MSBuild client_files/item_rarity/eq-core-dll/item-rarity-dll.sln /p:Configuration=Release /p:Platform=Win32`
- Install client DLL only to `D:\EQClients\EQClient-Item-Rarity\dinput8.dll`
- Install runtime: `.\install-server-runtime.ps1 item-rarity`
- Run DB updates: `.\run-db-updates.ps1 item-rarity`
- Validate install: `.\validate-install.ps1 item-rarity`

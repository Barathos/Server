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
- `features/item-rarity/sql/001_item_rarity.sql`
- `docs/item-rarity.md`
- `features/item-rarity/README.md`

## Current Scope

The first testable slice is server-side:

- Stores item rarity tags outside the stock `items` table.
- Adds a GM command for setting, linking, viewing, and injecting test loot.
- Adds a rarity-colored chat line when a tagged item is looted.

Native client behavior is intentionally not scaffolded yet. If the feature later
needs custom item inspect/link rendering, that work belongs in this checkout and
must deploy only to `D:\EQClients\EQClient-Item-Rarity`.

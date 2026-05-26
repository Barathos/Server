# Item Rarity

Standalone EQEmu feature branch for `item-rarity`.

## Test Target

- Server: `D:\EQServers\EQServer-Item-Rarity`
- Client: `D:\EQClients\EQClient-Item-Rarity`
- Database: `eqemu_item_rarity`

## First Build Loop

From `D:\Codex\Apps\EQEmu-feature-workspaces`:

~~~powershell
.\verify-feature.ps1 item-rarity
.\install-server-runtime.ps1 item-rarity
.\install-client-files.ps1 item-rarity
.\run-db-updates.ps1 item-rarity
.\validate-install.ps1 item-rarity
~~~

`install-server-runtime.ps1` also refreshes Windows firewall allow rules for the copied server binaries.

## Client UI

No native client XML window is required. This feature owns a small native
`dinput8.dll` under `client_files/item_rarity` that parses `ITEMRARITY|...`
server transport lines and recolors tagged item names in inspect/link windows.

Build `Release|Win32` from:

~~~text
client_files/item_rarity/eq-core-dll/item-rarity-dll.sln
~~~

Install the resulting DLL only to:

~~~text
D:\EQClients\EQClient-Item-Rarity\dinput8.dll
~~~

## Commands

`#itemrarity` is a GM management command for test setup and inspection:

- `#itemrarity init` creates the `item_rarity` table if it is missing.
- `#itemrarity legend` prints the Common, Uncommon, Rare, Legendary, and Unique color samples.
- `#itemrarity set <item_id> <rarity>` tags an item.
- `#itemrarity clear <item_id>` removes an explicit rarity tag.
- `#itemrarity show <item_id>` shows the current tag as a clickable item link.
- `#itemrarity link <item_id>` sends the same clickable rarity link to chat.
- `#itemrarity view <item_id>` opens the normal item inspect window and sends the rarity link.
- `#itemrarity loot <item_id> <rarity> [charges]` tags the item and adds it as test loot to the targeted NPC or corpse.

When a tagged item is looted from a corpse, the server keeps the normal EQ loot
message, sends the native rarity transport for the item-rarity DLL, and adds a
second rarity-colored line such as `Rare loot: <item link>`.

## Development Notes

The implementation stores rarity separately from the stock `items` table in
`item_rarity`, keeping the feature portable and easy to remove. Rarity-specific
native client logic lives only in this checkout.

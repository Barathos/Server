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

## Client Patch Sync

Client patch syncing is declared in `features/item-rarity/patcher.yml`. Add any
external/test-client files there with a repository `source` and a
client-root-relative `destination`.

Do not add files directly to the local EQ client as the source of truth. The
repo file plus `patcher.yml` entry owns the external payload.

Current patcher-owned client file:

~~~yaml
files:
  - source: client_files/item_rarity/eq-core-dll/bin/dinput8.dll
    destination: dinput8.dll
generated:
  eqhost: true
  equiXml: false
  equiIncludes: []
~~~

In `patcher.yml`, `source` is a path inside this repository, `destination` is a
path inside the EverQuest client root, `generated.eqhost` writes `eqhost.txt`,
`generated.equiXml` enables native UI XML include injection, and
`generated.equiIncludes` must explicitly list custom `EQUI_*.xml` windows.

Before regenerating, look up the patcher `-Project` value in
`D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`. It is the workspace
install `id`; it usually matches the feature id, but do not assume that
blindly.

After committing and pushing project changes, regenerate and test the patcher
feed on the patcher host:

~~~powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
~~~

The feed is published at:

~~~text
http://<patch-host>:8091/patcher/<project-id>/
~~~

External testers place the generated `eqemupatcher.exe` for this project in
their EQ client root and run it. Missing files are a release blocker for real
external syncs. Use `-AllowMissingClientFiles` only for partial local testing.

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

# Item Rarity Feature Handoff

This note is for future EQEmu feature chats that need to understand, test, fix,
or promote the standalone Item Rarity project without rediscovering the same
client/server limits.

## Project Identity

- Feature id: `item-rarity`
- Feature name: Item Rarity
- Repo after checkout: this repository root
- Branch in this checkout: `codex/feature-item-rarity`
- Server install: `D:\EQServers\EQServer-Item-Rarity`
- Client install: `D:\EQClients\EQClient-Item-Rarity`
- Database: `eqemu_item_rarity`
- Local feature-workspaces metadata, if that companion checkout is available:
  - `EQEmu-feature-workspaces/features.json`
  - `EQEmu-feature-workspaces/installs.json`
- Patcher project id currently resolves to `item-rarity`, but always verify it
  in `installs.json` before regenerating feeds.

## Current Feature Shape

Item Rarity adds explicit rarity tags to item IDs without changing the stock
`items` table. Rarity tiers are:

- `0` / `common`: Common, `#F0F0F0`, white chat
- `1` / `uncommon`: Uncommon, `#66FF66`, green chat
- `2` / `rare`: Rare, `#00FFFF`, cyan chat
- `3` / `legendary`: Legendary, `#FFD15C`, yellow chat
- `4` / `unique`: Unique, `#C080FF`, magenta chat

The server owns the data, commands, loot hooks, normal chat output, and a hidden
native-client transport. The item-rarity DLL owns client-side parsing/caching
and best-effort UI recoloring in the RoF2 client.

## Feature-Owned Source Paths

Core server code:

- `zone/item_rarity_manager.h`
- `zone/item_rarity_manager.cpp`
- `zone/gm_commands/itemrarity.cpp`
- `zone/corpse.cpp`
- `zone/command.cpp`
- `zone/command.h`
- `zone/CMakeLists.txt`

Schema and feature metadata:

- `features/item-rarity/sql/001_item_rarity.sql`
- `features/item-rarity/patcher.yml`
- `features/item-rarity/README.md`
- `features/item-rarity/MANIFEST.md`
- `docs/item-rarity.md`
- `docs/testbed-deployment-notes.md`
- `PROJECT.md`
- `AGENTS.md`

Native client code and outputs:

- `client_files/item_rarity/README.md`
- `client_files/item_rarity/eq-core-dll/item-rarity-dll.sln`
- `client_files/item_rarity/eq-core-dll/src/item-rarity-dll.vcxproj`
- `client_files/item_rarity/eq-core-dll/src/dinput8.def`
- `client_files/item_rarity/eq-core-dll/src/item_rarity_native.cpp`
- `client_files/item_rarity/eq-core-dll/bin/dinput8.dll`
- Generated local build outputs also exist under
  `client_files/item_rarity/eq-core-dll/bin` and
  `client_files/item_rarity/eq-core-dll/src/Release`; only
  `bin/dinput8.dll` is patcher payload today.

## Server Architecture

`ItemRarityManager` is the central server API:

- Creates/checks the `item_rarity` table.
- Parses string or numeric rarity values.
- Maps rarity names, hex colors, and EQ chat colors.
- Reads/writes/deletes rarity rows by `item_id`.
- Builds stock EQ say links and the current decorated text format.
- Sends hidden native transport lines:
  - `ITEMRARITY|set|item_id=<id>|rarity=<0-4>|name=<item name>`
  - `ITEMRARITY|clear|item_id=<id>|name=<item name>`
- Sends rarity link/test output and looted-item follow-up messages.

The decorated chat format is intentionally:

```text
[Legendary] Cloth Cap (inspect)
```

The `[Legendary] Cloth Cap` part can inherit rarity chat color from the whole
message. The `(inspect)` part is a normal clickable item link and remains the
client's stock link color.

Command registration:

- `zone\command.cpp` registers `#itemrarity`.
- `zone\command.h` declares `command_itemrarity`.
- `zone\CMakeLists.txt` includes `gm_commands/itemrarity.cpp`.

Loot hook:

- `zone\corpse.cpp` keeps the normal `LOOTED_MESSAGE` with the stock item link.
- If the item has a rarity tag, it sends native rarity transport before the
  normal loot message.
- It then calls `ItemRarityManager::SendLootedItemMessage`, which emits the
  additional rarity-colored line.

## Database Behavior

Schema file:

```text
features/item-rarity/sql/001_item_rarity.sql
```

Table:

```sql
CREATE TABLE IF NOT EXISTS `item_rarity` (
  `item_id` INT UNSIGNED NOT NULL,
  `rarity` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`item_id`),
  CONSTRAINT `item_rarity_rarity_chk` CHECK (`rarity` BETWEEN 0 AND 4)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

Normal feature DB work only creates/updates the `item_rarity` table. It should
not drop/reset tester accounts, characters, inventory, bots, guilds, variables,
or other global EQEmu state. Do not run clean DB, reseed, source-clean, or full
reset scripts against `eqemu_item_rarity` unless the tester database is
explicitly being reset and backed up.

Useful DB/deploy command paths:

- `EQEmu-feature-workspaces/run-db-updates.ps1`
- `D:\EQServers\EQServer-Item-Rarity\bin\world.exe database:updates`

`run-db-updates.ps1 item-rarity` runs `world.exe database:updates` and normally
uses `--skip-backup`. For tester state, use `-Backup` or take an external
`mysqldump` first.

## GM Commands And Test Flow

Available commands:

```text
#itemrarity init
#itemrarity legend
#itemrarity set <item_id> <common|uncommon|rare|legendary|unique|0|1|2|3|4>
#itemrarity clear <item_id>
#itemrarity show <item_id>
#itemrarity link <item_id>
#itemrarity view <item_id>
#itemrarity loot <item_id> <rarity> [charges]
```

Good single-item smoke test:

```text
#itemrarity init
#itemrarity legend
#itemrarity set 1001 legendary
#itemrarity view 1001
#itemrarity link 1001
```

Loot test:

```text
#itemrarity loot 1001 rare 1
```

Target an NPC or corpse first. For NPCs, kill the NPC and loot the item. For
corpses, open the corpse after adding the item.

## Native Client Architecture

Native code lives only in:

```text
client_files/item_rarity
```

Build:

```text
client_files/item_rarity/eq-core-dll/item-rarity-dll.sln
```

Use `Release|Win32`. The output is:

```text
client_files/item_rarity/eq-core-dll/bin/dinput8.dll
```

Install only to:

```text
D:\EQClients\EQClient-Item-Rarity\dinput8.dll
```

The DLL is a `dinput8.dll` proxy:

- It loads the real system DirectInput DLL from the Windows system directory.
- It exports the expected DirectInput entry points.
- It starts an init thread and installs inline hooks into the RoF2 client.
- It logs to:

```text
D:\EQClients\EQClient-Item-Rarity\item_rarity_native.log
```

Important native behavior in `item_rarity_native.cpp`:

- `DspChatDetour` consumes `ITEMRARITY|...` transport lines so they are not
  shown as player chat.
- `ParseRarityTransport` caches rarity by item ID and item name.
- `ItemDisplayUpdateDetour` and `LabelDrawDetour` try to recolor the rendered
  item-name label in item inspect/display windows.
- STML/saylink/draw hooks remain in the file from experiments, but the reliable
  path is inspect label recoloring after the rarity cache has been populated.
- `chat_manager_ok` and `chat_queue_copy_ok` are deliberately hardcoded `false`
  because those hooks caused character-login crashes.

Expected log lines when the client path is working:

```text
item rarity native hooks installed ...
cached rarity item=1001 tier=3 name=Cloth Cap
label recolored source=item_display matched=Cloth Cap rendered=Cloth Cap tier=3 ...
item display labels recolored count=...
```

If inspect coloring does not happen, first verify that the current client has
received a transport for that item in this session. `#itemrarity set`,
`#itemrarity view`, `#itemrarity link`, and looting a tagged item all send
transport for tagged items.

## Client Link Color Limitations

This was the main dead end.

Server-side message color can color a whole line, but the RoF2 client parses raw
EQ item links into clickable link spans and paints those spans with the stock
purple link color. After that conversion, the server's chat color no longer
controls the clickable item-name text.

Things already tried or observed:

- Plain server chat color works for normal text.
- Raw clickable item links remain purple.
- Putting `<c "#FFD15C">...</c>` inside a raw item-link payload renders literal
  markup or otherwise fails; it does not recolor the clickable item span.
- Hooking deeper chat manager / chat queue copy paths caused character-login
  crashes.
- The current stable compromise is rarity-colored plain text next to a normal
  purple clickable `(inspect)` link.

Do not keep repeating small server-side formatting changes expecting WoW-style
colored clickable item links. That probably requires a correct native hook at
the client's post-saylink rendering layer, with careful reverse engineering and
crash-safe calling conventions.

## Build And Install Commands

Run from:

```text
the `EQEmu-feature-workspaces` checkout
```

Standard local loop:

```powershell
.\verify-feature.ps1 item-rarity
.\install-server-runtime.ps1 item-rarity
.\install-client-files.ps1 item-rarity
.\run-db-updates.ps1 item-rarity
.\validate-install.ps1 item-rarity
```

Public testbed promotion command, only when explicitly approved:

```powershell
.\publish-testbed-project.ps1 item-rarity -ApplyServer -ApproveServiceRestart -RunDatabaseUpdates
```

Build preset details:

- `CMakePresets.json`
- Preset: `win-msvc`
- Generator: Visual Studio 17 2022
- Binary dir: `build\win-msvc`
- Architecture: `x64`
- Server targets: `zone`, `world`
- Native DLL target: `Release|Win32`

Perl must stay enabled and pinned:

```text
EQEMU_BUILD_PERL=ON
PERL_EXECUTABLE=C:\Strawberry\perl\bin\perl.exe
PERL_INCLUDE_PATH=C:\Strawberry\perl\lib\CORE
PERL_LIBRARY=C:\Strawberry\perl\lib\CORE\libperl524.a
```

`EQEmu-feature-workspaces/scripts/FeatureWorkspace.ps1` also injects and
validates those Perl settings. Do not promote a Perl-less CMake cache.

## Patcher Payload

Patcher source of truth:

```text
features/item-rarity/patcher.yml
```

Current payload:

```yaml
version: 1
id: item-rarity
label: Item Rarity
client: rof
files:
  - source: client_files/item_rarity/eq-core-dll/bin/dinput8.dll
    destination: dinput8.dll
generated:
  eqhost: true
  equiXml: false
  equiIncludes: []
```

Do not make the local EQ client folder the source of truth. Add or update files
in this repo, list them in `patcher.yml`, then regenerate the feed.

Patcher host command path:

```text
EQEmu-feature-patcher/features/patcher/eqemupatcher/service
```

Commands:

```powershell
.\New-WorkspacePatcherDeployment.ps1 -Project item-rarity -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project item-rarity -BaseUrl http://<patch-host>:8091/patcher/
```

Public feed after regeneration:

```text
http://<patch-host>:8091/patcher/item-rarity/
```

Missing client files are a release blocker for real external testers. Use
`-AllowMissingClientFiles` only for partial local testing.

## Ownership Boundaries

This project owns item-rarity behavior. Keep feature-specific code here:

- Rarity schema and commands
- Loot hook behavior
- Server rarity link/decorated text behavior
- Native client rarity transport parsing
- Native inspect/color support
- Patcher payload for the item-rarity client

Do not edit or deploy feature-specific rarity logic from:

```text
EQEmu-native-client-runtime
```

That repo should only be treated as reference/shared base work until a proper
native-client-base split exists. Also do not install a shared or unrelated
`dinput8.dll` into:

```text
D:\EQClients\EQClient-Item-Rarity
```

or install this feature DLL into other feature clients.

## Known Issues And Rollback

Known issues:

- Clickable chat item links are still purple in the stock client.
- Inspect-window item-name coloring depends on native cache state and RoF2
  client offsets.
- Native hooks are client-build-specific and fragile. A different EQ client
  binary can break offsets or crash.
- Chat manager and chat queue hooks caused character-login crashes and must stay
  disabled unless someone does a deliberate native reverse-engineering pass.
- Native UI/autoloot test servers from other projects can collide with this
  work; stop unrelated feature servers/clients before testing.

Rollback paths:

- Client rollback: remove or replace
  `D:\EQClients\EQClient-Item-Rarity\dinput8.dll`, then restart the EQ client.
- Server rollback: restore prior `world.exe` and `zone.exe` under
  `D:\EQServers\EQServer-Item-Rarity\bin`, then restart only the item-rarity
  server/world/zones.
- DB rollback: restore the `eqemu_item_rarity` backup. Feature-specific cleanup
  should be limited to `item_rarity` rows/table unless a full testbed reset is
  explicitly approved.

## What To Verify Before Promotion

- `git status --short` contains only intended item-rarity-owned changes.
- `features\item-rarity\patcher.yml` lists every client-facing payload file.
- `D:\EQClients\EQClient-Item-Rarity\dinput8.dll` matches this repo's
  `client_files\item_rarity\eq-core-dll\bin\dinput8.dll` after client install.
- `.\verify-feature.ps1 item-rarity` passes from the workspace repo.
- `.\validate-install.ps1 item-rarity` passes from the workspace repo.
- `#itemrarity legend` shows sensible rarity colors.
- `#itemrarity set 1001 legendary` then `#itemrarity view 1001` populates the
  native log and recolors the inspect item name.
- Looting a tagged item sends the normal loot message plus the additional
  rarity-colored decorated line.
- Public patcher feed validates after regeneration.

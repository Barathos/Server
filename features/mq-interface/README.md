# Native Interface

Standalone EQEmu feature branch for `mq-interface`.

## Test Target

- Server: `D:\EQServers\EQServer-Mq-Interface`
- Client: `D:\EQClients\EQClient-Mq-Interface`
- Database: `eqemu_mq_interface`

## First Build Loop

From `D:\Codex\Apps\EQEmu-feature-workspaces`:

~~~powershell
.\verify-feature.ps1 mq-interface
.\install-server-runtime.ps1 mq-interface
.\install-client-files.ps1 mq-interface
.\run-db-updates.ps1 mq-interface
.\validate-install.ps1 mq-interface
~~~

`install-server-runtime.ps1` also refreshes Windows firewall allow rules for the copied server binaries.

## Client Payload

The merged Native Interface/Gearscore client payload is the feature-owned
`dinput8.dll` plus `native_interface.ini` defaults. Map, ItemDisplay, spell
display, and Gearscore text are rendered through native hooks and inline
ItemDisplay STML, not through a custom `EQUI_*.xml` window.

## Client Patcher Sync

External/test-client patch syncing is owned by
`features/mq-interface/patcher.yml`. When adding a client-facing artifact,
commit the file in this repo and add a `files` mapping from the repo path to the
destination path inside the EQ client folder.

Do not treat files copied directly into the local EQ client as the source of
truth. The source of truth is the repo file plus its
`features/mq-interface/patcher.yml` mapping.

Patcher fields:

- `source`: path inside this repository.
- `destination`: path inside the EverQuest client root.
- `generated.eqhost`: writes `eqhost.txt`.
- `generated.equiXml`: injects native UI XML includes.
- `generated.equiIncludes`: explicit custom `EQUI_*.xml` windows.

This feature currently patches:

- `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll` -> `dinput8.dll`
- `client_files/native_autoloot/config/native_interface.ini` -> `native_interface.ini`

The patcher config also generates `eqhost`. Missing files should block external
releases; `-AllowMissingClientFiles` is only for partial local testing.

When regenerating the external feed, use the workspace install id from
`D:\Codex\Apps\EQEmu-feature-workspaces\installs.json` as `-Project`. It
usually matches the feature id, but do not assume that blindly. For this
checkout, `installs.json` currently lists `mq-interface`.

Regenerate the external feed on the patcher host:

~~~powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
~~~

Feed URL: `http://<patch-host>:8091/patcher/<project-id>/`.

## Native Client DLL

This feature builds its own `dinput8.dll` from:

- `client_files/native_autoloot/eq-core-dll/eq-core-dll-visualstudio2022.sln`
- `client_files/native_autoloot/eq-core-dll/src/native_interface.cpp`

Native client behavior:

- Adds live spawn labels to the EQ map draw pass. NPCs are enabled by default;
  players and corpses can be enabled in the client INI.
- Appends a `Native Interface` block to native item inspection windows with item
  id, flags, masks, stat details, aug slots, and item spell effect details.
- Adds Gearscore item level/score/role details. This project owns the server
  scorer, item power database tables, `#itemscore` command, hidden
  `ITEMPOWER|set|...` transport, and native ItemDisplay rendering.
- Appends a `Native Interface` block to native spell inspection windows when the
  spell info window exposes a valid spell id through the known client offset.

Merged Gearscore server behavior:

- `common/item_power.*` calculates item level, intrinsic score, and role scores.
- Custom DB update version `1` creates `item_power`, `item_power_override`, and
  `item_power_breakdown`.
- Item packets emit hidden `ITEMPOWER|set|...` chat transport that the native
  DLL consumes before rendering ItemDisplay text.
- `#itemscore init|show|recalc|explain|audit|override|clearoverride|view`
  provides operator tools for scoring and inspection.

Optional client config lives beside `eqgame.exe`:

~~~ini
[Map]
Enabled=1
ShowNPCs=1
ShowPlayers=0
ShowCorpses=0
ChainEQLabels=1
UseConColor=1
ShowTarget=1
TargetLine=1
MaxLabels=0
RefreshMs=1000
NameFilter=
HideFilter=

[Inspect]
Items=1
Spells=1
~~~

Diagnostics are written to `native_interface.log` in the client folder.

Useful local commands:

- `/nimap status`
- `/nativeinterfacemap status`
- `/nimap on`
- `/nimap off`
- `/nimap npcs on|off`
- `/nimap players on|off`
- `/nimap corpses on|off`
- `/nimap con on|off`
- `/nimap labels on|off`
- `/nimap filter <text>|clear`
- `/nimap hide <text>|clear`
- `/nimap target <name-or-id>|clear`
- `/nimap targetline on|off`
- `/mapfilter NPC|PC|Corpse|Target|TargetLine|NormalLabels|NPCConColor on|off`
- `/ni status`
- `/nativeinterface status`

`MaxLabels=0` means no display cap; the DLL still detects cycles in the
client's spawn linked list to avoid spinning forever on corrupt data.

## Development Notes

Reference code was pulled locally under `.reference/` for research only. The
implementation here is a feature-owned native DLL and should remain isolated to
`D:\EQClients\EQClient-Mq-Interface` during install/testing.

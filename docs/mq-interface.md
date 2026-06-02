# Native Interface

Design and operator documentation for the standalone `mq-interface` feature.

## Overview

`mq-interface` adds selected native conveniences directly to this feature's
custom client DLL so players can use them without running external macro tooling.

The first native client pass focuses on two high-value surfaces:

- Map inspection: live spawn labels are temporarily attached during the native
  map draw pass. NPC labels are enabled by default.
- Item/spell inspection: native item and spell display windows receive an
  appended `Native Interface` details block with ids, flags, stat/mask details,
  aug data, item effect spells, and spell slot data.
- Gearscore display: this merged project owns item power scoring, database
  tables, the `#itemscore` operator command, hidden `ITEMPOWER|set|...` chat
  transport, and native ItemDisplay rendering.

This implementation now includes both native client behavior and the merged
Gearscore server scorer/transport layer.

## Gearscore Server Layer

- `common/item_power.*` calculates item level, total score, and tank/melee/
  caster/healer/hybrid role scores.
- Custom DB update version `1` creates `item_power`, `item_power_override`, and
  `item_power_breakdown`.
- Item packets emit hidden `ITEMPOWER|set|...` chat transport before item data
  is sent, letting the native DLL cache score data by item id.
- `#itemscore init|show|recalc|explain|audit|override|clearoverride|view`
  provides operator tools for scoring, overrides, and transport testing.

## Client Files

- Build project: `client_files/native_autoloot/eq-core-dll/eq-core-dll-visualstudio2022.sln`
- Native source: `client_files/native_autoloot/eq-core-dll/src/native_interface.cpp`
- Built DLL: `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll`

The workspace client install script deploys the DLL and default config to
`D:\EQClients\EQClient-Mq-Interface`. The merged display path is inline
ItemDisplay STML, not a custom `EQUI_*.xml` window.

## External Patcher Feed

Client patch syncing for external/test clients is owned by
`features/mq-interface/patcher.yml`. Each `files` entry maps a source file in
this repo to the destination path inside an EverQuest client folder.

Do not use files copied directly into the local EQ client as the source of
truth. The source of truth is the repo file plus its
`features/mq-interface/patcher.yml` mapping.

Patcher fields:

- `source`: path inside this repository.
- `destination`: path inside the EverQuest client root.
- `generated.eqhost`: writes `eqhost.txt`.
- `generated.equiXml`: injects native UI XML includes.
- `generated.equiIncludes`: explicit custom `EQUI_*.xml` windows.

Current entries:

- `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll` -> `dinput8.dll`
- `client_files/native_autoloot/config/native_interface.ini` -> `native_interface.ini`

The patcher also generates `eqhost` from the `generated` section.

Normal external-sync workflow:

1. Add or update the client-facing file in this repo.
2. Add it to `features/mq-interface/patcher.yml`.
3. Commit/push/sync the project normally.
4. Read the workspace install id from
   `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`.
5. On the patcher host, regenerate and test the feed:

~~~powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
~~~

`-Project` is the workspace install id, not blindly the feature id. For this
checkout, `installs.json` currently lists `mq-interface`.

The feed is published at `http://<patch-host>:8091/patcher/<project-id>/`.
External testers place the generated `eqemupatcher.exe` into their EQ client
root and run it. Missing files are release blockers for real external syncs;
use `-AllowMissingClientFiles` only for partial local testing.

## Optional Client Config

The patcher installs default settings to
`D:\EQClients\EQClient-Mq-Interface\native_interface.ini`:

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

Diagnostics are written to
`D:\EQClients\EQClient-Mq-Interface\native_interface.log`.

`MaxLabels=0` means no display cap; the DLL still detects cycles in the
client's spawn linked list to avoid spinning forever on corrupt data.

## Test Notes

1. Start the prepared Native Interface client.
2. Log into a zone with nearby NPCs and open the native map. NPC labels should
   appear and move as spawns move.
3. Use `/nimap status` to report native map hook state in chat. Use
   `/nimap on` and `/nimap off` to toggle labels.
4. Useful map commands:
   - `/nimap filter <text>` shows only labels containing the text.
   - `/nimap hide <text>` hides labels containing the text.
   - `/nimap filter clear` and `/nimap hide clear` clear those filters.
   - `/nimap target <name-or-id>` targets the nearest matching spawn.
   - `/nimap target clear` clears the current target.
   - `/nimap targetline on|off` toggles the line from you to your target.
   - `/mapfilter NPC|PC|Corpse|Target|TargetLine|NormalLabels|NPCConColor on|off`
     provides a compact native alias for common filters.
   - `/nativeinterfacemap status` is the readable alias for `/nimap status`.
   - `/ni status` or `/nativeinterface status` reports diagnostic hook state.
5. Inspect an item. The item display should include a `Native Interface` block.
6. Inspect an item. The server should emit hidden item-power transport, and the
   item block should include `Item Level`, `Gearscore`, and `Best Role` when
   the item has a non-zero score. Operators can use `#itemscore recalc <id>`,
   `#itemscore show <id>`, and `#itemscore audit [limit]`.
7. Inspect a spell or an item effect spell. If the native spell info window is
   identified by the current offset, it should include a `Native Interface` block.

If item data appears but direct spell-window data does not, the next integration
step is adding a dedicated `CItemDisplayWnd::SetSpell` hook for the spell window
path.

## Local Verification

- Build: `.\verify-feature.ps1 mq-interface`
- Install runtime: `.\install-server-runtime.ps1 mq-interface`
- Install client files: `.\install-client-files.ps1 mq-interface`
- Run DB updates: `.\run-db-updates.ps1 mq-interface`
- Validate install: `.\validate-install.ps1 mq-interface`

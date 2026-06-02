# Native Interface Manifest

Feature id: `mq-interface`

This manifest tracks the standalone source and client artifacts for the Native
Interface feature.

## Test Target

- Server: `D:\EQServers\EQServer-Mq-Interface`
- Client: `D:\EQClients\EQClient-Mq-Interface`
- Database: `eqemu_mq_interface`

## Source Files

- `common/item_power.h`
- `common/item_power.cpp`
- `zone/gm_commands/itemscore.cpp`
- `client_files/native_autoloot/eq-core-dll/eq-core-dll-visualstudio2022.sln`
- `client_files/native_autoloot/eq-core-dll/src/eq-core-dll-vs2022.vcxproj`
- `client_files/native_autoloot/eq-core-dll/src/dinput8.def`
- `client_files/native_autoloot/eq-core-dll/src/native_interface.cpp`

## Built Client Artifact

- `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll`

## Client Patcher Feed

External/test-client sync is controlled by
`features/mq-interface/patcher.yml`. The patcher feed must include every
client-facing artifact required by testers. Missing files are release blockers
for external syncs; `-AllowMissingClientFiles` is only for partial local
testing.

Do not use files copied directly into a local EQ client as the source of truth.
`source` paths in `patcher.yml` are repository paths, and `destination` paths
are relative to the EverQuest client root. `generated.eqhost`,
`generated.equiXml`, and `generated.equiIncludes` control generated
`eqhost.txt` and native UI include injection.

When publishing the feed, pass the workspace install id from
`D:\Codex\Apps\EQEmu-feature-workspaces\installs.json` as `-Project`. It
usually matches the feature id, but do not assume that blindly.

## Behavior

- Live spawn labels on the native EQ map through a feature-owned map draw hook.
- Advanced native map filters for NPCs, players, corpses, normal labels, con
  colors, include/hide text filters, target highlighting, and target line
  drawing.
- `/nimap target <name-or-id>` native target selection by spawn label text or
  spawn id.
- Additional native item inspection text, including item ids, stat/mask details,
  aug slot data, and clicky/proc/worn/focus/scroll spell effect details.
- Gearscore item power scoring, item power database tables, `#itemscore`
  operator tools, hidden `ITEMPOWER|set|...` transport, and native ItemDisplay
  rendering.
- Additional native spell inspection text when the spell display window exposes
  a valid spell id.

## Schema and Server Runtime

Custom database update version `1` creates `item_power`,
`item_power_override`, and `item_power_breakdown`.

# Project Reference Map

Last reviewed: 2026-05-21

This document is a handoff map for future chats and feature planning. For the shortest bootstrapping context, read `docs\start-here.md` first; then use this file as the larger source/tool map.

The guiding rule for this branch is: source code owns reusable mechanics, DB/config owns persistent state and content data, scripts own zone-specific content flavor, and the native client DLL owns only the AutoLoot presentation layer.

## Local Project Context

Main EQEmu source checkout for this prototype:

`D:\Codex\Apps\Everquest-autoloot-native-ui`

Isolated server runtime:

`D:\server-autoloot-native-ui`

Isolated EQ client:

`D:\Codex\Apps\Everquest-EQ-autoloot-native-ui`

Current AutoLoot source points:

- `D:\Codex\Apps\Everquest\zone\autoloot_manager.cpp`
- `D:\Codex\Apps\Everquest\zone\autoloot_manager.h`
- `D:\Codex\Apps\Everquest\zone\gm_commands\autoloot.cpp`
- `D:\Codex\Apps\Everquest\zone\corpse.cpp`
- `D:\Codex\Apps\Everquest-autoloot-native-ui\client_files\native_autoloot\eq-core-dll\src\core_autoloot_native.h`
- `D:\Codex\Apps\Everquest-autoloot-native-ui\client_files\native_autoloot\ui\EQUI_AoTAutoLootWnd.xml`

Older local AutoLoot script reference:

`C:\Users\bemva\Downloads\autoloot.pl`

Important local design docs:

- `D:\Codex\Apps\Everquest-autoloot-native-ui\docs\start-here.md`
- `D:\Codex\Apps\Everquest-autoloot-native-ui\docs\custom-server-vision.md`
- `D:\Codex\Apps\Everquest-autoloot-native-ui\docs\autoloot-queue-v2.md`
- `D:\Codex\Apps\Everquest-autoloot-native-ui\docs\autoloot-ui.md`

Old custom script reference repo:

https://github.com/Barathos/WastingTime

Use that repo for feature intent and old behavior, especially `plugins`, `global_player`, and `global_npc`. Do not treat it as architecture to preserve. It had many global scripts, buckets, and timers that became hard to reason about.

## Decision Guide

Use EQEmu source when the system changes rules, math, loot transfer, combat behavior, progression, database-backed state, or anything players can exploit if the client lies.

Use quest scripts when a zone, NPC, task, boss, dialogue, or one-off event needs content logic.

Use EQ UI XML only for skinning or known existing client windows. RoF2 will not magically create a new packet-backed custom window from XML alone.

Use `eq-core-dll` or similar client patching only for client-side limitations that source cannot solve, such as custom native windows, custom asset support, model/zone injection, or client bug fixes.

Use MCP/database tools when designing content, debugging NPCs/spawns/loot, inspecting schema, or writing SQL.

## EQEmu Useful Links Page

Source page:

https://docs.eqemu.dev/developer/useful-links/

### Core Repositories

- The Grand Library: https://gitlab.com/TheGrandLibrary
  - Compilation of EQ-related repositories. Use when hunting for obscure tools, old formats, editors, or community code.

- EQEmu: https://github.com/EQEmu/EQEmu
  - Main server source. Use for authoritative mechanics, source edits, opcodes, packet paths, database migrations, command routing, zone/world/shared_memory behavior, and core C++ patterns.

- ProjectEQ Database: https://db.eqemu.dev/
  - PEQ database dumps. Use to source clean DBs, compare stock data, restore baseline tables, or inspect canonical content.

- ProjectEQ Quests: https://github.com/ProjectEQ/projecteqquests
  - Stock quest scripts. Use to compare normal Perl/Lua quest conventions, event names, task flows, and zone script structure.

- EQEmu Maps: https://github.com/EQEmu/maps
  - Base, nav, water, and pathing maps. Use when zones need pathing, water checks, navigation fixes, or fresh map assets.

- EQEmu Docs: https://github.com/EQEmu/eqemu-docs-v2
  - Documentation repo. Use when docs need source verification, local indexing, or PRs.

- Allaclone: https://github.com/EQEmuTools/EQEmuAllakhazamClone
  - Allakhazam-style web database. Use for player-facing item/NPC/spell browsing or inspiration for an internal content browser.

- CharBrowser: https://github.com/maudigan/charbrowser
  - Magelo-style character browser. Use for player profiles, gear inspection, or web-facing character progression ideas.

- Spire: https://github.com/EQEmu/spire
  - Modern EQEmu server editing/development toolkit. Use for DB editing, server management, and content operations.

- PEQPHPEditor: https://github.com/ProjectEQ/peqphpeditor
  - Full-featured PHP editor for EQEmu. Use as a content editing reference or fallback editor.

- EQEmu EOC: https://github.com/EQEmuTools/EQEmuEOC
  - Deprecated editor with some still useful tools. Use as reference, not as the first production dependency.

- EQEmuPatcher: https://github.com/xackery/eqemupatcher
  - Auto patcher for EQEmu servers. Use when planning player client distribution, MQ bundles, UI files, client DLLs, spell files, or asset patches.

- TalkEQ: https://github.com/xackery/talkeq
  - Bridges EQ links with outside services. Use as a reference for Discord/web integrations, item links, chat bridges, or external dashboards.

- Zone-Utilities: https://github.com/EQEmu/zone-utilities
  - Tools/libraries for parsing, rendering, and manipulating EQ zone files. Use for custom zones, asset inspection, or zone format work.

- MacroQuest: https://github.com/macroquest/macroquest
  - Open source scripting and plugin platform for EverQuest. Use for custom ImGui windows, local slash commands, client state inspection, plugins, and Lua tooling.

- MQ2TakeADump: https://github.com/maudigan/MQ2TakeADump
  - Dumps EQ data to CSV, including doors, ground spawns, objects, NPCs, current zone, and zone points. Use for reverse engineering live/client zone data.

- eq-core-dll: https://github.com/xackery/eq-core-dll/
  - Client patch DLL for opt-in EQ client features. Use for custom client fixes or custom model/zone support when MQ and server source cannot solve it cleanly.

- EQEmuParticleEditor: https://github.com/Zaela/EQEmuParticleEditor
  - Particle editor. Use for spell/visual effects work.

- EQGWeaponModelImporter: https://github.com/Zaela/EQGWeaponModelImporter
  - EQG weapon model import tool. Use for custom weapons or model pipeline experiments.

- EQGZoneImporter: https://github.com/Zaela/EQGZoneImporter
  - EQG zone importer. Use for custom zone asset pipeline experiments.

- S3DModelExtracter: https://github.com/Zaela/S3DModelExtracter
  - S3D model extractor. Use to inspect or extract older EQ model assets.

- EQTools / GeorgeS Tools remake: https://github.com/cdub321/EQTOOLS
  - Modern remake of George's tools. Use for content editing workflows and item/NPC/spell DB manipulation references.

### Online Server Tools And References

- EOC Online: https://eoc.eqemu.dev/
  - Deprecated editor with still useful tools. Use carefully for one-off inspection.

- Quest API Introduction: https://docs.eqemu.dev/quest-api/introduction/
  - First stop for Perl/Lua quest API behavior. Use when writing scripts or checking event/method signatures.

- Spire Online: https://spire.eqemu.dev/
  - Web-hosted Spire tooling. Use for editing and browsing server data when local Spire is not convenient.

- Allaclone Website: https://alla.eqemu.dev/
  - Online PEQ database browser. Use for quick item/NPC/spell/loot lookup.

- Shendare's Race Inventory: http://www.shendare.com/EQ/Emu/EQRI/
  - Finds races per zone and associated `_chr` links. Use for model loading, custom NPC race work, and client file troubleshooting.

### Gameplay And MacroQuest Tools

These are mostly references or player-tool inspiration. Do not copy code directly into this project unless license and intent are reviewed.

- DerpleTools rgmercs: https://github.com/DerpleMQ2/rgmercs
  - Automation with custom class logic. Use as inspiration for bot/box-friendly UI patterns.

- DerpleTools vendor: https://github.com/DerpleMQ2/vendor
  - Vendor helper and junk selling. Use for AutoSell UX ideas.

- DerpleTools parcel: https://github.com/DerpleMQ2/parcel
  - Parcel automation. Use for mail/parcel convenience ideas.

- DerpleTools bazaar: https://github.com/DerpleMQ2/bazaar
  - Bazaar pricing/history tool. Use for economy UI ideas.

- DerpleTools buttonmaster: https://github.com/DerpleMQ2/buttonmaster
  - Hotbar replacement with Lua/icon support. Use for custom UI/control inspiration.

- EmuBot: https://github.com/andude2/EmuBot
  - Bot management and inventory viewer. Use for bot-friendly UI ideas.

- GamParse: https://elitegamerslounge.com/home/gamparse/GamParse-2.0.0-Beta.exe
  - Log parser/overlay. Use for combat analysis expectations, not server architecture.

- EQLogParser: https://github.com/kauffman12/EQLogParser
  - Log parser, audio triggers, overlay. Use for parser-compatible logging ideas.

- RaidLoot LogSync: https://s3.amazonaws.com/raidloot/logsync.exe
  - Limited parser. Use only as a gameplay-tool reference.

- GINA: https://github.com/smasherprog/Gina/releases
  - Audio triggers. Use as reference for player alert expectations.

- EQNag: https://github.com/guildantix/eq-nag/releases
  - Audio triggers, overlay, floating combat text. Use for overlay inspiration.

- SmartLoot: https://github.com/andude2/smartloot
  - Loot manager and AutoLoot script for EQEmu. Use for UX inspiration: rule prompts, peer rules, whitelist-only mode, loot order, corpse processing states. Do not directly copy code.

- VegasLoot: https://www.dropbox.com/scl/fi/1di5aji4vseox7hlrkjks/MQ2VegasLoot.dll?rlkey=f1o79blqpx8fpifvr7g5m66jr&dl=0
  - AutoLoot plugin DLL. Use as a reference that plugin-based loot tools exist; avoid depending on opaque binaries.

### Raw Server Import SQLs

- Bot Tables Bootstrap: https://raw.githubusercontent.com/EQEmu/EQEmu/master/utils/sql/bot_tables_bootstrap.sql
  - Use when rebuilding or repairing bot tables.

- Merc Tables Bootstrap: https://raw.githubusercontent.com/EQEmu/EQEmu/master/utils/sql/merc_tables_bootstrap.sql
  - Use when rebuilding or repairing merc tables.

### AkkStack And Docker References

- Akk Stack: https://github.com/EQEmu/akk-stack
  - Dockerized EQEmu server stack. Use as reference for service layout, clean installs, and deployment patterns.

- Akk Stack introduction: https://docs.eqemu.dev/akk-stack/introduction/
- Akk Stack installation: https://docs.eqemu.dev/akk-stack/installation/
- Akk Stack backups: https://docs.eqemu.dev/akk-stack/operate/backups/
- Akk Stack services: https://docs.eqemu.dev/akk-stack/operate/services/
- Akk Stack update: https://docs.eqemu.dev/akk-stack/operate/update/

- Docker image collection: https://hub.docker.com/u/eqemulator
  - Use for containerized experiments or comparing service packaging.

## Extra Sources We Found

### EQEmu MCP Server

https://github.com/straps-eq/eqemu-mcp-server

Purpose: gives AI assistants structured access to EQEmu docs, schema, source, quests, config, logs, and live database data. It is read-only by default and exposes write tools only when explicitly enabled.

Use it for:

- schema-safe SQL
- spawn, NPC, loot, spell, faction, task, zone, merchant, door, forage/fishing inspection
- source searches
- quest API lookups
- logs and config checks
- live character/account inspection when allowed

In future Codex sessions, if the `eqemu` MCP tools are available, prefer them over ad hoc SQL or broad web search for EQEmu-specific questions.

### EQEmu Discord Archive

https://discord-archive.eqemu.dev/

Purpose: public archive of the old EQEmu Discord. Use it for historical implementation discussions, tribal knowledge, edge cases, and prior art.

Limits:

- It is an archive, not a live support channel.
- Verify claims against source code or docs before making changes.
- Live private Discord access would require permissions/bot access and is not currently available for non-owned servers.

### Official EverQuest Advanced Looting Guide

https://www.everquest.com/news/advanced-looting-system

Purpose: UX reference for the Live advanced loot flow. Useful concepts include personal loot list, shared loot list, Loot/Leave/Never, filters, auto-loot all, master looter, free grab, need/greed/pass, always need/greed, locked corpse state, and grouping by NPC.

Use it as behavioral inspiration only. RoF2 does not have the later-Live advanced loot client window/opcode path, so this branch uses source-backed EQEmu logic plus a native SIDL window loaded by the client DLL.

### Native AutoLoot UI

Native AutoLoot design direction:

- The `AoT AutoLoot` native window is the preferred AutoLoot UI surface for this branch.
- `dinput8.dll` creates the window from SIDL and parses `AUTOLOOT|...` transport lines.
- Server remains authoritative; the client window is a display/control surface.

### SmartLoot

https://github.com/andude2/smartloot

Purpose: strong inspiration for loot UX. SmartLoot shows useful patterns for pausing on unknown loot, local rule DBs, peer rules, whitelist-only mode, main/background looter modes, corpse caches, and visible state.

Project stance:

- Do not copy code directly.
- Borrow concepts and UX patterns where they fit.
- Prefer our source-backed backend for authoritative loot transfer and persistence.

### eq-core-dll

https://github.com/xackery/eq-core-dll/

Purpose: client patch DLL loaded as `dinput8.dll`. It is archived/read-only, but useful as a reference for client-side opt-in patches.

Potentially useful areas:

- gamma restore
- EQG route overrides
- map/bazaar window disable patches
- Luclin model toggles
- heroic stat display toggles
- high HP display fixes
- patchme bypass
- food/drink spam suppression
- spell data CRC experiments
- custom shields
- custom NPC models/races
- custom animations
- custom zones

Use it only for client limitations that source and MQ cannot solve. Keep any patch set small, documented, and opt-in.

### EQEmuPatcher

https://github.com/xackery/eqemupatcher

Purpose: possible player distribution layer for client files. Use when we need to ship MQ, plugin DLLs, UI files, spells, dbstr, eqstr, maps, custom assets, or client DLL changes in a repeatable way.

### WastingTime Scripts

https://github.com/Barathos/WastingTime

Purpose: old server feature contract and inspiration. Good places to inspect:

- `plugins`
- `global_player`
- `global_npc`
- old AutoLoot, rebirth, class mastery, and global systems

Use it to understand what players liked and which interactions caused complexity. Move reusable systems to source/DB/rules instead of repeating global timer-heavy script architecture.

## Feature-Specific Reference Routes

### AutoLoot / AutoSell / Loot UI

Current summary: AutoLoot is source-backed with a native C++/SIDL client window. The old Perl `autoloot.pl` is reference only. Player-facing rules are `Keep`, `Never`, and `Unset`; the DB still stores `include`/`exclude` internally for compatibility.

Start local:

- `zone/autoloot_manager.cpp`
- `zone/corpse.cpp`
- `zone/gm_commands/autoloot.cpp`
- `client_files\native_autoloot\eq-core-dll\src\core_autoloot_native.h`
- `client_files\native_autoloot\ui\EQUI_AoTAutoLootWnd.xml`
- `docs\autoloot-queue-v2.md`

External references:

- Official advanced loot guide
- SmartLoot
- Derple vendor
- VegasLoot as proof of plugin-based precedent only

Core rule: all actual item transfer should go through `Corpse::AutoLootItem` or an equally safe extracted source path.

There is no stack buffer now. Stackables use the source inventory path; failed transfers leave loot on the corpse.

### Rebirth / Perks / Class Mastery

Start local:

- `docs\custom-server-vision.md`
- old WastingTime scripts
- DB schema docs for character/account/custom tables
- EQEmu task system docs if progression uses task windows

Good architecture:

- source-owned math and validation
- DB-backed state
- UI through native client windows or task windows
- scripts only for zone/NPC interactions and unlock flavor

### Zone Difficulty / DDO-Style Modes

Useful references:

- EQEmu source: zone instance, dynamic zone, expedition, NPC scaling, spawn loading
- EQEmu docs: expedition, instances, zone version switching, NPC scaling
- ProjectEQ DB for baseline zone content
- maps/nav data for pathing impact

Design note: difficulty mode should be a first-class zone/instance state, not a pile of NPC scripts.

### Attunement / Evolving Gear / Permanent Stats

Useful references:

- EQEmu item/evolving item docs
- source item instance handling
- inventory repositories
- `Corpse::AutoLootItem` and item instance transfer logic for safe item data handling
- custom DB tables for mastered item state

Design note: permanent stat gain should feed into a power budget/gear score model to avoid hidden power creep.

### Custom Assets / Client Mods / New Zones

Useful references:

- EQEmu client docs
- Zone-Utilities
- EQGZoneImporter
- EQGWeaponModelImporter
- S3DModelExtracter
- EQEmuParticleEditor
- Shendare Race Inventory
- eq-core-dll
- EQEmuPatcher

Design note: custom assets are a distribution problem as much as a content problem. Plan patching early.

### Web / Admin / Data Browsing

Useful references:

- Spire
- PEQPHPEditor
- EQEmu EOC
- Allaclone
- CharBrowser
- EQEmu MCP Server

Design note: Spire/MCP should be first choices for active development; older editors are mostly reference or fallback.

### Bots / Boxing / Solo-Friendly Systems

Useful references:

- EQEmu bot docs and source
- EmuBot
- EZBots
- rgmercs
- old WastingTime solo/box assumptions

Design note: population can be 5 or 500, so core progression should not require a synchronized group at the same progression step.

## Operational Notes

When editing source:

- Prefer local source and official docs over memory.
- Use `rg` first.
- Keep source systems DB-backed and inspectable.
- Avoid quest-global bucket shards for live state.
- Avoid timers for mechanics source code can own.
- Do not delete loot or state until a safe transfer/write succeeds.

When using external repos:

- Treat GPL/licensing and attribution seriously.
- Use repositories as inspiration unless we intentionally vendor or fork them.
- Avoid depending on opaque DLLs for core server behavior.
- Verify old or archived repo behavior before adopting it.

When planning player distribution:

- Assume players may receive a bundled client patch package.
- Native client files should make features feel built in.
- EQEmuPatcher may become the distribution tool for UI, spell, asset, map, and DLL files.

When future chats need context:

1. Read `docs\start-here.md`.
2. Read `docs\custom-server-vision.md`.
3. Read this file for links, tools, and external references.
4. For AutoLoot work, read `docs\autoloot-ui.md`, then inspect `zone\autoloot_manager.cpp` and `client_files\native_autoloot\eq-core-dll\src\core_autoloot_native.h`.
5. Check whether EQEmu MCP tools are available before guessing schema or content relationships.
6. Use local paths above to inspect the actual implementation.

# Live Spells Feature Pack

Status: `draft`, `feature-dll`, `proved-build`

This pack describes the Live Spells system: generated spell definitions, generated scroll items, server-side spell patching, and the native Spell Forge input window.

## What This Feature Owns

- Reserved spell ID range `42500-42602` for generated spells.
- Generated spell definitions persisted in `data_buckets`.
- Server-side spell array patching when a zone first needs live spell data.
- Generated scroll items stored in `items`.
- Live item lookup support needed by generated scroll item IDs.
- `#livespell` command paths for dialog, crafting, sync, GM patch/test, and scribing.
- Native Spell Forge UI transport using `LIVESPELL|...` lines.

## What This Feature Does Not Require

- AutoLoot.
- Item Forge or item editing commands.
- Achievements.
- MacroQuest, Lua, ImGui, or overlay UI code.

## Dependencies

- EQEmu source rebuild.
- Blank spell rows in the reserved generated spell range.
- `rule_values` rows if the operator wants to override the generated scroll item live range.
- This feature's native client DLL if the operator wants the in-client Spell Forge window.

## Install Outline

1. Apply the source files and hook patches listed in `MANIFEST.md`.
2. Optionally apply `sql/001_live_spell_runtime_rules.sql` to make generated scroll item live lookup explicit.
3. Rebuild `zone` and `world`.
4. Add any client-facing files to `patcher.yml`, then regenerate the `live-spells` patch feed.
5. Use `#livespell dialog`, then craft a generated spell scroll.

## Smoke Test

1. Run `#livespell dialog` and confirm the native client receives the Spell Forge open line.
2. Run `#livespell craft element=fire target=target range=200 damage=100 recast=3000 name=Test_Flame`.
3. Confirm a generated scroll item is created in the live item range and placed on the cursor.
4. Scribe the scroll, memorize the generated spell, then cast it.
5. Restart a zone and confirm `#livespell ready` resyncs persisted generated spells.

## Client Patch Sync

Client patch syncing is owned by `features/live-spells/patcher.yml`. The manifest maps files from this repo to their destination inside the EverQuest client folder, including this feature's `dinput8.dll` and `EQUI_NativeSpellForgeWnd.xml`. Do not add files directly to the local EQ client as the source of truth; the client folder is only a deployment target.

In `patcher.yml`, `files[].source` is a path inside this repo and `files[].destination` is a path inside the EverQuest client root. Use `generated.eqhost: true` when the patcher should write `eqhost.txt`, `generated.equiXml: true` when native UI XML includes need to be injected, and `generated.equiIncludes` to list custom `EQUI_*.xml` windows explicitly.

Before regenerating the external patch feed, look up the workspace install id in `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`. It usually matches the feature id, but do not assume that blindly. Then run from the patcher host:

```powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
```

The feed is published at `http://<patch-host>:8091/patcher/<project-id>/`. Missing files listed in `patcher.yml` are release blockers for real external syncs; use `-AllowMissingClientFiles` only for partial local testing.

## Native DLL Ownership

Live Spells must not depend on `EQEmu-native-client-runtime` for a feature-specific `dinput8.dll`. Native client behavior, transport parsing, slash-command rewriting, and native EQ windows for this feature belong in this checkout and deploy only to `D:\EQClients\EQClient-Live-Spells`.

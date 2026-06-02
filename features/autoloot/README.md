# AutoLoot Feature Pack

Status: `draft`, `feature-owned-native-client`

This pack describes the source-backed AutoLoot system with the native EverQuest AutoLoot window. It is intended for a server operator who wants AutoLoot without also taking Live Items, Live Spells, or Achievements.

## What This Feature Owns

- Character AutoLoot settings.
- Per-character item filters using player-facing `Keep`, `Ignore`, and `Unset` language.
- Group loot settings, shared decision queues, and Need/Greed voting.
- AutoSell preview and confirm flow.
- Nearby corpse processing.
- Server-side item transfer through source-controlled corpse loot paths.
- Native AutoLoot UI transport using `AUTOLOOT|...` chat protocol lines.

## What This Feature Does Not Require

- Live Items.
- Live Spells.
- Achievements.
- Old Perl `autoloot.pl` execution.
- MacroQuest, Lua, ImGui, or an overlay AutoLoot window.

The old script and MQ/Lua approaches should stay out of this pack. The native client window is now the AutoLoot UI surface; the server remains authoritative for all loot decisions and item movement.

## Dependencies

- EQEmu source rebuild.
- Custom database update support, or manual execution of the SQL in this pack.
- A feature-owned native client DLL built from this checkout if the operator wants the in-client AutoLoot window.
- The client patcher manifest at `features/autoloot/patcher.yml` for external/test-client file sync.

The command surface can still be used without the native window, but the intended package is the native window plus server source.

## Native Client Ownership

The server-side AutoLoot boundary is reasonably separable now.

The client-side boundary must be feature-owned in this checkout. Do not depend on `EQEmu-native-client-runtime` for AutoLoot's feature-specific `dinput8.dll`; that project is reference/shared base work only until there is a proper `native-client-base` split.

If AutoLoot needs native client behavior, transport parsing, slash-command rewriting, or native EQ windows, keep that source and its build output under this project and deploy only to the matching AutoLoot client folder. If another feature name appears in DLL/runtime code while porting or trimming client code, stop and ask before removing it.

The current package includes the AutoLoot UI XML and a patcher manifest entry for the expected AutoLoot DLL path. A real external patcher feed is blocked until every mapped file exists in this repo.

## Client Patcher Manifest

Client patch syncing is owned by `features/autoloot/patcher.yml`. Do not add files directly to the local EQ client as the source of truth; the client folder is only a deployment target.

Add every external/test-client file to `patcher.yml`, including native DLLs, XML, config files, patch notes, status files, or other client assets. Each `files` entry maps a file in this repo to its destination inside the EverQuest client folder:

```yaml
files:
  - source: client_files/native_autoloot/ui/EQUI_NativeAutoLootWnd.xml
    destination: uifiles/default/EQUI_NativeAutoLootWnd.xml
```

In `patcher.yml`, `source` is a path inside this repo and `destination` is a path inside the EverQuest client root.

The manifest can also request generated client files:

```yaml
generated:
  eqhost: true
  equiXml: true
  equiIncludes:
    - EQUI_NativeAutoLootWnd.xml
```

Use `generated.eqhost = true` when the patcher should write `eqhost.txt`. Use `generated.equiXml = true` when native UI XML includes need to be injected, and list each custom `EQUI_*.xml` window explicitly in `generated.equiIncludes`.

When regenerating the patch feed, `-Project` is the workspace install id from `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`. That usually matches the feature id, but do not assume it blindly. For this checkout, the current install id is `autoloot`.

On the patcher host, regenerate and test this project's feed with the resolved project id:

```powershell
$projectId = "<project-id-from-installs.json>"
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project $projectId -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project $projectId -BaseUrl http://<patch-host>:8091/patcher/
```

The published feed is `http://<patch-host>:8091/patcher/<project-id>/`. External testers place this project's generated `eqemupatcher.exe` into their EQ client root and run it.

For real external syncs, missing files in `patcher.yml` are release blockers. Use `-AllowMissingClientFiles` only for partial local testing.

## Install Outline

1. Apply the server source files and hook patches listed in `MANIFEST.md`.
2. Apply the database schema from `sql/001_source_backed_autoloot.sql`, or port it into the server's custom migration system.
3. Rebuild `zone`.
4. Build this feature's native client DLL from this checkout, if using the native window.
5. Use `features/autoloot/patcher.yml` to deploy `dinput8.dll`, `EQUI_NativeAutoLootWnd.xml`, generated `eqhost`, and generated `EQUI.xml` includes to the target client.
6. Log in and run `#autoloot native show`.

## Smoke Test

1. Kill an NPC with loot.
2. Open the native AutoLoot window with `#autoloot native show`.
3. Mark an item `Keep`, then kill another NPC that drops the same item.
4. Confirm the server moves the item through the source loot path and leaves blocked loot on the corpse.
5. Mark an item `Ignore` and confirm it is left alone.
6. In a group, verify Need/Greed/Pass decisions use the server queue and do not trust client-only state.

## Next Split Work

- Bring AutoLoot-specific client DLL source and build scripts into this checkout.
- Use `native-client-runtime` only as reference/shared base work unless explicitly working in that project.
- Create a `native-client-base` pack later for shared DLL hooks, chat transport plumbing, and safe window lifecycle helpers.
- Keep `features/autoloot/patcher.yml` complete enough to generate an AutoLoot-only external patcher feed.

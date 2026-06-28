# Codex Instructions: EQEmu Item Rarity

This repository is the standalone `item-rarity` feature checkout.

- Keep changes scoped to Item Rarity.
- Matching server install: `D:\EQServers\EQServer-Item-Rarity`
- Matching client install: `D:\EQClients\EQClient-Item-Rarity`
- Matching database: `eqemu_item_rarity`
- Use D:\Codex\Apps\EQEmu-feature-workspaces for cross-feature validation and packaging.
- No native UI XML is registered for this feature yet.
- Native rarity inspect/link support is owned by this checkout under
  `client_files/item_rarity`.

## Native Client Ownership

If this feature has native client behavior, transport parsing, slash-command rewriting, or native EQ windows, the client DLL work belongs in this checkout. Build this feature's own `dinput8.dll` from this project and deploy it only to `D:\EQClients\EQClient-Item-Rarity`.

Before changing native client code:

- Run `git status --short`.
- Confirm this checkout is the intended `item-rarity` feature project.
- Confirm the target client is `D:\EQClients\EQClient-Item-Rarity`.
- If another feature name appears in the DLL/runtime code, stop and ask before removing it.

Do not edit, clean, strip, rebuild, or install another project's DLL. Treat `EQEmu-native-client-runtime` as reference/shared base work only until there is a proper native-client-base split.

## Client Patch Sync

Client patch syncing for external and test clients is owned by this project
through `features/item-rarity/patcher.yml`. Add every client-facing file that
must land in the EverQuest client to that file. Each `files` entry maps a
repository path to a path inside the EQ client root.

Do not add files directly to the local EQ client as the source of truth. The
repo file plus `patcher.yml` entry owns the external payload.

Example:

```yaml
files:
  - source: client_files/item_rarity/eq-core-dll/bin/dinput8.dll
    destination: dinput8.dll
generated:
  eqhost: true
  equiXml: false
  equiIncludes: []
```

In `patcher.yml`, `source` is a path inside this repository, `destination` is a
path inside the EverQuest client root, `generated.eqhost` controls generated
`eqhost.txt`, `generated.equiXml` controls native UI XML include injection, and
`generated.equiIncludes` must explicitly list any custom `EQUI_*.xml` windows.

Normal workflow:

- Add or update the client-facing file in this repository.
- Add or update its entry in `features/item-rarity/patcher.yml`.
- Commit, push, and sync this project normally.
- Look up the patcher `-Project` value in
  `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`; it is the workspace
  install `id`, which usually matches the feature id but must not be assumed.
- On the patcher host, regenerate and test the feed from
  `D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service`:

```powershell
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
```

The feed is published at `http://<patch-host>:8091/patcher/<project-id>/`.
External testers place that project's generated `eqemupatcher.exe` into their
EQ client root and run it. For real external syncs, missing client files are a
release blocker. Use `-AllowMissingClientFiles` only for partial local testing.

## Public Testbed Promotion

When the user says this feature/project is ready for external testing, run from `D:\Codex\Apps\EQEmu-feature-workspaces`:

```powershell
.\publish-testbed-project.ps1 item-rarity -ApplyServer -ApproveServiceRestart -RunDatabaseUpdates
```

This verifies/packages/uploads the server build, backs up the remote database before running updates, applies server files only with explicit restart approval, regenerates the public patcher feed from this repo's `patcher.yml`, uploads that feed to the testbed patcher root, and validates the feed URL.

Use -RunDatabaseUpdates for SQL compiled into world.exe database:updates. Use -ApplyFeatureSql only when this feature's standalone features/item-rarity/sql/*.sql files are intended to be applied directly to the testbed database.

## Startup Behavior

On the first chat in this project, orient yourself and verify the prepared environment only. Do not implement feature behavior, add schema, build binaries, install runtime files, or run DB migrations unless the user explicitly asks for that work.

Good first-chat actions:

- Read PROJECT.md, this AGENTS.md, features/item-rarity/README.md, features/item-rarity/MANIFEST.md, and docs/item-rarity.md.
- Run lightweight status checks such as git status --short, codegraph status ., and .\validate-install.ps1 item-rarity -AllowMissing from the workspace folder.
- Report what is ready, what is missing, and ask what the user wants built next.

Before changing code, read PROJECT.md, features/item-rarity/README.md,
features/item-rarity/MANIFEST.md, and docs/item-rarity.md.

After user-requested code changes, run .\verify-feature.ps1 item-rarity from the workspace folder.
After a successful build, run .\install-server-runtime.ps1 item-rarity,
.\run-db-updates.ps1 item-rarity, and .\validate-install.ps1 item-rarity. The
runtime install script refreshes Windows firewall allow rules for the copied
server binaries. If the native DLL was rebuilt, install this checkout's
`client_files/item_rarity/eq-core-dll/bin/dinput8.dll` only to
`D:\EQClients\EQClient-Item-Rarity`.

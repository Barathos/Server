# EQEmu Item Rarity

This checkout owns the standalone `item-rarity` feature branch.

- Server install: `D:\EQServers\EQServer-Item-Rarity`
- Client install: `D:\EQClients\EQClient-Item-Rarity`
- Database: `eqemu_item_rarity`

## Client UI

Native item inspect/link coloring is owned by this checkout under
`client_files/item_rarity`. Build this feature's `dinput8.dll` and install it
only to `D:\EQClients\EQClient-Item-Rarity`.

## Client Patch Sync

External and test-client patch syncing is owned by
`features/item-rarity/patcher.yml`. Add every client-facing file there, mapping
the repository source path to the destination path inside the EQ client root.
Do not add files directly to the local EQ client as the source of truth.
The current item-rarity patcher feed includes the feature-owned DLL:

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
`generated.equiIncludes` explicitly lists custom `EQUI_*.xml` windows.

Before regenerating, look up the patcher `-Project` value in
`D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`. It is the workspace
install `id`; it usually matches the feature id, but do not assume that
blindly.

On the patcher host, regenerate and test the feed from
`D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service`:

~~~powershell
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
~~~

The feed is published at `http://<patch-host>:8091/patcher/<project-id>/`.
Missing files are a release blocker for real external syncs; use
`-AllowMissingClientFiles` only for partial local testing.

## Build Checklist

Run these from `D:\Codex\Apps\EQEmu-feature-workspaces` only after the user asks for implementation/build work:

~~~powershell
.\verify-feature.ps1 item-rarity
.\install-server-runtime.ps1 item-rarity
.\install-client-files.ps1 item-rarity
.\run-db-updates.ps1 item-rarity
.\validate-install.ps1 item-rarity
.\status-installs.ps1
~~~

`install-server-runtime.ps1` also refreshes Windows firewall allow rules for
the copied server binaries.

Use this checkout for feature-owned source changes. Do not develop unrelated
features here; merge this branch into the all-features bundle only after the
standalone feature is working.

The client install should only receive the item-rarity-owned DLL from this
checkout.

# EQEmu Item Rarity

This checkout owns the standalone `item-rarity` feature branch.

- Server install: `D:\EQServers\EQServer-Item-Rarity`
- Client install: `D:\EQClients\EQClient-Item-Rarity`
- Database: `eqemu_item_rarity`

## Client UI

No native client XML window is required for the first server-side rarity slice.

## Build Checklist

Run these from `D:\Codex\Apps\EQEmu-feature-workspaces` only after the user asks for implementation/build work:

~~~powershell
.\verify-feature.ps1 item-rarity
.\install-server-runtime.ps1 item-rarity
.\run-db-updates.ps1 item-rarity
.\validate-install.ps1 item-rarity
.\status-installs.ps1
~~~

`install-server-runtime.ps1` also refreshes Windows firewall allow rules for
the copied server binaries.

Use this checkout for feature-owned source changes. Do not develop unrelated
features here; merge this branch into the all-features bundle only after the
standalone feature is working.

No client file install is required until the feature owns a native client
project.

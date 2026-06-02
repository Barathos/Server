# EQEMU General Code

Standalone EQEmu feature branch for `eqemu-general-code`.

## Test Target

- Server: `D:\EQServers\EQServer-Eqemu-General-Code`
- Client: `D:\EQClients\EQClient-Eqemu-General-Code`
- Database: `eqemu_eqemu_general_code`

## First Build Loop

From `D:\Codex\Apps\EQEmu-feature-workspaces`:

~~~powershell
.\verify-feature.ps1 eqemu-general-code
.\install-server-runtime.ps1 eqemu-general-code
.\install-client-files.ps1 eqemu-general-code
.\run-db-updates.ps1 eqemu-general-code
.\validate-install.ps1 eqemu-general-code
~~~

`install-server-runtime.ps1` also refreshes Windows firewall allow rules for the copied server binaries.

## Client UI

No native client XML window was scaffolded for this feature.

## Client Patcher Feed

External/test-client patch syncing is owned by `features/eqemu-general-code/patcher.yml`. Do not add files directly to `D:\EQClients\EQClient-Eqemu-General-Code` as the source of truth. Add any client-facing XML, DLL, config, zone asset, patch note, status file, or other tester-facing file to this repo and list it there before publishing a patcher feed.

## Development Notes

Replace this section with feature behavior, commands, SQL, client files, operator install notes, and known test cases as the feature takes shape.
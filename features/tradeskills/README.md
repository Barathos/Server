# Tradeskills

Standalone EQEmu feature branch for `tradeskills`.

## Test Target

- Server: `D:\EQServers\EQServer-Tradeskills`
- Client: `D:\EQClients\EQClient-Tradeskills`
- Database: `eqemu_tradeskills`

## First Build Loop

From `D:\Codex\Apps\EQEmu-feature-workspaces`:

~~~powershell
.\verify-feature.ps1 tradeskills
.\install-server-runtime.ps1 tradeskills
.\install-client-files.ps1 tradeskills
.\run-db-updates.ps1 tradeskills
.\validate-install.ps1 tradeskills
~~~

`install-server-runtime.ps1` also refreshes Windows firewall allow rules for the copied server binaries.

## Client UI

Native XML `EQUI_NativeTradeskillsWnd.xml` is installed into the prepared client and included from `uifiles/default/EQUI.xml` by the workspace client install scripts.

## Development Notes

Replace this section with feature behavior, commands, SQL, client files, operator install notes, and known test cases as the feature takes shape.
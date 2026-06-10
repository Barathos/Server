# HP Fix

Standalone EQEmu feature branch for `hpfix`.

## Test Target

- Server: `D:\EQServers\EQServer-Hp-Fix`
- Client: `D:\EQClients\EQClient-Hp-Fix`
- Database: `eqemu_hpfix`

## First Build Loop

From `D:\Codex\Apps\EQEmu-feature-workspaces`:

~~~powershell
.\verify-feature.ps1 hpfix
.\install-server-runtime.ps1 hpfix
.\install-client-files.ps1 hpfix
.\run-db-updates.ps1 hpfix
.\validate-install.ps1 hpfix
~~~

`install-server-runtime.ps1` also refreshes Windows firewall allow rules for the copied server binaries.

## Client UI

Native XML `EQUI_NativeHpFixWnd.xml` is installed into the prepared client and included from `uifiles/default/EQUI.xml` by the workspace client install scripts.

## Development Notes

- The DLL sends `#hpfix native ready` automatically after entering game.
- Native-ready clients receive `HPFIX|self|current=<current>|max=<max>|percent=<percent>` whenever self HP updates.
- The client suppresses that transport line and updates `NativeHpFixWnd`.
- `#hpfix items` summons 12M, 25M, and 50M HP test items for GM accounts after the SQL file has been applied.
- `#liveitem summon 199990`, `#liveitem summon 199991`, and `#liveitem summon 199992` summon the matching live-range HPFIX test items.
- `#hpfix refresh` forces the side-channel payload for quick testing after equipping or healing.

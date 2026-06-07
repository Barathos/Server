# Gearscore

Standalone EQEmu feature branch for `gearscore`.

## Test Target

- Server: `D:\EQServers\EQServer-Gearscore`
- Client: `D:\EQClients\EQClient-Gearscore`
- Database: `eqemu_gearscore`

## First Build Loop

From `D:\Codex\Apps\EQEmu-feature-workspaces`:

~~~powershell
.\verify-feature.ps1 gearscore
.\initialize-server-runtime-template.ps1 gearscore -Force -GrantRuntimeDbUser
.\install-server-runtime.ps1 gearscore
.\install-client-files.ps1 gearscore
.\run-db-updates.ps1 gearscore
.\validate-install.ps1 gearscore
~~~

`initialize-server-runtime-template.ps1` seeds the installer-style server baseline from `C:\install_template` without copying that template's source checkout. `install-server-runtime.ps1` overlays built feature binaries and refreshes Windows firewall allow rules.

## Display Integration

Gearscore ships a Gearscore-local native client DLL for the local Gearscore
client. The server emits a hidden `ITEMPOWER|set|...` transport line during
item packet sends so the native ItemDisplay hook can render item level and
score lines in the normal item display path. Missing `item_power` rows are
calculated and stored on demand before the transport is sent.

## Client Patcher Feed

External/test-client patch syncing is owned by `features/gearscore/patcher.yml`. Do not add files directly to `D:\EQClients\EQClient-Gearscore` as the source of truth. Add any client-facing XML, DLL, config, zone asset, patch note, status file, or other tester-facing file to this repo and list it there before publishing a patcher feed.

The patch feed includes `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll` plus generated `eqhost.txt`; Gearscore does not ship a custom UI window because item level details are appended in the normal ItemDisplay path.

## Development Notes

- Core scorer: `common/item_power.*`
- Admin command: `#itemscore`
- Compiled custom DB update: version `1`, `item_power` tables
- ItemDisplay bridge: hidden `ITEMPOWER|set|...` message on item packet sends
- Native hook: `client_files/native_autoloot/eq-core-dll/src/gearscore_native.cpp`

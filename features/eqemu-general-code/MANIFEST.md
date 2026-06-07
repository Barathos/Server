# EQEMU General Code Manifest

Feature id: `eqemu-general-code`

This manifest starts intentionally small. Add required source files, SQL files,
client UI files, and operator notes as the feature becomes real.

## Test Target

- Server: `D:\EQServers\EQServer-Eqemu-General-Code`
- Client: `D:\EQClients\EQClient-Eqemu-General-Code`
- Database: `eqemu_eqemu_general_code`

## Expected First Implementation Updates

- Add feature-owned source files to `features.json` `requiredFiles` once they exist.
- Add SQL migration files under `features/eqemu-general-code/sql` if the feature needs schema/rule data.
- Add command, packet, or manager entry points here as they become part of the portable feature.
- Add external/test-client files to `features/eqemu-general-code/patcher.yml` so the workspace patcher feed can publish them.
- In `patcher.yml`, `source` is repo-relative and `destination` is relative to the EverQuest client root.
- If the feature has native client behavior, add the feature-owned DLL project/source and build this checkout's own `dinput8.dll`.
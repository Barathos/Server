# Tradeskills Manifest

Feature id: `tradeskills`

This manifest starts intentionally small. Add required source files, SQL files,
client UI files, and operator notes as the feature becomes real.

## Test Target

- Server: `D:\EQServers\EQServer-Tradeskills`
- Client: `D:\EQClients\EQClient-Tradeskills`
- Database: `eqemu_tradeskills`

## Expected First Implementation Updates

- Add feature-owned source files to `features.json` `requiredFiles` once they exist.
- Add SQL migration files under `features/tradeskills/sql` if the feature needs schema/rule data.
- Add command, packet, or manager entry points here as they become part of the portable feature.
- If the feature has native client behavior, add the feature-owned DLL project/source and build this checkout's own `dinput8.dll`.
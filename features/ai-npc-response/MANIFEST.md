# AI NPC response Manifest

Feature id: `ai-npc-response`

This manifest starts intentionally small. Add required source files, SQL files,
client UI files, and operator notes as the feature becomes real.

## Test Target

- Server: `D:\EQServers\EQServer-Ai-Npc-Response`
- Client: `D:\EQClients\EQClient-Ai-Npc-Response`
- Database: `eqemu_ai_npc_response`

## Expected First Implementation Updates

- Add feature-owned source files to `features.json` `requiredFiles` once they exist.
- Add SQL migration files under `features/ai-npc-response/sql` if the feature needs schema/rule data.
- Add command, packet, or manager entry points here as they become part of the portable feature.
- Add external/test-client files to `features/ai-npc-response/patcher.yml` so the workspace patcher feed can publish them.
- In `patcher.yml`, `source` is repo-relative and `destination` is relative to the EverQuest client root.
- If the feature has native client behavior, add the feature-owned DLL project/source and build this checkout's own `dinput8.dll`.
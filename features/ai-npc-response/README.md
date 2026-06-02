# AI NPC response

Standalone EQEmu feature branch for `ai-npc-response`.

## Test Target

- Server: `D:\EQServers\EQServer-Ai-Npc-Response`
- Client: `D:\EQClients\EQClient-Ai-Npc-Response`
- Database: `eqemu_ai_npc_response`

## First Build Loop

From `D:\Codex\Apps\EQEmu-feature-workspaces`:

~~~powershell
.\verify-feature.ps1 ai-npc-response
.\install-server-runtime.ps1 ai-npc-response
.\install-client-files.ps1 ai-npc-response
.\run-db-updates.ps1 ai-npc-response
.\validate-install.ps1 ai-npc-response
~~~

`install-server-runtime.ps1` also refreshes Windows firewall allow rules for the copied server binaries.

## Client UI

Native XML `EQUI_NativeAiNpcResponseWnd.xml` is installed into the prepared client and included from `uifiles/default/EQUI.xml` by the workspace client install scripts.

## Client Patcher Feed

External/test-client patch syncing is owned by `features/ai-npc-response/patcher.yml`. Do not add files directly to `D:\EQClients\EQClient-Ai-Npc-Response` as the source of truth. Add any client-facing XML, DLL, config, zone asset, patch note, status file, or other tester-facing file to this repo and list it there before publishing a patcher feed.

## Development Notes

Replace this section with feature behavior, commands, SQL, client files, operator install notes, and known test cases as the feature takes shape.
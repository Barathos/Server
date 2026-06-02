# AI NPC response

Design and operator documentation for the standalone `ai-npc-response` feature.

## Overview

Describe the feature behavior, player/admin workflows, commands, database changes, client files, and how another server operator can install it.

## Local Verification

- Build: `.\verify-feature.ps1 ai-npc-response`
- Install runtime: `.\install-server-runtime.ps1 ai-npc-response`
- Install client files: `.\install-client-files.ps1 ai-npc-response`
- Run DB updates: `.\run-db-updates.ps1 ai-npc-response`
- Validate install: `.\validate-install.ps1 ai-npc-response`

## External Client Sync

- Client patch manifest: `features/ai-npc-response/patcher.yml`
- `-Project` is the workspace install id from `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`. It usually matches the feature id, but confirm it first.
- Patcher host commands:

~~~powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
~~~

Missing files are release blockers for real external syncs. Use `-AllowMissingClientFiles` only for partial local testing.
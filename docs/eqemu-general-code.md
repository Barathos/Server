# EQEMU General Code

Design and operator documentation for the standalone `eqemu-general-code` feature.

## Overview

Describe the feature behavior, player/admin workflows, commands, database changes, client files, and how another server operator can install it.

## Local Verification

- Build: `.\verify-feature.ps1 eqemu-general-code`
- Install runtime: `.\install-server-runtime.ps1 eqemu-general-code`
- Install client files: `.\install-client-files.ps1 eqemu-general-code`
- Run DB updates: `.\run-db-updates.ps1 eqemu-general-code`
- Validate install: `.\validate-install.ps1 eqemu-general-code`

## External Client Sync

- Client patch manifest: `features/eqemu-general-code/patcher.yml`
- `-Project` is the workspace install id from `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`. It usually matches the feature id, but confirm it first.
- Patcher host commands:

~~~powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
~~~

Missing files are release blockers for real external syncs. Use `-AllowMissingClientFiles` only for partial local testing.
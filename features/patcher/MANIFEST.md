# patcher Manifest

Feature id: `patcher`

This manifest starts intentionally small. Add required source files, SQL files,
client UI files, and operator notes as the feature becomes real.

## Imported Source

- `features/patcher/eqemupatcher` imports
  `https://github.com/xackery/eqemupatcher` at
  `645a5e73b1559976363585474bbd812faa196ea5`.
- Windows Forms patcher solution:
  `features/patcher/eqemupatcher/EQEmu Patcher/EQEmu Patcher.sln`
- Default filelistbuilder config:
  `features/patcher/eqemupatcher/filelistbuilder.yml`
- Seed client patch files:
  `features/patcher/eqemupatcher/rof`
- Player-facing patch notes seed:
  `features/patcher/eqemupatcher/rof/patch_notes.txt`
- Player-facing service status seed:
  `features/patcher/eqemupatcher/rof/patcher_status.yml`
- Static patch hosting scripts:
  `features/patcher/eqemupatcher/service`
- Workspace patch feed generator:
  `features/patcher/eqemupatcher/service/New-WorkspacePatcherDeployment.ps1`
- Workspace patch feed tester:
  `features/patcher/eqemupatcher/service/Test-WorkspacePatcherDeployment.ps1`
- Project-owned patcher manifest:
  `features/patcher/patcher.yml`

## Test Target

- Server: `D:\EQServers\EQServer-Patcher`
- Client: `D:\EQClients\EQClient-Patcher`
- Database: `eqemu_patcher`

## Expected First Implementation Updates

- Add feature-owned source files to `features.json` `requiredFiles` once they exist.
- Add SQL migration files under `features/patcher/sql` if the feature needs schema/rule data.
- Add command, packet, or manager entry points here as they become part of the portable feature.
- If the feature has native client behavior, add the feature-owned DLL project/source and build this checkout's own `dinput8.dll`.
- For player patch deployment, publish `eqemupatcher.exe`, its hash, `patch_notes.txt`,
  `patcher_status.yml`, `filelist_rof.yml`, and payload files from the generated service publish folder.
- For local feature testing, each feature owns its patch contents in
  `features/<feature-id>/patcher.yml`; run `New-WorkspacePatcherDeployment.ps1`
  to build one feed and one launcher per install listed in
  `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`.

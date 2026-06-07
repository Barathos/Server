# patcher

Standalone EQEmu feature branch for `patcher`.

## Test Target

- Server: `D:\EQServers\EQServer-Patcher`
- Client: `D:\EQClients\EQClient-Patcher`
- Database: `eqemu_patcher`

## First Build Loop

From `D:\Codex\Apps\EQEmu-feature-workspaces`:

~~~powershell
.\verify-feature.ps1 patcher
.\install-server-runtime.ps1 patcher
.\install-client-files.ps1 patcher
.\run-db-updates.ps1 patcher
.\validate-install.ps1 patcher
~~~

`install-server-runtime.ps1` also refreshes Windows firewall allow rules for the copied server binaries.

## Client UI

No native client XML window was scaffolded for this feature.

## Development Notes

The upstream EQEmu patcher source has been imported under
`features/patcher/eqemupatcher`.

- Source: `https://github.com/xackery/eqemupatcher`
- Imported commit: `645a5e73b1559976363585474bbd812faa196ea5`
- Client solution: `features/patcher/eqemupatcher/EQEmu Patcher/EQEmu Patcher.sln`
- File list builder config: `features/patcher/eqemupatcher/filelistbuilder.yml`
- Seed RoF patch payload: `features/patcher/eqemupatcher/rof`

The patcher now supports a player-facing patch notes tab. Build with
`PATCH_NOTES_URL` set to an exact `patch_notes.txt` URL, or leave it empty and
the patcher will look for `patch_notes.txt` beside the file list URL. The
sample notes file is `features/patcher/eqemupatcher/rof/patch_notes.txt`.

The patcher also supports a lightweight status feed through `SERVICE_STATUS_URL`.
Publish `patcher_status.yml` beside the patch notes to update the launcher
service badge and MOTD without rebuilding the client.

## Deployable Patch Service

Static patch hosting scripts live in `features/patcher/eqemupatcher/service`.
They build a publish folder containing the patcher executable, self-update hash,
file list, patch notes, service status, and downloadable payload files.

Typical VM flow:

~~~powershell
cd features\patcher\eqemupatcher\service
.\New-PatcherRelease.ps1 `
  -Client rof `
  -PayloadPath ..\rof `
  -OutputPath .\publish `
  -BaseUrl http://patch-vm.example.com/patcher/ `
  -PatcherExe "..\EQEmu Patcher\EQEmu Patcher\bin\Release\eqemupatcher.exe" `
  -PatcherFileName eqemupatcher `
  -Clean

.\Start-PatcherService.ps1 -Root .\publish -UrlPrefix http://localhost:8080/patcher/
.\Test-PatcherService.ps1 -BaseUrl http://localhost:8080/patcher/
~~~

For a boot-time Windows VM service, run PowerShell as Administrator and use
`Install-PatcherService.ps1` from the same folder.

## Workspace Test Patchers

The service can also generate one patcher feed per local EQEmu feature install
using project-owned `features/<feature-id>/patcher.yml` manifests:

~~~powershell
cd features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Clean -BaseUrl http://localhost:8091/patcher/
.\Start-PatcherService.ps1 -Root .\publish -UrlPrefix http://localhost:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -BaseUrl http://localhost:8091/patcher/
~~~

Each generated launcher is copied to its matching `D:\EQClients\EQClient-*`
folder and points at `http://localhost:8091/patcher/<project>/`.

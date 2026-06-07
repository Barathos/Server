# EQEmu Patcher Service

This folder packages the static HTTP service consumed by `eqemupatcher.exe`.

## Build A Publish Folder

From this folder:

```powershell
.\New-PatcherRelease.ps1 `
  -Client rof `
  -PayloadPath ..\rof `
  -OutputPath .\publish `
  -BaseUrl http://your-patch-vm.example.com/patcher/ `
  -PatcherExe "..\EQEmu Patcher\EQEmu Patcher\bin\Release\eqemupatcher.exe" `
  -PatcherFileName eqemupatcher `
  -Clean
```

The publish folder contains:

- `eqemupatcher.exe`
- `eqemupatcher-hash.txt`
- `patch_notes.txt`
- `patcher_status.yml`
- `service_manifest.yml`
- `rof/filelist_rof.yml`
- copied RoF payload files

## Run Locally

```powershell
.\Start-PatcherService.ps1 -Root .\publish -UrlPrefix http://localhost:8080/patcher/
```

Then test in another terminal:

```powershell
.\Test-PatcherService.ps1 -BaseUrl http://localhost:8080/patcher/
```

## Build Workspace Test Feeds

For local feature testing, generate one patch feed per install in
`D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`:

```powershell
.\New-WorkspacePatcherDeployment.ps1 `
  -Clean `
  -BaseUrl http://localhost:8091/patcher/
```

Each feature repo owns the patch contents through
`features/<feature-id>/patcher.yml`. The workspace manifest is only used to
discover project paths, labels, login settings, and matching local client
folders.

The generator creates `publish/<project>/` feeds, builds a project-specific
`eqemupatcher.exe` for each feed URL, and installs each launcher into its
matching `D:\EQClients\EQClient-*` folder.

Use `-Project all-features,multiclass` to regenerate only selected feeds.
Use `-SkipClientInstall` to publish feeds without copying launchers into client
folders. Use `-AllowMissingClientFiles` only when you want a degraded local feed
even though a feature-owned client asset listed in that project's `patcher.yml`
has not been built yet.

Serve and validate the generated workspace feeds:

```powershell
.\Start-PatcherService.ps1 -Root .\publish -UrlPrefix http://localhost:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -BaseUrl http://localhost:8091/patcher/
```

## Install On A Windows VM

Run PowerShell as Administrator:

```powershell
.\Install-PatcherService.ps1 `
  -Root C:\eqemu-patcher\publish `
  -UrlPrefix http://+:8080/patcher/
```

The installer creates:

- an HTTP URL reservation
- a Windows Firewall allow rule
- a startup scheduled task named `EQEmuPatcherService`

For production, IIS or nginx can serve the same publish folder. The patcher only
needs ordinary static HTTP access to the files.

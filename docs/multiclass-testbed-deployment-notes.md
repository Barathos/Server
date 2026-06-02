# Multiclass Testbed Deployment Notes

Last updated: 2026-05-27

## Project

- Project/install id: `multiclass`
- Name: Multiclass
- Repo: `D:\Codex\Apps\EQEmu-feature-multiclass`
- Local server: `D:\EQServers\EQServer-Multiclass`
- Local client: `D:\EQClients\EQClient-Multiclass`
- Local database: `eqemu_multiclass`
- Workspace install source of truth: `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`

## Current Status

- Local build/install validation passed after commit `f80c00983`.
- Patcher source-of-truth docs and `features/multiclass/patcher.yml` were added in commits:
  - `aff90cabf Document Multiclass patcher sync`
  - `58a8f78ff Clarify Multiclass patcher source of truth`
- No destructive deploy, testbed DB reset, or remote service restart was run while writing this note.
- Current expected public patch feed after publishing: `http://47.181.1.223:8091/patcher/multiclass/`.
- Feed publication status from this repo: not verified in this note pass; regenerate/test through the patcher service before giving testers a patcher.

## Build

- Normal local verifier:

```powershell
cd D:\Codex\Apps\EQEmu-feature-workspaces
.\verify-feature.ps1 multiclass
```

- Build preset: `win-msvc` from `CMakePresets.json`.
- Current dirty local `CMakePresets.json` pins Perl on Windows:
  - `EQEMU_BUILD_PERL=ON`
  - `PERL_EXECUTABLE=C:/Strawberry/perl/bin/perl.exe`
  - `PERL_INCLUDE_PATH=C:/Strawberry/perl/lib/CORE`
  - `PERL_LIBRARY=C:/Strawberry/perl/lib/CORE/libperl524.a`
- Previous verified build before that dirty preset showed Perl disabled because `PERL_LIBRARY` was missing. If Perl is required for testbed parity, commit or reproduce the pin before promotion.
- Native DLL build is feature-local:
  - Solution: `client_files/native_autoloot/eq-core-dll/eq-core-dll-visualstudio2022.sln`
  - Output: `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll`

## Build Gotchas Fixed

- Multiclass native runtime must live in this repo, not another feature/runtime checkout.
- `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll` is ignored by default; force-add it when intentionally checkpointing the feature-owned runtime DLL.
- Windows shared-memory mapping may need sanitized mapping names; there is an existing dirty local change in `common/memory_mapped_file.cpp` that is not committed in this project note. Verify ownership before promoting it.
- Always use `git status --short` before and after build/deploy work. At the time of this note, unrelated dirty files existed outside the note scope.

## Local Deploy / Validation

Safe local install/validation commands:

```powershell
cd D:\Codex\Apps\EQEmu-feature-workspaces
.\install-server-runtime.ps1 multiclass
.\install-client-files.ps1 multiclass
.\validate-install.ps1 multiclass
```

Observed local behavior:

- `install-server-runtime.ps1 multiclass` copies runtime directories such as `D:\server\assets` and `D:\server\maps` to `D:\EQServers\EQServer-Multiclass` and ensures firewall rules.
- `install-client-files.ps1 multiclass` installs client files from the repo-owned manifest and injects native XML includes.
- `validate-install.ps1 multiclass` checks required server/client files, client assets, and native XML inclusion.

Do not treat `D:\EQClients\EQClient-Multiclass` as source of truth. Add client-facing files to this repo and list them in `features/multiclass/patcher.yml`.

## Database Behavior

Feature migrations live in `common/database/database_update_manifest_custom.h` and reference SQL lives in `features/multiclass/sql/001_multiclass_schema.sql`.

Custom migrations:

- Version 1 creates `custom_multiclass_profiles` and `custom_multiclass_profile_audit`.
- Version 2 migrates an older local Multiclass scaffold if present, then drops only old custom scaffold tables:
  - `custom_multiclass_character_settings`
  - `custom_multiclass_character_classes`
  - `custom_multiclass_audit`
- Version 3 creates `custom_multiclass_pet_state`.
- Version 4 creates `custom_multiclass_bard_melody`.

These migrations should not drop/reset accounts, characters, inventory, bots, guilds, variables, or normal tester state.

Danger paths:

- `D:\Codex\Apps\EQEmu-feature-workspaces\import-clean-db.ps1` can run `DROP DATABASE IF EXISTS` when used with `-ConfirmCreate`; do not run it on a live/tester DB unless intentionally wiping the project database.
- `run-db-updates.ps1 multiclass` runs `world.exe database:updates`; by default it passes `--skip-backup`. Use `-Backup` when preserving tester state matters.
- Testbed deploy can back up the database if called with `-BackupDatabase`; do not combine DB-changing flags with `-SkipDatabaseBackup` for real tester data.

## Server Payload

Core feature-owned server sources are listed in `features/multiclass/MANIFEST.md`.

Runtime deploy includes:

- Built EQEmu binaries from `build/win-msvc/bin/Release`.
- Server config generated for `eqemu_multiclass`.
- Login config for client port `5999`.
- Runtime directories copied from `D:\server`, including `assets` and `maps`.
- Quest/plugin assets come from the normal workspace/runtime seed, not from a Multiclass-only quest package yet.

## Client Patcher Payload

Source of truth: `features/multiclass/patcher.yml`.

Current files:

```yaml
files:
  - source: client_files/native_autoloot/eq-core-dll/bin/dinput8.dll
    destination: dinput8.dll
  - source: client_files/native_autoloot/ui/EQUI_NativeMulticlassWnd.xml
    destination: uifiles/default/EQUI_NativeMulticlassWnd.xml
generated:
  eqhost: true
  equiXml: true
  equiIncludes:
    - EQUI_NativeMulticlassWnd.xml
```

Missing client files are release blockers for real external/tester syncs. Use `-AllowMissingClientFiles` only for partial local testing.

## Patcher Feed

Regenerate/test from:

```powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project multiclass -BaseUrl http://47.181.1.223:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project multiclass -BaseUrl http://47.181.1.223:8091/patcher/
```

Important: `-Project` is the workspace install `id` from `installs.json`, not blindly the feature id. It currently matches: `multiclass`.

Publish pipeline wrapper:

```powershell
cd D:\Codex\Apps\EQEmu-feature-workspaces
.\publish-testbed-project.ps1 multiclass -Target eqemu-testbed -ApplyServer -ApproveServiceRestart -RunDatabaseUpdates -BackupDatabase
```

Remote testbed target from `testbed-targets.json`:

- SSH host: `100.115.94.1`
- Public login/patch host: `47.181.1.223`
- Public login port: `5999`
- Remote server: `D:\EQEmu\Testbed\server`
- Remote staging: `D:\EQEmu\Testbed\staging`
- Remote backups: `D:\EQEmu\Testbed\backups`
- Remote patcher publish: `D:\EQEmu\Testbed\patcher\publish`
- Service task: `EQEmu Testbed Server`

## Backups / Rollback

- Testbed server deploy can create `server-before-<stamp>.zip` under `D:\EQEmu\Testbed\backups`.
- Testbed patcher publish creates `patcher-publish-before-<stamp>.zip` under `D:\EQEmu\Testbed\backups` before replacing publish files.
- Client-side local patch scripts back up `uifiles/default/EQUI.xml` as `EQUI.xml.testbed-backup-<stamp>`.
- Database backups are optional and must be requested with `-BackupDatabase` or `run-db-updates.ps1 -Backup`.
- Rollback is manual: stop service, restore server zip, restore DB dump if one was made, restore patcher publish zip, then restart service.

## Service Restart

- Remote server replacement requires explicit `-ApproveServiceRestart`.
- Do not restart the local Multiclass server/client unless the current testing request explicitly asks for it.
- For final tester deploys, expect to restart the remote scheduled task/service after applying server binaries/config.

## Copy / Avoid / Verify

- Copy: project-owned `features/<feature-id>/patcher.yml` pattern for all future feature client payloads.
- Copy: generated `eqhost`, generated `EQUI.xml`, and explicit `generated.equiIncludes` for native UI windows.
- Avoid: editing the local EQ client as the source of truth.
- Avoid: `import-clean-db.ps1 -ConfirmCreate` on tester databases unless intentionally wiping state.
- Avoid: `run-db-updates.ps1` without `-Backup` when tester state matters.
- Verify before promotion:
  - `git status --short` contains only intended changes.
  - `verify-feature.ps1 multiclass` passes.
  - `validate-install.ps1 multiclass` passes.
  - `features/multiclass/patcher.yml` includes every external client-facing file.
  - `Test-WorkspacePatcherDeployment.ps1 -Project multiclass` passes against the public base URL.
  - Perl pinning is either committed or intentionally disabled for the target.

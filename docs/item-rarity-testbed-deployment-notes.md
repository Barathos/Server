# Item Rarity Testbed Deployment Notes

## Project

- Feature id/name: `item-rarity` / Item Rarity.
- Repo: `D:\Codex\Apps\EQEmu-feature-item-rarity`.
- Current branch/base checked while writing this note: `codex/feature-item-rarity` at `3b26b9dd0`.
- Workspace install id: `item-rarity` in `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`. Verify this before patcher or publish commands; do not assume the feature id is always the patcher `-Project` value.
- Server/client/database: `D:\EQServers\EQServer-Item-Rarity`, `D:\EQClients\EQClient-Item-Rarity`, `eqemu_item_rarity`.

## Current Status

- Item-rarity server/client outputs were previously built and installed locally; `zone.exe` and the feature `dinput8.dll` matched the installed copies during prior validation.
- No destructive deploy, DB reset, migration, or service restart was run while writing this note.
- Public testbed promotion should be treated as pending until `.\verify-feature.ps1 item-rarity`, runtime/client install, DB update review, and `.\validate-install.ps1 item-rarity` pass from `D:\Codex\Apps\EQEmu-feature-workspaces`.
- The repo currently owns the external client payload through `features\item-rarity\patcher.yml`; do not use files copied directly out of the local EQ client as source of truth.

## Build

- Server build command: `cd D:\Codex\Apps\EQEmu-feature-workspaces; .\verify-feature.ps1 item-rarity`.
- CMake preset: `win-msvc`, build dir `build/win-msvc`, `Release`, targets `zone` and `world`.
- Native DLL build: `client_files\item_rarity\eq-core-dll\item-rarity-dll.sln`, `Release|Win32`.
- Perl is enabled and pinned for the MSVC preset:
  - `EQEMU_BUILD_PERL=ON`
  - `PERL_EXECUTABLE=C:\Strawberry\perl\bin\perl.exe`
  - `PERL_INCLUDE_PATH=C:\Strawberry\perl\lib\CORE`
  - `PERL_LIBRARY=C:\Strawberry\perl\lib\CORE\libperl524.a`
- `D:\Codex\Apps\EQEmu-feature-workspaces\scripts\FeatureWorkspace.ps1` injects the same Perl args and refuses a stale CMake cache if Perl is missing or disabled.

## Fixed Gotchas

- Do not configure this project with a Perl-less EQEmu server cache; zones can build but Perl-backed quest behavior is wrong for the testbed.
- `install-client-files.ps1` / `Copy-InstallClientFiles` enforce that `dinput8.dll` comes from this feature's `client_files\item_rarity` payload, preventing shared native runtime DLL bleed into unrelated clients.
- Native chat manager / chat queue hooks caused character-login crashes during link-color experiments. Keep those disabled unless doing a dedicated native-client reverse-engineering pass.
- Server-side chat color cannot override the stock purple clickable item-link span after the RoF2 client parses raw item links. The stable fallback is rarity-colored plain text plus an optional purple inspect/click link, not embedded `<c>` tags inside raw item-link payloads.

## Database

- Feature SQL: `features\item-rarity\sql\001_item_rarity.sql`; expected feature table is `item_rarity`.
- Runtime DB command: `.\run-db-updates.ps1 item-rarity` runs `world.exe database:updates` against `eqemu_item_rarity`.
- Default DB update behavior appends `--skip-backup`. For tester state, prefer `.\run-db-updates.ps1 item-rarity -Backup` or an external `mysqldump` before schema changes.
- This feature's normal SQL/update path should not drop or reset accounts, characters, inventory, bots, guilds, variables, or other tester state. Do not run clean-db, reseed, source-clean, or full reset scripts against `eqemu_item_rarity` unless the reset is explicitly approved and backed up.

## Server Payload

- Runtime install command: `cd D:\Codex\Apps\EQEmu-feature-workspaces; .\install-server-runtime.ps1 item-rarity`.
- Installs feature `world.exe` and `zone.exe` into `D:\EQServers\EQServer-Item-Rarity\bin`.
- Also copies common runtime binaries/DLLs from the server seed (`D:\server\bin` by default), plus assets/maps/default support files such as `assets`, `maps`, `utils\defaults\log.ini`, and `mime.types`.
- Isolated install paths should remain under `D:\EQServers\EQServer-Item-Rarity`; quests/plugins/lua modules are copied from the clean PEQ seed when the workspace install layout is prepared.
- Server binary/config changes require stopping and restarting the item-rarity server only. Avoid touching other feature servers.

## Client Patcher Payload

- Client install command for local testing: `cd D:\Codex\Apps\EQEmu-feature-workspaces; .\install-client-files.ps1 item-rarity`.
- `features\item-rarity\patcher.yml` currently publishes:
  - `client_files\item_rarity\eq-core-dll\bin\dinput8.dll` -> `dinput8.dll`
  - `generated.eqhost=true`
  - `generated.equiXml=false`
  - `generated.equiIncludes=[]`
- Native DLL deploy target is only `D:\EQClients\EQClient-Item-Rarity\dinput8.dll`.
- DLL changes require a full EQ client restart.

## Patcher Feed

- Regenerate from `D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service`:

```powershell
.\New-WorkspacePatcherDeployment.ps1 -Project item-rarity -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project item-rarity -BaseUrl http://<patch-host>:8091/patcher/
```

- Public feed URL after regeneration: `http://<patch-host>:8091/patcher/item-rarity/`.
- Missing client files are a release blocker for external testers. Use `-AllowMissingClientFiles` only for partial local testing.

## Known Testbed Issues And Rollback

- Clickable chat item links remain purple in the stock client. Inspect-window item-name coloring is the reliable native path; verify `D:\EQClients\EQClient-Item-Rarity\item_rarity_native.log` for rarity cache and label-recolor lines before changing hooks.
- If character login crashes after a native DLL change, rollback by replacing or removing `D:\EQClients\EQClient-Item-Rarity\dinput8.dll`, then restart the EQ client. Do not swap in another feature's DLL.
- If server binaries regress, restore prior `world.exe`/`zone.exe` under `D:\EQServers\EQServer-Item-Rarity\bin` and restart only the item-rarity world/zones.
- Stop unrelated native UI/autoloot test servers before running this project if ports or injected DLLs collide.
- Before promoting another feature, copy the Perl pin/cache validation pattern, the feature-owned `patcher.yml` client payload model, and the `dinput8.dll` source-feature guard. Avoid copying the failed raw-link recolor and native chat queue hook experiments.

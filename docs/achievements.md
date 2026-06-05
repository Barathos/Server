# Achievements

The custom Achievement system tracks player progress against database-defined objectives and can render the results through text commands or the native Achievement window.

## Server Flow

1. `#ach` opens or refreshes the native achievement window.
2. Gameplay hooks call `AchievementManager` for level, zone visit, task completion, skill, and kill progress.
3. Matching objectives update `custom_character_achievement_progress`.
4. When all required objectives are complete, the character gets a row in `custom_character_achievements`.
5. Reward definitions are queued in `custom_character_achievement_rewards`; auto-claim rewards are granted immediately and manual rewards can be claimed with `#ach claim`.
6. The server sends `ACH|...` lines to the native client runtime for window refreshes, including reward previews.

## Database

This branch uses `common/database/database_update_manifest_custom.h` as the database source of truth. The standalone branch renumbers the achievements migrations to custom versions `1-4`, and `common/version.h` sets `CUSTOM_BINARY_DATABASE_VERSION` to `4`.

Version `4` adds achievement reward definitions, per-character reward claim queues, a Live Item request handoff table, and cross-achievement meta objectives.

## Commands

- `#ach`
- `#ach window`
- `#ach status`
- `#ach categories`
- `#ach category [category_id]`
- `#ach detail [achievement_id]`
- `#ach rewards`
- `#ach claim [reward_id|all]`
- `#ach check`

## Rewards

Supported reward types:

- `title_text`
- `title_suffix`
- `title_set`
- `item`
- `currency`
- `coin`
- `live_item_request`

Live Item rewards are queued as requests in `custom_achievement_live_item_requests`. This branch does not generate the item directly; the Live Items feature can consume that request table when installed.

## Client Assets

Client patch syncing is owned by `features/achievements/patcher.yml`.

The patcher manifest maps repo files to paths inside the EverQuest client folder. The current client-facing file is:

- `client_files/native_autoloot/ui/EQUI_NativeAchievementWnd.xml` -> `uifiles/default/EQUI_NativeAchievementWnd.xml`

The manifest also requests generated `eqhost`, generated `EQUI.xml`, and an include for `EQUI_NativeAchievementWnd.xml`.

The XML uses the Live EQ achievement window as its visual reference and ships the matching `Achievement_*.tga` art files through the feature patcher manifest. Feature-specific native DLL work belongs in this checkout and should deploy only to the matching Achievements client folder.

If this feature adds a `dinput8.dll`, config file, patch notes, zone asset, or other external/test-client file, add it to `features/achievements/patcher.yml`. Missing files in the manifest are release blockers for real external syncs; use `-AllowMissingClientFiles` only for partial local testing.

Patcher feed regeneration is done from the patcher project:

```powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project achievements -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project achievements -BaseUrl http://<patch-host>:8091/patcher/
```

The feed is published at:

```text
http://<patch-host>:8091/patcher/achievements/
```

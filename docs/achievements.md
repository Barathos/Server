# Achievements

The custom Achievement system tracks player progress against database-defined objectives and can render the results through text commands or the native Achievement window.

## Server Flow

1. `#ach` opens or refreshes the native achievement window.
2. Gameplay hooks call `AchievementManager` for level, zone visit, task completion, skill, and kill progress.
3. Matching objectives update `custom_character_achievement_progress`.
4. When all required objectives are complete, the character gets a row in `custom_character_achievements`.
5. The server sends `ACH|...` lines to the native client runtime for window refreshes.

## Database

This branch uses `common/database/database_update_manifest_custom.h` as the database source of truth. The standalone branch renumbers the achievements migrations to custom versions `1-3`, and `common/version.h` sets `CUSTOM_BINARY_DATABASE_VERSION` to `3`.

## Commands

- `#ach`
- `#ach window`
- `#ach status`
- `#ach categories`
- `#ach category [category_id]`
- `#ach detail [achievement_id]`
- `#ach check`

## Client Assets

Deploy `client_files/native_autoloot/ui/EQUI_NativeAchievementWnd.xml` to the target client UI folder and include it from `EQUI.xml`.

The XML is standalone, but the current native DLL code that listens for `ACH|...` is still shared with the lab native-client runtime.

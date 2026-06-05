# Achievements To All-Features Handoff

This document is the merge note for pulling the recent standalone Achievements work into `D:\Codex\Apps\EQEmu-feature-all`.

## Source State

- Source checkout: `D:\Codex\Apps\EQEmu-feature-achievements`
- Source branch: `codex/feature-achievements`
- Current handoff commit: `9a65d9d2f`
- Matching local test target: `D:\EQServers\EQServer-Achievements`, `D:\EQClients\EQClient-Achievements`, database `eqemu_achievements`
- Tested by opening the updated native Achievements UI in-game from the matching Achievements client.

Recent commits to port:

- `2b201510c` - Live-EQ-inspired Achievements UI polish.
- `9a65d9d2f` - Feature-owned Achievements native DLL, reward-list binding, and patcher DLL entry.

## Important Merge Direction

Do not copy the Achievements-only `dinput8.dll` over the all-features DLL.

The standalone Achievements checkout intentionally owns a cleaned, Achievements-only native DLL. The all-features project must keep its combined native runtime and port only the Achievements-specific pieces:

- `ACH|...` transport parsing fixes.
- `NativeAchievementWnd` binding/layout fixes.
- Reward list support.
- Slash-command rewrite support for `/ach`, `/achievement`, `/achievements`, and common misspellings if desired.
- Clean UI/reload handling for the Achievements window.

The all-features DLL must continue to include the other all-features native windows and transports.

## Files To Port

Client UI and assets:

- `client_files/native_autoloot/ui/EQUI_NativeAchievementWnd.xml`
- `client_files/native_autoloot/ui/Achievement_account_unlock_color.tga`
- `client_files/native_autoloot/ui/Achievement_achieved_color.tga`
- `client_files/native_autoloot/ui/Achievement_hunter_collect_base.tga`
- `client_files/native_autoloot/ui/Achievement_hunter_collect_mouseover.tga`
- `client_files/native_autoloot/ui/Achievement_hunter_collect_selected.tga`
- `client_files/native_autoloot/ui/Achievement_lock_color.tga`
- `client_files/native_autoloot/ui/Achievement_subcat_base.tga`
- `client_files/native_autoloot/ui/Achievement_subcat_base01.tga`
- `client_files/native_autoloot/ui/Achievement_subcat_mouseover.tga`
- `client_files/native_autoloot/ui/Achievement_subcat_mouseover01.tga`
- `client_files/native_autoloot/ui/Achievement_subcat_selected.tga`
- `client_files/native_autoloot/ui/Achievement_subcat_selected01.tga`
- `client_files/native_autoloot/ui/Achievement_Titlebar.tga`
- `client_files/native_autoloot/ui/Achievement_TitleBorders.tga`
- `client_files/native_autoloot/ui/Achievement_unlock_color.tga`

Native reference source:

- `client_files/native_autoloot/eq-core-dll/src/core_achievements_native.h`
- `client_files/native_autoloot/eq-core-dll/src/core_init.h`
- `client_files/native_autoloot/eq-core-dll/src/eqgame.cpp`

Use those native files as a reference only. In all-features, merge the Achievements-specific logic into the existing combined native source rather than replacing it.

Docs and patcher metadata:

- `docs/achievements.md`
- `features/achievements/MANIFEST.md`
- `features/achievements/patcher.yml`

All-features should add the new Achievements XML/TGA files and its built combined `dinput8.dll` to its own all-features patcher manifest.

## Native UI Details

The updated XML keeps the existing `NAW_*` control names expected by the native DLL and adds:

- `NAW_RewardList` listbox.
- Narrower `NAW_ObjectiveList` columns to make room for reward preview rows.
- Live-like achievement category/list/detail spacing and `Achievement_*.tga` art.

The native code should bind and populate:

- `NAW_CategoryList`
- `NAW_AchievementList`
- `NAW_ObjectiveList`
- `NAW_RewardList`
- `NAW_TitleLabel`
- `NAW_SummaryLabel`
- `NAW_PointsLabel`
- `NAW_StatusLabel`
- `NAW_RewardHintLabel`

Transport lines handled by the current standalone native implementation:

- `ACH|window|...`
- `ACH|categories|clear`
- `ACH|category|...`
- `ACH|achievements|clear`
- `ACH|achievement|...`
- `ACH|objectives|clear`
- `ACH|objective|...`
- `ACH|rewards|clear`
- `ACH|reward|...`

## Server Feature Surface

Standalone Achievements owns:

- `zone/achievement_manager.cpp`
- `zone/achievement_manager.h`
- `zone/gm_commands/achievements.cpp`
- Hook patches in `zone/attack.cpp`, `zone/client.cpp`, `zone/client_packet.cpp`, `zone/exp.cpp`, and `zone/task_client_state.cpp`
- Command registration in `zone/command.cpp` and `zone/command.h`
- Build entry in `zone/CMakeLists.txt`
- Custom DB entries in `common/database/database_update_manifest_custom.h`
- `CUSTOM_BINARY_DATABASE_VERSION 4` in `common/version.h`

Database objects:

- `custom_achievement_categories`
- `custom_achievements`
- `custom_achievement_objectives`
- `custom_character_achievement_progress`
- `custom_character_achievements`
- `custom_achievement_audit`
- `custom_achievement_rewards`
- `custom_character_achievement_rewards`
- `custom_achievement_live_item_requests`
- `custom_account_achievement_unlocks`

Supported reward types:

- `title_text`
- `title_suffix`
- `title_set`
- `item`
- `currency`
- `coin`
- `live_item_request`

`live_item_request` rewards queue rows in `custom_achievement_live_item_requests`; all-features can let the Live Items system consume those rows when that rule is enabled.

## Rule Gating Recommendations

Standalone Achievements is always enabled. All-features should add rule gates.

Recommended minimum gate:

- `RuleB(Custom, AchievementsEnabled)`

Gate these with `AchievementsEnabled`:

- `#ach`, `#achievement`, `#achievements` command behavior.
- All gameplay hook calls into `AchievementManager`.
- Completion writes to achievement progress/completion tables.
- Reward queueing/granting.
- `ACH|...` native transport output.

Optional granular gates:

- `RuleB(Custom, AchievementsNativeWindowEnabled)` - allows text commands while disabling the native window transport.
- `RuleB(Custom, AchievementRewardsEnabled)` - allows progress/completion while disabling reward grants.
- `RuleB(Custom, AchievementLiveItemRewardsEnabled)` - allows ordinary rewards while preventing `custom_achievement_live_item_requests` creation unless Live Items is enabled.

Suggested behavior when disabled:

- `#ach` should print a short disabled message and avoid sending `ACH|...`.
- Gameplay hooks should return before touching `AchievementManager`.
- Claims should reject cleanly without modifying reward rows.

Do not gate database schema creation behind runtime rules; migrations should be deterministic.

## Patcher Notes

Standalone patcher manifest: `features/achievements/patcher.yml`

Current standalone payload:

- `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll` -> `dinput8.dll`
- `client_files/native_autoloot/ui/EQUI_NativeAchievementWnd.xml` -> `uifiles/default/EQUI_NativeAchievementWnd.xml`
- `client_files/native_autoloot/ui/Achievement_*.tga` -> `uifiles/default/Achievement_*.tga`
- generated `eqhost`
- generated `EQUI.xml`
- `EQUI_NativeAchievementWnd.xml` include

For all-features, point `dinput8.dll` at the all-features built DLL, not the standalone Achievements DLL.

Missing patcher sources should block release feeds. Use `-AllowMissingClientFiles` only for partial local testing.

## Build And Test Commands

Standalone server verification:

```powershell
cd D:\Codex\Apps\EQEmu-feature-achievements
cmake --preset win-msvc
cmake --build build\win-msvc --config Release --target zone -- /m
cmake --build build\win-msvc --config Release --target world -- /m
git diff --check
```

Standalone native DLL reference build:

```powershell
cd D:\Codex\Apps\EQEmu-feature-achievements
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe' `
  client_files\native_autoloot\eq-core-dll\eq-core-dll-visualstudio2022.sln `
  /p:Configuration=Release /p:Platform=Win32 /m /verbosity:quiet /consoleloggerparameters:ErrorsOnly
```

All-features merge verification should include:

- Build all-features `zone` and `world`.
- Build all-features combined native DLL.
- Deploy the combined all-features DLL and updated Achievements XML/TGAs to `D:\EQClients\EQClient-All-Features`.
- Run `#ach window`.
- Confirm categories load.
- Confirm achievement rows load.
- Confirm selecting achievements updates detail/objective/reward panes.
- Confirm closing/reopening does not duplicate rows.
- Confirm disabling the new rule hides or disables Achievements without breaking other features.
- Confirm other native windows still work after the Achievements native changes.

## Smoke Test Checklist

In an enabled all-features ruleset:

1. Log in with the all-features client.
2. Run `#ach status`.
3. Run `#ach categories`.
4. Run `#ach category 1`.
5. Run `#ach window`.
6. Select multiple categories and achievements.
7. Verify objectives and reward previews update.
8. Complete or manually trigger a simple objective with `#ach check`.
9. Run `#ach rewards`.
10. Run `#ach claim all`.
11. If Live Items is enabled, verify `live_item_request` rewards queue or process correctly.

In a disabled all-features ruleset:

1. Run `#ach`.
2. Confirm a clean disabled message.
3. Confirm gameplay hooks do not create progress rows.
4. Confirm native `ACH|...` lines are not emitted.

## Safety Notes

- Do not touch `D:\Codex\Apps\EQEmu-native-client-runtime` for this merge.
- Do not deploy the standalone Achievements-only DLL into `D:\EQClients\EQClient-All-Features`.
- Do not remove AutoLoot, Live Items, Live Spells, Item Forge, Spell Forge, Multiclass, or other all-features native code while porting the Achievements window fixes.
- Preserve unrelated dirty worktree changes in all-features.
- Back up any target database before running updates or applying feature SQL.

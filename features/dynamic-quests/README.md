# Dynamic Quests

Standalone EQEmu feature branch for `dynamic-quests`.

## Recommendation

The first prototype builds on the existing EQEmu task system instead of creating a parallel quest engine. `TaskType::Quest` already gives players an accepted quest list, objective rows, live progress packets, completion state, abandon/cancel behavior, persistence, and Perl/Lua callbacks.

Task definitions are cached in each zone process. Players do not need any reload command to accept, progress, complete, or abandon already-loaded tasks. This branch adds lazy single-task loading on task selector/accept misses, including task-set and shared-task selectors, so a newly inserted task id can be offered without a full task reload. Edited existing task definitions still need a targeted dev reload:

```text
#task reload task 910001
```

Use broad reload only when touching many task rows or task sets:

```text
#reload tasks
#task reloadall
```

## Prototype Content

- Task `910001`, `First Steps in Gloomingdeep`
- NPC `910001`, `Scout_Deryn`, spawned in `tutorialb`
- Quest script `features/dynamic-quests/quests/tutorialb/910001.lua`
- Seed SQL `features/dynamic-quests/sql/001_dynamic_quests_task_seed.sql`
- Objective types:
  - Kill: defeat 3 NPCs matching `kobold`
  - SpeakWith: report back to `Scout_Deryn`

The authoritative player UI for this prototype is the built-in Task/Quest Journal. The native XML file is a tracked shell for the future custom tracker window; no feature DLL is required for this proof.

## Development Loop

1. Edit `tasks` / `task_activities` rows or the source-backed SQL.
2. Apply the additive feature SQL to the local feature database when needed.
3. Use `#task reload task 910001` for an edited existing task, or simply offer a newly inserted task id and let the zone lazy-load it on cache miss.
4. Use `#reload quest` after Lua/Perl script edits.
5. Hail Scout Deryn in `tutorialb` and say `dynamic quest`.

## Test Target

- Server: `D:\EQServers\EQServer-Dynamic-Quests`
- Client: `D:\EQClients\EQClient-Dynamic-Quests`
- Database: `eqemu_dynamic_quests`

## Build Loop

From `D:\Codex\Apps\EQEmu-feature-workspaces` after implementation is approved:

```powershell
.\verify-feature.ps1 dynamic-quests
.\initialize-server-runtime-template.ps1 dynamic-quests -Force -GrantRuntimeDbUser
.\install-server-runtime.ps1 dynamic-quests
.\install-client-files.ps1 dynamic-quests
.\run-db-updates.ps1 dynamic-quests
.\validate-install.ps1 dynamic-quests
```

Standalone feature SQL is intentionally kept under `features/dynamic-quests/sql`. Apply it only when this prototype content should be seeded into the target database.

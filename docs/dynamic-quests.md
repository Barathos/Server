# Dynamic Quests

## Feasibility Read

The existing EQEmu task system is viable for the first Dynamic Quests / WoW-style tracker prototype.

What it already provides:

- `TaskType::Quest` active quest slots, separate from the single task and shared task slots.
- Persistent accepted/completed state in `character_tasks`, `character_activities`, and `completed_tasks`.
- Objective types for kill, loot, speak with, explore, deliver, give cash, touch/use-like triggers, tradeskill, fish, forage, cast, skill, and collect.
- Live progress packets through `OP_TaskActivity` plus completion packets through `OP_TaskActivityComplete`.
- Task description/select packets for the built-in task journal.
- Perl/Lua entry points such as `taskselector`, `assigntask`, `UpdateTaskActivity`, and task completion/update events.

The static part is task definition loading. Definitions are cached per zone process, so edited existing definitions still need a reload. This prototype makes that reload targeted and adds lazy loading for new task ids on selector/accept misses. Players do not need to run any reload command when they receive a task.

## Architecture Recommendation

Use the built-in task system now. Do not build a separate Live Quest schema yet.

The smallest useful architecture is:

- Quest definitions: source-backed SQL rows in `tasks` and `task_activities`.
- Character progress: existing EQEmu task state tables.
- Player accept path: Lua/Perl NPC scripts using `TaskSelector` or `AssignTask`.
- Live updates: existing task activity update hooks and packets.
- Development reload: targeted `#task reload task <id>` for edited rows; lazy load for newly inserted ids.
- UI: built-in task journal for v0, with `EQUI_NativeDynamicQuestsWnd.xml` kept as a future custom tracker shell.

A new Live Quest system should wait until we prove the built-in task UI cannot present the desired tracker or until authoring needs exceed task/activity rows.

## Runtime Reload Notes

Existing broad reloads still work:

```text
#reload tasks
#task reloadall
```

Targeted reload now preserves the rest of the task cache:

```text
#task reload task 910001
```

Task sets can still be refreshed separately:

```text
#task reload sets
```

Lazy loading is intentionally narrow. If a task id is not present in a zone's task cache, the task selector, task-set selector, shared-task selector, and accept path attempt to load that single id from the content database. This helps newly added tasks during development without turning every accept into a database reload.

## Prototype Definition Format

The current authoring format is SQL because it maps directly to EQEmu task repositories and avoids a new parser in the prototype.

Task rows live in:

```text
features/dynamic-quests/sql/001_dynamic_quests_task_seed.sql
```

The sample task:

- `tasks.id = 910001`
- `tasks.type = 2` (`TaskType::Quest`)
- `task_activities.activitytype = 2` for `Kill`
- `task_activities.activitytype = 4` for `SpeakWith`

Future authoring can layer YAML/JSON over this by generating the same task/activity rows.

## Sample Quest

`Scout_Deryn` spawns in `tutorialb` from the feature SQL. Hail him and say `dynamic quest`.

The offered quest, `First Steps in Gloomingdeep`, has two visible objectives:

- Defeat 3 kobold invaders.
- Report back to Scout Deryn.

Kill progress and speak progress update without relogging.

## Local Test Checklist

1. Build `zone` and `world` with Perl enabled through `verify-feature.ps1`.
2. Install runtime/client files only after approval.
3. Apply `features/dynamic-quests/sql/001_dynamic_quests_task_seed.sql` to `eqemu_dynamic_quests` when ready to seed prototype content.
4. Reload quests after Lua edits: `#reload quest`.
5. For edited task rows, run `#task reload task 910001`.
6. Log into `tutorialb`, hail Scout Deryn, say `dynamic quest`, and accept the task.
7. Confirm the task appears in the built-in task journal.
8. Kill kobolds and confirm objective count updates live.
9. Speak to Scout Deryn and confirm the SpeakWith objective completes.
10. Confirm completion reward/message, completed task history, and abandon/cancel behavior.

## Public Testbed Checklist

- Commit and push the feature branch.
- Ensure `features/dynamic-quests/patcher.yml` lists every client-facing file.
- Treat missing patcher files as release blockers.
- Promote only with explicit approval:

```powershell
cd D:\Codex\Apps\EQEmu-feature-workspaces
.\publish-testbed-project.ps1 dynamic-quests -ApplyServer -ApproveServiceRestart -RunDatabaseUpdates
```

Use `-ApplyFeatureSql` only when the standalone SQL seed should be applied directly to the testbed database.

## Risks And Migration Path

- Built-in task UI may not feel enough like a compact WoW tracker. If that fails testing, reuse the achievement-style `ACH|...` chat transport pattern for a native `DQUEST|...` tracker DLL.
- Existing task tables are content-facing and cached. Targeted reload and lazy load reduce iteration pain but do not provide automatic hot reload for edits.
- `TaskType::Quest` limits active quests to the client-supported quest slot count.
- Some objective types depend on existing EQEmu hooks. Custom script signals can use `UpdateTaskActivity` for now.
- If authoring SQL becomes too clumsy, add a YAML/JSON compiler that writes `tasks` / `task_activities` rows instead of replacing the runtime.

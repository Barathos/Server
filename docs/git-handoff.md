# Item Rarity Git Handoff

Use this note when another developer or another EQEmu project chat needs to
pick up the Item Rarity work from Git.

## Current Branch Location

- Local branch: `codex/feature-item-rarity`
- Pushed remote branch:
  `https://github.com/Barathos/Server/tree/codex/feature-item-rarity`
- Remote name in this checkout: `barathos`
- Remote URL: `https://github.com/Barathos/Server.git`
- Latest handoff commit when this note was written:
  `61d871219` / `Checkpoint item-rarity handoff state`

The upstream remote is still:

```text
origin https://github.com/EQEmu/EQEmu.git
```

Pushing directly to `origin` failed with a 403 for the available credentials, so
the handoff branch lives on the `Barathos/Server` fork.

## How To Pick It Up

Fresh clone:

```powershell
git clone https://github.com/Barathos/Server.git
cd Server
git checkout codex/feature-item-rarity
```

Existing EQEmu clone:

```powershell
git remote add barathos https://github.com/Barathos/Server.git
git fetch barathos codex/feature-item-rarity
git switch -c codex/feature-item-rarity --track barathos/codex/feature-item-rarity
```

If the `barathos` remote already exists:

```powershell
git fetch barathos
git switch codex/feature-item-rarity
```

## Where To Look Next

Read these files in this order:

All paths below are relative to the checked-out repository root:

- `AGENTS.md` - project rules, ownership boundaries, deployment workflow, and
  patcher policy.
- `docs/item-rarity-handoff.md` - detailed feature architecture, source paths,
  known limitations, and rollback notes.
- `docs/testbed-deployment-notes.md` - deployment/testbed status, build
  gotchas, database safety notes, and patcher feed notes.
- `docs/item-rarity.md` - operator commands, schema, quick test flow, and
  native client install notes.
- `features/item-rarity/README.md` - feature-facing summary and command list.
- `features/item-rarity/MANIFEST.md` - feature-owned source/schema/client
  payload list.
- `features/item-rarity/patcher.yml` - source of truth for client patcher
  payload files.

## How To Verify This Note Is Still Current

Run from the checked-out repository root:

```powershell
git status -sb
git branch -vv
git remote -v
git log --oneline --decorate -n 5
git ls-remote --heads barathos codex/feature-item-rarity
```

Expected shape:

- `codex/feature-item-rarity` tracks `barathos/codex/feature-item-rarity`.
- `origin` points to `https://github.com/EQEmu/EQEmu.git`.
- `barathos` points to `https://github.com/Barathos/Server.git`.
- The remote branch contains the latest local handoff commit.

## Important Caution

Do not assume `origin/master` contains Item Rarity. The current handoff branch is
ahead of upstream EQEmu and includes feature-specific server code, native client
DLL work, patcher config, and docs. Future work should stay on the feature
branch until it is intentionally promoted or merged.

Machine-specific paths in the deeper deployment docs, such as EQ server/client
install directories, describe this project's prepared test environment. They
are not required for reading the source from Git and may need to be remapped by
the next developer.

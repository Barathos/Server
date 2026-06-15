# Advanced Loot Standalone Handoff - 2026-06-15

This handoff is for the standalone Advanced Loot project at:

```text
D:\Codex\Apps\EQEmu-feature-autoloot-live
```

It explains the current ownership decision, why the existing workflow is too
expensive, and what the project is expected to become.

## Decision

For active maintained Advanced Loot work, `EQEmu-feature-all` is the current
source of truth.

The standalone AutoLoot project should not be treated as a parallel live
implementation that receives independent bug fixes first. While THJ and the
all-features testbed are both exercising the feature, keeping three editable
copies in sync is creating avoidable drift:

- `D:\Codex\Apps\EQEmu-feature-all`
- `D:\Codex\Apps\EQEmu-feature-autoloot-live`
- THJ

The immediate goal is to keep Advanced Loot stable inside `feature-all`, use
that repo as the active maintenance branch, and reduce the standalone project
to an extraction and portability target until it is clean enough to become the
real canonical branch.

## Why

Right now fixes can start in any of these places:

- the all-features public testbed
- THJ live
- the standalone AutoLoot repo

That causes the same problem every time:

1. a bug is fixed in one checkout
2. the same fix then has to be rediscovered or manually ported elsewhere
3. the feature drifts again because the touched files are not isolated to one
   folder

Advanced Loot is not a small isolated patch. It touches:

- shared server code
- zone command paths
- loot and corpse behavior
- native client DLL code
- UI XML
- patcher payload
- docs and operator workflow

That is too much shared surface for "auto sync between projects" to be safe.

## Current Ownership Rule

Until the standalone AutoLoot repo is truly extractable, use this rule:

- `EQEmu-feature-all` is the active maintained branch for Advanced Loot.
- THJ is a consumer branch that should receive promoted fixes.
- `EQEmu-feature-autoloot-live` is a portability/extraction branch, not an
  independently evolving implementation.

If a bug is found on THJ or the testbed:

1. reproduce it where it appears
2. make the fix in `EQEmu-feature-all`
3. commit it there first
4. promote the exact commit into THJ
5. then port or extract it into `EQEmu-feature-autoloot-live`

If production pressure requires a direct THJ hotfix:

1. commit the THJ fix
2. immediately bring that exact commit back into `EQEmu-feature-all`
3. only after that, update the standalone AutoLoot repo

Do not let THJ and `autoloot-live` become peer branches that both define the
feature differently.

## Goal For The Standalone Repo

`EQEmu-feature-autoloot-live` should become a clean feature package that another
project can realistically grab and merge.

That means it needs to be honest about what Advanced Loot owns:

- every added file
- every patched existing file
- every SQL object and migration
- every client payload file
- every shared runtime dependency that still prevents clean extraction

The standalone repo should be treated as successful only when it can answer:

- What exact files does Advanced Loot own?
- What exact files must another server patch?
- What can stay behind in the consumer project?
- What still needs a shared base extraction before the feature is truly
  portable?

If it cannot answer those questions clearly, it is not ready to be the primary
branch.

## Practical Workflow

Use this workflow until the standalone branch is genuinely clean:

1. Maintain Advanced Loot in `D:\Codex\Apps\EQEmu-feature-all`.
2. Use runtime gates and bundle-owned assets there.
3. Promote stable fixes outward to THJ.
4. Periodically extract the same fixes into
   `D:\Codex\Apps\EQEmu-feature-autoloot-live`.
5. Keep the standalone repo focused on portability, not on being the first
   place production fixes land.

The standalone project should lag slightly behind active bundle work if needed.
That is acceptable. What is not acceptable is three different "current"
implementations.

## What "Done" Looks Like

The standalone AutoLoot project can become the real source branch later, but
only after these conditions are true:

- it builds cleanly on a clean EQEmu base
- it owns a complete manifest of touched files
- it has feature-owned docs, SQL, patcher payload, and verification steps
- it does not rely on mystery changes that only exist in `feature-all` or THJ
- promotion from the standalone repo into `feature-all` and THJ is cheaper than
  fixing directly in `feature-all`

Until then, `feature-all` remains the active maintained home for Advanced Loot.

## Immediate Expectation For Autoloot Live

When resuming work in `D:\Codex\Apps\EQEmu-feature-autoloot-live`:

- do not start by adding new behavior there first
- first compare against the current `feature-all` Advanced Loot state
- pull over the maintained fixes from `feature-all`
- document the extraction boundaries and missing shared-runtime splits
- only make standalone-specific cleanup changes that improve portability

This keeps the feature stable for live use while still moving toward a reusable
standalone package.

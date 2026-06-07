# Feature Pack Template

Status: `draft`

Use this shape when separating another custom system into a grab-and-build feature pack.

## What This Feature Owns

- Source-owned mechanics.
- Database-owned state.
- Client assets.
- Player/admin commands.
- Server-to-client transport lines.

## What This Feature Does Not Require

- Other custom systems that are not true dependencies.
- Old prototype scripts, plugins, or overlays.
- Client-trusted validation for exploit-sensitive behavior.

## Dependencies

- Required EQEmu source areas.
- Required database migration support.
- Required client runtime or UI files.
- Optional feature packs.

## Added Files

- `path/to/new/file.cpp`

## Existing Files To Patch

| File | Purpose |
| --- | --- |
| `path/to/existing/file.cpp` | Describe the narrow hook point. |

## Database Objects

- `custom_feature_table`

Put standalone SQL under `sql/`.

## Native Client Assets

- `client_files/...`

Call out shared runtime code separately from feature-specific window code.

## Commands

- `/command`
- `#command`

## Transport

- `FEATURE|...`

The server owns truth. The client displays server state and sends normal commands back.

## Smoke Test

1. Start from a clean login.
2. Exercise the primary command path.
3. Exercise persistence.
4. Exercise the failure path that should leave state unchanged.
5. Verify the feature works when unrelated custom systems are absent.

## Next Split Work

- List the code that still needs to move out of shared files.
- List any generated patch or installer work still missing.

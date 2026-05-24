# Feature Pack Template

Status: `draft`

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

## Existing Files To Patch

| File | Purpose |
| --- | --- |
| `path/to/existing/file.cpp` | Describe the narrow hook point. |

## Smoke Test

1. Start from a clean login.
2. Exercise the primary command path.
3. Exercise persistence.
4. Exercise the failure path that should leave state unchanged.
5. Verify the feature works when unrelated custom systems are absent.

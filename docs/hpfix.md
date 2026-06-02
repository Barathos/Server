# HP Fix

This feature tests a native client-side overlay for self HP totals that exceed the RoF2 client display path.

The server keeps normal `OP_HPUpdate` behavior intact. The DLL opts in automatically with `#hpfix native ready`, the zone server marks that client as native-ready, and self HP changes send a hidden side-channel payload:

```text
HPFIX|self|current=<current>|max=<maximum>|percent=<percent>
```

The DLL suppresses that transport line, opens `NativeHpFixWnd`, and shows the authoritative server current/max/percent values. This is intentionally an overlay first so vanilla client semantics are not changed while the high-HP behavior is tested.

GM test flow:

```text
#hpfix items
#hpfix refresh
#hpfix status
```

`features/hpfix/sql/001_hpfix_test_items.sql` adds 12M, 25M, and 50M HP equipment for validation.

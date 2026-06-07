# Live Spells

Live Spells lets an operator generate spell definitions and matching scroll items at runtime, then sync those definitions to a native client window/transport.

## Server Flow

1. A player opens the Spell Forge with `#livespell dialog`.
2. The native window submits a `#livespell craft ...` command.
3. The server chooses a free spell ID in the reserved range, builds a spell definition, creates a matching scroll item, and stores the definition in `data_buckets`.
4. Zones load persisted definitions on demand before cast, scribe, or memorize paths need spell data.
5. The client receives `LIVESPELL|upsert|...` lines so the native DLL can patch its local spell table.

## Persistence

- Spell definitions use `data_buckets` keys under `live_spell.spell.`.
- The next generated scroll item pointer uses `live_spell.next_scroll_item_id`.
- Generated scroll item rows use the existing `items` table.

## Commands

- `#livespell dialog`
- `#livespell craft element=fire target=target range=200 damage=100 recast=3000 name=Test_Flame`
- `#livespell ready`
- `#livespell patch [spell_id] [base_spell_id]`
- `#livespell test [spell_id] [base_spell_id] [gem]`
- `#livespell scribe [spell_id] [gem]`

## Client Assets

Client patch syncing is owned by `features/live-spells/patcher.yml`. Client-facing files must be added to this repo first, then listed in the patcher manifest with their destination inside the EverQuest client folder. Do not add files directly to the local EQ client as the source of truth; the client folder is only a deployment target.

In `patcher.yml`:

- `files[].source` is a path inside this repo.
- `files[].destination` is a path inside the EverQuest client root.
- `generated.eqhost: true` means the patcher should write `eqhost.txt`.
- `generated.equiXml: true` means native UI XML includes need to be injected.
- `generated.equiIncludes` explicitly lists custom `EQUI_*.xml` windows.

The current patcher manifest publishes:

- `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll` to `dinput8.dll`
- `client_files/native_autoloot/ui/EQUI_NativeSpellForgeWnd.xml` to `uifiles/default/EQUI_NativeSpellForgeWnd.xml`

It also generates `eqhost`, regenerates `EQUI.xml`, and includes `EQUI_NativeSpellForgeWnd.xml`.

For external syncs, look up the workspace install id in `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`, then regenerate the project feed on the patcher host. The id usually matches the feature id, but do not assume that blindly.

```powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
```

The feed is published at `http://<patch-host>:8091/patcher/<project-id>/`. Missing files listed in `patcher.yml` block real external releases; use `-AllowMissingClientFiles` only for partial local testing.

## Native DLL Ownership

Live Spells must not depend on `EQEmu-native-client-runtime` for a feature-specific `dinput8.dll`. If this feature has native client behavior, transport parsing, slash-command rewriting, or native EQ windows, that client DLL work belongs in this checkout and is deployed only to `D:\EQClients\EQClient-Live-Spells`.

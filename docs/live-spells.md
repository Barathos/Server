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

Deploy `client_files/native_autoloot/ui/EQUI_NativeSpellForgeWnd.xml` to the target client UI folder and include it from `EQUI.xml`.

The XML is standalone, but the current native DLL code that listens for `LIVESPELL|...` is still shared with the lab native-client runtime.

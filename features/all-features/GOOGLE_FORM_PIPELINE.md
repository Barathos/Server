# All Features Google Form Pipeline

Use this when the all-features public testbed needs one repeatable player
feedback form covering the combined package.

## All Features Form

The all-features form generator is:

- `features/all-features/google-forms/all-features-public-test-form.gs`

It creates:

- A Google Form named `EQEmu All Features Public Test Feedback`.
- Focused player-side test sections for the features currently visible in the
  all-features bundle.
- A linked Google Sheet for responses, if `CREATE_RESPONSE_SHEET` is `true`.

## Run It

1. Go to `https://script.google.com/`.
2. Create a new Apps Script project.
3. Replace the default `Code.gs` contents with
   `all-features-public-test-form.gs`.
4. Run `createAllFeaturesPublicTestFeedbackForm`.
5. Approve the Google permissions.
6. Open `View > Logs` or `Executions` to copy:
   - Edit form URL
   - Public form URL
   - Responses sheet URL

## Form Scope

The form avoids repeating setup checks per feature. It asks patch/login once,
then groups tests by player-visible behavior:

- Patch/login and native UI loading.
- Live Items, Item Forge, and augment/fusion flows.
- Advanced Loot.
- Achievements.
- Multiclass.
- Live Spells.
- AI NPC Response.
- Dynamic Quests.
- Gearscore, Item Rarity, and Native Interface inspection.
- HP Fix.
- Tradeskill and general stability smoke checks.
- Faction, DPS, and Fellowship tools.
- Pet Bags, Autoskills, UseItem, AutoFollow, and server-auth native stat checks.
- Overall feedback and bugs.

Features with no meaningful player-facing test content yet are represented as
short observation/stability checks instead of artificial tasks.

## Supported Question Types

- `text`
- `paragraph`
- `date`
- `multipleChoice`
- `checkbox`
- `scale`

## Notes

- Keep "Limit to 1 response" disabled unless every tester has a Google account.
- Ask players to test what they can. The form includes "I did not test this"
  answers so partial sessions are still useful.
- The response sheet is the easiest artifact to hand back for review.

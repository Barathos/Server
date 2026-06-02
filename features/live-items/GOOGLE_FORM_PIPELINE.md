# Google Form Pipeline

Use this when a feature needs a repeatable tester feedback form.

## Live Items Form

The Live Items form generator is:

- `features/live-items/google-forms/live-items-public-test-form.gs`

It creates:

- A Google Form named `Live Items Public Test Feedback`.
- All sections and questions from the Live Items tester form blueprint.
- A linked Google Sheet for responses, if `CREATE_RESPONSE_SHEET` is `true`.

## Run It

1. Go to `https://script.google.com/`.
2. Create a new Apps Script project.
3. Replace the default `Code.gs` contents with `live-items-public-test-form.gs`.
4. Run `createLiveItemsPublicTestFeedbackForm`.
5. Approve the Google permissions.
6. Open `View > Logs` or `Executions` to copy:
   - Edit form URL
   - Public form URL
   - Responses sheet URL

## Reuse For Another Feature

1. Copy `live-items-public-test-form.gs`.
2. Rename the file for the new feature.
3. Change:
   - `LIVE_ITEMS_FORM_SPEC.title`
   - `LIVE_ITEMS_FORM_SPEC.description`
   - `LIVE_ITEMS_FORM_SPEC.responseSheetTitle`
   - The `sections` array
4. Rename the entry function, for example:

```javascript
function createMyFeaturePublicTestFeedbackForm() {
  const result = createGoogleFormFromSpec(MY_FEATURE_FORM_SPEC);
  Logger.log('Edit form: ' + result.editUrl);
  Logger.log('Public form: ' + result.publishedUrl);
  Logger.log('Responses sheet: ' + result.responseSheetUrl);
}
```

Leave `createGoogleFormFromSpec` and `addItemToForm` alone unless the new form needs a new question type.

## Supported Question Types

- `text`
- `paragraph`
- `date`
- `multipleChoice`
- `checkbox`
- `scale`

## Notes

- Google Forms does not have to be edited manually after this script runs, but you can still tweak the generated form in the browser.
- If testers do not all have Google accounts, keep "Limit to 1 response" disabled in the form settings.
- The response sheet is the easiest artifact to hand back for review.

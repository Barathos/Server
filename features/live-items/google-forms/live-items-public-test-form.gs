/**
 * Live Items Public Test Feedback form generator.
 *
 * How to use:
 * 1. Go to https://script.google.com/
 * 2. Create a new project.
 * 3. Paste this whole file into Code.gs.
 * 4. Run createLiveItemsPublicTestFeedbackForm().
 * 5. Approve permissions.
 * 6. Open the logged URLs for the edit form, public form, and response sheet.
 */

const CREATE_RESPONSE_SHEET = true;

const LIVE_ITEMS_FORM_SPEC = {
  title: 'Live Items Public Test Feedback',
  description:
    'Thanks for helping test Live Items. Please answer what you can. You do not need perfect notes. ' +
    'If something felt weird, confusing, too strong, too weak, or broken, tell us in plain English. ' +
    'Item links, screenshots, and mob names help if you have them.',
  confirmationMessage: 'Thank you for testing Live Items. Your feedback helps shape the next pass.',
  collectEmail: false,
  allowResponseEdits: true,
  showProgressBar: true,
  shuffleQuestions: false,
  responseSheetTitle: 'Live Items Public Test Feedback Responses',
  sections: [
    {
      title: 'Tester Info',
      items: [
        { type: 'text', title: 'Tester name or Discord name', required: true },
        { type: 'text', title: 'Character name', required: true },
        {
          type: 'text',
          title: 'Class and level',
          helpText: 'Example: Warrior level 12',
          required: true,
        },
        { type: 'date', title: 'Date tested' },
      ],
    },
    {
      title: 'Login And Patching',
      items: [
        {
          type: 'multipleChoice',
          title: 'Were you able to patch your RoF2 client and log in?',
          required: true,
          choices: [
            'Yes, patching and login worked',
            'I patched, but login had problems',
            'I could not patch',
            'I could not reach character select',
          ],
        },
        {
          type: 'multipleChoice',
          title: 'Were you able to enter tutorialb?',
          required: true,
          choices: ['Yes', 'No'],
        },
        {
          type: 'multipleChoice',
          title: 'Did you see any missing file, XML, UI, or startup errors?',
          required: true,
          choices: ['No errors', 'Yes, I saw errors', 'Not sure'],
        },
        {
          type: 'paragraph',
          title: 'If patching, login, or startup had problems, describe what happened.',
        },
      ],
    },
    {
      title: 'Test NPCs',
      items: [
        {
          type: 'checkbox',
          title: 'Which NPCs responded when hailed?',
          required: true,
          choices: ['Vedra Forgecall', 'Orin Augspinner', 'Mavren Instancewright', 'Talia Heirloomkeeper', 'Nalyx Augmentweaver', 'None of them'],
        },
        {
          type: 'multipleChoice',
          title: 'Did the clickable dialogue text work?',
          required: true,
          choices: ['Yes', 'No', 'Some worked, some did not', 'I did not try clickable dialogue'],
        },
        { type: 'paragraph', title: 'Was any NPC text confusing?' },
      ],
    },
    {
      title: 'Tester Helper Commands',
      helpText:
        'These commands are optional, but they make bug reports much easier to trace. Hold an item on your cursor before using #helditemid.',
      items: [
        {
          type: 'checkbox',
          title: 'Which helper commands did you try?',
          choices: ['#helditemid / #itemid / #cursoritemid', '#heirloomdebug / #evolvingdebug', 'I did not try helper commands'],
        },
        {
          type: 'paragraph',
          title: 'Paste any useful helper command output here.',
          helpText: 'Item IDs, heirloom debug output, or anything that seemed wrong.',
        },
      ],
    },
    {
      title: 'Random Loot From Tutorialb Mobs',
      helpText:
        'Kill normal mobs in tutorialb and loot them like normal. The random Live Items loot should appear directly on corpses. It should not be a box or cache you have to open.',
      items: [
        {
          type: 'multipleChoice',
          title: 'About how many normal mobs did you kill?',
          required: true,
          choices: ['1-4', '5-9', '10-19', '20+'],
        },
        {
          type: 'multipleChoice',
          title: 'Did normal mobs have random Live Items loot directly on their corpses?',
          required: true,
          choices: ['Yes, every mob I checked had one', 'Most did, but not all', 'Only some did', 'None did', 'Not sure'],
        },
        {
          type: 'multipleChoice',
          title: 'Did the loot look like normal corpse loot?',
          required: true,
          choices: ['Yes', 'No, it appeared as a box/cache', 'No, something else seemed wrong', 'Not sure'],
        },
        {
          type: 'multipleChoice',
          title: 'Were you able to loot the random items normally?',
          required: true,
          choices: ['Yes', 'No', 'Some worked, some did not'],
        },
        {
          type: 'multipleChoice',
          title: 'Did different mobs give different names or stat rolls?',
          required: true,
          choices: ['Yes, the items felt varied', 'A little, but they felt too similar', 'No, they looked mostly the same', 'Not sure'],
        },
        {
          type: 'multipleChoice',
          title: 'Did random loot keep its stats after zoning, camping, or relogging?',
          choices: ['Yes', 'No', 'I did not test this'],
        },
        {
          type: 'text',
          title: 'What was the best random item you found?',
          helpText: 'Item name/link if you have it.',
        },
        { type: 'text', title: 'What was the strangest, worst, or most broken random item you found?' },
        { type: 'paragraph', title: 'Any notes about random loot?' },
      ],
    },
    {
      title: 'Item Forge Window',
      helpText: 'Talk to Vedra Forgecall and choose open forge. Try making at least one item.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Did the Item Forge window open?',
          required: true,
          choices: ['Yes', 'No', 'It opened, but looked broken'],
        },
        {
          type: 'checkbox',
          title: 'Which item types did you try creating?',
          choices: ['Weapon', 'Armor', 'Jewelry', 'Charm', 'Shield', 'Augment'],
        },
        {
          type: 'multipleChoice',
          title: 'Did the item you created match the name and stats you entered?',
          choices: ['Yes', 'No', 'Some stats worked, some did not', 'I could not create an item'],
        },
        {
          type: 'multipleChoice',
          title: 'Did the step buttons for stat changes work as expected?',
          choices: ['Yes', 'No', 'Some worked, some did not', 'I did not try changing the step size'],
        },
        { type: 'paragraph', title: 'Did any stat field not work or look wrong?' },
        { type: 'paragraph', title: 'Item Forge notes' },
      ],
    },
    {
      title: 'Augment NPC',
      helpText: 'Talk to Orin Augspinner. Say start, add the upgrades you want, optionally name the augment, then say confirm. Use test shards if you need Blood Shards for testing.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Could you build a shardwork augment with the upgrades you wanted?',
          required: true,
          choices: ['Yes', 'No', 'I did not test augments'],
        },
        {
          type: 'multipleChoice',
          title: 'Did Orin spend Blood Shards and give you a custom augment when you confirmed?',
          required: true,
          choices: ['Yes', 'No', 'I did not test augments'],
        },
        {
          type: 'multipleChoice',
          title: 'Did the augment have the stats you purchased?',
          choices: ['Yes', 'No', 'Not sure'],
        },
        {
          type: 'multipleChoice',
          title: 'Could the augment be inserted into normal equipment slots?',
          choices: ['Yes', 'No', 'I did not try inserting it', 'Not sure'],
        },
        {
          type: 'multipleChoice',
          title: 'After inserting the augment, did the target item still look usable and keep its normal slots?',
          choices: ['Yes', 'No', 'I did not try inserting it', 'Not sure'],
        },
        { type: 'paragraph', title: 'If you tried inserting the augment into an item, what happened?' },
        { type: 'paragraph', title: 'Augment notes' },
      ],
    },
    {
      title: 'Instance Upgrade NPC',
      helpText:
        'Talk to Mavren Instancewright. Hand him exactly one Live Items item with no coins and no extra items. He should return that same item as a +1 version with its supported positive stats increased by 10.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Did Mavren return your item as a +1 upgraded version?',
          choices: ['Yes', 'No', 'I did not test this'],
        },
        {
          type: 'multipleChoice',
          title: 'Did Mavren return only the upgraded item, without also giving back the original?',
          choices: ['Yes, only the upgraded item came back', 'No, I also got the original back', 'No, something vanished or got stuck', 'I did not test this'],
        },
        {
          type: 'multipleChoice',
          title: 'Did the upgraded item keep the correct name and stats after zoning, camping, or relogging?',
          choices: ['Yes', 'No', 'I did not test persistence'],
        },
        { type: 'paragraph', title: 'Instance upgrade notes' },
      ],
    },
    {
      title: 'Evolving Heirloom',
      helpText:
        'Talk to Talia Heirloomkeeper and say heirloom. Keep the item equipped or in your inventory, then gain a level if you can. The heirloom should grow stronger when you level.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Did Talia give you a class-matched heirloom weapon?',
          choices: ['Yes', 'No', 'I did not test this'],
        },
        {
          type: 'multipleChoice',
          title: 'Did the heirloom grow stronger when you gained a level?',
          choices: ['Yes', 'No', 'I could not gain a level during testing', 'I did not test this'],
        },
        {
          type: 'multipleChoice',
          title: 'Did #heirloomdebug or #evolvingdebug find the heirloom while it was in inventory or equipped?',
          choices: ['Yes', 'No', 'I did not try the debug command', 'Not sure'],
        },
        { type: 'paragraph', title: 'Evolving heirloom notes' },
      ],
    },
    {
      title: 'Augment Fusion',
      helpText:
        'Talk to Nalyx Augmentweaver. Say catalyst if you need an Augment Catalyst, then hand in exactly one Augment Catalyst plus one to three augment items, with no coins. Nalyx should return one fused augment with the supported stats combined.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Did Nalyx give you an Augment Catalyst when you asked for one?',
          choices: ['Yes', 'No', 'I already had one', 'I did not test this'],
        },
        {
          type: 'multipleChoice',
          title: 'Did Nalyx return one fused augment?',
          choices: ['Yes', 'No', 'I did not test this'],
        },
        {
          type: 'multipleChoice',
          title: 'Did the fused augment show the combined stats you expected?',
          choices: ['Yes', 'No', 'Not sure', 'I did not test this'],
        },
        { type: 'paragraph', title: 'Augment fusion notes' },
      ],
    },
    {
      title: 'Overall Feel',
      helpText: 'These questions are just as useful as bug reports.',
      items: [
        { type: 'paragraph', title: 'What felt fun?' },
        { type: 'paragraph', title: 'What felt annoying?' },
        { type: 'paragraph', title: 'What was confusing?' },
        { type: 'paragraph', title: 'Did any item seem way too strong or way too weak?' },
        {
          type: 'multipleChoice',
          title: 'Would you want this kind of loot while leveling normally?',
          choices: ['Yes', 'Maybe, with changes', 'No', 'Not sure'],
        },
        {
          type: 'scale',
          title: 'Overall, how excited are you about Live Items?',
          lower: 1,
          upper: 5,
          lowLabel: 'Not excited',
          highLabel: 'Very excited',
        },
      ],
    },
    {
      title: 'Bugs Or Problems',
      helpText:
        'If something broke, describe it in plain English. Example: I killed a kobold near the cave entrance. The corpse had no random item. I was a level 5 warrior.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Did you hit any bugs or broken behavior?',
          required: true,
          choices: ['No bugs noticed', 'Yes, one bug', 'Yes, multiple bugs', 'Not sure'],
        },
        {
          type: 'paragraph',
          title: 'Bug report',
          helpText: 'What were you doing? What did you expect? What actually happened?',
        },
        { type: 'paragraph', title: 'Extra screenshots, item links, or notes' },
      ],
    },
  ],
};

function createLiveItemsPublicTestFeedbackForm() {
  const result = createGoogleFormFromSpec(LIVE_ITEMS_FORM_SPEC);

  Logger.log('Edit form: ' + result.editUrl);
  Logger.log('Public form: ' + result.publishedUrl);
  if (result.responseSheetUrl) {
    Logger.log('Responses sheet: ' + result.responseSheetUrl);
  }
}

function createGoogleFormFromSpec(spec) {
  const form = FormApp.create(spec.title);
  form.setDescription(spec.description || '');
  form.setConfirmationMessage(spec.confirmationMessage || 'Thank you.');
  form.setCollectEmail(Boolean(spec.collectEmail));
  form.setAllowResponseEdits(Boolean(spec.allowResponseEdits));
  form.setProgressBar(Boolean(spec.showProgressBar));
  form.setShuffleQuestions(Boolean(spec.shuffleQuestions));

  (spec.sections || []).forEach(function(section, index) {
    if (index === 0) {
      form.addSectionHeaderItem().setTitle(section.title).setHelpText(section.helpText || '');
    } else {
      form.addPageBreakItem().setTitle(section.title).setHelpText(section.helpText || '');
    }

    (section.items || []).forEach(function(itemSpec) {
      addItemToForm(form, itemSpec);
    });
  });

  let responseSheetUrl = '';
  if (CREATE_RESPONSE_SHEET) {
    const sheet = SpreadsheetApp.create(spec.responseSheetTitle || spec.title + ' Responses');
    form.setDestination(FormApp.DestinationType.SPREADSHEET, sheet.getId());
    responseSheetUrl = sheet.getUrl();
  }

  return {
    editUrl: form.getEditUrl(),
    publishedUrl: form.getPublishedUrl(),
    responseSheetUrl: responseSheetUrl,
  };
}

function addItemToForm(form, itemSpec) {
  let item;

  switch (itemSpec.type) {
    case 'text':
      item = form.addTextItem();
      break;
    case 'paragraph':
      item = form.addParagraphTextItem();
      break;
    case 'date':
      item = form.addDateItem();
      break;
    case 'multipleChoice':
      item = form.addMultipleChoiceItem();
      item.setChoiceValues(itemSpec.choices || []);
      break;
    case 'checkbox':
      item = form.addCheckboxItem();
      item.setChoiceValues(itemSpec.choices || []);
      break;
    case 'scale':
      item = form.addScaleItem();
      item.setBounds(itemSpec.lower || 1, itemSpec.upper || 5);
      item.setLabels(itemSpec.lowLabel || '', itemSpec.highLabel || '');
      break;
    default:
      throw new Error('Unsupported form item type: ' + itemSpec.type);
  }

  item.setTitle(itemSpec.title);

  if (itemSpec.helpText) {
    item.setHelpText(itemSpec.helpText);
  }

  if (itemSpec.required) {
    item.setRequired(true);
  }

  return item;
}

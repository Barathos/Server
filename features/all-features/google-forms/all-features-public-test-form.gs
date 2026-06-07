/**
 * EQEmu All Features Public Test Feedback form generator.
 *
 * How to use:
 * 1. Go to https://script.google.com/
 * 2. Create a new project.
 * 3. Paste this whole file into Code.gs.
 * 4. Run createAllFeaturesPublicTestFeedbackForm().
 * 5. Approve permissions.
 * 6. Open the logged URLs for the edit form, public form, and response sheet.
 */

const CREATE_RESPONSE_SHEET = true;

const ALL_FEATURES_FORM_SPEC = {
  title: 'EQEmu All Features Public Test Feedback',
  description:
    'Thanks for testing the all-features server. Please test what you can and skip what does not apply to your character. ' +
    'Short plain-English notes are perfect: what worked, what broke, what felt confusing, and what felt fun.',
  confirmationMessage: 'Thank you for testing the all-features server. Your feedback helps prioritize the next fixes.',
  collectEmail: false,
  allowResponseEdits: true,
  showProgressBar: true,
  shuffleQuestions: false,
  responseSheetTitle: 'EQEmu All Features Public Test Feedback Responses',
  sections: [
    {
      title: 'Tester Info',
      items: [
        { type: 'text', title: 'Tester name or Discord name', required: true },
        { type: 'text', title: 'Character name', required: true },
        {
          type: 'text',
          title: 'Class, level, and rough play time',
          helpText: 'Example: Warrior level 12, tested about 45 minutes',
          required: true,
        },
        { type: 'date', title: 'Date tested' },
      ],
    },
    {
      title: 'Patch, Login, And Native UI',
      helpText:
        'Patch once with the all-features patcher, log in, and enter tutorialb. This covers the shared client payload, XML includes, eqhost, and startup path.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Were you able to patch, log in, and enter tutorialb?',
          required: true,
          choices: [
            'Yes, patching/login/tutorialb all worked',
            'I patched but had login or zone problems',
            'I could not patch',
            'I could not reach character select',
          ],
        },
        {
          type: 'multipleChoice',
          title: 'Did the client show missing XML, missing file, or startup errors?',
          required: true,
          choices: ['No errors', 'Yes, I saw errors', 'Not sure'],
        },
        {
          type: 'checkbox',
          title: 'Which native/custom UI surfaces opened or appeared correctly?',
          choices: [
            'AutoLoot window',
            'Item Forge window',
            'Spell Forge window',
            'Achievements window',
            'Multiclass window',
            'HP Fix overlay/window',
            'Native item display or map info',
            'I did not try native/custom UI',
          ],
        },
        { type: 'paragraph', title: 'Patch, login, or UI loading notes' },
      ],
    },
    {
      title: 'Live Items, Forge, And Augs-In-Augs',
      helpText:
        'Live Items creates dynamic item instances. In tutorialb, normal mobs should carry one random Live Item directly on the corpse. The nearby Live Items NPCs test forge, augment, upgrade, heirloom, and Augs-in-Augs fusion flows.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Did normal tutorialb mobs drop random Live Items directly on corpses?',
          required: true,
          choices: ['Yes, most or all checked mobs had one', 'Some did', 'None did', 'I did not test this'],
        },
        {
          type: 'multipleChoice',
          title: 'Did those random items loot normally and keep their names/stats after zoning or relogging?',
          choices: ['Yes', 'No', 'Partly', 'I did not test persistence'],
        },
        {
          type: 'checkbox',
          title: 'Which Live Items NPC flows did you complete?',
          choices: [
            'Vedra Item Forge opened and created an item',
            'Orin created a shardwork augment',
            'Mavren upgraded a Live Item to +1',
            'Talia gave an heirloom that grew or debugged correctly',
            'Nalyx fused augments with a catalyst',
            'I did not test NPC flows',
          ],
        },
        {
          type: 'paragraph',
          title: 'Live Items notes',
          helpText:
            'Best item, broken item, NPC issue, missing stat, persistence problem, duplicate/lost item, or anything confusing.',
        },
      ],
    },
    {
      title: 'AutoLoot',
      helpText:
        'AutoLoot lets players configure loot handling from the native window and server-owned loot filters. Test with normal corpse loot if possible.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Did the AutoLoot window open and show status or filter information?',
          required: true,
          choices: ['Yes', 'No', 'It opened but looked wrong', 'I did not test this'],
        },
        {
          type: 'multipleChoice',
          title: 'Could you mark an item Keep, Ignore, or Unset and see the expected loot behavior later?',
          choices: ['Yes', 'No', 'Partly', 'I did not test filters'],
        },
        { type: 'paragraph', title: 'AutoLoot notes' },
      ],
    },
    {
      title: 'Achievements',
      helpText:
        'Achievements tracks categories, objectives, progress, and completion. The native window should query without red database errors.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Did the Achievements window open and load categories/progress?',
          required: true,
          choices: ['Yes', 'No', 'It opened but did not load data', 'I did not test this'],
        },
        {
          type: 'multipleChoice',
          title: 'Did any achievement progress update from normal play, such as leveling, zoning, kills, or tasks?',
          choices: ['Yes', 'No', 'Not sure', 'I did not test progress'],
        },
        { type: 'paragraph', title: 'Achievements notes' },
      ],
    },
    {
      title: 'Multiclass',
      helpText:
        'Multiclass lets a character use a trio profile for class capability, pets, skills, disciplines, melody, and item eligibility. Test the main window first, then any class-specific tools that apply to your trio.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Did /mc or #mc open open the Multiclass window and show your profile?',
          required: true,
          choices: ['Yes', 'No', 'It opened but looked wrong', 'I did not test this'],
        },
        {
          type: 'checkbox',
          title: 'Which Multiclass behavior did you try?',
          choices: [
            'Chose or viewed a trio profile',
            'Used a spell, skill, discipline, or item from an off-base class',
            'Used the pet window or multiple-pet commands',
            'Used Melody on a bard-capable trio',
            'Checked item usability/equipment masks',
            'I did not test Multiclass behavior',
          ],
        },
        { type: 'paragraph', title: 'Multiclass notes' },
      ],
    },
    {
      title: 'Live Spells',
      helpText:
        'Live Spells creates generated spell scrolls and server-patched spell data. Test one simple spell if your character can scribe/cast it.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Did the Spell Forge window open?',
          required: true,
          choices: ['Yes', 'No', 'It opened but looked wrong', 'I did not test this'],
        },
        {
          type: 'multipleChoice',
          title: 'Were you able to create, scribe, memorize, and cast a generated spell?',
          choices: [
            'Yes',
            'No',
            'I created a scroll but could not complete the full flow',
            'I did not test generated spells',
          ],
        },
        { type: 'paragraph', title: 'Live Spells notes' },
      ],
    },
    {
      title: 'AI NPC Response',
      helpText:
        'Sage Aurelian in tutorialb uses the AI NPC Response bridge. Ask one short in-character question and watch whether the response arrives cleanly.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Did Sage Aurelian respond to hail or a simple question?',
          required: true,
          choices: ['Yes, response arrived', 'No response', 'Response was extremely delayed', 'I did not test this'],
        },
        {
          type: 'multipleChoice',
          title: 'Did the AI response feel useful and in-character?',
          choices: ['Yes', 'Mixed', 'No', 'I did not test enough'],
        },
        { type: 'paragraph', title: 'AI NPC notes' },
      ],
    },
    {
      title: 'Dynamic Quests',
      helpText:
        "Scout Deryn in tutorialb offers a prototype task. Accept it, make progress, and check the normal quest/task journal.",
      items: [
        {
          type: 'multipleChoice',
          title: "Were you able to accept and progress Scout Deryn's dynamic quest?",
          required: true,
          choices: [
            'Yes, accepted and made progress',
            'Accepted but progress did not update',
            'Could not accept it',
            'I did not test this',
          ],
        },
        {
          type: 'multipleChoice',
          title: 'Did the quest journal/task UI match what happened in game?',
          choices: ['Yes', 'No', 'Partly', 'I did not check the journal'],
        },
        { type: 'paragraph', title: 'Dynamic Quest notes' },
      ],
    },
    {
      title: 'Item Inspection, Rarity, And Native Interface',
      helpText:
        'Gearscore, Item Rarity, and Native Interface add information to item links, item inspection, and sometimes map/spawn display. Inspect several items, including any random Live Items you find.',
      items: [
        {
          type: 'multipleChoice',
          title:
            'Did item inspect/link windows show extra useful information such as item level, score, rarity, item id, stats, aug slots, or spell details?',
          required: true,
          choices: ['Yes', 'No', 'Some items did, some did not', 'I did not inspect items'],
        },
        {
          type: 'multipleChoice',
          title: 'Did rarity colors or extra item text make items easier to understand without making the window messy?',
          choices: ['Yes', 'No', 'Mixed', 'I did not see rarity or score info'],
        },
        { type: 'paragraph', title: 'Native Interface / Gearscore / Item Rarity notes' },
      ],
    },
    {
      title: 'HP Fix',
      helpText:
        'HP Fix adds a native side-channel for accurate high-HP display while keeping normal client behavior intact. Even without high-HP gear, the overlay should not spam chat or show nonsense.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Did the HP display/overlay look sane during normal damage, healing, zoning, or equipping?',
          choices: ['Yes', 'No', 'I saw no HP Fix UI', 'I did not test this'],
        },
        { type: 'paragraph', title: 'HP Fix notes' },
      ],
    },
    {
      title: 'Tradeskill And General Stability Smoke Check',
      helpText:
        'Tradeskills and general-code changes are not currently a separate public quest line. Do one normal gameplay check instead: open a tradeskill container or perform ordinary actions you would normally do, and report if anything regressed.',
      items: [
        {
          type: 'multipleChoice',
          title: 'Did normal tradeskill/container UI and general gameplay remain stable?',
          choices: ['Yes', 'No, I saw a regression', 'I did not test this'],
        },
        { type: 'paragraph', title: 'Stability notes' },
      ],
    },
    {
      title: 'Overall Feedback',
      items: [
        { type: 'paragraph', title: 'Which features felt ready or fun?' },
        { type: 'paragraph', title: 'Which features felt confusing, noisy, too strong, too weak, or unfinished?' },
        {
          type: 'scale',
          title: 'Overall, how excited are you about the all-features server package?',
          lower: 1,
          upper: 5,
          lowLabel: 'Not excited',
          highLabel: 'Very excited',
        },
        {
          type: 'multipleChoice',
          title: 'Would you keep playing on a server with these systems enabled?',
          choices: ['Yes', 'Maybe, with fixes', 'No', 'Not sure'],
        },
      ],
    },
    {
      title: 'Bugs Or Problems',
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
          helpText:
            'What were you doing? What did you expect? What actually happened? Include item links, NPC names, commands, screenshots, or zone names if useful.',
        },
        { type: 'paragraph', title: 'Extra notes' },
      ],
    },
  ],
};

function createAllFeaturesPublicTestFeedbackForm() {
  const result = createGoogleFormFromSpec(ALL_FEATURES_FORM_SPEC);

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

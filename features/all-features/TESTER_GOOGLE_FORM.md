# EQEmu All Features Public Test Feedback Blueprint

Use this to build the combined all-features tester form. The goal is concise,
player-side feedback on the feature bundle, not a professional QA script.

Recommended form title:

`EQEmu All Features Public Test Feedback`

Recommended form description:

`Thanks for testing the all-features server. Please test what you can and skip what does not apply to your character. Short plain-English notes are perfect: what worked, what broke, what felt confusing, and what felt fun.`

## Form Settings

- Collect email addresses: optional.
- Limit to 1 response: off unless all testers have Google accounts.
- Allow response editing: on.
- Show progress bar: on.
- Shuffle question order: off.

## Section 1: Tester Info

1. Tester name or Discord name
   - Type: Short answer
   - Required: Yes

2. Character name
   - Type: Short answer
   - Required: Yes

3. Class, level, and rough play time
   - Type: Short answer
   - Required: Yes
   - Help text: `Example: Warrior level 12, tested about 45 minutes`

4. Date tested
   - Type: Date
   - Required: No

## Section 2: Patch, Login, And Native UI

Description:

`Patch once with the all-features patcher, log in, and enter tutorialb. This covers the shared client payload, XML includes, eqhost, and startup path.`

5. Were you able to patch, log in, and enter tutorialb?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - Yes, patching/login/tutorialb all worked
     - I patched but had login or zone problems
     - I could not patch
     - I could not reach character select

6. Did the client show missing XML, missing file, or startup errors?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - No errors
     - Yes, I saw errors
     - Not sure

7. Which native/custom UI surfaces opened or appeared correctly?
   - Type: Checkboxes
   - Required: No
   - Choices:
     - AutoLoot window
     - Item Forge window
     - Spell Forge window
     - Achievements window
     - Multiclass window
     - HP Fix overlay/window
     - Native item display or map info
     - I did not try native/custom UI

8. Patch, login, or UI loading notes
   - Type: Paragraph
   - Required: No

## Section 3: Live Items, Forge, And Augs-In-Augs

Description:

`Live Items creates dynamic item instances. In tutorialb, normal mobs should carry one random Live Item directly on the corpse. The nearby Live Items NPCs test forge, augment, upgrade, heirloom, and Augs-in-Augs fusion flows.`

9. Did normal tutorialb mobs drop random Live Items directly on corpses?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - Yes, most or all checked mobs had one
     - Some did
     - None did
     - I did not test this

10. Did those random items loot normally and keep their names/stats after zoning or relogging?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Partly
      - I did not test persistence

11. Which Live Items NPC flows did you complete?
    - Type: Checkboxes
    - Required: No
    - Choices:
      - Vedra Item Forge opened and created an item
      - Orin created a shardwork augment
      - Mavren upgraded a Live Item to +1
      - Talia gave an heirloom that grew or debugged correctly
      - Nalyx fused augments with a catalyst
      - I did not test NPC flows

12. Live Items notes
    - Type: Paragraph
    - Required: No
    - Help text: `Best item, broken item, NPC issue, missing stat, persistence problem, duplicate/lost item, or anything confusing.`

## Section 4: AutoLoot

Description:

`AutoLoot lets players configure loot handling from the native window and server-owned loot filters. Test with normal corpse loot if possible.`

13. Did the AutoLoot window open and show status or filter information?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes
      - No
      - It opened but looked wrong
      - I did not test this

14. Could you mark an item Keep, Ignore, or Unset and see the expected loot behavior later?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Partly
      - I did not test filters

15. AutoLoot notes
    - Type: Paragraph
    - Required: No

## Section 5: Achievements

Description:

`Achievements tracks categories, objectives, progress, and completion. The native window should query without red database errors.`

16. Did the Achievements window open and load categories/progress?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes
      - No
      - It opened but did not load data
      - I did not test this

17. Did any achievement progress update from normal play, such as leveling, zoning, kills, or tasks?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Not sure
      - I did not test progress

18. Achievements notes
    - Type: Paragraph
    - Required: No

## Section 6: Multiclass

Description:

`Multiclass lets a character use a trio profile for class capability, pets, skills, disciplines, melody, and item eligibility. Test the main window first, then any class-specific tools that apply to your trio.`

19. Did `/mc` or `#mc open` open the Multiclass window and show your profile?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes
      - No
      - It opened but looked wrong
      - I did not test this

20. Which Multiclass behavior did you try?
    - Type: Checkboxes
    - Required: No
    - Choices:
      - Chose or viewed a trio profile
      - Used a spell, skill, discipline, or item from an off-base class
      - Used the pet window or multiple-pet commands
      - Used Melody on a bard-capable trio
      - Checked item usability/equipment masks
      - I did not test Multiclass behavior

21. Multiclass notes
    - Type: Paragraph
    - Required: No

## Section 7: Live Spells

Description:

`Live Spells creates generated spell scrolls and server-patched spell data. Test one simple spell if your character can scribe/cast it.`

22. Did the Spell Forge window open?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes
      - No
      - It opened but looked wrong
      - I did not test this

23. Were you able to create, scribe, memorize, and cast a generated spell?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - I created a scroll but could not complete the full flow
      - I did not test generated spells

24. Live Spells notes
    - Type: Paragraph
    - Required: No

## Section 8: AI NPC Response

Description:

`Sage Aurelian in tutorialb uses the AI NPC Response bridge. Ask one short in-character question and watch whether the response arrives cleanly.`

25. Did Sage Aurelian respond to hail or a simple question?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes, response arrived
      - No response
      - Response was extremely delayed
      - I did not test this

26. Did the AI response feel useful and in-character?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - Mixed
      - No
      - I did not test enough

27. AI NPC notes
    - Type: Paragraph
    - Required: No

## Section 9: Dynamic Quests

Description:

`Scout Deryn in tutorialb offers a prototype task. Accept it, make progress, and check the normal quest/task journal.`

28. Were you able to accept and progress Scout Deryn's dynamic quest?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes, accepted and made progress
      - Accepted but progress did not update
      - Could not accept it
      - I did not test this

29. Did the quest journal/task UI match what happened in game?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Partly
      - I did not check the journal

30. Dynamic Quest notes
    - Type: Paragraph
    - Required: No

## Section 10: Item Inspection, Rarity, And Native Interface

Description:

`Gearscore, Item Rarity, and Native Interface add information to item links, item inspection, and sometimes map/spawn display. Inspect several items, including any random Live Items you find.`

31. Did item inspect/link windows show extra useful information such as item level, score, rarity, item id, stats, aug slots, or spell details?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes
      - No
      - Some items did, some did not
      - I did not inspect items

32. Did rarity colors or extra item text make items easier to understand without making the window messy?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Mixed
      - I did not see rarity or score info

33. Native Interface / Gearscore / Item Rarity notes
    - Type: Paragraph
    - Required: No

## Section 11: HP Fix

Description:

`HP Fix adds a native side-channel for accurate high-HP display while keeping normal client behavior intact. Even without high-HP gear, the overlay should not spam chat or show nonsense.`

34. Did the HP display/overlay look sane during normal damage, healing, zoning, or equipping?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - I saw no HP Fix UI
      - I did not test this

35. HP Fix notes
    - Type: Paragraph
    - Required: No

## Section 12: Tradeskill And General Stability Smoke Check

Description:

`Tradeskills and general-code changes are not currently a separate public quest line. Do one normal gameplay check instead: open a tradeskill container or perform ordinary actions you would normally do, and report if anything regressed.`

36. Did normal tradeskill/container UI and general gameplay remain stable?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No, I saw a regression
      - I did not test this

37. Stability notes
    - Type: Paragraph
    - Required: No

## Section 13: Overall Feedback

38. Which features felt ready or fun?
    - Type: Paragraph
    - Required: No

39. Which features felt confusing, noisy, too strong, too weak, or unfinished?
    - Type: Paragraph
    - Required: No

40. Overall, how excited are you about the all-features server package?
    - Type: Linear scale
    - Required: No
    - Scale: 1 to 5
    - Low label: `Not excited`
    - High label: `Very excited`

41. Would you keep playing on a server with these systems enabled?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - Maybe, with fixes
      - No
      - Not sure

## Section 14: Bugs Or Problems

42. Did you hit any bugs or broken behavior?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - No bugs noticed
      - Yes, one bug
      - Yes, multiple bugs
      - Not sure

43. Bug report
    - Type: Paragraph
    - Required: No
    - Help text: `What were you doing? What did you expect? What actually happened? Include item links, NPC names, commands, screenshots, or zone names if useful.`

44. Extra notes
    - Type: Paragraph
    - Required: No

## Optional Discord Post

```text
All-features public test is ready.

Patch your RoF2 client with the all-features patcher, log in, and spend a little time in tutorialb. Please test what you can: Live Items, AutoLoot, Achievements, Multiclass, Live Spells, Sage Aurelian AI dialogue, Dynamic Quests, item inspect/rarity/gearscore, and general stability.

When you are done, fill out this Google Form:
<form link here>

Short notes are useful. You do not need to test every section. Tell us what worked, what broke, what felt confusing, and what felt fun.
```

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
     - Advanced Loot window
     - Item Forge window
     - Spell Forge window
     - Achievements window
     - Multiclass window
     - HP Fix overlay/window
     - Faction window
     - DPS Parser window
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

## Section 4: Advanced Loot

Description:

`Advanced Loot lets players configure loot handling from the native window and server-owned loot filters. Test with normal corpse loot if possible.`

13. Did the Advanced Loot window open and show status or filter information?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes
      - No
      - It opened but looked wrong
      - I did not test this

14. Could you mark an item Always Need, Always Greed, Never, or Unset and see the expected behavior later?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Partly
      - I did not test filters

15. If you grouped, did Shared Loot, Master Looter, Need, Greed, No, or timeout behavior make sense?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Partly
      - I only tested solo loot
      - I did not test this

16. Advanced Loot notes
    - Type: Paragraph
    - Required: No

## Section 5: Achievements

Description:

`Achievements tracks categories, objectives, progress, and completion. The native window should query without red database errors.`

17. Did the Achievements window open and load categories/progress?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes
      - No
      - It opened but did not load data
      - I did not test this

18. Did any achievement progress update from normal play, such as leveling, zoning, kills, or tasks?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Not sure
      - I did not test progress

19. Achievements notes
    - Type: Paragraph
    - Required: No

## Section 6: Multiclass

Description:

`Multiclass lets a character use a trio profile for class capability, pets, skills, disciplines, melody, and item eligibility. Test the main window first, then any class-specific tools that apply to your trio.`

20. Did `/mc` or `#mc open` open the Multiclass window and show your profile?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes
      - No
      - It opened but looked wrong
      - I did not test this

21. Which Multiclass behavior did you try?
    - Type: Checkboxes
    - Required: No
    - Choices:
      - Chose or viewed a trio profile
      - Used a spell, skill, discipline, or item from an off-base class
      - Used the pet window or multiple-pet commands
      - Used Melody on a bard-capable trio
      - Checked item usability/equipment masks
      - I did not test Multiclass behavior

22. Multiclass notes
    - Type: Paragraph
    - Required: No

## Section 7: Live Spells

Description:

`Live Spells creates generated spell scrolls and server-patched spell data. Test one simple spell if your character can scribe/cast it.`

23. Did the Spell Forge window open?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes
      - No
      - It opened but looked wrong
      - I did not test this

24. Were you able to create, scribe, memorize, and cast a generated spell?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - I created a scroll but could not complete the full flow
      - I did not test generated spells

25. Live Spells notes
    - Type: Paragraph
    - Required: No

## Section 8: AI NPC Response

Description:

`Sage Aurelian in tutorialb uses the AI NPC Response bridge. Ask one short in-character question and watch whether the response arrives cleanly.`

26. Did Sage Aurelian respond to hail or a simple question?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes, response arrived
      - No response
      - Response was extremely delayed
      - I did not test this

27. Did the AI response feel useful and in-character?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - Mixed
      - No
      - I did not test enough

28. AI NPC notes
    - Type: Paragraph
    - Required: No

## Section 9: Dynamic Quests

Description:

`Scout Deryn in tutorialb offers a prototype task. Accept it, make progress, and check the normal quest/task journal.`

29. Were you able to accept and progress Scout Deryn's dynamic quest?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes, accepted and made progress
      - Accepted but progress did not update
      - Could not accept it
      - I did not test this

30. Did the quest journal/task UI match what happened in game?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Partly
      - I did not check the journal

31. Dynamic Quest notes
    - Type: Paragraph
    - Required: No

## Section 10: Item Inspection, Rarity, And Native Interface

Description:

`Gearscore, Item Rarity, and Native Interface add information to item links, item inspection, and sometimes map/spawn display. Inspect several items, including any random Live Items you find.`

32. Did item inspect/link windows show extra useful information such as item level, score, rarity, item id, stats, aug slots, or spell details?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - Yes
      - No
      - Some items did, some did not
      - I did not inspect items

33. Did rarity colors or extra item text make items easier to understand without making the window messy?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Mixed
      - I did not see rarity or score info

34. Native Interface / Gearscore / Item Rarity notes
    - Type: Paragraph
    - Required: No

## Section 11: HP Fix

Description:

`HP Fix adds a native side-channel for accurate high-HP display while keeping normal client behavior intact. Even without high-HP gear, the overlay should not spam chat or show nonsense.`

35. Did the HP display/overlay look sane during normal damage, healing, zoning, or equipping?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - I saw no HP Fix UI
      - I did not test this

36. HP Fix notes
    - Type: Paragraph
    - Required: No

## Section 12: Tradeskill And General Stability Smoke Check

Description:

`Tradeskills includes the server-owned make-all path, command checks, and a native window shell. Open a tradeskill container or perform ordinary actions you would normally do, and report if anything regressed.`

37. Did normal tradeskill/container UI, make-all behavior, and general gameplay remain stable?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No, I saw a regression
      - Partly
      - I did not test this

38. Stability notes
    - Type: Paragraph
    - Required: No

## Section 13: Faction, DPS, And Fellowship Tools

Description:

`Faction and DPS use custom native windows. Fellowships currently use server persistence, stock/native packet work, and command/debug paths while the stock UI behavior is being finalized.`

39. Did `#rep` or the Faction window show standings, search, pin, hide, or target-faction information correctly?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Partly
      - I did not test this

40. Did `#dps` or the DPS Parser window show sensible combat damage, healing, incoming damage, or pet contribution rows?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Partly
      - I did not test this

41. If you had multiple testers, did Fellowship create, invite, accept, leave, remove, campfire, or stock UI behavior work?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Partly
      - I did not have multiple testers
      - I did not test this

42. Faction, DPS, or Fellowship notes
    - Type: Paragraph
    - Required: No

## Section 14: Pet Bags, Autoskills, UseItem, And AutoFollow

Description:

`These are utility systems without a separate quest line. Try only the helpers that apply to your character and report whether they obey normal gameplay limits.`

43. Which utility systems did you try?
    - Type: Checkboxes
    - Required: No
    - Choices:
      - Syncrosatchel pet bags on a summoned or charmed pet
      - #autoskill setup or automatic combat skill use
      - #useitem or native /useitem rewrite
      - Improved AutoFollow
      - Server-auth/native stat display behavior
      - I did not test utility systems

44. Did those utilities respect expected restrictions such as class, level, cooldown, charges, range, pet state, or disabled feature gates?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - No
      - Partly
      - I did not test restrictions

45. Pet Bags / Autoskills / UseItem / AutoFollow notes
    - Type: Paragraph
    - Required: No

## Section 15: Overall Feedback

46. Which features felt ready or fun?
    - Type: Paragraph
    - Required: No

47. Which features felt confusing, noisy, too strong, too weak, or unfinished?
    - Type: Paragraph
    - Required: No

48. Overall, how excited are you about the all-features server package?
    - Type: Linear scale
    - Required: No
    - Scale: 1 to 5
    - Low label: `Not excited`
    - High label: `Very excited`

49. Would you keep playing on a server with these systems enabled?
    - Type: Multiple choice
    - Required: No
    - Choices:
      - Yes
      - Maybe, with fixes
      - No
      - Not sure

## Section 16: Bugs Or Problems

50. Did you hit any bugs or broken behavior?
    - Type: Multiple choice
    - Required: Yes
    - Choices:
      - No bugs noticed
      - Yes, one bug
      - Yes, multiple bugs
      - Not sure

51. Bug report
    - Type: Paragraph
    - Required: No
    - Help text: `What were you doing? What did you expect? What actually happened? Include item links, NPC names, commands, screenshots, or zone names if useful.`

52. Extra notes
    - Type: Paragraph
    - Required: No

## Optional Discord Post

```text
All-features public test is ready.

Patch your RoF2 client with the all-features patcher, log in, and spend a little time in tutorialb. Please test what you can: Live Items, Advanced Loot, Achievements, Multiclass, Live Spells, Sage Aurelian AI dialogue, Dynamic Quests, item inspect/rarity/gearscore, Faction, DPS, Fellowships, Pet Bags, Autoskills, UseItem, AutoFollow, and general stability.

When you are done, fill out this Google Form:
<form link here>

Short notes are useful. You do not need to test every section. Tell us what worked, what broke, what felt confusing, and what felt fun.
```

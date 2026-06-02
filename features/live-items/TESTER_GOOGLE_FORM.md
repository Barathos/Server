# Live Items Google Form Blueprint

Use this to build a Google Form for volunteer testers. The goal is quick, low-friction feedback from players, not a professional QA report.

Recommended form title:

`Live Items Public Test Feedback`

Recommended form description:

`Thanks for helping test Live Items. Please answer what you can. You do not need perfect notes. If something felt weird, confusing, too strong, too weak, or broken, tell us in plain English. Item links, screenshots, and mob names help if you have them.`

## Form Settings

- Collect email addresses: optional.
- Limit to 1 response: off unless all testers have Google accounts.
- Allow response editing: on.
- Show progress bar: on.
- Shuffle question order: off.
- Make questions required only where marked below.

## Section 1: Tester Info

1. Tester name or Discord name
   - Type: Short answer
   - Required: Yes

2. Character name
   - Type: Short answer
   - Required: Yes

3. Class and level
   - Type: Short answer
   - Required: Yes
   - Help text: `Example: Warrior level 12`

4. Date tested
   - Type: Date
   - Required: No

## Section 2: Login And Patching

5. Were you able to patch your RoF2 client and log in?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - Yes, patching and login worked
     - I patched, but login had problems
     - I could not patch
     - I could not reach character select

6. Were you able to enter `tutorialb`?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - Yes
     - No

7. Did you see any missing file, XML, UI, or startup errors?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - No errors
     - Yes, I saw errors
     - Not sure

8. If patching, login, or startup had problems, describe what happened.
   - Type: Paragraph
   - Required: No

## Section 3: Test NPCs

9. Which NPCs responded when hailed?
   - Type: Checkboxes
   - Required: Yes
     - Choices:
     - Vedra Forgecall
     - Orin Augspinner
     - Mavren Instancewright
     - Talia Heirloomkeeper
     - Nalyx Augmentweaver
     - None of them

10. Did the clickable dialogue text work?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - Yes
     - No
     - Some worked, some did not
     - I did not try clickable dialogue

11. Was any NPC text confusing?
   - Type: Paragraph
   - Required: No

## Section 4: Tester Helper Commands

Description:

`These commands are optional, but they make bug reports much easier to trace. Hold an item on your cursor before using #helditemid.`

12. Which helper commands did you try?
   - Type: Checkboxes
   - Required: No
   - Choices:
     - #helditemid / #itemid / #cursoritemid
     - #heirloomdebug / #evolvingdebug
     - I did not try helper commands

13. Paste any useful helper command output here.
   - Type: Paragraph
   - Required: No
   - Help text: `Item IDs, heirloom debug output, or anything that seemed wrong.`

## Section 5: Random Loot From Tutorialb Mobs

Description:

`Kill normal mobs in tutorialb and loot them like normal. The random Live Items loot should appear directly on corpses. It should not be a box or cache you have to open.`

14. About how many normal mobs did you kill?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - 1-4
     - 5-9
     - 10-19
     - 20+

15. Did normal mobs have random Live Items loot directly on their corpses?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - Yes, every mob I checked had one
     - Most did, but not all
     - Only some did
     - None did
     - Not sure

16. Did the loot look like normal corpse loot?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - Yes
     - No, it appeared as a box/cache
     - No, something else seemed wrong
     - Not sure

17. Were you able to loot the random items normally?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - Yes
     - No
     - Some worked, some did not

18. Did different mobs give different names or stat rolls?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - Yes, the items felt varied
     - A little, but they felt too similar
     - No, they looked mostly the same
     - Not sure

19. Did random loot keep its stats after zoning, camping, or relogging?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - I did not test this

20. What was the best random item you found?
   - Type: Short answer
   - Required: No
   - Help text: `Item name/link if you have it.`

21. What was the strangest, worst, or most broken random item you found?
   - Type: Short answer
   - Required: No

22. Any notes about random loot?
   - Type: Paragraph
   - Required: No

## Section 6: Item Forge Window

Description:

`Talk to Vedra Forgecall and choose open forge. Try making at least one item.`

23. Did the Item Forge window open?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - Yes
     - No
     - It opened, but looked broken

24. Which item types did you try creating?
   - Type: Checkboxes
   - Required: No
   - Choices:
     - Weapon
     - Armor
     - Jewelry
     - Charm
     - Shield
     - Augment

25. Did the item you created match the name and stats you entered?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - Some stats worked, some did not
     - I could not create an item

26. Did the step buttons for stat changes work as expected?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - Some worked, some did not
     - I did not try changing the step size

27. Did any stat field not work or look wrong?
   - Type: Paragraph
   - Required: No

28. Item Forge notes
   - Type: Paragraph
   - Required: No

## Section 7: Augment NPC

Description:

`Talk to Orin Augspinner. Say start, add the upgrades you want, optionally name the augment, then say confirm. Use test shards if you need Blood Shards for testing.`

29. Could you build a shardwork augment with the upgrades you wanted?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - Yes
     - No
     - I did not test augments

30. Did Orin spend Blood Shards and give you a custom augment when you confirmed?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - Yes
     - No
     - I did not test augments

31. Did the augment have the stats you purchased?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - Not sure

32. Could the augment be inserted into normal equipment slots?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - I did not try inserting it
     - Not sure

33. After inserting the augment, did the target item still look usable and keep its normal slots?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - I did not try inserting it
     - Not sure

34. If you tried inserting the augment into an item, what happened?
   - Type: Paragraph
   - Required: No

35. Augment notes
   - Type: Paragraph
   - Required: No

## Section 8: Instance Upgrade NPC

Description:

`Talk to Mavren Instancewright. Hand him exactly one Live Items item with no coins and no extra items. He should return that same item as a +1 version with its supported positive stats increased by 10.`

36. Did Mavren return your item as a +1 upgraded version?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - I did not test this

37. Did Mavren return only the upgraded item, without also giving back the original?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes, only the upgraded item came back
     - No, I also got the original back
     - No, something vanished or got stuck
     - I did not test this

38. Did the upgraded item keep the correct name and stats after zoning, camping, or relogging?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - I did not test persistence

39. Instance upgrade notes
   - Type: Paragraph
   - Required: No

## Section 9: Evolving Heirloom

Description:

`Talk to Talia Heirloomkeeper and say heirloom. Keep the item equipped or in your inventory, then gain a level if you can. The heirloom should grow stronger when you level.`

40. Did Talia give you a class-matched heirloom weapon?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - I did not test this

41. Did the heirloom grow stronger when you gained a level?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - I could not gain a level during testing
     - I did not test this

42. Did #heirloomdebug or #evolvingdebug find the heirloom while it was in inventory or equipped?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - I did not try the debug command
     - Not sure

43. Evolving heirloom notes
   - Type: Paragraph
   - Required: No

## Section 10: Augment Fusion

Description:

`Talk to Nalyx Augmentweaver. Say catalyst if you need an Augment Catalyst, then hand in exactly one Augment Catalyst plus one to three augment items, with no coins. Nalyx should return one fused augment with the supported stats combined.`

44. Did Nalyx give you an Augment Catalyst when you asked for one?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - I already had one
     - I did not test this

45. Did Nalyx return one fused augment?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - I did not test this

46. Did the fused augment show the combined stats you expected?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - No
     - Not sure
     - I did not test this

47. Augment fusion notes
   - Type: Paragraph
   - Required: No

## Section 11: Overall Feel

48. What felt fun?
   - Type: Paragraph
   - Required: No

49. What felt annoying?
   - Type: Paragraph
   - Required: No

50. What was confusing?
   - Type: Paragraph
   - Required: No

51. Did any item seem way too strong or way too weak?
   - Type: Paragraph
   - Required: No

52. Would you want this kind of loot while leveling normally?
   - Type: Multiple choice
   - Required: No
   - Choices:
     - Yes
     - Maybe, with changes
     - No
     - Not sure

53. Overall, how excited are you about Live Items?
   - Type: Linear scale
   - Required: No
   - Scale: 1 to 5
   - Low label: `Not excited`
   - High label: `Very excited`

## Section 12: Bugs Or Problems

Description:

`If something broke, describe it in plain English. Example: I killed a kobold near the cave entrance. The corpse had no random item. I was a level 5 warrior.`

54. Did you hit any bugs or broken behavior?
   - Type: Multiple choice
   - Required: Yes
   - Choices:
     - No bugs noticed
     - Yes, one bug
     - Yes, multiple bugs
     - Not sure

55. Bug report
   - Type: Paragraph
   - Required: No
   - Help text: `What were you doing? What did you expect? What actually happened?`

56. Extra screenshots, item links, or notes
   - Type: Paragraph
   - Required: No

## Optional Discord Post

Use this when sending the form link:

```text
Live Items test is ready.

Please patch your RoF2 client, log in, and spend a little time in tutorialb. The main thing to test is random Live Items loot dropping directly from normal mobs, plus the forge NPC and augment NPC.

When you are done, fill out this Google Form:
<form link here>

You do not need perfect bug reports. Plain English is fine. Tell us what worked, what broke, what felt fun, and what felt confusing.
```

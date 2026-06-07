local DYNAMIC_QUEST_ID = 910001

local function offer_dynamic_quest(e)
	if e.other:IsTaskCompleted(DYNAMIC_QUEST_ID) then
		e.self:QuestSay(e.other, "You have already proven the dynamic quest loop. Thank you.")
		return
	end

	if e.other:IsTaskActive(DYNAMIC_QUEST_ID) then
		e.self:QuestSay(e.other, "Keep the quest journal open and watch the objectives update as you work.")
		return
	end

	e.self:QuestSay(e.other, "I have a dynamic quest ready for testing.")
	e.other:TaskSelector({ DYNAMIC_QUEST_ID }, true)
end

function event_say(e)
	if e.message:findi("hail") then
		e.self:QuestSay(
			e.other,
			"Say " .. eq.say_link("dynamic quest") ..
			" to accept a prototype quest with live objective updates."
		)
		return
	end

	if e.message:findi("dynamic quest") or e.message:findi("quest") then
		offer_dynamic_quest(e)
	end
end

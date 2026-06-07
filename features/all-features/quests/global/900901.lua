local TEST_LOOT_CACHE_ID = 199091

local function live_items_enabled()
	local value = eq.get_rule("CustomFeatures:LiveItemsEnabled")
	return value == nil or value == "" or value == "true" or value == "1"
end

local forge_payload = table.concat({
	"LIVEITEM|ui|open",
	"types=weapon,armor,jewelry,charm,shield,augment",
	"max_hp=500",
	"max_mana=500",
	"max_ac=100",
	"max_damage=100",
	"max_haste=50",
	"stats=hp,mana,endur,ac,str,sta,dex,agi,int,wis,cha,mr,fr,cr,pr,dr,svcorruption,attack,accuracy,avoidance,regen,manaregen,enduranceregen,haste,damage,delay,shielding,spellshield,dotshielding,stunresist,strikethrough,damageshield,dsmitigation,healamt,spelldmg,clairvoyance,backstabdmg,heroic_str,heroic_sta,heroic_dex,heroic_agi,heroic_int,heroic_wis,heroic_cha,heroic_mr,heroic_fr,heroic_cr,heroic_pr,heroic_dr,heroic_svcorrup",
}, "|")

function event_say(e)
	if not live_items_enabled() then
		e.self:QuestSay(e.other, "The Item Forge is disabled right now.")
		return
	end

	if e.message:findi("hail") then
		e.self:QuestSay(
			e.other,
			"Item Forge testing is ready. Use " ..
			eq.say_link("open forge") ..
			" to open the native forge, " ..
			eq.say_link("random cache") ..
			" to get a test loot cache, or " ..
			eq.say_link("five caches") ..
			" for a small batch."
		)
		return
	end

	if e.message:findi("open forge") or e.message:findi("forge") then
		e.other:Message(10, forge_payload)
		e.self:QuestSay(e.other, "The forge interface is open. Try every item type and stat field.")
		return
	end

	if e.message:findi("five caches") then
		e.other:SummonItem(TEST_LOOT_CACHE_ID, 5)
		e.self:QuestSay(e.other, "Five test caches are on your cursor. Open them to create per-instance random loot.")
		return
	end

	if e.message:findi("random cache") or e.message:findi("cache") then
		e.other:SummonItem(TEST_LOOT_CACHE_ID, 1)
		e.self:QuestSay(e.other, "A test cache is on your cursor. Open it to create a per-instance random item.")
	end
end

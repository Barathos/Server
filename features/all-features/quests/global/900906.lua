local UPGRADE_AMOUNT = 10

local function live_items_enabled()
	local value = eq.get_rule("CustomFeatures:LiveItemsEnabled")
	return value == nil or value == "" or value == "true" or value == "1"
end

local stat_fields = {
	{ key = "hp", label = "HP", cap = 5000, get = function(item) return item:HP() end },
	{ key = "mana", label = "Mana", cap = 5000, get = function(item) return item:Mana() end },
	{ key = "endur", label = "Endurance", cap = 5000, get = function(item) return item:Endur() end },
	{ key = "ac", label = "AC", cap = 1000, get = function(item) return item:AC() end },
	{ key = "damage", label = "Damage", cap = 500, get = function(item) return item:Damage() end },
	{ key = "str", label = "STR", cap = 500, get = function(item) return item:AStr() end },
	{ key = "sta", label = "STA", cap = 500, get = function(item) return item:ASta() end },
	{ key = "dex", label = "DEX", cap = 500, get = function(item) return item:ADex() end },
	{ key = "agi", label = "AGI", cap = 500, get = function(item) return item:AAgi() end },
	{ key = "int", label = "INT", cap = 500, get = function(item) return item:AInt() end },
	{ key = "wis", label = "WIS", cap = 500, get = function(item) return item:AWis() end },
	{ key = "cha", label = "CHA", cap = 500, get = function(item) return item:ACha() end },
	{ key = "mr", label = "MR", cap = 500, get = function(item) return item:MR() end },
	{ key = "fr", label = "FR", cap = 500, get = function(item) return item:FR() end },
	{ key = "cr", label = "CR", cap = 500, get = function(item) return item:CR() end },
	{ key = "pr", label = "PR", cap = 500, get = function(item) return item:PR() end },
	{ key = "dr", label = "DR", cap = 500, get = function(item) return item:DR() end },
	{ key = "corruption", label = "Corruption", cap = 500, get = function(item) return item:SVCorruption() end },
	{ key = "hstr", label = "Heroic STR", cap = 500, get = function(item) return item:HeroicStr() end },
	{ key = "hsta", label = "Heroic STA", cap = 500, get = function(item) return item:HeroicSta() end },
	{ key = "hdex", label = "Heroic DEX", cap = 500, get = function(item) return item:HeroicDex() end },
	{ key = "hagi", label = "Heroic AGI", cap = 500, get = function(item) return item:HeroicAgi() end },
	{ key = "hint", label = "Heroic INT", cap = 500, get = function(item) return item:HeroicInt() end },
	{ key = "hwis", label = "Heroic WIS", cap = 500, get = function(item) return item:HeroicWis() end },
	{ key = "hcha", label = "Heroic CHA", cap = 500, get = function(item) return item:HeroicCha() end },
	{ key = "hmr", label = "Heroic MR", cap = 500, get = function(item) return item:HeroicMR() end },
	{ key = "hfr", label = "Heroic FR", cap = 500, get = function(item) return item:HeroicFR() end },
	{ key = "hcr", label = "Heroic CR", cap = 500, get = function(item) return item:HeroicCR() end },
	{ key = "hpr", label = "Heroic PR", cap = 500, get = function(item) return item:HeroicPR() end },
	{ key = "hdr", label = "Heroic DR", cap = 500, get = function(item) return item:HeroicDR() end },
	{ key = "hsvcorruption", label = "Heroic Corrup", cap = 500, get = function(item) return item:HeroicSVCorrup() end },
	{ key = "attack", label = "Attack", cap = 500, get = function(item) return item:Attack() end },
	{ key = "accuracy", label = "Accuracy", cap = 500, get = function(item) return item:Accuracy() end },
	{ key = "avoidance", label = "Avoidance", cap = 500, get = function(item) return item:Avoidance() end },
	{ key = "combateffects", label = "Combat Effects", cap = 500, get = function(item) return item:CombatEffects() end },
	{ key = "shielding", label = "Shielding", cap = 500, get = function(item) return item:Shielding() end },
	{ key = "spellshield", label = "Spell Shield", cap = 500, get = function(item) return item:SpellShield() end },
	{ key = "dotshielding", label = "DoT Shield", cap = 500, get = function(item) return item:DotShielding() end },
	{ key = "stunresist", label = "Stun Resist", cap = 500, get = function(item) return item:StunResist() end },
	{ key = "strikethrough", label = "Strikethrough", cap = 500, get = function(item) return item:StrikeThrough() end },
	{ key = "regen", label = "HP Regen", cap = 500, get = function(item) return item:Regen() end },
	{ key = "manaregen", label = "Mana Regen", cap = 500, get = function(item) return item:ManaRegen() end },
	{ key = "enduranceregen", label = "End Regen", cap = 500, get = function(item) return item:EnduranceRegen() end },
	{ key = "damageshield", label = "Damage Shield", cap = 500, get = function(item) return item:DamageShield() end },
	{ key = "dsmitigation", label = "DS Mitigation", cap = 500, get = function(item) return item:DSMitigation() end },
	{ key = "healamt", label = "Heal Amount", cap = 500, get = function(item) return item:HealAmt() end },
	{ key = "spelldmg", label = "Spell Damage", cap = 500, get = function(item) return item:SpellDmg() end },
	{ key = "clairvoyance", label = "Clairvoyance", cap = 500, get = function(item) return item:Clairvoyance() end },
	{ key = "backstabdmg", label = "Backstab Dmg", cap = 500, get = function(item) return item:BackstabDmg() end },
	{ key = "haste", label = "Haste", cap = 100, get = function(item) return item:Haste() end },
}

local function number(value, default)
	local converted = tonumber(value)
	if converted == nil then
		return default
	end
	return converted
end

local function trim_item_name(name)
	name = tostring(name or "Live Item")
	name = name:gsub("%s+", " "):gsub("^%s+", ""):gsub("%s+$", "")
	return string.sub(name, 1, 64)
end

local function root_name(inst)
	local stored = inst:GetCustomData("live_items_original_name")
	if stored and stored ~= "" then
		return stored
	end

	local name = tostring(inst:GetName() or "Live Item")
	name = name:gsub("%s%+%d+$", "")
	inst:SetCustomData("live_items_original_name", name)
	return name
end

local function summarize(parts)
	if #parts <= 8 then
		return table.concat(parts, ", ")
	end

	local visible = {}
	for index = 1, 8 do
		table.insert(visible, parts[index])
	end

	table.insert(visible, "and " .. tostring(#parts - 8) .. " more")
	return table.concat(visible, ", ")
end

local function upgrade_instance(inst, amount)
	local item = inst:GetItem()
	if not item or not item:valid() then
		return nil, "The handed-in item could not be inspected."
	end

	local rank = number(inst:GetCustomData("live_items_instance_rank"), 0) + 1
	local changed = {}

	for _, field in ipairs(stat_fields) do
		local current = number(field.get(item), 0)
		if current > 0 then
			local next_value = math.min(field.cap, current + amount)
			inst:DeleteDynamicItemModifier(field.key)
			inst:SetDynamicItemData(field.key, next_value)
			table.insert(changed, field.label .. " " .. tostring(current) .. "->" .. tostring(next_value))
		end
	end

	if #changed == 0 then
		for _, field in ipairs({
			{ key = "hp", label = "HP", value = amount },
			{ key = "mana", label = "Mana", value = amount },
			{ key = "endur", label = "Endurance", value = amount },
			{ key = "ac", label = "AC", value = amount },
		}) do
			inst:DeleteDynamicItemModifier(field.key)
			inst:SetDynamicItemData(field.key, field.value)
			table.insert(changed, field.label .. " 0->" .. tostring(field.value))
		end
	end

	local name = trim_item_name(root_name(inst) .. " +" .. tostring(rank))
	inst:SetDynamicItemData("name", name)
	inst:SetDynamicItemData("comment", "Live Items instance upgrade rank " .. tostring(rank) .. ": " .. summarize(changed))
	inst:SetCustomData("live_items_instance_rank", tostring(rank))
	inst:SetCustomData("live_items_last_instance_upgrade", tostring(os.time()))
	inst:RebuildDynamicItemData()

	return {
		name = name,
		rank = rank,
		changed_count = #changed,
		summary = summarize(changed),
	}
end

local function trade_items(trade)
	local items = {}
	for slot = 1, 4 do
		local inst = trade["item" .. tostring(slot)]
		if inst and inst.valid then
			table.insert(items, inst)
		end
	end
	return items
end

function event_say(e)
	if not live_items_enabled() then
		e.self:Say("Instance upgrades are disabled right now.")
		return
	end

	if not e.message:findi("hail") then
		return
	end

	e.self:Say(
		"Hand me exactly one item and I will return that same item instance as a +1 version. " ..
		"Every supported positive stat it already has gains +" .. tostring(UPGRADE_AMOUNT) ..
		". If it has no supported stats yet, I seed HP, mana, endurance, and AC so the mutation is visible."
	)
end

function event_trade(e)
	local item_lib = require("items")
	if not live_items_enabled() then
		e.self:Say("Instance upgrades are disabled right now.")
		item_lib.return_items(e.self, e.other, e.trade)
		return
	end

	local items = trade_items(e.trade)

	if #items ~= 1 or e.trade.platinum > 0 or e.trade.gold > 0 or e.trade.silver > 0 or e.trade.copper > 0 then
		e.self:Say("Hand me exactly one item and no coins. I need a clean handoff so I can mutate that exact instance.")
		item_lib.return_items(e.self, e.other, e.trade)
		return
	end

	local original = items[1]
	local original_id = original:GetID()
	local original_restore = original:Clone()
	local upgraded = original:Clone()
	local result, error_message = upgrade_instance(upgraded, UPGRADE_AMOUNT)
	if not result then
		e.self:Say(error_message)
		item_lib.return_items(e.self, e.other, e.trade)
		return
	end

	if not e.self:CheckHandin(e.other, {}, { [tostring(original_id)] = 1 }, {}) then
		e.self:Say("The hand-in did not line up cleanly. Hand me exactly one item and no coins.")
		item_lib.return_items(e.self, e.other, e.trade)
		return
	end

	if not e.other:PushItemOnCursor(upgraded) then
		e.self:Say("The hand-in slipped out of the forge's grasp. I could not place the upgraded item on your cursor, so I returned the original item.")
		e.other:PushItemOnCursor(original_restore)
		return
	end

	e.self:Say(
		"Returned " .. upgraded:GetItemLink() .. " at +" .. tostring(result.rank) ..
		". Updated " .. tostring(result.changed_count) .. " fields: " .. result.summary .. "."
	)
	item_lib.return_items(e.self, e.other, e.trade)
end

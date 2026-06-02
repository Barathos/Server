local CATALYST_ITEM_ID = 199211
local MAX_TRADE_SLOTS = 4
local MAX_SOURCE_AUGMENTS = MAX_TRADE_SLOTS - 1
local MIN_SOURCE_AUGMENTS = 1

local stat_fields = {
	{ key = "ac", label = "AC", get = function(item) return item:AC() end },
	{ key = "hp", label = "HP", get = function(item) return item:HP() end },
	{ key = "mana", label = "Mana", get = function(item) return item:Mana() end },
	{ key = "endur", label = "Endurance", get = function(item) return item:Endur() end },
	{ key = "str", label = "STR", get = function(item) return item:AStr() end },
	{ key = "sta", label = "STA", get = function(item) return item:ASta() end },
	{ key = "dex", label = "DEX", get = function(item) return item:ADex() end },
	{ key = "agi", label = "AGI", get = function(item) return item:AAgi() end },
	{ key = "int", label = "INT", get = function(item) return item:AInt() end },
	{ key = "wis", label = "WIS", get = function(item) return item:AWis() end },
	{ key = "cha", label = "CHA", get = function(item) return item:ACha() end },
	{ key = "mr", label = "MR", get = function(item) return item:MR() end },
	{ key = "fr", label = "FR", get = function(item) return item:FR() end },
	{ key = "cr", label = "CR", get = function(item) return item:CR() end },
	{ key = "pr", label = "PR", get = function(item) return item:PR() end },
	{ key = "dr", label = "DR", get = function(item) return item:DR() end },
	{ key = "svcorruption", label = "Corruption", get = function(item) return item:SVCorruption() end },
	{ key = "attack", label = "Attack", get = function(item) return item:Attack() end },
	{ key = "accuracy", label = "Accuracy", get = function(item) return item:Accuracy() end },
	{ key = "avoidance", label = "Avoidance", get = function(item) return item:Avoidance() end },
	{ key = "regen", label = "HP Regen", get = function(item) return item:Regen() end },
	{ key = "manaregen", label = "Mana Regen", get = function(item) return item:ManaRegen() end },
	{ key = "enduranceregen", label = "End Regen", get = function(item) return item:EnduranceRegen() end },
	{ key = "damageshield", label = "Damage Shield", get = function(item) return item:DamageShield() end },
	{ key = "healamt", label = "Heal Amount", get = function(item) return item:HealAmt() end },
	{ key = "spelldmg", label = "Spell Damage", get = function(item) return item:SpellDmg() end },
	{ key = "clairvoyance", label = "Clairvoyance", get = function(item) return item:Clairvoyance() end },
	{ key = "hstr", label = "Heroic STR", get = function(item) return item:HeroicStr() end },
	{ key = "hsta", label = "Heroic STA", get = function(item) return item:HeroicSta() end },
	{ key = "hdex", label = "Heroic DEX", get = function(item) return item:HeroicDex() end },
	{ key = "hagi", label = "Heroic AGI", get = function(item) return item:HeroicAgi() end },
	{ key = "hint", label = "Heroic INT", get = function(item) return item:HeroicInt() end },
	{ key = "hwis", label = "Heroic WIS", get = function(item) return item:HeroicWis() end },
	{ key = "hcha", label = "Heroic CHA", get = function(item) return item:HeroicCha() end },
	{ key = "hmr", label = "Heroic MR", get = function(item) return item:HeroicMR() end },
	{ key = "hfr", label = "Heroic FR", get = function(item) return item:HeroicFR() end },
	{ key = "hcr", label = "Heroic CR", get = function(item) return item:HeroicCR() end },
	{ key = "hdr", label = "Heroic DR", get = function(item) return item:HeroicDR() end },
	{ key = "hpr", label = "Heroic PR", get = function(item) return item:HeroicPR() end },
	{ key = "hsvcorruption", label = "Heroic Corruption", get = function(item) return item:HeroicSVCorrup() end },
}

local function as_number(value)
	return tonumber(value) or 0
end

local function clean_name(name)
	name = tostring(name or "Augment")
	name = name:gsub("^Fused%s+", "")
	name = name:gsub("%s+", " "):gsub("^%s+", ""):gsub("%s+$", "")
	if name == "" then
		return "Augment"
	end
	return name
end

local function short_name(name)
	return string.sub(name, 1, 63)
end

local function trade_items(trade)
	local items = {}
	for slot = 1, MAX_TRADE_SLOTS do
		local inst = trade["item" .. tostring(slot)]
		if inst and inst.valid then
			table.insert(items, inst)
		end
	end
	return items
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

local function split_trade_items(items)
	local catalysts = {}
	local sources = {}

	for _, inst in ipairs(items) do
		if inst:GetID() == CATALYST_ITEM_ID then
			table.insert(catalysts, inst)
		else
			table.insert(sources, inst)
		end
	end

	return catalysts, sources
end

local function validate_augments(catalysts, sources)
	if #catalysts ~= 1 then
		return nil, "I need exactly one Augment Catalyst with the augments you want fused."
	end

	if #sources < MIN_SOURCE_AUGMENTS or #sources > MAX_SOURCE_AUGMENTS then
		return nil, "Hand me one Augment Catalyst and one to three augment items."
	end

	local required = { [tostring(CATALYST_ITEM_ID)] = 1 }
	local source_ids = {}

	for _, inst in ipairs(sources) do
		local base_id = inst:GetID()
		local item = inst:GetItem()
		if not item or as_number(item:ID()) == 0 or as_number(item:AugType()) == 0 then
			return nil, "Every non-catalyst hand-in must be an augment."
		end

		required[tostring(base_id)] = (required[tostring(base_id)] or 0) + 1
		table.insert(source_ids, tostring(base_id))
	end

	return {
		required = required,
		source_ids = source_ids,
	}
end

local function fuse_augments(sources, source_ids)
	local totals = {}
	local changed = {}

	for _, field in ipairs(stat_fields) do
		local total = 0
		for _, inst in ipairs(sources) do
			local item = inst:GetItem()
			total = total + as_number(field.get(item))
		end
		totals[field.key] = total
	end

	local output = sources[1]:Clone()
	output:ClearDynamicItemData()

	for _, field in ipairs(stat_fields) do
		local total = totals[field.key]
		if total ~= 0 then
			output:SetDynamicItemData(field.key, total)
			table.insert(changed, field.label .. " " .. tostring(total))
		end
	end

	if #changed == 0 then
		return nil, "Those augments do not have any supported stats to fuse."
	end

	local source_name = output:GetCustomData("live_items_aug_fusion_root_name")
	if source_name == "" then
		local item = sources[1]:GetItem()
		source_name = item and clean_name(item:Name()) or "Augment"
	end

	local name = short_name("Fused " .. clean_name(source_name))
	output:SetDynamicItemData("name", name)
	output:SetCustomData("live_items_aug_fusion", "1")
	output:SetCustomData("live_items_aug_fusion_root_name", source_name)
	output:SetCustomData("live_items_aug_fusion_component_count", tostring(#sources))
	output:SetCustomData("live_items_aug_fusion_sources", table.concat(source_ids, ","))
	output:RebuildDynamicItemData()

	return {
		item = output,
		name = name,
		changed = changed,
	}
end

function event_say(e)
	if e.message:findi("catalyst") then
		e.other:SummonItem(CATALYST_ITEM_ID, 1)
		e.self:Say("An Augment Catalyst is on your cursor. Hand it back to me with one to three augment items and no coins when you are ready to test fusion.")
		return
	end

	if not e.message:findi("hail") then
		return
	end

	e.self:Say(
		"Hand me one Augment Catalyst plus one to three augment items and no coins. " ..
		"I will return one fused augment that carries the combined supported stats from the augment items. " ..
		"Say " .. eq.say_link("catalyst") .. " if you need a catalyst for testing."
	)
end

function event_trade(e)
	local item_lib = require("items")
	local items = trade_items(e.trade)

	if #items < 2 or #items > MAX_TRADE_SLOTS or e.trade.platinum > 0 or e.trade.gold > 0 or e.trade.silver > 0 or e.trade.copper > 0 then
		e.self:Say("Hand me one Augment Catalyst plus one to three augment items and no coins.")
		item_lib.return_items(e.self, e.other, e.trade)
		return
	end

	local catalysts, sources = split_trade_items(items)
	local validation, validation_error = validate_augments(catalysts, sources)
	if not validation then
		e.self:Say(validation_error)
		item_lib.return_items(e.self, e.other, e.trade)
		return
	end

	local result, fuse_error = fuse_augments(sources, validation.source_ids)
	if not result then
		e.self:Say(fuse_error)
		item_lib.return_items(e.self, e.other, e.trade)
		return
	end

	if not e.self:CheckHandin(e.other, {}, validation.required, {}) then
		e.self:Say("The hand-in did not line up cleanly. Try handing me only the catalyst and augments you want fused.")
		item_lib.return_items(e.self, e.other, e.trade)
		return
	end

	if not e.other:PushItemOnCursor(result.item) then
		e.self:Say("The fusion worked, but I could not place the fused augment on your cursor.")
		item_lib.return_items(e.self, e.other, e.trade)
		return
	end

	e.self:Say("Returned " .. result.item:GetItemLink() .. " with fused stats: " .. summarize(result.changed) .. ".")
	item_lib.return_items(e.self, e.other, e.trade)
end

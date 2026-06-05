local skipped_npcs = {
	[900901] = true,
	[900902] = true,
	[900903] = true, -- Reserved by the AI dialogue Sage Aurelian testbed NPC.
	[900904] = true,
	[900905] = true,
	[900906] = true,
}

local seed_marker = "live_items_tutorialb_spawn_loot_seeded"

local function live_items_enabled()
	local value = eq.get_rule("CustomFeatures:LiveItemsEnabled")
	return value == nil or value == "" or value == "true" or value == "1"
end

local templates = {
	{ id = 6002, type = "Weapon", name = "Staff", damage = true },
	{ id = 6919, type = "Weapon", name = "Forlorn Bow", damage = true },
	{ id = 7007, type = "Weapon", name = "Rusty Dagger", damage = true },
	{ id = 10686, type = "Weapon", name = "Warhammer of Ethereal Energy", damage = true },
	{ id = 14148, type = "Jewelry", name = "Ornate Ring" },
	{ id = 59945, type = "Armor", name = "Kobold Leather Belt" },
	{ id = 82924, type = "Armor", name = "Gloomiron Bracer" },
	{ id = 82925, type = "Armor", name = "Gloomiron Gauntlets" },
	{ id = 82926, type = "Armor", name = "Gloomiron Boots" },
	{ id = 82927, type = "Armor", name = "Gloomiron Helm" },
	{ id = 82931, type = "Armor", name = "Gloomchain Bracer" },
	{ id = 82932, type = "Armor", name = "Gloomchain Gauntlets" },
	{ id = 82933, type = "Armor", name = "Gloomchain Boots" },
	{ id = 82934, type = "Armor", name = "Gloomchain Coif" },
	{ id = 82952, type = "Weapon", name = "Gloomsteel Blade", damage = true },
	{ id = 82953, type = "Weapon", name = "Gloomsteel Dagger", damage = true },
	{ id = 82954, type = "Weapon", name = "Gloomsteel Hammer", damage = true },
	{ id = 82955, type = "Weapon", name = "Gloomsteel Axe", damage = true },
	{ id = 82956, type = "Weapon", name = "Gloomsteel Staff", damage = true },
}

local tiers = {
	{ name = "Magic", weight = 65, rolls = 6, min = 2, max = 8 },
	{ name = "Rare", weight = 28, rolls = 8, min = 4, max = 12 },
	{ name = "Legendary", weight = 7, rolls = 10, min = 7, max = 16 },
}

local stat_pool = {
	{ label = "STR", keys = { "str" } },
	{ label = "STA", keys = { "sta" } },
	{ label = "DEX", keys = { "dex" } },
	{ label = "AGI", keys = { "agi" } },
	{ label = "INT", keys = { "int" } },
	{ label = "WIS", keys = { "wis" } },
	{ label = "CHA", keys = { "cha" } },
	{ label = "HP", data = "hp" },
	{ label = "Mana", data = "mana" },
	{ label = "Endurance", data = "endur" },
	{ label = "AC", data = "ac" },
	{ label = "Resist", keys = { "mr", "fr", "cr", "pr", "dr" } },
	{ label = "Accuracy", keys = { "accuracy" } },
	{ label = "Avoidance", keys = { "avoidance" } },
	{ label = "Attack", keys = { "attack" } },
}

local suffixes = {
	"of Embers",
	"of Sparks",
	"of the Rift",
	"of the Noble Cause",
	"of the Bloodfield",
	"of the Sanctum",
	"of Slaughter",
}

local function roll_tier()
	local total = 0
	for _, tier in ipairs(tiers) do
		total = total + tier.weight
	end

	local roll = math.random(total)
	for _, tier in ipairs(tiers) do
		if roll <= tier.weight then
			return tier
		end
		roll = roll - tier.weight
	end

	return tiers[1]
end

local function add_roll(totals, stat, value)
	totals[stat.label] = (totals[stat.label] or 0) + value
	if stat.data then
		totals[stat.data] = (totals[stat.data] or 0) + value
	end
	if stat.keys then
		for _, key in ipairs(stat.keys) do
			totals[key] = (totals[key] or 0) + value
		end
	end
end

local function roll_stats(tier)
	local totals = {}
	local available = {}

	for _, stat in ipairs(stat_pool) do
		table.insert(available, stat)
	end

	for _ = 1, tier.rolls do
		local source = available
		if #source == 0 then
			source = stat_pool
		end

		local index = math.random(#source)
		local stat = source[index]
		add_roll(totals, stat, math.random(tier.min, tier.max))

		if source == available then
			table.remove(available, index)
		end
	end

	return totals
end

local function summarize(stats)
	local order = { "STR", "STA", "DEX", "AGI", "INT", "WIS", "CHA", "HP", "Mana", "Endurance", "AC", "Resist", "Accuracy", "Avoidance", "Attack" }
	local parts = {}
	for _, label in ipairs(order) do
		if stats[label] and stats[label] > 0 then
			table.insert(parts, label .. " +" .. stats[label])
		end
	end
	return table.concat(parts, ", ")
end

local function create_random_item_from_template(template)
	local tier = roll_tier()
	local stats = roll_stats(tier)
	local name = tier.name .. " " .. template.name .. " " .. suffixes[math.random(#suffixes)]
	local summary = summarize(stats)
	local data = {
		name = name,
		lore = "A unique tester roll carried by a tutorialb creature.",
		comment = "Live Items tester spawn loot roll: " .. summary,
	}
	local modifiers = {}

	for key, value in pairs(stats) do
		if key == "hp" or key == "mana" or key == "endur" or key == "ac" then
			data[key] = value
		elseif type(key) == "string" and key:lower() == key then
			modifiers[key] = value
		end
	end

	if template.damage then
		data.damage = math.random(8, 26)
		data.delay = math.random(20, 34)
	elseif template.augment then
		data.ac = math.max(data.ac or 0, math.random(2, 8))
	else
		data.damage = 0
		data.delay = 0
	end

	local item = eq.create_live_item({
		item_id = template.id,
		charges = 1,
		data = data,
		modifiers = modifiers,
		custom_data = {
			live_items_test_roll = "tutorialb-spawn-loot",
			live_items_tier = tier.name,
			live_items_summary = summary,
		},
	})

	if not item or not item.valid then
		error("Template item " .. tostring(template.id) .. " was not found.")
	end

	return item
end

local function create_random_item()
	local candidates = {}
	for _, template in ipairs(templates) do
		table.insert(candidates, template)
	end

	local last_error = nil
	while #candidates > 0 do
		local index = math.random(#candidates)
		local template = candidates[index]
		table.remove(candidates, index)

		local ok, item = pcall(create_random_item_from_template, template)
		if ok and item and item.valid then
			return item
		end

		last_error = item
	end

	error("No Live Items tutorialb spawn loot templates were available. Last error: " .. tostring(last_error))
end

local function seed_npc(npc)
	if not npc or not npc.valid then
		return
	end

	if skipped_npcs[npc:GetNPCTypeID()] then
		return
	end

	if npc:EntityVariableExists(seed_marker) then
		return
	end

	local ok, item = pcall(create_random_item)
	if ok and item and item.valid then
		npc:AddLiveItem(item, false)
		npc:SetEntityVariable(seed_marker, "1")
	elseif eq and eq.debug then
		eq.debug("Live Items tutorialb spawn loot error: " .. tostring(item), 1)
	end
end

local function seed_existing_npcs()
	local list = eq.get_entity_list():GetNPCList()
	for npc in list.entries do
		seed_npc(npc)
	end
end

function event_spawn_zone(e)
	if not live_items_enabled() then
		return
	end

	seed_npc(e.other)
end

function event_enter_zone(e)
	if not live_items_enabled() then
		return
	end

	seed_existing_npcs()
end

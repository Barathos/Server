local COFFER_ITEM_ID = 900090
local RANDOM_ROLL_COUNT = 15

local classes = {
	{ key = "warrior", name = "Warrior", class_id = 1, base_item_id = 900101 },
	{ key = "cleric", name = "Cleric", class_id = 2, base_item_id = 900102 },
	{ key = "paladin", name = "Paladin", class_id = 3, base_item_id = 900103 },
	{ key = "ranger", name = "Ranger", class_id = 4, base_item_id = 900104 },
	{ key = "shadowknight", name = "Shadow Knight", class_id = 5, base_item_id = 900105 },
	{ key = "druid", name = "Druid", class_id = 6, base_item_id = 900106 },
	{ key = "monk", name = "Monk", class_id = 7, base_item_id = 900107 },
	{ key = "bard", name = "Bard", class_id = 8, base_item_id = 900108 },
	{ key = "rogue", name = "Rogue", class_id = 9, base_item_id = 900109 },
	{ key = "shaman", name = "Shaman", class_id = 10, base_item_id = 900110 },
	{ key = "necromancer", name = "Necromancer", class_id = 11, base_item_id = 900111 },
	{ key = "wizard", name = "Wizard", class_id = 12, base_item_id = 900112 },
	{ key = "magician", name = "Magician", class_id = 13, base_item_id = 900113 },
	{ key = "enchanter", name = "Enchanter", class_id = 14, base_item_id = 900114 },
	{ key = "beastlord", name = "Beastlord", class_id = 15, base_item_id = 900115 },
	{ key = "berserker", name = "Berserker", class_id = 16, base_item_id = 900116 },
}

local class_by_key = {}
local class_by_id = {}
for _, class in ipairs(classes) do
	class_by_key[class.key] = class
	class_by_id[class.class_id] = class
end

local static_stats = {
	{ key = "ac", label = "AC", value = 15 },
	{ key = "hp", label = "HP", value = 100 },
	{ key = "mana", label = "Mana", value = 100 },
}

local cleared_template_stats = {
	"endur",
	"haste",
	"attack",
	"accuracy",
	"avoidance",
}

local stat_pool = {
	{ label = "STR", keys = { "str" }, min = 5, max = 15 },
	{ label = "DEX", keys = { "dex" }, min = 5, max = 15 },
	{ label = "AGI", keys = { "agi" }, min = 5, max = 15 },
	{ label = "INT", keys = { "int" }, min = 5, max = 15 },
	{ label = "WIS", keys = { "wis" }, min = 5, max = 15 },
	{ label = "CON", keys = { "sta" }, min = 5, max = 15 },
	{ label = "CHA", keys = { "cha" }, min = 5, max = 15 },
	{ label = "Resist", keys = { "mr", "fr", "cr", "pr", "dr" }, min = 5, max = 15 },
}

local suffixes = {
	"the Ashwake",
	"the Riftborn",
	"the War-Sung",
	"the Spirebound",
	"the Sanctum Echo",
	"the Harbinger",
	"the Bloodfield",
	"the Noble Cause",
}

local function normalize(value)
	return (value or ""):lower():gsub("[%s_%-]", "")
end

local function selected_class(e)
	local class_key = normalize(e.self:GetCustomData("live_items_epic_class"))
	if class_by_key[class_key] then
		return class_by_key[class_key]
	end

	local class_name = normalize(e.self:GetCustomData("live_items_epic_class_name"))
	if class_by_key[class_name] then
		return class_by_key[class_name]
	end

	return class_by_id[e.owner:GetClass()]
end

local function add_roll(rolls, stat, value)
	rolls[stat.label] = (rolls[stat.label] or 0) + value
	for _, key in ipairs(stat.keys) do
		rolls[key] = (rolls[key] or 0) + value
	end
end

local function roll_stats()
	local totals = {}
	local count = 0

	for _, stat in ipairs(stat_pool) do
		add_roll(totals, stat, math.random(stat.min, stat.max))
		count = count + 1
	end

	for _ = count + 1, RANDOM_ROLL_COUNT do
		local stat = stat_pool[math.random(#stat_pool)]
		add_roll(totals, stat, math.random(stat.min, stat.max))
	end

	local rolls = {}
	for _, stat in ipairs(stat_pool) do
		table.insert(rolls, {
			keys = stat.keys,
			label = stat.label,
			value = totals[stat.label] or 0,
		})
	end

	return rolls
end

local function static_summary()
	local parts = {}
	for _, stat in ipairs(static_stats) do
		table.insert(parts, string.format("%s +%d", stat.label, stat.value))
	end

	return table.concat(parts, ", ")
end

local function roll_summary(rolls)
	local parts = {}
	for _, roll in ipairs(rolls) do
		table.insert(parts, string.format("%s +%d", roll.label, roll.value))
	end

	return table.concat(parts, ", ")
end

local function create_rolled_epic(e, class)
	local suffix = suffixes[math.random(#suffixes)]
	local rolls = roll_stats()
	local summary = roll_summary(rolls)
	local item_name = string.format("%s Epic of %s", class.name, suffix)
	local data = {
		name = item_name,
		lore = "A unique live epic rolled for " .. e.owner:GetCleanName() .. ".",
		comment = "Live Items epic instance: " .. static_summary() .. "; random rolls: " .. summary,
	}
	local modifiers = {}

	for _, stat in ipairs(static_stats) do
		data[stat.key] = stat.value
	end

	for _, key in ipairs(cleared_template_stats) do
		data[key] = 0
	end

	for _, roll in ipairs(rolls) do
		for _, key in ipairs(roll.keys) do
			modifiers[key] = roll.value
		end
	end

	local epic = eq.create_live_item({
		item_id = class.base_item_id,
		charges = 1,
		data = data,
		modifiers = modifiers,
		custom_data = {
			live_items_epic_class = class.key,
			live_items_epic_rolls = summary,
			live_items_epic_roll_mode = "instance",
		},
	})

	if not epic or not epic.valid then
		error("Class epic template " .. tostring(class.base_item_id) .. " was not found.")
	end

	return {
		inst = epic,
		item_name = item_name,
		summary = summary,
	}
end

function event_item_click(e)
	if e.self:GetID() ~= COFFER_ITEM_ID then
		return
	end

	local class = selected_class(e)
	if not class then
		e.owner:Message(13, "The coffer cannot determine which class epic to roll.")
		return
	end

	local ok, rolled = pcall(create_rolled_epic, e, class)
	if not ok or rolled == nil then
		e.owner:Message(13, "The coffer sputters. The Item Forge could not roll this epic.")
		if eq and eq.debug then
			eq.debug("Live Items epic coffer error: " .. tostring(rolled), 1)
		end
		return
	end

	e.owner:DeleteItemInInventory(e.slot_id, 1, true)
	e.owner:RewardLiveItem(rolled.inst)
	e.owner:Message(15, "The coffer opens into " .. rolled.item_name .. ": " .. static_summary() .. "; " .. rolled.summary)
end

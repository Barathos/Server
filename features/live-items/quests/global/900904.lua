local classes = {
	[1] = { name = "Warrior", base_item_id = 54231, class_mask = 1, itemtype = 0, slots = 24576, damage = 24, delay = 24 },
	[2] = { name = "Cleric", base_item_id = 54232, class_mask = 2, itemtype = 3, slots = 24576, damage = 18, delay = 28 },
	[3] = { name = "Paladin", base_item_id = 50515, class_mask = 4, itemtype = 1, slots = 8192, damage = 34, delay = 36 },
	[4] = { name = "Ranger", base_item_id = 54231, class_mask = 8, itemtype = 0, slots = 24576, damage = 26, delay = 30 },
	[5] = { name = "Shadow Knight", base_item_id = 50515, class_mask = 16, itemtype = 1, slots = 8192, damage = 36, delay = 38 },
	[6] = { name = "Druid", base_item_id = 50513, class_mask = 32, itemtype = 4, slots = 8192, damage = 16, delay = 28 },
	[7] = { name = "Monk", base_item_id = 50513, class_mask = 64, itemtype = 4, slots = 8192, damage = 22, delay = 22 },
	[8] = { name = "Bard", base_item_id = 54231, class_mask = 128, itemtype = 0, slots = 24576, damage = 20, delay = 24 },
	[9] = { name = "Rogue", base_item_id = 54230, class_mask = 256, itemtype = 2, slots = 24576, damage = 23, delay = 21 },
	[10] = { name = "Shaman", base_item_id = 54232, class_mask = 512, itemtype = 3, slots = 24576, damage = 17, delay = 29 },
	[11] = { name = "Necromancer", base_item_id = 50513, class_mask = 1024, itemtype = 4, slots = 8192, damage = 14, delay = 30 },
	[12] = { name = "Wizard", base_item_id = 50513, class_mask = 2048, itemtype = 4, slots = 8192, damage = 14, delay = 30 },
	[13] = { name = "Magician", base_item_id = 50513, class_mask = 4096, itemtype = 4, slots = 8192, damage = 14, delay = 30 },
	[14] = { name = "Enchanter", base_item_id = 50513, class_mask = 8192, itemtype = 4, slots = 8192, damage = 14, delay = 30 },
	[15] = { name = "Beastlord", base_item_id = 67128, class_mask = 16384, itemtype = 3, slots = 8192, damage = 21, delay = 24 },
	[16] = { name = "Berserker", base_item_id = 50515, class_mask = 32768, itemtype = 1, slots = 8192, damage = 38, delay = 40 },
}

local initial_stats = {
	hp = 50,
	mana = 50,
	endur = 50,
	ac = 5,
	str = 5,
	sta = 5,
	dex = 5,
	agi = 5,
	int = 5,
	wis = 5,
	cha = 5,
	mr = 5,
	fr = 5,
	cr = 5,
	pr = 5,
	dr = 5,
	corruption = 5,
	attack = 5,
	accuracy = 5,
}

local function trim_item_name(name)
	name = tostring(name or "Live Items Heirloom")
	name = name:gsub("%s+", " "):gsub("^%s+", ""):gsub("%s+$", "")
	return string.sub(name, 1, 64)
end

local function class_definition(client)
	return classes[client:GetClass()] or classes[1]
end

local function create_heirloom(client)
	local class = class_definition(client)
	local player_name = client:GetCleanName()
	local item_name = trim_item_name(player_name .. "'s " .. class.name .. " Heirloom")

	local data = {
		name = item_name,
		lore = "A Live Items heirloom that grows when its owner gains levels.",
		comment = "Live Items evolving heirloom. Level-up quest hook mutates this item instance.",
		classes = class.class_mask,
		races = 65535,
		itemtype = class.itemtype,
		slots = class.slots,
		hp = initial_stats.hp,
		mana = initial_stats.mana,
		endur = initial_stats.endur,
		ac = initial_stats.ac,
		damage = class.damage,
		delay = class.delay,
	}

	local modifiers = {}
	for key, value in pairs(initial_stats) do
		if key ~= "hp" and key ~= "mana" and key ~= "endur" and key ~= "ac" then
			modifiers[key] = value
		end
	end

	return eq.create_live_item({
		item_id = class.base_item_id,
		charges = 1,
		data = data,
		modifiers = modifiers,
		custom_data = {
			live_items_heirloom = "1",
			live_items_heirloom_owner = player_name,
			live_items_heirloom_class = class.name,
			live_items_heirloom_level = tostring(client:GetLevel()),
			live_items_instance_rank = "0",
			live_items_original_name = item_name,
			live_items_summary = "Heirloom starter: all core stats/resists +5, HP/Mana/End +50, AC +5.",
		},
	})
end

function event_say_heirloom(e)
	local ok, item = pcall(create_heirloom, e.other)
	if not ok or not item or not item.valid then
		e.self:Say("The heirloom pattern failed. The class templates may need to be seeded.")
		if not ok and eq and eq.debug then
			eq.debug("Live Items heirloom creation error: " .. tostring(item), 1)
		end
		return
	end

	e.other:RewardLiveItem(item)
	e.self:Say(
		"Your evolving heirloom is on your cursor: " .. item:GetItemLink() ..
		". This is a Live Items test weapon for your class. Keep it equipped or in your inventory when you level up. " ..
		"Instead of creating a new item ID, the level-up hook finds this exact item instance and rewrites its dynamic stats, so this same weapon grows with you."
	)
end

function event_say(e)
	local message = (e.message or ""):lower()

	if message:find("heirloom") then
		event_say_heirloom(e)
		return
	end

	if message:find("hail") then
		local heirloom_link = eq.say_link("heirloom", false, "heirloom")
		e.self:Say(
			"I can hand you a class-matched " .. heirloom_link ..
			". Keep it equipped or in your inventory. Each level-up will mutate that exact item instance and add +10 to its supported stats. Use #heirloomdebug if you want to confirm the marker before leveling."
		)
	end
end

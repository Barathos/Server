local CURRENCY_ID = 90
local CURRENCY_NAME = "Blood Shards"
local CURRENCY_ITEM_ID = 81436
local TEMPLATE_ITEM_ID = 199207
local DEFAULT_NAME = "Bloodbound Augment"
local ALL_EQUIPMENT_SLOTS = 8388607
local ALL_AUGMENT_TYPES = 2147483647
local CURSOR_SLOT = 33

local function live_items_enabled()
	local value = eq.get_rule("CustomFeatures:LiveItemsEnabled")
	return value == nil or value == "" or value == "true" or value == "1"
end

local upgrade_order = {
	"hp", "mana", "endur", "ac",
	"str", "sta", "agi", "dex", "int", "wis", "cha",
	"fr", "cr", "mr", "dr", "pr",
	"allstats", "allresists",
	"attack", "accuracy", "regen", "manaregen", "damageshield", "spelldmg", "healamt", "haste",
}

local upgrades = {
	hp = { name = "HP", cost = 5, amount = 10, fields = { "hp" } },
	mana = { name = "Mana", cost = 5, amount = 10, fields = { "mana" } },
	endur = { name = "Endurance", cost = 5, amount = 10, fields = { "endur" } },
	ac = { name = "AC", cost = 5, amount = 2, fields = { "ac" } },
	str = { name = "Strength", cost = 5, amount = 1, fields = { "astr" } },
	sta = { name = "Stamina", cost = 5, amount = 1, fields = { "asta" } },
	agi = { name = "Agility", cost = 5, amount = 1, fields = { "aagi" } },
	dex = { name = "Dexterity", cost = 5, amount = 1, fields = { "adex" } },
	int = { name = "Intelligence", cost = 5, amount = 1, fields = { "aint" } },
	wis = { name = "Wisdom", cost = 5, amount = 1, fields = { "awis" } },
	cha = { name = "Charisma", cost = 5, amount = 1, fields = { "acha" } },
	fr = { name = "Fire Resist", cost = 5, amount = 2, fields = { "fr" } },
	cr = { name = "Cold Resist", cost = 5, amount = 2, fields = { "cr" } },
	mr = { name = "Magic Resist", cost = 5, amount = 2, fields = { "mr" } },
	dr = { name = "Disease Resist", cost = 5, amount = 2, fields = { "dr" } },
	pr = { name = "Poison Resist", cost = 5, amount = 2, fields = { "pr" } },
	allstats = { name = "All Stats", cost = 20, amount = 1, fields = { "astr", "asta", "aagi", "adex", "aint", "awis", "acha" } },
	allresists = { name = "All Resists", cost = 15, amount = 2, fields = { "fr", "cr", "mr", "dr", "pr" } },
	attack = { name = "Attack", cost = 10, amount = 2, fields = { "attack" } },
	accuracy = { name = "Accuracy", cost = 10, amount = 2, fields = { "accuracy" } },
	regen = { name = "HP Regen", cost = 10, amount = 1, fields = { "regen" } },
	manaregen = { name = "Mana Regen", cost = 10, amount = 1, fields = { "manaregen" } },
	damageshield = { name = "Damage Shield", cost = 10, amount = 1, fields = { "damageshield" } },
	spelldmg = { name = "Spell Damage", cost = 10, amount = 1, fields = { "spelldmg" } },
	healamt = { name = "Heal Amount", cost = 10, amount = 1, fields = { "healamt" } },
	haste = { name = "Haste", cost = 25, amount = 1, fields = { "haste" } },
}

local field_labels = {
	hp = "HP", mana = "Mana", endur = "Endurance", ac = "AC",
	astr = "STR", asta = "STA", aagi = "AGI", adex = "DEX", aint = "INT", awis = "WIS", acha = "CHA",
	fr = "FR", cr = "CR", mr = "MR", dr = "DR", pr = "PR",
	attack = "Attack", accuracy = "Accuracy", regen = "HP Regen", manaregen = "Mana Regen",
	damageshield = "Damage Shield", spelldmg = "Spell Damage", healamt = "Heal Amount", haste = "Haste",
}

local field_order = {
	"hp", "mana", "endur", "ac",
	"astr", "asta", "aagi", "adex", "aint", "awis", "acha",
	"fr", "cr", "mr", "dr", "pr",
	"attack", "accuracy", "regen", "manaregen", "damageshield", "spelldmg", "healamt", "haste",
}

local field_caps = {
	hp = 1000, mana = 1000, endur = 1000, ac = 100,
	astr = 127, asta = 127, aagi = 127, adex = 127, aint = 127, awis = 127, acha = 127,
	fr = 127, cr = 127, mr = 127, dr = 127, pr = 127,
	attack = 250, accuracy = 127, regen = 100, manaregen = 100,
	damageshield = 100, spelldmg = 100, healamt = 100, haste = 100,
}

local function normalize(value)
	return (value or ""):lower():gsub("^%s+", ""):gsub("%s+$", ""):gsub("[%s_%-]", "")
end

local function say_link(command, label)
	return eq.say_link(command, false, label or command)
end

local function bucket_prefix(client)
	return "live_items_orin_shardwork." .. client:CharacterID()
end

local function bucket_name(client, key)
	return bucket_prefix(client) .. "." .. key
end

local function get_bucket(client, key)
	return client:GetBucket(bucket_name(client, key)) or ""
end

local function set_bucket(client, key, value)
	client:SetBucket(bucket_name(client, key), tostring(value or ""))
end

local function get_count(client, key)
	return tonumber(get_bucket(client, "upgrade." .. key)) or 0
end

local function set_count(client, key, count)
	set_bucket(client, "upgrade." .. key, math.max(0, count or 0))
end

local function is_started(client)
	return get_bucket(client, "started") == "1"
end

local function get_build_name(client)
	local name = get_bucket(client, "name")
	if name == "" then
		return DEFAULT_NAME
	end
	return name
end

local function sanitize_name(name)
	name = tostring(name or ""):gsub("[\r\n\t]", " "):gsub("%s+", " "):gsub("^%s+", ""):gsub("%s+$", "")
	if name == "" then
		return DEFAULT_NAME
	end
	return string.sub(name, 1, 50)
end

local function reset_build(client)
	client:DeleteBucket(bucket_prefix(client))
end

local function start_build(client)
	reset_build(client)
	set_bucket(client, "started", "1")
	set_bucket(client, "name", DEFAULT_NAME)
end

local function build_totals(client)
	local total_cost = 0
	local field_totals = {}
	local lines = {}

	for _, key in ipairs(upgrade_order) do
		local count = get_count(client, key)
		local upgrade = upgrades[key]
		if upgrade and count > 0 then
			total_cost = total_cost + (upgrade.cost * count)
			table.insert(lines, upgrade.name .. " x" .. count .. " = " .. (upgrade.cost * count) .. " " .. CURRENCY_NAME)
			for _, field in ipairs(upgrade.fields) do
				field_totals[field] = (field_totals[field] or 0) + (upgrade.amount * count)
			end
		end
	end

	return total_cost, field_totals, lines
end

local function summarize_fields(field_totals)
	local parts = {}
	for _, field in ipairs(field_order) do
		if field_totals[field] and field_totals[field] > 0 then
			table.insert(parts, field_labels[field] .. " +" .. field_totals[field])
		end
	end
	return table.concat(parts, ", ")
end

local function get_upgrade_cap(upgrade)
	local cap = nil
	for _, field in ipairs(upgrade.fields) do
		if field_caps[field] then
			if cap == nil or field_caps[field] < cap then
				cap = field_caps[field]
			end
		end
	end
	return cap
end

local function find_limit_violations(field_totals)
	local violations = {}
	for _, field in ipairs(field_order) do
		local value = field_totals[field] or 0
		local cap = field_caps[field]
		if cap and value > cap then
			table.insert(violations, field_labels[field] .. " " .. value .. "/" .. cap)
		end
	end
	return violations
end

local function can_add_upgrade(client, upgrade)
	local _, field_totals = build_totals(client)
	for _, field in ipairs(upgrade.fields) do
		local cap = field_caps[field]
		local projected = (field_totals[field] or 0) + upgrade.amount
		if cap and projected > cap then
			return false, field, cap, projected
		end
	end
	return true
end

local function show_greeting(e)
	local balance = e.other:GetAlternateCurrencyValue(CURRENCY_ID) or 0
	e.self:QuestSay(
		e.other,
		"Good. You found the quiet heat of the forge. I can bind little truths into an augment if you bring me " ..
		CURRENCY_NAME .. ". You have " .. balance .. " " .. CURRENCY_NAME .. ". Say " ..
		say_link("start", "start") .. " to begin, " ..
		say_link("show", "show") .. " to review your current work, or " ..
		say_link("confirm", "finish it") .. " when it is ready."
	)
end

local function show_upgrade_menu(e)
	local links = {}
	for _, key in ipairs(upgrade_order) do
		local upgrade = upgrades[key]
		local cap = get_upgrade_cap(upgrade)
		local cap_text = cap and (", cap " .. cap) or ""
		table.insert(
			links,
			say_link("add " .. key, upgrade.name) ..
			" (" .. upgrade.cost .. " for +" .. upgrade.amount .. cap_text .. ")"
		)
	end
	e.other:Message(15, table.concat(links, ", "))
end

local function show_build(e)
	if not is_started(e.other) then
		e.self:QuestSay(e.other, "No shardwork is active. Say " .. say_link("start", "start") .. " to begin one.")
		return
	end

	local total_cost, field_totals, lines = build_totals(e.other)
	local summary = summarize_fields(field_totals)
	if summary == "" then
		summary = "no upgrades yet"
	end

	e.self:QuestSay(e.other, "Current shardwork: " .. get_build_name(e.other) .. ". Cost: " .. total_cost .. " " .. CURRENCY_NAME .. ".")
	e.other:Message(15, "Stats: " .. summary)
	if #lines > 0 then
		e.other:Message(15, "Upgrades: " .. table.concat(lines, "; "))
	end
	e.other:Message(
		15,
		"Commands: " .. say_link("name Bloodbound Focus", "name <your augment>") .. ", " ..
		say_link("confirm", "finish it") .. ", " ..
		say_link("reset", "reset") .. ", " ..
		say_link("test shards", "test shards")
	)
	show_upgrade_menu(e)
end

local function add_upgrade(e, raw_key)
	if not is_started(e.other) then
		start_build(e.other)
	end

	local key = normalize(raw_key)
	local upgrade = upgrades[key]
	if not upgrade then
		e.self:QuestSay(e.other, "I do not know that shardwork. Say " .. say_link("show", "show") .. " to see the list.")
		return
	end

	local allowed, field, cap, projected = can_add_upgrade(e.other, upgrade)
	if not allowed then
		e.self:QuestSay(
			e.other,
			upgrade.name .. " would push " .. field_labels[field] .. " to " .. projected ..
			", above the safe augment cap of " .. cap .. "."
		)
		return
	end

	set_count(e.other, key, get_count(e.other, key) + 1)
	local total_cost = build_totals(e.other)
	e.self:QuestSay(
		e.other,
		upgrade.name .. " added. Current cost is " .. total_cost .. " " .. CURRENCY_NAME .. "."
	)
end

local function set_build_name(e, raw_name)
	if not is_started(e.other) then
		start_build(e.other)
	end

	local name = sanitize_name(raw_name)
	set_bucket(e.other, "name", name)
	e.self:QuestSay(e.other, "The augment will be named '" .. name .. "'.")
end

local function cursor_is_empty(client)
	local inst = client:GetInventory():GetItem(CURSOR_SLOT)
	return not inst or not inst.valid
end

local function create_augment(client, field_totals, total_cost)
	local name = get_build_name(client)
	local summary = summarize_fields(field_totals)
	local data = {
		name = name,
		lore = "A custom augment shaped from Blood Shards by Orin Augspinner.",
		comment = "Live Items shardwork augment. Cost " .. total_cost .. " " .. CURRENCY_NAME .. ". " .. summary,
		slots = ALL_EQUIPMENT_SLOTS,
		augtype = ALL_AUGMENT_TYPES,
	}
	local modifiers = {}

	for field, value in pairs(field_totals) do
		local cap = field_caps[field]
		if cap and value > cap then
			value = cap
		end
		modifiers[field] = value
	end

	local augment = eq.create_live_item({
		item_id = TEMPLATE_ITEM_ID,
		charges = 1,
		data = data,
		modifiers = modifiers,
		custom_data = {
			live_items_test_roll = "orin-shardwork-augment",
			live_items_currency_id = CURRENCY_ID,
			live_items_currency_spent = total_cost,
			live_items_summary = summary,
		},
	})

	if not augment or not augment.valid then
		error("Augment template " .. tostring(TEMPLATE_ITEM_ID) .. " was not found.")
	end

	return augment, summary
end

local function confirm_build(e)
	if not is_started(e.other) then
		e.self:QuestSay(e.other, "No shardwork is active. Say " .. say_link("start", "start") .. " to begin one.")
		return
	end

	local total_cost, field_totals = build_totals(e.other)
	if total_cost <= 0 then
		e.self:QuestSay(e.other, "Add at least one upgrade before finishing the augment.")
		show_build(e)
		return
	end

	local violations = find_limit_violations(field_totals)
	if #violations > 0 then
		e.other:Message(13, "This shardwork exceeds safe augment caps: " .. table.concat(violations, "; ") .. ". Say reset and rebuild it.")
		return
	end

	if not cursor_is_empty(e.other) then
		e.self:QuestSay(e.other, "Clear your cursor before finishing the shardwork, or the new augment can be hidden behind the item you are already holding.")
		return
	end

	local balance = e.other:GetAlternateCurrencyValue(CURRENCY_ID) or 0
	if balance < total_cost then
		e.self:QuestSay(e.other, "You need " .. total_cost .. " " .. CURRENCY_NAME .. ", but you only have " .. balance .. ".")
		return
	end

	if not e.other:RemoveAlternateCurrencyValue(CURRENCY_ID, total_cost) then
		e.self:QuestSay(e.other, "The shardwork would not take payment. Try again after checking your " .. CURRENCY_NAME .. ".")
		return
	end

	local ok, augment, summary = pcall(create_augment, e.other, field_totals, total_cost)
	if not ok or not augment then
		e.other:AddAlternateCurrencyValue(CURRENCY_ID, total_cost)
		e.other:Message(13, "The augment weave failed and your " .. CURRENCY_NAME .. " were returned.")
		if eq and eq.debug then
			eq.debug("Live Items Orin shardwork error: " .. tostring(augment), 1)
		end
		return
	end

	local reward_name = get_build_name(e.other)
	if not e.other:RewardLiveItem(augment) then
		e.other:AddAlternateCurrencyValue(CURRENCY_ID, total_cost)
		e.other:Message(13, "The augment weave finished, but I could not place it on your cursor. Your " .. CURRENCY_NAME .. " were returned.")
		return
	end

	reset_build(e.other)
	e.self:QuestSay(e.other, "Done. The Blood Shards are bound into " .. reward_name .. ": " .. summary .. ".")
end

function event_say(e)
	if not live_items_enabled() then
		e.self:QuestSay(e.other, "Shardwork is disabled right now.")
		return
	end

	local raw_message = e.message or ""
	local message = normalize(raw_message)

	if message == "hail" then
		show_greeting(e)
		return
	end

	if message == "start" or message == "begin" or message == "startashardwork" then
		start_build(e.other)
		e.self:QuestSay(e.other, "The shardwork is started. Add upgrades, name it if you like, then finish it.")
		show_build(e)
		return
	end

	if message == "show" or message == "status" or message == "showmyshardwork" then
		show_build(e)
		return
	end

	if message == "reset" or message == "forget" or message == "forgetit" then
		reset_build(e.other)
		e.self:QuestSay(e.other, "The shardwork is cleared.")
		return
	end

	if message == "confirm" or message == "finish" or message == "finishit" then
		confirm_build(e)
		return
	end

	if message == "testshards" or message == "shards" then
		e.other:AddAlternateCurrencyValue(CURRENCY_ID, 250)
		e.other:SummonItem(CURRENCY_ITEM_ID, 20)
		e.self:QuestSay(e.other, "I added 250 " .. CURRENCY_NAME .. " for testing and gave you a few shard tokens.")
		return
	end

	local name = raw_message:match("^[Nn][Aa][Mm][Ee]%s+(.+)$") or raw_message:match("^%<(.+)%>$")
	if name then
		set_build_name(e, name)
		return
	end

	local add_key = raw_message:match("^[Aa][Dd][Dd]%s+([%a]+)$")
	if add_key then
		add_upgrade(e, add_key)
		return
	end

	e.self:QuestSay(e.other, "Say " .. say_link("show", "show") .. " to see your shardwork, or " .. say_link("start", "start") .. " to begin one.")
end

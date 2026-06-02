local REQUIRED_COUNT = 5
local COFFER_ITEM_ID = 900090
local CLASS_LINKS_PER_LINE = 4

local fragments = {
	{ item_id = 900010, key = "scarred", name = "Scarred Dranik War Fragment" },
	{ item_id = 900011, key = "causeway", name = "Noble Causeway Battle Fragment" },
	{ item_id = 900012, key = "bloodfields", name = "Bloodfields War Fragment" },
	{ item_id = 900013, key = "ruined", name = "Ruined Dranik Relic Fragment" },
	{ item_id = 900014, key = "harbinger", name = "Harbinger Spire Relic Fragment" },
	{ item_id = 900015, key = "wall", name = "Wall of Slaughter War Fragment" },
	{ item_id = 900016, key = "riftseeker", name = "Riftseeker Sanctum Relic Fragment" },
}

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

local fragment_by_id = {}
for _, fragment in ipairs(fragments) do
	fragment_by_id[fragment.item_id] = fragment
end

local class_by_key = {}
for _, class in ipairs(classes) do
	class_by_key[class.key] = class
end

local function normalize(value)
	return (value or ""):lower():gsub("[%s_%-]", "")
end

local function bucket_name(key)
	return "live_items_epic." .. key
end

local function get_bucket(client, key)
	return client:GetBucket(bucket_name(key))
end

local function set_bucket(client, key, value)
	client:SetBucket(bucket_name(key), tostring(value))
end

local function selected_class(client)
	return class_by_key[normalize(get_bucket(client, "class"))]
end

local function reset_progress(client)
	for _, fragment in ipairs(fragments) do
		set_bucket(client, "fragment." .. fragment.key, 0)
	end
end

local function progress_for(client, fragment)
	return tonumber(get_bucket(client, "fragment." .. fragment.key)) or 0
end

local function set_progress(client, fragment, count)
	set_bucket(client, "fragment." .. fragment.key, math.max(0, math.min(REQUIRED_COUNT, count)))
end

local function is_complete(client)
	for _, fragment in ipairs(fragments) do
		if progress_for(client, fragment) < REQUIRED_COUNT then
			return false
		end
	end

	return true
end

local function class_link_rows()
	local rows = {}
	local links = {}

	for index, class in ipairs(classes) do
		table.insert(links, eq.say_link("choose " .. class.key, false, class.name))
		if #links == CLASS_LINKS_PER_LINE or index == #classes then
			table.insert(rows, table.concat(links, ", "))
			links = {}
		end
	end

	return rows
end

local function say_class_links(npc)
	npc:Say("Choose your class epic:")

	for _, row in ipairs(class_link_rows()) do
		npc:Say(row)
	end
end

local function show_progress(client)
	local class = selected_class(client)
	if class then
		client:Message(15, "Requested epic: " .. class.name)
	else
		client:Message(15, "Requested epic: none selected")
	end

	for _, fragment in ipairs(fragments) do
		client:Message(15, string.format("%s: %d/%d", fragment.name, progress_for(client, fragment), REQUIRED_COUNT))
	end
end

local function choose_class(e, key)
	local class = class_by_key[normalize(key)]
	if not class then
		e.self:Say("I can shape epics for these disciplines.")
		say_class_links(e.self)
		return
	end

	set_bucket(e.other, "class", class.key)
	set_bucket(e.other, "class_name", class.name)
	set_bucket(e.other, "base_item_id", class.base_item_id)
	reset_progress(e.other)
	e.self:Say(string.format("Then we forge for a %s. Bring me five of each war fragment and relic fragment.", class.name))
	show_progress(e.other)
end

local function fragment_quantity(inst)
	local charges = inst:GetCharges()
	if charges and charges > 0 then
		return charges
	end

	return 1
end

local function trade_fragment_quantity(trade, fragment)
	local quantity = 0
	for slot = 1, 4 do
		local inst = trade["item" .. slot]
		if inst and inst.valid and inst:GetID() == fragment.item_id then
			quantity = quantity + fragment_quantity(inst)
		end
	end

	return quantity
end

local function consume_fragment(e, fragment, quantity)
	return e.self:CheckHandin(e.other, {}, { [tostring(fragment.item_id)] = quantity }, {})
end

local function reward_coffer(e, class)
	local coffer = ItemInst(COFFER_ITEM_ID, 1)
	coffer:SetCustomData("live_items_epic_class", class.key)
	coffer:SetCustomData("live_items_epic_class_name", class.name)
	coffer:SetCustomData("live_items_epic_base_item_id", tostring(class.base_item_id))
	coffer:SetDynamicItemData("name", class.name .. " Sealed Epic Coffer")
	coffer:SetDynamicItemData("lore", "Open this coffer to roll a " .. class.name .. " Item Forge epic.")
	coffer:RebuildDynamicItemData()

	e.other:PushItemOnCursor(coffer)
	e.self:Say("The fragments answer the forge. Open the coffer when you are ready to roll your epic.")

	set_bucket(e.other, "class", "")
	set_bucket(e.other, "class_name", "")
	set_bucket(e.other, "base_item_id", 0)
	reset_progress(e.other)
end

local function reward_if_complete(e)
	local class = selected_class(e.other)
	if class and is_complete(e.other) then
		reward_coffer(e, class)
		return true
	end

	return false
end

function event_say(e)
	local message = normalize(e.message)

	if message:find("^hail") then
		if reward_if_complete(e) then
			return
		end

		e.self:Say("The Item Forge is awake. Choose the class epic you want, then bring me five of each fragment.")
		say_class_links(e.self)
		show_progress(e.other)
		return
	end

	local chosen_key = message:match("^choose(.+)$")
	if chosen_key then
		choose_class(e, chosen_key)
		return
	end

	if message == "progress" or message == "status" then
		show_progress(e.other)
		return
	end

	if message == "reset" then
		set_bucket(e.other, "class", "")
		set_bucket(e.other, "class_name", "")
		set_bucket(e.other, "base_item_id", 0)
		reset_progress(e.other)
		e.self:Say("Your Item Forge epic request has been reset.")
		return
	end
end

function event_trade(e)
	local item_lib = require("items")
	local class = selected_class(e.other)
	if not class then
		e.self:Say("Choose the class epic first.")
		say_class_links(e.self)
		item_lib.return_items(e.self, e.other, e.trade)
		return
	end

	local accepted_any = false
	local fragment_handed = false

	for _, fragment in ipairs(fragments) do
		local quantity = trade_fragment_quantity(e.trade, fragment)
		if quantity > 0 then
			fragment_handed = true

			local current = progress_for(e.other, fragment)
			local needed = math.max(0, REQUIRED_COUNT - current)
			local accepted = math.min(needed, quantity)

			if accepted > 0 and consume_fragment(e, fragment, accepted) then
				set_progress(e.other, fragment, current + accepted)
				accepted_any = true
			end
		end
	end

	item_lib.return_items(e.self, e.other, e.trade)

	if accepted_any then
		show_progress(e.other)
		if is_complete(e.other) then
			reward_coffer(e, class)
		else
			e.self:Say("The forge remembers your progress. Bring the rest when you have them.")
		end
	elseif is_complete(e.other) then
		reward_coffer(e, class)
	elseif fragment_handed then
		e.self:Say("I already have enough of those fragments for this request.")
	else
		e.self:Say("Those are not the fragments this forge needs.")
	end
end

local CURSOR_SLOT = 33
local HEIRLOOM_GROWTH_PER_LEVEL = 10
local DEBUG_HEIRLOOM_LEVEL_UP = true

local commands = {
	helditemid = true,
	itemid = true,
	cursoritemid = true,
	heirloomdebug = true,
	evolvingdebug = true,
	liveitemdebug = true,
}

local function live_items_enabled()
	local value = eq.get_rule("CustomFeatures:LiveItemsEnabled")
	return value == nil or value == "" or value == "true" or value == "1"
end

local function is_handled_command(command)
	return commands[string.lower(tostring(command or ""))] == true
end

local function message(client, text)
	client:Message(15, text)
end

local function debug_message(client, text)
	if DEBUG_HEIRLOOM_LEVEL_UP then
		message(client, "[LiveItems Debug] " .. text)
	end
end

local function number(value, default)
	local converted = tonumber(value)
	if converted == nil then
		return default
	end
	return converted
end

local function trim_item_name(name)
	name = tostring(name or "Live Items Heirloom")
	name = name:gsub("%s+", " "):gsub("^%s+", ""):gsub("%s+$", "")
	return string.sub(name, 1, 64)
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

local function root_name(inst)
	local stored = inst:GetCustomData("live_items_original_name")
	if stored and stored ~= "" then
		return stored
	end

	local name = tostring(inst:GetName() or "Live Items Heirloom")
	name = name:gsub("%s%+%d+$", "")
	inst:SetCustomData("live_items_original_name", name)
	return name
end

local function is_heirloom(inst)
	if not inst or not inst.valid then
		return false
	end

	return inst:GetCustomData("live_items_heirloom") == "1"
end

local function short_item_label(inst)
	if not inst or not inst.valid then
		return "empty"
	end

	local link = inst:GetItemLink()
	if link and link ~= "" then
		return link
	end

	return tostring(inst:GetName() or ("item " .. tostring(inst:GetID())))
end

local function scan_heirloom_slots(client, send_debug)
	local inventory = client:GetInventory()
	local result = {
		scanned = 0,
		occupied = 0,
		found = {},
		cursor = nil,
	}

	for _, slot_id in pairs(client:GetInventorySlots()) do
		if slot_id < 2000 then
			local inst = inventory:GetItem(slot_id)

			if slot_id == CURSOR_SLOT then
				if inst and inst.valid then
					result.cursor = inst
				end
			else
				result.scanned = result.scanned + 1
				if inst and inst.valid then
					result.occupied = result.occupied + 1
					if is_heirloom(inst) then
						table.insert(result.found, { slot_id = slot_id, inst = inst })
					end
				end
			end
		end
	end

	if send_debug then
		debug_message(
			client,
			"Heirloom scan: scanned " .. tostring(result.scanned) ..
			" normal slots, occupied " .. tostring(result.occupied) ..
			", marked heirlooms found " .. tostring(#result.found) .. "."
		)

		for _, entry in ipairs(result.found) do
			local inst = entry.inst
			debug_message(
				client,
				"Found heirloom slot " .. tostring(entry.slot_id) ..
				": " .. short_item_label(inst) ..
				", rank " .. tostring(inst:GetCustomData("live_items_instance_rank") or "0") ..
				", owner " .. tostring(inst:GetCustomData("live_items_heirloom_owner") or "")
			)
		end

		if result.cursor and result.cursor.valid then
			debug_message(
				client,
				"Cursor holds " .. short_item_label(result.cursor) ..
				" with heirloom flag '" .. tostring(result.cursor:GetCustomData("live_items_heirloom") or "") ..
				"'. Cursor items are not auto-grown; put the heirloom in inventory or equipment before leveling."
			)
		end
	end

	return result
end

local function summarize(parts)
	if #parts <= 6 then
		return table.concat(parts, ", ")
	end

	local visible = {}
	for index = 1, 6 do
		table.insert(visible, parts[index])
	end
	table.insert(visible, "and " .. tostring(#parts - 6) .. " more")
	return table.concat(visible, ", ")
end

local function grow_heirloom(inst, levels_gained)
	local item = inst:GetItem()
	if not item or not item:valid() then
		return nil
	end

	local amount = HEIRLOOM_GROWTH_PER_LEVEL * math.max(1, levels_gained)
	local rank = number(inst:GetCustomData("live_items_instance_rank"), 0) + math.max(1, levels_gained)
	local changed = {}

	for _, field in ipairs(stat_fields) do
		local current = number(field.get(item), 0)
		if current > 0 then
			local next_value = math.min(field.cap, current + amount)
			inst:DeleteDynamicItemModifier(field.key)
			inst:SetDynamicItemData(field.key, next_value)
			table.insert(changed, field.label .. " +" .. tostring(next_value - current))
		end
	end

	if #changed == 0 then
		return nil
	end

	local name = trim_item_name(root_name(inst) .. " +" .. tostring(rank))
	inst:SetDynamicItemData("name", name)
	inst:SetDynamicItemData("comment", "Live Items heirloom growth rank " .. tostring(rank) .. ": " .. summarize(changed))
	inst:SetCustomData("live_items_heirloom", "1")
	inst:SetCustomData("live_items_instance_rank", tostring(rank))
	inst:SetCustomData("live_items_heirloom_level", tostring(number(inst:GetCustomData("live_items_heirloom_level"), 0) + math.max(1, levels_gained)))
	inst:SetCustomData("live_items_last_heirloom_growth", tostring(os.time()))
	inst:RebuildDynamicItemData()

	return {
		name = name,
		rank = rank,
		changed_count = #changed,
		summary = summarize(changed),
	}
end

function event_command(e)
	if not is_handled_command(e.command) then
		return 0
	end

	if not live_items_enabled() then
		return 0
	end

	local command = string.lower(tostring(e.command or ""))
	if command == "heirloomdebug" or command == "evolvingdebug" or command == "liveitemdebug" then
		debug_message(e.self, "Manual heirloom debug command received.")
		scan_heirloom_slots(e.self, true)
		return 1
	end

	local inst = e.self:GetInventory():GetItem(CURSOR_SLOT)
	if not inst or not inst.valid then
		message(e.self, "Hold an item on your cursor, then use #helditemid.")
		return 1
	end

	local item_id = inst:GetID()
	if item_id == nil or item_id <= 0 then
		message(e.self, "I could not read an item ID from the held cursor item.")
		return 1
	end

	local name = inst:GetName()
	local link = inst:GetItemLink()
	local serial = inst:GetSerialNumber()
	local parts = {
		"Held item ID: " .. tostring(item_id),
	}

	if name and name ~= "" then
		table.insert(parts, "Name: " .. name)
	end

	if link and link ~= "" then
		table.insert(parts, "Link: " .. link)
	end

	if serial and serial > 0 then
		table.insert(parts, "Serial: " .. tostring(serial))
	end

	message(e.self, table.concat(parts, " | "))
	return 1
end

function event_level_up(e)
	if not live_items_enabled() then
		return
	end

	local client = e.self
	local levels_gained = math.max(1, number(e.levels_gained, 1))
	local grown = 0
	local first_link = nil
	local first_summary = nil

	debug_message(client, "event_level_up fired; levels_gained=" .. tostring(levels_gained) .. ".")
	local scan = scan_heirloom_slots(client, true)

	for _, entry in ipairs(scan.found) do
		local slot_id = entry.slot_id
		local inst = entry.inst
		debug_message(client, "Attempting heirloom growth in slot " .. tostring(slot_id) .. ": " .. short_item_label(inst))

		local upgraded = inst:Clone()
		local result = grow_heirloom(upgraded, levels_gained)
		if result and client:PutItemInInventory(slot_id, upgraded) then
			grown = grown + 1
			debug_message(client, "Slot " .. tostring(slot_id) .. " updated to rank " .. tostring(result.rank) .. ".")
			if first_link == nil then
				first_link = upgraded:GetItemLink()
				first_summary = result.summary
			end
		elseif result then
			debug_message(client, "Slot " .. tostring(slot_id) .. " growth failed while saving back to inventory.")
		else
			debug_message(client, "Slot " .. tostring(slot_id) .. " had no supported positive stats to grow.")
		end
	end

	if grown > 0 then
		local level_text = levels_gained == 1 and "level" or "levels"
		message(
			client,
			"Your Live Items heirloom grew for " .. tostring(levels_gained) .. " " .. level_text ..
			": " .. tostring(first_link or "heirloom") .. " (" .. tostring(first_summary or "stats increased") .. ")."
		)

		if grown > 1 then
			message(client, tostring(grown) .. " heirloom items were updated.")
		end
	else
		debug_message(client, "No heirloom items were updated on this level-up.")
	end
end

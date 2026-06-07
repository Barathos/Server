/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "common/emu_constants.h"
#include "common/item_instance.h"
#include "common/rulesys.h"
#include "common/seperator.h"
#include "common/strings.h"
#include "zone/client.h"
#include "zone/mob.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {

struct UseItemMatch {
	int16 slot_id = EQ::invslot::SLOT_INVALID;
	std::string name;
};

std::string NormalizeItemName(std::string value)
{
	Strings::Trim(value);
	if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
		value = value.substr(1, value.size() - 2);
		Strings::Trim(value);
	}

	return Strings::ToLower(value);
}

bool IsClickableItem(const EQ::ItemInstance *inst)
{
	if (!inst || !inst->IsClassCommon()) {
		return false;
	}

	const auto *item = inst->GetItem();
	if (!item) {
		return false;
	}

	if (item->ItemType == EQ::item::ItemTypeSpell) {
		return true;
	}

	if (
		item->Click.Effect > 0 &&
		(
			item->Click.Type == EQ::item::ItemEffectClick ||
			item->Click.Type == EQ::item::ItemEffectExpendable ||
			item->Click.Type == EQ::item::ItemEffectEquipClick ||
			item->Click.Type == EQ::item::ItemEffectClick2
		)
	) {
		return true;
	}

	for (int socket = EQ::invaug::SOCKET_BEGIN; socket <= EQ::invaug::SOCKET_END; ++socket) {
		const auto *augment = inst->GetAugment(socket);
		const auto *augment_item = augment ? augment->GetItem() : nullptr;
		if (
			augment_item &&
			augment_item->Click.Effect > 0 &&
			(
				augment_item->Click.Type == EQ::item::ItemEffectClick ||
				augment_item->Click.Type == EQ::item::ItemEffectExpendable ||
				augment_item->Click.Type == EQ::item::ItemEffectEquipClick ||
				augment_item->Click.Type == EQ::item::ItemEffectClick2
			)
		) {
			return true;
		}
	}

	return false;
}

void AddSlotIfMatch(Client *client, int16 slot_id, const std::string &needle, std::vector<UseItemMatch> &exact, std::vector<UseItemMatch> &partial)
{
	auto &inventory = client->GetInv();
	const auto *inst = inventory.GetItem(slot_id);
	if (!inst || !IsClickableItem(inst)) {
		return;
	}

	const auto *item = inst->GetItem();
	if (!item) {
		return;
	}

	if (
		!inventory.SupportsClickCasting(slot_id) &&
		!((item->ItemType == EQ::item::ItemTypePotion || item->PotionBelt) && inventory.SupportsPotionBeltCasting(slot_id))
	) {
		return;
	}

	const std::string item_name = item->Name;
	const auto lower_name = Strings::ToLower(item_name);
	if (lower_name == needle) {
		exact.push_back({ slot_id, item_name });
	} else if (lower_name.find(needle) != std::string::npos) {
		partial.push_back({ slot_id, item_name });
	}
}

void AddRangeMatches(Client *client, int16 begin_slot, int16 end_slot, const std::string &needle, std::vector<UseItemMatch> &exact, std::vector<UseItemMatch> &partial)
{
	for (int16 slot_id = begin_slot; slot_id <= end_slot; ++slot_id) {
		AddSlotIfMatch(client, slot_id, needle, exact, partial);
	}
}

} // namespace

void command_useitem(Client *c, const Seperator *sep)
{
	if (!c) {
		return;
	}

	if (!RuleB(CustomFeatures, UseItemCommandEnabled)) {
		c->Message(Chat::White, "The useitem command is disabled on this server.");
		return;
	}

	std::string item_name = sep && sep->argplus[1] ? sep->argplus[1] : "";
	const auto needle = NormalizeItemName(item_name);
	if (needle.empty()) {
		c->Message(Chat::White, "Usage: #useitem <item name>");
		return;
	}

	std::vector<UseItemMatch> exact;
	std::vector<UseItemMatch> partial;

	AddRangeMatches(c, EQ::invslot::EQUIPMENT_BEGIN, EQ::invslot::EQUIPMENT_END, needle, exact, partial);
	AddRangeMatches(c, EQ::invslot::GENERAL_BEGIN, EQ::invslot::GENERAL_END, needle, exact, partial);
	AddRangeMatches(c, EQ::invbag::GENERAL_BAGS_BEGIN, EQ::invbag::GENERAL_BAGS_END, needle, exact, partial);
	AddSlotIfMatch(c, EQ::invslot::slotCursor, needle, exact, partial);
	AddRangeMatches(c, EQ::invbag::CURSOR_BAG_BEGIN, EQ::invbag::CURSOR_BAG_END, needle, exact, partial);

	const auto &matches = !exact.empty() ? exact : partial;
	if (matches.empty()) {
		c->Message(Chat::White, "No clickable carried item matched '%s'.", item_name.c_str());
		return;
	}

	if (matches.size() > 1) {
		c->Message(Chat::White, "Multiple clickable items matched '%s':", item_name.c_str());
		const auto limit = std::min<size_t>(matches.size(), 8);
		for (size_t index = 0; index < limit; ++index) {
			c->Message(Chat::White, "- %s (slot %d)", matches[index].name.c_str(), matches[index].slot_id);
		}
		if (matches.size() > limit) {
			c->Message(Chat::White, "...and %u more.", static_cast<unsigned>(matches.size() - limit));
		}
		return;
	}

	const auto target_id = c->GetTarget() ? c->GetTarget()->GetID() : c->GetID();
	c->UseItemSlot(matches.front().slot_id, target_id);
}

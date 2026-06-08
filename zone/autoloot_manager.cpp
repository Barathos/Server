/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "autoloot_manager.h"

#include "common/data_verification.h"
#include "common/emu_constants.h"
#include "common/inventory_profile.h"
#include "common/item_instance.h"
#include "common/rulesys.h"
#include "common/say_link.h"
#include "common/seperator.h"
#include "common/strings.h"
#include "zone/client.h"
#include "zone/corpse.h"
#include "zone/entity.h"
#include "zone/groups.h"
#include "zone/mob.h"
#include "zone/zone.h"
#include "zone/zonedb.h"

#include "fmt/format.h"

#include <algorithm>
#include <cstring>
#include <memory>

extern EntityList entity_list;
extern Zone *zone;

AutoLootManager auto_loot_manager;

namespace {
	constexpr uint32 kNeedGreedSeconds = 60;

	bool AutoLootEnabled()
	{
		return RuleB(CustomFeatures, AutoLootEnabled);
	}

	bool RequireAutoLootEnabled(Client *client)
	{
		if (AutoLootEnabled()) {
			return true;
		}

		if (client) {
			client->Message(Chat::White, "Advanced Loot is disabled on this server.");
		}

		return false;
	}

	bool IsBagChildSlot(int16 slot_id)
	{
		return EQ::ValueWithin(slot_id, EQ::invbag::GENERAL_BAGS_BEGIN, EQ::invbag::GENERAL_BAGS_END) ||
			EQ::ValueWithin(slot_id, EQ::invbag::CURSOR_BAG_BEGIN, EQ::invbag::CURSOR_BAG_END) ||
			EQ::ValueWithin(slot_id, EQ::invbag::BANK_BAGS_BEGIN, EQ::invbag::BANK_BAGS_END) ||
			EQ::ValueWithin(slot_id, EQ::invbag::SHARED_BANK_BAGS_BEGIN, EQ::invbag::SHARED_BANK_BAGS_END) ||
			EQ::ValueWithin(slot_id, EQ::invbag::TRADE_BAGS_BEGIN, EQ::invbag::TRADE_BAGS_END);
	}

	uint32 GetCorpseItemQuantity(Corpse *corpse, uint16 loot_slot)
	{
		auto item_data = corpse ? corpse->GetItem(loot_slot) : nullptr;
		if (!item_data) {
			return 0;
		}

		const auto *item = database.GetItem(item_data->item_id);
		if (!item) {
			return 0;
		}

		return item->Stackable ? std::max<uint32>(1, item_data->charges) : 1;
	}

	bool IsDynamicCorpseLootItem(const LootItem *item_data)
	{
		if (!item_data || item_data->custom_data.empty()) {
			return false;
		}

		const auto custom_data = Strings::ToLower(item_data->custom_data);
		return custom_data.find("dynamic_item.") != std::string::npos ||
			custom_data.find("live_items_") != std::string::npos;
	}

	std::string QuantitySuffix(uint32 quantity)
	{
		return quantity > 1 ? fmt::format(" x{}", quantity) : "";
	}

	EQ::ItemInstance *CreateCorpseLootItemInstance(Client *client, const LootItem *item_data)
	{
		if (!client || !item_data) {
			return nullptr;
		}

		const auto *item = database.GetItem(item_data->item_id);
		if (!item) {
			return nullptr;
		}

		auto *inst = database.CreateItem(
			item,
			item_data->charges,
			item_data->aug_1,
			item_data->aug_2,
			item_data->aug_3,
			item_data->aug_4,
			item_data->aug_5,
			item_data->aug_6,
			item_data->attuned,
			item_data->custom_data,
			item_data->ornamenticon,
			item_data->ornamentidfile,
			item_data->ornament_hero_model
		);

		if (!inst) {
			return nullptr;
		}

		if (item->RecastDelay) {
			auto timestamps = database.GetItemRecastTimestamps(client->CharacterID());
			if (item->RecastType != RECAST_TYPE_UNLINKED_ITEM) {
				inst->SetRecastTimestamp(timestamps.count(item->RecastType) ? timestamps.at(item->RecastType) : 0);
			}
			else {
				inst->SetRecastTimestamp(timestamps.count(item->ID) ? timestamps.at(item->ID) : 0);
			}
		}

		return inst;
	}

	const EQ::ItemData *GetCorpseLootDisplayItem(Client *client, const LootItem *item_data, std::unique_ptr<EQ::ItemInstance> &inst)
	{
		inst.reset(CreateCorpseLootItemInstance(client, item_data));
		if (inst && inst->GetItem()) {
			return inst->GetItem();
		}

		return item_data ? database.GetItem(item_data->item_id) : nullptr;
	}

	std::string ProtocolValue(std::string value)
	{
		for (auto &c : value) {
			if (c == '|' || c == '\r' || c == '\n' || c == '\t') {
				c = ' ';
			}
		}

		return value;
	}

	bool WantsWindowRefresh(const Seperator *sep)
	{
		if (!sep) {
			return false;
		}

		for (int argument_index = 1; argument_index <= sep->argnum; ++argument_index) {
			if (
				!strcasecmp(sep->arg[argument_index], "ui") ||
				!strcasecmp(sep->arg[argument_index], "window") ||
				!strcasecmp(sep->arg[argument_index], "wnd") ||
				!strcasecmp(sep->arg[argument_index], "panel")
			) {
				return true;
			}
		}

		return false;
	}

	void RefreshWindowIfRequested(AutoLootManager *manager, Client *client, const Seperator *sep)
	{
		if (WantsWindowRefresh(sep)) {
			manager->ShowWindow(client);
		}
	}

	std::string VoteChoiceState(AutoLootManager::VoteChoice choice)
	{
		switch (choice) {
		case AutoLootManager::VoteChoice::Need:
			return "need";
		case AutoLootManager::VoteChoice::Greed:
			return "greed";
		case AutoLootManager::VoteChoice::Pass:
			return "no";
		default:
			return "waiting";
		}
	}

	std::string VoteChoiceLabel(AutoLootManager::VoteChoice choice)
	{
		switch (choice) {
		case AutoLootManager::VoteChoice::Need:
			return "Need";
		case AutoLootManager::VoteChoice::Greed:
			return "Greed";
		case AutoLootManager::VoteChoice::Pass:
			return "No";
		default:
			return "Waiting";
		}
	}

	bool IsValidFilterDecision(const std::string &decision)
	{
		return Strings::EqualFold(decision, "unset") ||
			Strings::EqualFold(decision, "always_need") ||
			Strings::EqualFold(decision, "always_greed") ||
			Strings::EqualFold(decision, "never") ||
			Strings::EqualFold(decision, "an") ||
			Strings::EqualFold(decision, "ag") ||
			Strings::EqualFold(decision, "nv") ||
			Strings::EqualFold(decision, "need") ||
			Strings::EqualFold(decision, "greed") ||
			Strings::EqualFold(decision, "no");
	}

	AutoLootManager::LootFilterDecision ParseFilterDecision(const std::string &decision)
	{
		if (Strings::EqualFold(decision, "always_need") || Strings::EqualFold(decision, "an") || Strings::EqualFold(decision, "need")) {
			return AutoLootManager::LootFilterDecision::AlwaysNeed;
		}

		if (Strings::EqualFold(decision, "always_greed") || Strings::EqualFold(decision, "ag") || Strings::EqualFold(decision, "greed")) {
			return AutoLootManager::LootFilterDecision::AlwaysGreed;
		}

		if (Strings::EqualFold(decision, "never") || Strings::EqualFold(decision, "nv")) {
			return AutoLootManager::LootFilterDecision::Never;
		}

		return AutoLootManager::LootFilterDecision::Unset;
	}

	std::string FilterDecisionKey(AutoLootManager::LootFilterDecision decision)
	{
		switch (decision) {
		case AutoLootManager::LootFilterDecision::AlwaysNeed:
			return "always_need";
		case AutoLootManager::LootFilterDecision::AlwaysGreed:
			return "always_greed";
		case AutoLootManager::LootFilterDecision::Never:
			return "never";
		default:
			return "unset";
		}
	}

	std::string FilterDecisionLabel(AutoLootManager::LootFilterDecision decision)
	{
		switch (decision) {
		case AutoLootManager::LootFilterDecision::AlwaysNeed:
			return "AN";
		case AutoLootManager::LootFilterDecision::AlwaysGreed:
			return "AG";
		case AutoLootManager::LootFilterDecision::Never:
			return "NV";
		default:
			return "-";
		}
	}

}

void AutoLootManager::Process()
{
	if (!AutoLootEnabled()) {
		m_loot_entries.clear();
		return;
	}

	const time_t now = std::time(nullptr);
	if (now == m_last_process) {
		return;
	}

	m_last_process = now;

	std::vector<uint32> expired_shared_votes;
	for (const auto &[entry_id, entry] : m_loot_entries) {
		if (entry.shared && entry.vote_started_at > 0 && entry.vote_started_at <= now - kNeedGreedSeconds) {
			expired_shared_votes.push_back(entry_id);
		}
	}

	for (const auto entry_id : expired_shared_votes) {
		ResolveSharedVote(entry_id, true);
	}

	PruneLootEntries();
}

AutoLootManager::CharacterSettings AutoLootManager::GetCharacterSettings(uint32 character_id, bool create_enabled)
{
	CharacterSettings settings;
	if (!character_id) {
		return settings;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `use_advanced_looting`, `apply_filters`, `auto_split_coin`, `confirm_remove_filter`, "
			"`auto_remove_looted_lore`, `auto_show_loot_window`, `show_new_items_only`, `auto_loot_all`, "
			"`master_looter_candidate`, `debug_enabled`, `log_enabled` "
			"FROM `custom_advloot_settings` WHERE `character_id` = {} LIMIT 1",
			character_id
		)
	);

	if (!results.Success() || !results.RowCount()) {
		if (results.Success() && create_enabled) {
			SaveCharacterSettings(character_id, settings);
		}

		return settings;
	}

	auto row = results.begin();
	settings.use_advanced_looting    = row[0] ? Strings::ToBool(row[0]) : true;
	settings.apply_filters           = row[1] ? Strings::ToBool(row[1]) : true;
	settings.auto_split_coin         = row[2] ? Strings::ToBool(row[2]) : true;
	settings.confirm_remove_filter   = row[3] ? Strings::ToBool(row[3]) : true;
	settings.auto_remove_looted_lore = row[4] ? Strings::ToBool(row[4]) : true;
	settings.auto_show_loot_window   = row[5] ? Strings::ToBool(row[5]) : true;
	settings.show_new_items_only     = row[6] ? Strings::ToBool(row[6]) : false;
	settings.auto_loot_all           = row[7] ? Strings::ToBool(row[7]) : false;
	settings.master_looter_candidate = row[8] ? Strings::ToBool(row[8]) : true;
	settings.debug_enabled           = row[9] ? Strings::ToBool(row[9]) : false;
	settings.log_enabled             = row[10] ? Strings::ToBool(row[10]) : false;
	return settings;
}

void AutoLootManager::SaveCharacterSettings(uint32 character_id, const CharacterSettings &settings)
{
	database.QueryDatabase(
		fmt::format(
			"INSERT INTO `custom_advloot_settings` "
			"(`character_id`, `use_advanced_looting`, `apply_filters`, `auto_split_coin`, `confirm_remove_filter`, "
			"`auto_remove_looted_lore`, `auto_show_loot_window`, `show_new_items_only`, `auto_loot_all`, "
			"`master_looter_candidate`, `debug_enabled`, `log_enabled`, `updated_at`) "
			"VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, UNIX_TIMESTAMP()) "
			"ON DUPLICATE KEY UPDATE "
			"`use_advanced_looting` = VALUES(`use_advanced_looting`), `apply_filters` = VALUES(`apply_filters`), "
			"`auto_split_coin` = VALUES(`auto_split_coin`), `confirm_remove_filter` = VALUES(`confirm_remove_filter`), "
			"`auto_remove_looted_lore` = VALUES(`auto_remove_looted_lore`), `auto_show_loot_window` = VALUES(`auto_show_loot_window`), "
			"`show_new_items_only` = VALUES(`show_new_items_only`), `auto_loot_all` = VALUES(`auto_loot_all`), "
			"`master_looter_candidate` = VALUES(`master_looter_candidate`), `debug_enabled` = VALUES(`debug_enabled`), "
			"`log_enabled` = VALUES(`log_enabled`), "
			"`updated_at` = UNIX_TIMESTAMP()",
			character_id,
			settings.use_advanced_looting ? 1 : 0,
			settings.apply_filters ? 1 : 0,
			settings.auto_split_coin ? 1 : 0,
			settings.confirm_remove_filter ? 1 : 0,
			settings.auto_remove_looted_lore ? 1 : 0,
			settings.auto_show_loot_window ? 1 : 0,
			settings.show_new_items_only ? 1 : 0,
			settings.auto_loot_all ? 1 : 0,
			settings.master_looter_candidate ? 1 : 0,
			settings.debug_enabled ? 1 : 0,
			settings.log_enabled ? 1 : 0
		)
	);
}

void AutoLootManager::DebugMessage(Client *client, const CharacterSettings &settings, const std::string &message)
{
	if (!client || !settings.debug_enabled || message.empty()) {
		return;
	}

	client->Message(Chat::Yellow, fmt::format("[Advanced Loot Debug] {}", message).c_str());
}

AutoLootManager::FilterEntry AutoLootManager::GetFilter(uint32 character_id, uint32 item_id)
{
	FilterEntry filter;
	filter.item_id = item_id;
	if (!character_id || !item_id) {
		return filter;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `decision`, `auto_ask_roll` FROM `custom_advloot_filters` "
			"WHERE `character_id` = {} AND `item_id` = {} LIMIT 1",
			character_id,
			item_id
		)
	);

	if (!results.Success() || !results.RowCount()) {
		return filter;
	}

	auto row = results.begin();
	filter.decision = row[0] ? ParseFilterDecision(row[0]) : LootFilterDecision::Unset;
	filter.auto_ask_roll = row[1] ? Strings::ToBool(row[1]) : false;
	return filter;
}

void AutoLootManager::SetFilter(uint32 character_id, uint32 item_id, LootFilterDecision decision, bool auto_ask_roll)
{
	if (!character_id || !item_id) {
		return;
	}

	if (decision == LootFilterDecision::Unset && !auto_ask_roll) {
		RemoveFilter(character_id, item_id);
		return;
	}

	database.QueryDatabase(
		fmt::format(
			"INSERT INTO `custom_advloot_filters` (`character_id`, `item_id`, `decision`, `auto_ask_roll`, `created_at`, `updated_at`) "
			"VALUES ({}, {}, '{}', {}, UNIX_TIMESTAMP(), UNIX_TIMESTAMP()) "
			"ON DUPLICATE KEY UPDATE `decision` = VALUES(`decision`), `auto_ask_roll` = VALUES(`auto_ask_roll`), `updated_at` = UNIX_TIMESTAMP()",
			character_id,
			item_id,
			Strings::Escape(FilterDecisionKey(decision)),
			auto_ask_roll ? 1 : 0
		)
	);
}

void AutoLootManager::RemoveFilter(uint32 character_id, uint32 item_id)
{
	if (!character_id || !item_id) {
		return;
	}

	database.QueryDatabase(
		fmt::format(
			"DELETE FROM `custom_advloot_filters` WHERE `character_id` = {} AND `item_id` = {}",
			character_id,
			item_id
		)
	);
}

std::vector<AutoLootManager::FilterEntry> AutoLootManager::GetFilters(uint32 character_id)
{
	std::vector<FilterEntry> filters;
	if (!character_id) {
		return filters;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `item_id`, `decision`, `auto_ask_roll` FROM `custom_advloot_filters` "
			"WHERE `character_id` = {} ORDER BY `decision`, `item_id`",
			character_id
		)
	);

	if (!results.Success()) {
		return filters;
	}

	for (auto row = results.begin(); row != results.end(); ++row) {
		FilterEntry filter;
		filter.item_id = row[0] ? Strings::ToUnsignedInt(row[0]) : 0;
		filter.decision = row[1] ? ParseFilterDecision(row[1]) : LootFilterDecision::Unset;
		filter.auto_ask_roll = row[2] ? Strings::ToBool(row[2]) : false;
		if (filter.item_id) {
			filters.push_back(filter);
		}
	}

	return filters;
}

Client *AutoLootManager::ResolveLootClient(Mob *killer)
{
	if (!killer) {
		return nullptr;
	}

	if (killer->IsClient()) {
		return killer->CastToClient();
	}

	auto ultimate_owner = killer->GetUltimateOwner();
	if (ultimate_owner && ultimate_owner->IsClient()) {
		return ultimate_owner->CastToClient();
	}

	auto owner = killer->GetOwner();
	if (owner && owner->IsClient()) {
		return owner->CastToClient();
	}

	return nullptr;
}

std::vector<Client *> AutoLootManager::GetGroupClients(Group *group)
{
	std::vector<Client *> clients;
	if (!group) {
		return clients;
	}

	for (auto member : group->members) {
		if (member && member->IsClient()) {
			clients.push_back(member->CastToClient());
		}
	}

	return clients;
}

Client *AutoLootManager::FindAutoLootClient(Client *resolved_client, Corpse *corpse)
{
	if (!resolved_client || !corpse || !corpse->CanPlayerLoot(resolved_client->CharacterID())) {
		return nullptr;
	}

	auto settings = GetCharacterSettings(resolved_client->CharacterID(), true);
	if (settings.use_advanced_looting) {
		return resolved_client;
	}

	return nullptr;
}

Client *AutoLootManager::DetermineMasterLooter(Group *group, Corpse *corpse, Client *fallback)
{
	if (!group) {
		return fallback;
	}

	auto leader = group->GetLeader();
	if (leader && leader->IsClient()) {
		auto leader_client = leader->CastToClient();
		const auto leader_settings = GetCharacterSettings(leader_client->CharacterID(), true);
		if (leader_settings.master_looter_candidate && (!corpse || corpse->CanPlayerLoot(leader_client->CharacterID()))) {
			return leader_client;
		}
	}

	for (auto member : GetGroupClients(group)) {
		if (!member) {
			continue;
		}

		const auto settings = GetCharacterSettings(member->CharacterID(), true);
		if (settings.master_looter_candidate && (!corpse || corpse->CanPlayerLoot(member->CharacterID()))) {
			return member;
		}
	}

	return fallback;
}

void AutoLootManager::ProcessCorpseDeath(Corpse *corpse, Mob *killer)
{
	if (!AutoLootEnabled()) {
		return;
	}

	if (!corpse || !corpse->IsNPCCorpse()) {
		return;
	}

	auto client = ResolveLootClient(killer);
	if (!client) {
		return;
	}

	ProcessCorpse(corpse, client);
}

bool AutoLootManager::ProcessCorpse(Corpse *corpse, Client *resolved_client)
{
	if (!AutoLootEnabled()) {
		return false;
	}

	if (!corpse || !resolved_client || !corpse->IsNPCCorpse() || corpse->IsBeingLooted()) {
		return false;
	}

	if (QueueCorpseEntries(corpse, resolved_client)) {
		auto autoloot_client = FindAutoLootClient(resolved_client, corpse);
		if (autoloot_client) {
			SendNativeUpdate(autoloot_client);
		}

		return true;
	}

	return false;
}

bool AutoLootManager::QueueCorpseEntries(Corpse *corpse, Client *resolved_client)
{
	if (!AutoLootEnabled()) {
		return false;
	}

	if (!corpse || !resolved_client || !corpse->IsNPCCorpse() || corpse->IsBeingLooted()) {
		return false;
	}

	if (corpse->IsLocked()) {
		return false;
	}

	auto autoloot_client = FindAutoLootClient(resolved_client, corpse);
	if (!autoloot_client) {
		return false;
	}

	auto settings = GetCharacterSettings(autoloot_client->CharacterID(), true);
	bool drop_debug_sent = false;
	auto send_drop_debug = [&]() {
		if (drop_debug_sent || !settings.debug_enabled) {
			return;
		}

		drop_debug_sent = true;
		uint32 visible_loot = 0;
		for (auto item_data : corpse->GetLootItems()) {
			if (!item_data || !item_data->item_id || IsBagChildSlot(item_data->equip_slot)) {
				continue;
			}

			++visible_loot;
			std::unique_ptr<EQ::ItemInstance> display_inst;
			const auto *item = GetCorpseLootDisplayItem(autoloot_client, item_data, display_inst);
			const auto item_name = item ? item->Name : fmt::format("Unknown Item {}", item_data->item_id);
			DebugMessage(
				autoloot_client,
				settings,
				fmt::format(
					"{} dropped {}{}.",
					corpse->GetCleanName(),
					item_name,
					QuantitySuffix(GetCorpseItemQuantity(corpse, item_data->lootslot))
				)
			);
		}

		if (!visible_loot) {
			DebugMessage(autoloot_client, settings, fmt::format("{} had no top-level loot for Advanced Loot.", corpse->GetCleanName()));
		}
	};

	if (!settings.use_advanced_looting) {
		send_drop_debug();
		DebugMessage(autoloot_client, settings, fmt::format("{} was not queued because Advanced Loot is off for this character.", corpse->GetCleanName()));
		return false;
	}

	auto group = autoloot_client->GetGroup();
	const bool shared_loot = group && group->GroupCount() > 1;
	auto master_looter = shared_loot ? DetermineMasterLooter(group, corpse, autoloot_client) : nullptr;
	if (shared_loot && !master_looter) {
		send_drop_debug();
		DebugMessage(autoloot_client, settings, fmt::format("{} was not queued because no master looter could be calculated.", corpse->GetCleanName()));
		return false;
	}
	const auto master_settings = master_looter ? GetCharacterSettings(master_looter->CharacterID(), true) : CharacterSettings{};

	std::vector<uint16> loot_slots;
	for (auto item_data : corpse->GetLootItems()) {
		if (!item_data || IsBagChildSlot(item_data->equip_slot)) {
			continue;
		}

		loot_slots.push_back(item_data->lootslot);
	}
	send_drop_debug();

	const auto corpse_id = corpse->GetID();
	const auto corpse_name = corpse->GetCleanName();
	bool queued = false;
	std::vector<std::pair<Client *, uint32>> auto_loot_entries;
	std::vector<uint32> auto_roll_entries;
	for (const auto loot_slot : loot_slots) {
		auto item_data = corpse->GetItem(loot_slot);
		if (!item_data || !item_data->item_id) {
			continue;
		}

		std::unique_ptr<EQ::ItemInstance> display_inst;
		const auto *item = GetCorpseLootDisplayItem(autoloot_client, item_data, display_inst);
		const auto item_name = item ? item->Name : fmt::format("Unknown Item {}", item_data->item_id);
		const bool dynamic_instance = IsDynamicCorpseLootItem(item_data);
		const auto quantity = GetCorpseItemQuantity(corpse, loot_slot);
		if (HasQueuedEntry(corpse->GetID(), loot_slot)) {
			DebugMessage(
				autoloot_client,
				settings,
				fmt::format(
					"{}{} from {} was skipped because that corpse slot is already queued.",
					item_name,
					QuantitySuffix(quantity),
					corpse_name
				)
			);
			continue;
		}

		const auto personal_filter = dynamic_instance ? FilterEntry{} : GetFilter(autoloot_client->CharacterID(), item_data->item_id);
		if (!shared_loot && settings.apply_filters && personal_filter.decision == LootFilterDecision::Never) {
			DebugMessage(
				autoloot_client,
				settings,
				fmt::format(
					"{}{} from {} ignored by Advanced Loot never filter.",
					item_name,
					QuantitySuffix(quantity),
					corpse_name
				)
			);
			continue;
		}

		auto recipient = shared_loot ? master_looter : autoloot_client;
		if (!recipient || !corpse->CanPlayerLoot(recipient->CharacterID())) {
			DebugMessage(
				autoloot_client,
				settings,
				fmt::format(
					"{}{} from {} was not queued because no eligible recipient could loot it.",
					item_name,
					QuantitySuffix(quantity),
					corpse_name
				)
			);
			continue;
		}

		if (!item) {
			DebugMessage(
				autoloot_client,
				settings,
				fmt::format("Item {} from {} was not queued because item data could not be loaded.", item_data->item_id, corpse_name)
			);
			continue;
		}

		LootEntry entry;
		entry.entry_id = m_next_loot_entry_id++;
		entry.corpse_id = corpse->GetID();
		entry.loot_slot = loot_slot;
		entry.item_id = item_data->item_id;
		entry.icon_id = item->Icon;
		entry.quantity = quantity;
		entry.owner_character_id = recipient->CharacterID();
		entry.master_looter_character_id = master_looter ? master_looter->CharacterID() : 0;
		entry.group_id = group ? group->GetID() : 0;
		entry.shared = shared_loot;
		entry.no_drop = item->NoDrop == 0;
		entry.dynamic_instance = dynamic_instance;
		entry.item_name = item->Name;
		entry.corpse_name = corpse->GetCleanName();
		entry.state = "waiting";
		entry.rule = FilterDecisionKey(personal_filter.decision);
		entry.created_at = std::time(nullptr);

		if (entry.shared) {
			const auto master_filter = dynamic_instance ? FilterEntry{} : GetFilter(master_looter->CharacterID(), item_data->item_id);
			entry.rule = FilterDecisionKey(master_filter.decision);
			entry.auto_roll = master_settings.apply_filters && master_filter.auto_ask_roll;
			entry.state = entry.auto_roll ? "ask" : "waiting";
			entry.vote_started_at = entry.auto_roll ? std::time(nullptr) : 0;

			for (auto member : GetGroupClients(group)) {
				if (!member || !corpse->CanPlayerLoot(member->CharacterID())) {
					continue;
				}

				VoteChoice vote = VoteChoice::Unset;
				const auto member_settings = GetCharacterSettings(member->CharacterID(), true);
				if (!dynamic_instance && member_settings.apply_filters) {
					const auto member_filter = GetFilter(member->CharacterID(), item_data->item_id);
					if (member_filter.decision == LootFilterDecision::AlwaysNeed) {
						vote = VoteChoice::Need;
					}
					else if (member_filter.decision == LootFilterDecision::AlwaysGreed) {
						vote = VoteChoice::Greed;
					}
					else if (member_filter.decision == LootFilterDecision::Never) {
						vote = VoteChoice::Pass;
					}
				}

				entry.votes[member->CharacterID()] = vote;
			}
		}

		m_loot_entries[entry.entry_id] = entry;
		queued = true;
		DebugMessage(
			autoloot_client,
			settings,
			fmt::format(
				"{}{} from {} added to Advanced Loot for {} (entry {}, rule: {}, state: {}).",
				entry.item_name,
				QuantitySuffix(entry.quantity),
				entry.corpse_name,
				recipient->GetCleanName(),
				entry.entry_id,
				entry.rule,
				entry.state
			)
		);

		if (!entry.shared && settings.auto_loot_all) {
			auto_loot_entries.emplace_back(recipient, entry.entry_id);
		}
		else if (entry.shared && entry.auto_roll) {
			auto_roll_entries.push_back(entry.entry_id);
		}
	}

	for (const auto &[recipient, entry_id] : auto_loot_entries) {
		if (recipient) {
			LootEntryForClient(recipient, entry_id);
		}
	}

	for (const auto entry_id : auto_roll_entries) {
		ResolveSharedVote(entry_id, false);
	}

	corpse = entity_list.GetCorpseByID(corpse_id);

	if (queued) {
		if (corpse) {
			LootCoin(corpse, shared_loot && master_looter ? master_looter : autoloot_client);
			corpse->ResetDecayTimer();
		}

		if (settings.log_enabled) {
			Audit(
				autoloot_client->CharacterID(),
				"kill_queue",
				0,
				0,
				corpse_name
			);
		}
	}

	return queued;
}

bool AutoLootManager::HasQueuedEntry(uint16 corpse_id, uint16 loot_slot) const
{
	for (const auto &[entry_id, entry] : m_loot_entries) {
		if (entry.corpse_id == corpse_id && entry.loot_slot == loot_slot) {
			return true;
		}
	}

	return false;
}

bool AutoLootManager::IsManualLootLocked(Corpse *corpse, uint16 loot_slot, std::string *reason) const
{
	if (!AutoLootEnabled() || !corpse || !corpse->IsNPCCorpse()) {
		return false;
	}

	auto item_data = corpse->GetItem(loot_slot);
	if (!item_data || !item_data->item_id) {
		return false;
	}

	const auto corpse_id = corpse->GetID();
	for (const auto &[entry_id, entry] : m_loot_entries) {
		if (!entry.shared || entry.corpse_id != corpse_id || entry.loot_slot != item_data->lootslot || entry.item_id != item_data->item_id) {
			continue;
		}

		if (reason) {
			if (entry.state == "rolling" || !entry.votes.empty()) {
				*reason = fmt::format("{} is locked by Advanced Loot while a roll is active.", entry.item_name);
			}
			else {
				*reason = fmt::format("{} is reserved by Advanced Loot.", entry.item_name);
			}
		}

		return true;
	}

	return false;
}

bool AutoLootManager::IsEntryVisibleToClient(const LootEntry &entry, Client *client) const
{
	if (!client) {
		return false;
	}

	if (!entry.shared) {
		return entry.owner_character_id == client->CharacterID();
	}

	auto group = client->GetGroup();
	return group && group->GetID() == entry.group_id;
}

void AutoLootManager::PruneLootEntries()
{
	for (auto iter = m_loot_entries.begin(); iter != m_loot_entries.end();) {
		auto corpse = entity_list.GetCorpseByID(iter->second.corpse_id);
		auto item_data = corpse ? corpse->GetItem(iter->second.loot_slot) : nullptr;
		if (!corpse || !item_data || item_data->item_id != iter->second.item_id) {
			iter = m_loot_entries.erase(iter);
		}
		else {
			++iter;
		}
	}
}

void AutoLootManager::SendNativeSnapshot(Client *client)
{
	if (!AutoLootEnabled()) {
		return;
	}

	if (!client) {
		return;
	}

	PruneLootEntries();

	client->Message(Chat::White, "ADVLOOT|snapshot|begin");
	for (const auto &[entry_id, entry] : m_loot_entries) {
		if (!IsEntryVisibleToClient(entry, client)) {
			continue;
		}

		auto corpse = entity_list.GetCorpseByID(entry.corpse_id);
		const bool locked = corpse && (corpse->IsLocked() || corpse->IsBeingLooted() || !corpse->CanPlayerLoot(client->CharacterID()));
		auto state = locked ? std::string("locked") : entry.state;
		if (!locked && entry.shared) {
			auto vote_iter = entry.votes.find(client->CharacterID());
			if (vote_iter != entry.votes.end()) {
				state = VoteChoiceState(vote_iter->second);
			}
		}
		client->Message(
			Chat::White,
			fmt::format(
				"ADVLOOT|entry|scope={}|id={}|corpse_id={}|slot={}|item_id={}|icon={}|name={}|qty={}|source={}|state={}|rule={}|locked={}|nodrop={}|master={}|autoroll={}|freegrab={}",
				entry.shared ? "shared" : "personal",
				entry.entry_id,
				entry.corpse_id,
				entry.loot_slot,
				entry.item_id,
				entry.icon_id,
				ProtocolValue(entry.item_name),
				std::max<uint32>(1, entry.quantity),
				ProtocolValue(entry.corpse_name),
				state,
				ProtocolValue(entry.rule),
				locked ? 1 : 0,
				entry.no_drop ? 1 : 0,
				entry.master_looter_character_id,
				entry.auto_roll ? 1 : 0,
				entry.free_grab ? 1 : 0
			).c_str()
		);
	}
	client->Message(Chat::White, "ADVLOOT|snapshot|end");
}

void AutoLootManager::SendNativeUpdate(Client *client)
{
	if (!AutoLootEnabled()) {
		return;
	}

	SendNativeStatus(client);
	SendNativeSnapshot(client);
}

void AutoLootManager::RefreshQueuedRulesForClient(Client *client)
{
	if (!AutoLootEnabled()) {
		return;
	}

	if (!client) {
		return;
	}

	const auto settings = GetCharacterSettings(client->CharacterID(), true);
	for (auto &[entry_id, entry] : m_loot_entries) {
		if (!IsEntryVisibleToClient(entry, client)) {
			continue;
		}

		if (entry.dynamic_instance) {
			entry.rule = "unset";
			continue;
		}

		const auto filter = GetFilter(client->CharacterID(), entry.item_id);
		entry.rule = FilterDecisionKey(settings.apply_filters ? filter.decision : LootFilterDecision::Unset);
	}
}

void AutoLootManager::SendNativeFilterUpdate(Client *client)
{
	if (!AutoLootEnabled()) {
		return;
	}

	RefreshQueuedRulesForClient(client);
	SendNativeUpdate(client);
	SendNativeFilters(client);
}

void AutoLootManager::HandleLootAction(Client *client, const Seperator *sep)
{
	if (!RequireAutoLootEnabled(client)) {
		return;
	}

	if (!client || !sep || sep->argnum < 3 || !sep->IsNumber(2)) {
		client->Message(Chat::White, "Usage: #advloot action [Entry ID] [loot|leave|never|need|greed|no|alwaysneed|alwaysgreed|ask|roll|freegrab|give]");
		return;
	}

	const uint32 entry_id = Strings::ToUnsignedInt(sep->arg[2]);
	const std::string action = Strings::ToLower(sep->arg[3]);
	auto iter = m_loot_entries.find(entry_id);
	if (iter == m_loot_entries.end() || !IsEntryVisibleToClient(iter->second, client)) {
		client->Message(Chat::Red, "That Advanced Loot entry is no longer available.");
		SendNativeUpdate(client);
		return;
	}

	if (iter->second.shared) {
		HandleSharedLootAction(client, entry_id, action, sep);
		return;
	}

	if (action == "loot" || action == "alwaysloot") {
		if (action == "alwaysloot") {
			if (iter != m_loot_entries.end() && IsEntryVisibleToClient(iter->second, client) && !iter->second.dynamic_instance) {
				SetFilter(client->CharacterID(), iter->second.item_id, LootFilterDecision::AlwaysNeed, GetFilter(client->CharacterID(), iter->second.item_id).auto_ask_roll);
			}
		}

		LootEntryForClient(client, entry_id);
		SendNativeFilterUpdate(client);
		return;
	}

	if (action == "leave" || action == "pass" || action == "no") {
		LeaveEntryForClient(client, entry_id, false);
		SendNativeUpdate(client);
		return;
	}

	if (action == "never") {
		if (!iter->second.dynamic_instance) {
			SetFilter(client->CharacterID(), iter->second.item_id, LootFilterDecision::Never, false);
		}
		LeaveEntryForClient(client, entry_id, false);
		SendNativeFilterUpdate(client);
		return;
	}

	if (action == "need" || action == "greed" || action == "alwaysneed" || action == "alwaysgreed") {
		if ((action == "alwaysneed" || action == "alwaysgreed") && !iter->second.dynamic_instance) {
			const auto existing = GetFilter(client->CharacterID(), iter->second.item_id);
			SetFilter(
				client->CharacterID(),
				iter->second.item_id,
				(action == "alwaysneed") ? LootFilterDecision::AlwaysNeed : LootFilterDecision::AlwaysGreed,
				existing.auto_ask_roll
			);
		}

		iter->second.state = action;
		client->Message(Chat::White, fmt::format("Advanced Loot marked {} as {}.", iter->second.item_name, action).c_str());
		SendNativeFilterUpdate(client);
		return;
	}

	client->Message(Chat::White, "Usage: #advloot action [Entry ID] [loot|leave|never|need|greed|no|alwaysneed|alwaysgreed|ask|roll|freegrab|give]");
}

void AutoLootManager::HandleSharedLootAction(Client *client, uint32 entry_id, const std::string &action, const Seperator *sep)
{
	auto iter = m_loot_entries.find(entry_id);
	if (iter == m_loot_entries.end() || !client || !IsEntryVisibleToClient(iter->second, client)) {
		if (client) {
			client->Message(Chat::Red, "That Advanced Loot entry is no longer available.");
		}
		return;
	}

	auto &entry = iter->second;
	auto corpse = entity_list.GetCorpseByID(entry.corpse_id);
	auto eligible_clients = GetEligibleSharedLootClients(entry, corpse);
	const bool master = entry.master_looter_character_id == client->CharacterID() || client->Admin() >= AccountStatus::GMAdmin;

	if (action == "need" || action == "greed" || action == "no" || action == "pass" || action == "alwaysneed" || action == "alwaysgreed" || action == "never") {
		bool filter_changed = false;
		if (!entry.dynamic_instance && (action == "alwaysneed" || action == "alwaysgreed" || action == "never")) {
			const auto existing = GetFilter(client->CharacterID(), entry.item_id);
			const auto decision = action == "alwaysneed" ? LootFilterDecision::AlwaysNeed :
				action == "alwaysgreed" ? LootFilterDecision::AlwaysGreed :
				LootFilterDecision::Never;
			SetFilter(client->CharacterID(), entry.item_id, decision, existing.auto_ask_roll);
			filter_changed = true;
		}

		const auto choice = (action == "need" || action == "alwaysneed") ? VoteChoice::Need :
			(action == "greed" || action == "alwaysgreed") ? VoteChoice::Greed :
			VoteChoice::Pass;
		RecordSharedVote(client, entry_id, choice, filter_changed);
		return;
	}

	if (!master) {
		client->Message(Chat::Red, "Only the Master Looter can manage that shared loot item.");
		SendNativeUpdate(client);
		return;
	}

	if (action == "ask") {
		entry.state = "ask";
		entry.vote_started_at = std::time(nullptr);
		if (corpse) {
			corpse->Lock();
		}
		for (auto member : eligible_clients) {
			if (member) {
				member->Message(Chat::Yellow, fmt::format("{} is asking Need/Greed for {}.", client->GetCleanName(), entry.item_name).c_str());
			}
		}
		SendSharedLootUpdate(eligible_clients);
		return;
	}

	if (action == "roll") {
		for (auto &[character_id, vote] : entry.votes) {
			if (vote == VoteChoice::Unset) {
				vote = VoteChoice::Pass;
			}
		}
		ResolveSharedVote(entry_id, false);
		return;
	}

	if (action == "freegrab") {
		entry.free_grab = true;
		entry.state = "freegrab";
		entry.vote_started_at = 0;
		SendSharedLootUpdate(eligible_clients);
		return;
	}

	if (action == "loot") {
		if (!entry.free_grab) {
			client->Message(Chat::Red, "That shared loot item is not Free Grab.");
			SendNativeUpdate(client);
			return;
		}

		entry.shared = false;
		entry.owner_character_id = client->CharacterID();
		entry.group_id = 0;
		entry.master_looter_character_id = 0;
		entry.state = "waiting";
		entry.votes.clear();
		entry.free_grab = false;
		SendSharedLootUpdate(eligible_clients);
		SendNativeUpdate(client);
		return;
	}

	if (action == "give") {
		if (!sep || sep->argnum < 4) {
			client->Message(Chat::White, "Usage: #advloot action [Entry ID] give [Character Name]");
			return;
		}

		auto recipient = entity_list.GetClientByName(sep->arg[4]);
		if (!recipient || !recipient->GetGroup() || recipient->GetGroup()->GetID() != entry.group_id) {
			client->Message(Chat::Red, "That player is not in this group and zone.");
			return;
		}

		entry.shared = false;
		entry.owner_character_id = recipient->CharacterID();
		entry.group_id = 0;
		entry.master_looter_character_id = 0;
		entry.state = "waiting";
		entry.votes.clear();
		entry.free_grab = false;
		for (auto member : eligible_clients) {
			if (member) {
				member->Message(Chat::Yellow, fmt::format("{} assigned {} to {}.", client->GetCleanName(), entry.item_name, recipient->GetCleanName()).c_str());
			}
		}
		SendSharedLootUpdate(eligible_clients);
		SendNativeUpdate(recipient);
		return;
	}

	if (action == "leave") {
		LeaveEntryForClient(client, entry_id, false);
		SendSharedLootUpdate(eligible_clients);
		return;
	}

	client->Message(Chat::White, "Usage: #advloot action [Entry ID] [ask|roll|freegrab|give|leave]");
}

void AutoLootManager::HandlePersonalLootCommand(Client *client, const Seperator *sep)
{
	if (!RequireAutoLootEnabled(client)) {
		return;
	}

	if (!client || !sep || sep->argnum < 2) {
		client->Message(Chat::White, "Usage: #advloot personal [lootall|leaveall]");
		return;
	}

	PruneLootEntries();

	std::vector<uint32> entry_ids;
	for (const auto &[entry_id, entry] : m_loot_entries) {
		if (!entry.shared && IsEntryVisibleToClient(entry, client)) {
			entry_ids.push_back(entry_id);
		}
	}

	if (!strcasecmp(sep->arg[2], "lootall")) {
		for (const auto entry_id : entry_ids) {
			LootEntryForClient(client, entry_id);
		}
		SendNativeUpdate(client);
		return;
	}

	if (!strcasecmp(sep->arg[2], "leaveall")) {
		for (const auto entry_id : entry_ids) {
			LeaveEntryForClient(client, entry_id, false);
		}
		SendNativeUpdate(client);
		return;
	}

	client->Message(Chat::White, "Usage: #advloot personal [lootall|leaveall]");
}

void AutoLootManager::InspectEntryForClient(Client *client, uint32 entry_id)
{
	auto iter = m_loot_entries.find(entry_id);
	if (iter == m_loot_entries.end() || !IsEntryVisibleToClient(iter->second, client)) {
		client->Message(Chat::Red, "That Advanced Loot entry is no longer available.");
		SendNativeUpdate(client);
		return;
	}

	const auto entry = iter->second;
	auto corpse = entity_list.GetCorpseByID(entry.corpse_id);
	if (!corpse || !corpse->IsNPCCorpse()) {
		m_loot_entries.erase(iter);
		client->Message(Chat::Red, "That corpse is no longer available.");
		SendNativeUpdate(client);
		return;
	}

	auto item_data = corpse->GetItem(entry.loot_slot);
	if (!item_data || item_data->item_id != entry.item_id) {
		m_loot_entries.erase(iter);
		client->Message(Chat::Red, "That item is no longer on the corpse.");
		SendNativeUpdate(client);
		return;
	}

	auto *inst = CreateCorpseLootItemInstance(client, item_data);
	if (!inst) {
		client->Message(Chat::Red, "That item could not be inspected.");
		return;
	}

	client->SendItemPacket(0, inst, ItemPacketViewLink);
	safe_delete(inst);
}

std::vector<Client *> AutoLootManager::GetEligibleSharedLootClients(const LootEntry &entry, Corpse *corpse)
{
	std::vector<Client *> clients;
	if (!entry.shared || !entry.group_id || !corpse) {
		return clients;
	}

	Group *group = nullptr;
	for (auto &[client_id, candidate] : entity_list.GetClientList()) {
		if (!candidate || !candidate->GetGroup() || candidate->GetGroup()->GetID() != entry.group_id) {
			continue;
		}

		group = candidate->GetGroup();
		break;
	}

	if (!group) {
		return clients;
	}

	clients = GetGroupClients(group);
	clients.erase(
		std::remove_if(
			clients.begin(),
			clients.end(),
			[corpse](Client *client) { return !client || !corpse->CanPlayerLoot(client->CharacterID()); }
		),
		clients.end()
	);

	return clients;
}

void AutoLootManager::SendSharedLootUpdate(const std::vector<Client *> &clients)
{
	for (auto client : clients) {
		if (client) {
			SendNativeUpdate(client);
		}
	}
}

void AutoLootManager::RecordSharedVote(Client *client, uint32 entry_id, VoteChoice choice, bool set_always_rule)
{
	auto iter = m_loot_entries.find(entry_id);
	if (iter == m_loot_entries.end() || !IsEntryVisibleToClient(iter->second, client)) {
		client->Message(Chat::Red, "That Advanced Loot entry is no longer available.");
		SendNativeUpdate(client);
		return;
	}

	auto &entry = iter->second;
	if (!entry.shared) {
		entry.state = VoteChoiceState(choice);
		client->Message(Chat::White, fmt::format("Advanced Loot marked {} as {}.", entry.item_name, VoteChoiceState(choice)).c_str());
		SendNativeUpdate(client);
		return;
	}

	auto corpse = entity_list.GetCorpseByID(entry.corpse_id);
	if (!corpse || !corpse->IsNPCCorpse()) {
		m_loot_entries.erase(iter);
		client->Message(Chat::Red, "That corpse is no longer available.");
		SendNativeUpdate(client);
		return;
	}

	auto eligible_clients = GetEligibleSharedLootClients(entry, corpse);
	const bool client_is_eligible = std::any_of(
		eligible_clients.begin(),
		eligible_clients.end(),
		[client](Client *candidate) { return candidate && candidate->CharacterID() == client->CharacterID(); }
	);

	if (!client_is_eligible) {
		client->Message(Chat::Red, "You are not eligible to vote on that Advanced Loot item.");
		SendNativeUpdate(client);
		return;
	}

	std::map<uint32, VoteChoice> refreshed_votes;
	for (auto eligible : eligible_clients) {
		auto vote_iter = entry.votes.find(eligible->CharacterID());
		refreshed_votes[eligible->CharacterID()] = vote_iter != entry.votes.end() ? vote_iter->second : VoteChoice::Unset;
	}

	entry.votes.swap(refreshed_votes);
	entry.votes[client->CharacterID()] = choice;
	entry.vote_started_at = entry.vote_started_at > 0 ? entry.vote_started_at : std::time(nullptr);
	entry.state = "ask";
	if (set_always_rule) {
		entry.rule = FilterDecisionKey(GetFilter(client->CharacterID(), entry.item_id).decision);
	}

	const auto choice_label = VoteChoiceLabel(choice);
	const auto message = fmt::format("{} voted {} on {}.", client->GetCleanName(), choice_label, entry.item_name);
	for (auto eligible : eligible_clients) {
		if (eligible) {
			eligible->Message(Chat::Yellow, message.c_str());
		}
	}

	const bool complete = std::all_of(
		entry.votes.begin(),
		entry.votes.end(),
		[](const auto &vote) { return vote.second != VoteChoice::Unset; }
	);

	if (complete) {
		ResolveSharedVote(entry_id, false);
		return;
	}

	SendSharedLootUpdate(eligible_clients);
}

void AutoLootManager::ResolveSharedVote(uint32 entry_id, bool timeout)
{
	auto iter = m_loot_entries.find(entry_id);
	if (iter == m_loot_entries.end()) {
		return;
	}

	auto entry = iter->second;
	if (!entry.shared || entry.votes.empty()) {
		return;
	}

	auto corpse = entity_list.GetCorpseByID(entry.corpse_id);
	if (!corpse || !corpse->IsNPCCorpse()) {
		m_loot_entries.erase(iter);
		return;
	}

	auto eligible_clients = GetEligibleSharedLootClients(entry, corpse);
	if (eligible_clients.empty()) {
		m_loot_entries.erase(iter);
		corpse->ResetDecayTimer();
		return;
	}

	if (!timeout) {
		for (auto client : eligible_clients) {
			auto vote_iter = entry.votes.find(client->CharacterID());
			if (vote_iter == entry.votes.end() || vote_iter->second == VoteChoice::Unset) {
				return;
			}
		}
	}

	std::vector<uint32> need;
	std::vector<uint32> greed;
	for (auto client : eligible_clients) {
		auto vote_iter = entry.votes.find(client->CharacterID());
		if (vote_iter == entry.votes.end()) {
			continue;
		}

		if (vote_iter->second == VoteChoice::Need) {
			need.push_back(client->CharacterID());
		}
		else if (vote_iter->second == VoteChoice::Greed) {
			greed.push_back(client->CharacterID());
		}
	}

	std::vector<uint32> pool = !need.empty() ? need : greed;
	if (pool.empty()) {
		const auto message = fmt::format("Need/Greed roll for {} ended with no winner; it remains on {}.", entry.item_name, entry.corpse_name);
		for (auto client : eligible_clients) {
			if (client) {
				client->Message(Chat::Yellow, message.c_str());
			}
		}

		m_loot_entries.erase(iter);
		corpse->ResetDecayTimer();
		SendSharedLootUpdate(eligible_clients);
		return;
	}

	const uint32 winner_character_id = pool[zone ? zone->random.Int(0, static_cast<int>(pool.size() - 1)) : 0];
	auto winner = entity_list.GetClientByCharID(winner_character_id);
	if (!winner || !corpse->CanPlayerLoot(winner->CharacterID())) {
		const auto message = fmt::format("Need/Greed winner for {} is no longer eligible; it remains on {}.", entry.item_name, entry.corpse_name);
		for (auto client : eligible_clients) {
			if (client) {
				client->Message(Chat::Yellow, message.c_str());
			}
		}

		iter->second.state = "waiting";
		iter->second.votes.clear();
		iter->second.vote_started_at = 0;
		corpse->ResetDecayTimer();
		SendSharedLootUpdate(eligible_clients);
		return;
	}

	const auto winning_choice = std::find(need.begin(), need.end(), winner_character_id) != need.end() ? "Need" : "Greed";
	const auto message = fmt::format(
		"{} won {} with {}{}.",
		winner->GetCleanName(),
		entry.item_name,
		winning_choice,
		timeout ? " after timeout" : ""
	);
	for (auto client : eligible_clients) {
		if (client) {
			client->Message(Chat::Yellow, message.c_str());
		}
	}

	auto update_iter = m_loot_entries.find(entry_id);
	if (update_iter != m_loot_entries.end()) {
		update_iter->second.shared = false;
		update_iter->second.owner_character_id = winner->CharacterID();
		update_iter->second.master_looter_character_id = 0;
		update_iter->second.group_id = 0;
		update_iter->second.state = "waiting";
		update_iter->second.votes.clear();
		update_iter->second.vote_started_at = 0;
		update_iter->second.auto_roll = false;
		update_iter->second.free_grab = false;
	}

	SendSharedLootUpdate(eligible_clients);
	SendNativeUpdate(winner);
}

void AutoLootManager::LootEntryForClient(Client *client, uint32 entry_id)
{
	auto iter = m_loot_entries.find(entry_id);
	if (iter == m_loot_entries.end() || !IsEntryVisibleToClient(iter->second, client)) {
		client->Message(Chat::Red, "That Advanced Loot entry is no longer available.");
		return;
	}

	const auto entry = iter->second;
	auto corpse = entity_list.GetCorpseByID(entry.corpse_id);
	if (!corpse || !corpse->IsNPCCorpse()) {
		m_loot_entries.erase(iter);
		client->Message(Chat::Red, "That corpse is no longer available.");
		return;
	}

	auto item_data = corpse->GetItem(entry.loot_slot);
	if (!item_data || item_data->item_id != entry.item_id) {
		m_loot_entries.erase(iter);
		client->Message(Chat::Red, "That item is no longer on the corpse.");
		return;
	}

	Client *recipient = client;
	if (!entry.shared && entry.owner_character_id != client->CharacterID()) {
		recipient = entity_list.GetClientByCharID(entry.owner_character_id);
	}

	if (!recipient || !corpse->CanPlayerLoot(recipient->CharacterID())) {
		iter->second.state = "denied";
		client->Message(Chat::Red, "You cannot loot that corpse.");
		DebugMessage(client, GetCharacterSettings(client->CharacterID()), fmt::format("{}{} from {} could not be looted because corpse access was denied.", entry.item_name, QuantitySuffix(entry.quantity), entry.corpse_name));
		return;
	}

	const auto recipient_settings = GetCharacterSettings(recipient->CharacterID());
	auto result = corpse->AutoLootItem(recipient, entry.loot_slot, true);

	if (result.IsSuccess()) {
		if (recipient_settings.log_enabled) {
			Audit(recipient->CharacterID(), "queued_loot", result.item_id, result.item_count, corpse->GetCleanName());
		}
		DebugMessage(
			recipient,
			recipient_settings,
			fmt::format("{} looted {}{} from {}.", recipient->GetCleanName(), entry.item_name, QuantitySuffix(result.item_count), entry.corpse_name)
		);
		m_loot_entries.erase(entry_id);
		FinalizeCorpse(corpse, recipient);
		return;
	}

	auto update_iter = m_loot_entries.find(entry_id);
	if (update_iter != m_loot_entries.end()) {
		if (result.code == CorpseAutoLootResultCode::PartialStacked && result.remaining_count > 0) {
			update_iter->second.quantity = result.remaining_count;
			update_iter->second.state = "inventory_full";
			DebugMessage(
				recipient,
				recipient_settings,
				fmt::format("{} partially looted {} from {}; {} remain because inventory filled.", recipient->GetCleanName(), entry.item_name, entry.corpse_name, result.remaining_count)
			);
		}
		else {
			update_iter->second.state = result.code == CorpseAutoLootResultCode::InventoryFull ? "inventory_full" : "failed";
			DebugMessage(
				recipient,
				recipient_settings,
				fmt::format(
					"{}{} from {} was not looted (result {}, remaining {}).",
					entry.item_name,
					QuantitySuffix(entry.quantity),
					entry.corpse_name,
					static_cast<int>(result.code),
					result.remaining_count
				)
			);
		}
	}
}

bool AutoLootManager::LeaveEntryForClient(Client *client, uint32 entry_id, bool add_never_filter)
{
	auto iter = m_loot_entries.find(entry_id);
	if (iter == m_loot_entries.end() || !IsEntryVisibleToClient(iter->second, client)) {
		client->Message(Chat::Red, "That Advanced Loot entry is no longer available.");
		return false;
	}

	const auto entry = iter->second;
	bool filter_changed = false;
	if (add_never_filter && !entry.dynamic_instance) {
		SetFilter(client->CharacterID(), entry.item_id, LootFilterDecision::Never, false);
		client->Message(Chat::White, fmt::format("Advanced Loot will never select {} for this character.", entry.item_name).c_str());
		filter_changed = true;
	}
	else if (add_never_filter) {
		client->Message(Chat::White, fmt::format("Advanced Loot left {} on the corpse. Dynamic item filters are not saved by template item.", entry.item_name).c_str());
	}

	if (auto corpse = entity_list.GetCorpseByID(entry.corpse_id)) {
		corpse->ResetDecayTimer();
	}

	m_loot_entries.erase(iter);
	return filter_changed;
}

void AutoLootManager::FinalizeCorpse(Corpse *corpse, Client *coin_client)
{
	if (!corpse) {
		return;
	}

	corpse->UnLock();
	LootCoin(corpse, coin_client);

	if (corpse->IsEmpty()) {
		corpse->Delete();
	}
	else {
		corpse->ResetDecayTimer();
	}
}

void AutoLootManager::LootCoin(Corpse *corpse, Client *client)
{
	if (!corpse || !client || !corpse->CanPlayerLoot(client->CharacterID())) {
		return;
	}

	const auto copper = corpse->GetCopper();
	const auto silver = corpse->GetSilver();
	const auto gold = corpse->GetGold();
	const auto platinum = corpse->GetPlatinum();

	if (!copper && !silver && !gold && !platinum) {
		return;
	}

	client->AddMoneyToPP(copper, silver, gold, platinum, true);
	client->SaveCurrency();
	corpse->RemoveCash();

	client->Message(
		Chat::Loot,
		fmt::format(
			"Advanced Loot looted {} from {}.",
			Strings::Money(platinum, gold, silver, copper),
			corpse->GetCleanName()
		).c_str()
	);
}

void AutoLootManager::HandleAdvancedLootCommand(Client *client, const Seperator *sep)
{
	if (!RequireAutoLootEnabled(client)) {
		return;
	}

	if (!client || !sep) {
		return;
	}

	auto settings = GetCharacterSettings(client->CharacterID(), true);
	const int arguments = sep->argnum;

	if (!arguments || !strcasecmp(sep->arg[1], "status")) {
		SendStatus(client);
		return;
	}

	if (
		!strcasecmp(sep->arg[1], "window") ||
		!strcasecmp(sep->arg[1], "ui") ||
		!strcasecmp(sep->arg[1], "panel")
	) {
		client->Message(Chat::White, "ADVLOOT|window|show");
		SendNativeUpdate(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "native")) {
		if (
			arguments >= 2 &&
			(
				!strcasecmp(sep->arg[2], "show") ||
				!strcasecmp(sep->arg[2], "open") ||
				!strcasecmp(sep->arg[2], "window")
			)
		) {
			client->Message(Chat::White, "ADVLOOT|window|show");
			SendNativeUpdate(client);
			return;
		}

		if (
			arguments < 2 ||
			!strcasecmp(sep->arg[2], "status") ||
			!strcasecmp(sep->arg[2], "snapshot")
		) {
			SendNativeUpdate(client);
			return;
		}

		client->Message(Chat::White, "Usage: #advloot native [show|status|snapshot]");
		return;
	}

	if (!strcasecmp(sep->arg[1], "action")) {
		HandleLootAction(client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "personal")) {
		HandlePersonalLootCommand(client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "inspect") || !strcasecmp(sep->arg[1], "preview")) {
		if (arguments < 2 || !sep->IsNumber(2)) {
			client->Message(Chat::White, "Usage: #advloot inspect [Entry ID]");
			return;
		}

		InspectEntryForClient(client, Strings::ToUnsignedInt(sep->arg[2]));
		return;
	}

	if (!strcasecmp(sep->arg[1], "help")) {
		SendHelp(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "on")) {
		settings.use_advanced_looting = true;
		SaveCharacterSettings(client->CharacterID(), settings);
		client->Message(Chat::White, "Advanced Loot enabled.");
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "off")) {
		settings.use_advanced_looting = false;
		SaveCharacterSettings(client->CharacterID(), settings);
		client->Message(Chat::White, "Advanced Loot disabled.");
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "applyfilters")) {
		if (arguments < 2) {
			client->Message(Chat::White, "Usage: #advloot applyfilters [on|off]");
			return;
		}

		settings.apply_filters = Strings::ToBool(sep->arg[2]);
		SaveCharacterSettings(client->CharacterID(), settings);
		client->Message(Chat::White, fmt::format("Advanced Loot Apply Filters {}.", settings.apply_filters ? "enabled" : "disabled").c_str());
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "debug") || !strcasecmp(sep->arg[1], "verbose") || !strcasecmp(sep->arg[1], "log")) {
		if (arguments < 2) {
			client->Message(Chat::White, "Usage: #advloot debug [on|off], #advloot verbose [on|off], or #advloot log [on|off]");
			return;
		}

		const bool enabled = Strings::ToBool(sep->arg[2]);
		if (!strcasecmp(sep->arg[1], "debug") || !strcasecmp(sep->arg[1], "verbose")) {
			settings.debug_enabled = enabled;
		}
		else {
			settings.log_enabled = enabled;
		}

		SaveCharacterSettings(client->CharacterID(), settings);
		client->Message(
			Chat::White,
			fmt::format(
				"Advanced Loot {} {}.",
				!strcasecmp(sep->arg[1], "log") ? "log" : "debug chat",
				enabled ? "enabled" : "disabled"
			).c_str()
		);
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "autosplit") || !strcasecmp(sep->arg[1], "splitcoin")) {
		if (arguments < 2) {
			client->Message(Chat::White, "Usage: #advloot autosplit [on|off]");
			return;
		}

		settings.auto_split_coin = Strings::ToBool(sep->arg[2]);
		SaveCharacterSettings(client->CharacterID(), settings);
		client->Message(Chat::White, fmt::format("Advanced Loot auto split coin {}.", settings.auto_split_coin ? "enabled" : "disabled").c_str());
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "autolootall") || !strcasecmp(sep->arg[1], "lootallsetting")) {
		if (arguments < 2) {
			client->Message(Chat::White, "Usage: #advloot autolootall [on|off]");
			return;
		}

		settings.auto_loot_all = Strings::ToBool(sep->arg[2]);
		SaveCharacterSettings(client->CharacterID(), settings);
		client->Message(Chat::White, fmt::format("Advanced Loot auto loot all {}.", settings.auto_loot_all ? "enabled" : "disabled").c_str());
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "masterlooter") || !strcasecmp(sep->arg[1], "mlcandidate")) {
		if (arguments < 2) {
			client->Message(Chat::White, "Usage: #advloot masterlooter [on|off]");
			return;
		}

		settings.master_looter_candidate = Strings::ToBool(sep->arg[2]);
		SaveCharacterSettings(client->CharacterID(), settings);
		client->Message(Chat::White, fmt::format("Advanced Loot Master Looter candidate {}.", settings.master_looter_candidate ? "enabled" : "disabled").c_str());
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "filter")) {
		HandleAdvancedLootFilterCommand(client, sep);
		return;
	}

	SendHelp(client);
}

void AutoLootManager::HandleAdvancedLootFilterCommand(Client *client, const Seperator *sep)
{
	if (!RequireAutoLootEnabled(client)) {
		return;
	}

	if (!client || !sep) {
		return;
	}

	int command_index = 1;
	if (sep->argnum >= 1 && !strcasecmp(sep->arg[1], "filter")) {
		command_index = 2;
	}

	auto show_usage = [client]() {
		client->Message(Chat::White, "Usage: #advloot filter list");
		client->Message(Chat::White, "Usage: #advloot filter set [Item ID] [unset|always_need|always_greed|never]");
		client->Message(Chat::White, "Usage: #advloot filter autoroll [Item ID] [on|off]");
		client->Message(Chat::White, "Usage: #advloot filter remove [Item ID]");
	};

	if (command_index > sep->argnum || !sep->arg[command_index][0] || !strcasecmp(sep->arg[command_index], "help")) {
		show_usage();
		return;
	}

	const std::string action = Strings::ToLower(sep->arg[command_index]);

	if (action == "native") {
		if (sep->argnum >= command_index + 1 && !strcasecmp(sep->arg[command_index + 1], "list")) {
			SendNativeStatus(client);
			SendNativeFilters(client);
			return;
		}

		client->Message(Chat::White, "Usage: #advloot filter native list");
		return;
	}

	if (action == "list") {
		const auto filters = GetFilters(client->CharacterID());
		if (filters.empty()) {
			client->Message(Chat::White, "No Advanced Loot filters found.");
			return;
		}

		for (const auto &filter : filters) {
			const auto *item = database.GetItem(filter.item_id);
			client->Message(
				Chat::White,
				fmt::format(
					"{}{}: {} ({})",
					FilterDecisionLabel(filter.decision),
					filter.auto_ask_roll ? " Auto Roll" : "",
					item ? item->Name : "Unknown Item",
					filter.item_id
				).c_str()
			);
		}
		return;
	}

	if (action == "remove" || action == "unset") {
		const int item_index = command_index + 1;
		if (sep->argnum < item_index || !sep->IsNumber(item_index)) {
			client->Message(Chat::White, "Usage: #advloot filter remove [Item ID]");
			return;
		}

		const uint32 item_id = Strings::ToUnsignedInt(sep->arg[item_index]);
		RemoveFilter(client->CharacterID(), item_id);
		client->Message(Chat::White, fmt::format("Removed Advanced Loot filter for item {}.", item_id).c_str());
		SendNativeFilterUpdate(client);
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (action == "autoroll" || action == "autoask" || action == "roll") {
		const int item_index = command_index + 1;
		const int enabled_index = command_index + 2;
		if (sep->argnum < enabled_index || !sep->IsNumber(item_index)) {
			client->Message(Chat::White, "Usage: #advloot filter autoroll [Item ID] [on|off]");
			return;
		}

		const uint32 item_id = Strings::ToUnsignedInt(sep->arg[item_index]);
		const auto *item = database.GetItem(item_id);
		if (!item) {
			client->Message(Chat::Red, "Invalid item ID.");
			return;
		}

		const auto existing = GetFilter(client->CharacterID(), item_id);
		const bool enabled = Strings::ToBool(sep->arg[enabled_index]);
		SetFilter(client->CharacterID(), item_id, existing.decision, enabled);
		client->Message(Chat::White, fmt::format("Advanced Loot Auto Roll {} for {} ({}).", enabled ? "enabled" : "disabled", item->Name, item_id).c_str());
		SendNativeFilterUpdate(client);
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (action == "set" || action == "add" || IsValidFilterDecision(action)) {
		const int item_index = (action == "set" || action == "add") ? command_index + 1 : command_index + 1;
		const int decision_index = (action == "set" || action == "add") ? command_index + 2 : command_index;
		if (sep->argnum < item_index || !sep->IsNumber(item_index)) {
			client->Message(Chat::White, "Usage: #advloot filter set [Item ID] [unset|always_need|always_greed|never]");
			return;
		}

		if (decision_index > sep->argnum || !IsValidFilterDecision(sep->arg[decision_index])) {
			client->Message(Chat::White, "Usage: #advloot filter set [Item ID] [unset|always_need|always_greed|never]");
			return;
		}

		const uint32 item_id = Strings::ToUnsignedInt(sep->arg[item_index]);
		const auto *item = database.GetItem(item_id);
		if (!item) {
			client->Message(Chat::Red, "Invalid item ID.");
			return;
		}

		const auto existing = GetFilter(client->CharacterID(), item_id);
		bool auto_ask_roll = existing.auto_ask_roll;
		for (int argument_index = item_index + 1; argument_index <= sep->argnum; ++argument_index) {
			if (!strcasecmp(sep->arg[argument_index], "autoroll") || !strcasecmp(sep->arg[argument_index], "autoask")) {
				if (argument_index + 1 <= sep->argnum) {
					auto_ask_roll = Strings::ToBool(sep->arg[argument_index + 1]);
				}
			}
		}

		const auto decision = ParseFilterDecision(sep->arg[decision_index]);
		SetFilter(client->CharacterID(), item_id, decision, auto_ask_roll);
		client->Message(
			Chat::White,
			fmt::format(
				"Advanced Loot filter for {} ({}) set to {}{}.",
				item->Name,
				item_id,
				FilterDecisionKey(decision),
				auto_ask_roll ? " with Auto Roll" : ""
			).c_str()
		);
		SendNativeFilterUpdate(client);
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	show_usage();
}

void AutoLootManager::ShowWindow(Client *client)
{
	if (!RequireAutoLootEnabled(client)) {
		return;
	}

	if (!client) {
		return;
	}

	client->Message(Chat::White, "ADVLOOT|window|show");
	SendNativeUpdate(client);
}

void AutoLootManager::SendStatus(Client *client)
{
	const auto settings = GetCharacterSettings(client->CharacterID(), true);
	client->Message(
		Chat::White,
		fmt::format(
			"Advanced Loot: {}, Apply Filters: {}, Auto Split Coin: {}, Auto Loot All: {}, Master Looter candidate: {}, debug: {}, log: {}.",
			settings.use_advanced_looting ? "on" : "off",
			settings.apply_filters ? "on" : "off",
			settings.auto_split_coin ? "on" : "off",
			settings.auto_loot_all ? "on" : "off",
			settings.master_looter_candidate ? "on" : "off",
			settings.debug_enabled ? "on" : "off",
			settings.log_enabled ? "on" : "off"
		).c_str()
	);

	SendNativeUpdate(client);
}

void AutoLootManager::SendNativeStatus(Client *client)
{
	if (!client) {
		return;
	}

	const auto settings = GetCharacterSettings(client->CharacterID(), true);
	size_t always_need_count = 0;
	size_t always_greed_count = 0;
	size_t never_count = 0;
	size_t auto_roll_count = 0;
	for (const auto &filter : GetFilters(client->CharacterID())) {
		if (filter.decision == LootFilterDecision::AlwaysNeed) {
			++always_need_count;
		}
		else if (filter.decision == LootFilterDecision::AlwaysGreed) {
			++always_greed_count;
		}
		else if (filter.decision == LootFilterDecision::Never) {
			++never_count;
		}

		if (filter.auto_ask_roll) {
			++auto_roll_count;
		}
	}

	bool grouped = false;
	bool leader = false;

	auto group = client->GetGroup();
	if (group) {
		grouped = true;
		leader = group->IsLeader(client) || client->Admin() >= AccountStatus::GMAdmin;
	}

	const auto status = fmt::format(
		"ADVLOOT|status|enabled={}|applyfilters={}|alwaysneed={}|alwaysgreed={}|never={}|autoroll={}|grouped={}|leader={}|mastercandidate={}|autosplit={}|autolootall={}|debug={}|log={}",
		settings.use_advanced_looting ? 1 : 0,
		settings.apply_filters ? 1 : 0,
		always_need_count,
		always_greed_count,
		never_count,
		auto_roll_count,
		grouped ? 1 : 0,
		leader ? 1 : 0,
		settings.master_looter_candidate ? 1 : 0,
		settings.auto_split_coin ? 1 : 0,
		settings.auto_loot_all ? 1 : 0,
		settings.debug_enabled ? 1 : 0,
		settings.log_enabled ? 1 : 0
	);

	client->Message(Chat::White, status.c_str());
}

void AutoLootManager::SendNativeFilters(Client *client)
{
	if (!client) {
		return;
	}

	client->Message(Chat::White, "ADVLOOT|filters|begin|mode=all");

	for (const auto &filter : GetFilters(client->CharacterID())) {
		const auto *item = database.GetItem(filter.item_id);
		client->Message(
			Chat::White,
			fmt::format(
				"ADVLOOT|filter|decision={}|autoroll={}|item_id={}|icon={}|name={}",
				ProtocolValue(FilterDecisionKey(filter.decision)),
				filter.auto_ask_roll ? 1 : 0,
				filter.item_id,
				item ? item->Icon : 0,
				ProtocolValue(item ? item->Name : "Unknown Item")
			).c_str()
		);
	}

	client->Message(Chat::White, "ADVLOOT|filters|end|mode=all");
}

void AutoLootManager::SendHelp(Client *client)
{
	client->Message(Chat::White, "Usage: #advloot [status|window|on|off]");
	client->Message(Chat::White, "Usage: #advloot applyfilters [on|off]");
	client->Message(Chat::White, "Usage: #advloot autosplit [on|off]");
	client->Message(Chat::White, "Usage: #advloot autolootall [on|off]");
	client->Message(Chat::White, "Usage: #advloot masterlooter [on|off]");
	client->Message(Chat::White, "Usage: #advloot debug [on|off] or #advloot log [on|off]");
	client->Message(Chat::White, "Usage: #advloot inspect [Entry ID]");
	client->Message(Chat::White, "Usage: #advloot action [Entry ID] [loot|leave|never|need|greed|no|alwaysneed|alwaysgreed|ask|roll|freegrab|give]");
	client->Message(Chat::White, "Usage: #advloot filter [list|set|autoroll|remove]");
}

void AutoLootManager::SendGroupHelp(Client *client)
{
	client->Message(Chat::White, "Advanced Loot group controls are managed through shared loot rows with #advloot action.");
}

void AutoLootManager::Audit(uint32 character_id, const std::string &action, uint32 item_id, uint32 quantity, const std::string &detail)
{
	database.QueryDatabase(
		fmt::format(
			"INSERT INTO `custom_advloot_audit` (`character_id`, `action`, `item_id`, `quantity`, `detail`, `created_at`) "
			"VALUES ({}, '{}', {}, {}, '{}', UNIX_TIMESTAMP())",
			character_id,
			Strings::Escape(action),
			item_id,
			quantity,
			Strings::Escape(detail)
		)
	);
}

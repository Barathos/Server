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

extern EntityList entity_list;
extern Zone *zone;

AutoLootManager auto_loot_manager;

namespace {
	constexpr float kDefaultNearbyRadius = 75.0f;
	constexpr float kMaxNearbyRadius = 250.0f;
	constexpr uint32 kNeedGreedSeconds = 45;
	constexpr uint32 kAutosellSessionSeconds = 60;

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
			client->Message(Chat::White, "AutoLoot is disabled on this server.");
		}

		return false;
	}

	bool IsValidFilterMode(const std::string &mode)
	{
		return Strings::EqualFold(mode, "both") ||
			Strings::EqualFold(mode, "include") ||
			Strings::EqualFold(mode, "exclude");
	}

	std::string NormalizeFilterMode(const std::string &mode)
	{
		const auto normalized = Strings::ToLower(mode);
		return IsValidFilterMode(normalized) ? normalized : "both";
	}

	std::string NormalizeRuleMode(const std::string &mode)
	{
		const auto normalized = Strings::ToLower(mode);
		if (Strings::EqualFold(normalized, "keep") || Strings::EqualFold(normalized, "always")) {
			return "include";
		}

		if (Strings::EqualFold(normalized, "ignore") || Strings::EqualFold(normalized, "never")) {
			return "exclude";
		}

		if (Strings::EqualFold(normalized, "unset")) {
			return "both";
		}

		return NormalizeFilterMode(normalized);
	}

	std::string DisplayRuleMode(const std::string &mode)
	{
		if (Strings::EqualFold(mode, "include")) {
			return "Keep";
		}

		if (Strings::EqualFold(mode, "exclude")) {
			return "Ignore";
		}

		return "Unset";
	}

	bool IsValidGroupMode(const std::string &mode)
	{
		return Strings::EqualFold(mode, "none") ||
			Strings::EqualFold(mode, "solo") ||
			Strings::EqualFold(mode, "master") ||
			Strings::EqualFold(mode, "robin") ||
			Strings::EqualFold(mode, "killer") ||
			Strings::EqualFold(mode, "assigned");
	}

	std::string NormalizeGroupMode(const std::string &mode)
	{
		const auto normalized = Strings::ToLower(mode);
		return IsValidGroupMode(normalized) ? normalized : "solo";
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

	bool IsAutosellBag(uint32 item_id)
	{
		return item_id >= 45500 && item_id <= 45505;
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

	bool IsAutosellProtected(EQ::ItemInstance *inst)
	{
		if (!inst || !inst->GetItem()) {
			return true;
		}

		const auto *item = inst->GetItem();
		return item->NoDrop == 0 ||
			inst->IsAttuned() ||
			inst->IsAugmented() ||
			inst->IsEvolving() ||
			item->Price == 0;
	}

	AutoLootManager::VoteChoice ParseVoteChoice(const std::string &choice)
	{
		if (Strings::EqualFold(choice, "need")) {
			return AutoLootManager::VoteChoice::Need;
		}

		if (Strings::EqualFold(choice, "greed")) {
			return AutoLootManager::VoteChoice::Greed;
		}

		return AutoLootManager::VoteChoice::Pass;
	}
}

void AutoLootManager::Process()
{
	if (!AutoLootEnabled()) {
		m_pending_votes.clear();
		m_autosell_sessions.clear();
		m_loot_entries.clear();
		return;
	}

	const time_t now = std::time(nullptr);
	if (now == m_last_process) {
		return;
	}

	m_last_process = now;

	std::vector<uint32> expired_votes;
	for (const auto &[vote_id, vote] : m_pending_votes) {
		if (vote.expires_at <= now) {
			expired_votes.push_back(vote_id);
		}
	}

	for (const auto vote_id : expired_votes) {
		ProcessVote(vote_id, true);
	}

	for (auto iter = m_autosell_sessions.begin(); iter != m_autosell_sessions.end();) {
		if (iter->second.expires_at <= now) {
			iter = m_autosell_sessions.erase(iter);
		}
		else {
			++iter;
		}
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
			"SELECT `enabled`, `filter_mode`, `debug_enabled`, `log_enabled` "
			"FROM `custom_autoloot_settings` WHERE `character_id` = {} LIMIT 1",
			character_id
		)
	);

	if (!results.Success() || !results.RowCount()) {
		if (results.Success() && create_enabled) {
			settings.enabled = true;
			SaveCharacterSettings(character_id, settings);
		}

		return settings;
	}

	auto row = results.begin();
	settings.enabled       = row[0] ? Strings::ToBool(row[0]) : false;
	settings.filter_mode   = row[1] ? NormalizeFilterMode(row[1]) : "both";
	settings.debug_enabled = row[2] ? Strings::ToBool(row[2]) : false;
	settings.log_enabled   = row[3] ? Strings::ToBool(row[3]) : false;
	return settings;
}

void AutoLootManager::SaveCharacterSettings(uint32 character_id, const CharacterSettings &settings)
{
	database.QueryDatabase(
		fmt::format(
			"INSERT INTO `custom_autoloot_settings` "
			"(`character_id`, `enabled`, `filter_mode`, `debug_enabled`, `log_enabled`, `updated_at`) "
			"VALUES ({}, {}, '{}', {}, {}, UNIX_TIMESTAMP()) "
			"ON DUPLICATE KEY UPDATE "
			"`enabled` = VALUES(`enabled`), `filter_mode` = VALUES(`filter_mode`), "
			"`debug_enabled` = VALUES(`debug_enabled`), `log_enabled` = VALUES(`log_enabled`), "
			"`updated_at` = UNIX_TIMESTAMP()",
			character_id,
			settings.enabled ? 1 : 0,
			Strings::Escape(NormalizeFilterMode(settings.filter_mode)),
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

	client->Message(Chat::Yellow, fmt::format("[AutoLoot Debug] {}", message).c_str());
}

AutoLootManager::GroupSettings AutoLootManager::GetGroupSettings(uint32 group_id)
{
	GroupSettings settings;
	if (!group_id) {
		return settings;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `loot_mode`, `assigned_character_id`, `round_robin_index`, `need_greed_enabled` "
			"FROM `custom_autoloot_group_settings` WHERE `group_id` = {} LIMIT 1",
			group_id
		)
	);

	if (!results.Success() || !results.RowCount()) {
		return settings;
	}

	auto row = results.begin();
	settings.loot_mode             = row[0] ? NormalizeGroupMode(row[0]) : "solo";
	settings.assigned_character_id = row[1] ? Strings::ToUnsignedInt(row[1]) : 0;
	settings.round_robin_index     = row[2] ? Strings::ToUnsignedInt(row[2]) : 0;
	settings.need_greed_enabled    = row[3] ? Strings::ToBool(row[3]) : false;
	return settings;
}

void AutoLootManager::SaveGroupSettings(uint32 group_id, const GroupSettings &settings)
{
	if (!group_id) {
		return;
	}

	database.QueryDatabase(
		fmt::format(
			"INSERT INTO `custom_autoloot_group_settings` "
			"(`group_id`, `loot_mode`, `assigned_character_id`, `round_robin_index`, `need_greed_enabled`, `updated_at`) "
			"VALUES ({}, '{}', {}, {}, {}, UNIX_TIMESTAMP()) "
			"ON DUPLICATE KEY UPDATE "
			"`loot_mode` = VALUES(`loot_mode`), `assigned_character_id` = VALUES(`assigned_character_id`), "
			"`round_robin_index` = VALUES(`round_robin_index`), `need_greed_enabled` = VALUES(`need_greed_enabled`), "
			"`updated_at` = UNIX_TIMESTAMP()",
			group_id,
			Strings::Escape(NormalizeGroupMode(settings.loot_mode)),
			settings.assigned_character_id,
			settings.round_robin_index,
			settings.need_greed_enabled ? 1 : 0
		)
	);
}

bool AutoLootManager::HasFilter(uint32 character_id, uint32 item_id, const std::string &filter_mode)
{
	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT 1 FROM `custom_autoloot_filters` "
			"WHERE `character_id` = {} AND `item_id` = {} AND `filter_mode` = '{}' LIMIT 1",
			character_id,
			item_id,
			Strings::Escape(NormalizeFilterMode(filter_mode))
		)
	);

	return results.Success() && results.RowCount() > 0;
}

bool AutoLootManager::ShouldLootItem(uint32 character_id, uint32 item_id, const std::string &filter_mode)
{
	return GetFilterAction(character_id, item_id, filter_mode) != "skip";
}

std::string AutoLootManager::GetFilterAction(uint32 character_id, uint32 item_id, const std::string &filter_mode)
{
	const auto mode = NormalizeRuleMode(filter_mode);
	const bool is_included = HasFilter(character_id, item_id, "include");
	const bool is_excluded = HasFilter(character_id, item_id, "exclude");

	if (is_excluded) {
		return "skip";
	}

	if (mode == "include") {
		return is_included ? "loot" : "skip";
	}

	if (mode == "exclude") {
		return "loot";
	}

	return is_included ? "loot" : "queue";
}

void AutoLootManager::SetFilter(uint32 character_id, uint32 item_id, const std::string &filter_mode)
{
	const auto mode = NormalizeRuleMode(filter_mode);
	if (mode == "both") {
		return;
	}

	RemoveFilter(character_id, item_id, mode == "include" ? "exclude" : "include");

	database.QueryDatabase(
		fmt::format(
			"INSERT IGNORE INTO `custom_autoloot_filters` (`character_id`, `item_id`, `filter_mode`, `created_at`) "
			"VALUES ({}, {}, '{}', UNIX_TIMESTAMP())",
			character_id,
			item_id,
			Strings::Escape(mode)
		)
	);
}

void AutoLootManager::RemoveFilter(uint32 character_id, uint32 item_id, const std::string &filter_mode)
{
	const auto mode = NormalizeRuleMode(filter_mode);
	if (mode == "both") {
		database.QueryDatabase(
			fmt::format(
				"DELETE FROM `custom_autoloot_filters` WHERE `character_id` = {} AND `item_id` = {}",
				character_id,
				item_id
			)
		);
		return;
	}

	database.QueryDatabase(
		fmt::format(
			"DELETE FROM `custom_autoloot_filters` "
			"WHERE `character_id` = {} AND `item_id` = {} AND `filter_mode` = '{}'",
			character_id,
			item_id,
			Strings::Escape(mode)
		)
	);
}

std::vector<std::pair<uint32, std::string>> AutoLootManager::GetFilters(uint32 character_id, const std::string &filter_mode)
{
	std::vector<std::pair<uint32, std::string>> filters;
	const auto mode = NormalizeRuleMode(filter_mode);

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `item_id`, `filter_mode` FROM `custom_autoloot_filters` "
			"WHERE `character_id` = {}{} ORDER BY `filter_mode`, `item_id`",
			character_id,
			mode == "both" ? "" : fmt::format(" AND `filter_mode` = '{}'", Strings::Escape(mode))
		)
	);

	if (!results.Success()) {
		return filters;
	}

	for (auto row = results.begin(); row != results.end(); ++row) {
		filters.emplace_back(row[0] ? Strings::ToUnsignedInt(row[0]) : 0, row[1] ? row[1] : "");
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

	auto settings = GetCharacterSettings(resolved_client->CharacterID());
	if (settings.enabled) {
		return resolved_client;
	}

	auto group = resolved_client->GetGroup();
	if (!group) {
		return nullptr;
	}

	for (auto member : GetGroupClients(group)) {
		if (!corpse->CanPlayerLoot(member->CharacterID())) {
			continue;
		}

		settings = GetCharacterSettings(member->CharacterID());
		if (settings.enabled) {
			return member;
		}
	}

	return nullptr;
}

Client *AutoLootManager::DetermineRecipient(Client *resolved_client, Corpse *corpse, const GroupSettings &settings)
{
	if (!resolved_client || !corpse) {
		return nullptr;
	}

	auto group = resolved_client->GetGroup();
	if (!group || group->GroupCount() <= 1) {
		return resolved_client;
	}

	const auto mode = NormalizeGroupMode(settings.loot_mode);
	if (mode == "none") {
		return nullptr;
	}

	if (mode == "solo" || mode == "killer") {
		return resolved_client;
	}

	if (mode == "master") {
		auto leader = group->GetLeader();
		if (leader && leader->IsClient() && corpse->CanPlayerLoot(leader->CastToClient()->CharacterID())) {
			return leader->CastToClient();
		}
		return resolved_client;
	}

	if (mode == "assigned") {
		for (auto member : GetGroupClients(group)) {
			if (member->CharacterID() == settings.assigned_character_id && corpse->CanPlayerLoot(member->CharacterID())) {
				return member;
			}
		}
		return resolved_client;
	}

	if (mode == "robin") {
		auto clients = GetGroupClients(group);
		clients.erase(
			std::remove_if(
				clients.begin(),
				clients.end(),
				[corpse](Client *member) { return !corpse->CanPlayerLoot(member->CharacterID()); }
			),
			clients.end()
		);

		if (clients.empty()) {
			return resolved_client;
		}

		auto updated_settings = settings;
		const uint32 index = updated_settings.round_robin_index % clients.size();
		updated_settings.round_robin_index++;
		SaveGroupSettings(group->GetID(), updated_settings);
		return clients[index];
	}

	return resolved_client;
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

	ProcessCorpse(corpse, client, false);
}

void AutoLootManager::ProcessNearby(Client *client, float radius)
{
	if (!RequireAutoLootEnabled(client)) {
		return;
	}

	if (!client) {
		return;
	}

	if (radius <= 0.0f) {
		radius = kDefaultNearbyRadius;
	}

	if (radius > kMaxNearbyRadius) {
		radius = kMaxNearbyRadius;
	}

	uint32 scanned = 0;
	uint32 queued = 0;
	for (auto &[corpse_id, corpse] : entity_list.GetCorpseList()) {
		if (!corpse || !corpse->IsNPCCorpse() || corpse->IsLocked() || corpse->IsBeingLooted()) {
			continue;
		}

		if (corpse->CalculateDistance(client) > radius) {
			continue;
		}

		if (!corpse->CanPlayerLoot(client->CharacterID())) {
			continue;
		}

		scanned++;
		if (ProcessCorpse(corpse, client, true)) {
			queued++;
		}
	}

	SendNativeUpdate(client);
	client->Message(
		Chat::White,
		fmt::format(
			"AutoLoot nearby scanned {} corpse{} and queued loot from {}.",
			scanned,
			scanned == 1 ? "" : "s",
			queued
		).c_str()
	);
}

bool AutoLootManager::ProcessCorpse(Corpse *corpse, Client *resolved_client, bool nearby)
{
	if (!AutoLootEnabled()) {
		return false;
	}

	if (!corpse || !resolved_client || !corpse->IsNPCCorpse() || corpse->IsBeingLooted()) {
		return false;
	}

	if (QueueCorpseEntries(corpse, resolved_client, nearby)) {
		auto autoloot_client = FindAutoLootClient(resolved_client, corpse);
		if (autoloot_client) {
			SendNativeUpdate(autoloot_client);
		}

		return true;
	}

	return false;
}

bool AutoLootManager::QueueCorpseEntries(Corpse *corpse, Client *resolved_client, bool nearby)
{
	if (!AutoLootEnabled()) {
		return false;
	}

	if (!corpse || !resolved_client || !corpse->IsNPCCorpse() || corpse->IsBeingLooted()) {
		return false;
	}

	if (corpse->IsLocked() && !HasPendingVotes(corpse->GetID())) {
		return false;
	}

	auto autoloot_client = FindAutoLootClient(resolved_client, corpse);
	if (!autoloot_client) {
		return false;
	}

	auto settings = GetCharacterSettings(autoloot_client->CharacterID());
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
			const auto *item = database.GetItem(item_data->item_id);
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
			DebugMessage(autoloot_client, settings, fmt::format("{} had no top-level loot for AutoLoot.", corpse->GetCleanName()));
		}
	};

	if (!settings.enabled) {
		send_drop_debug();
		DebugMessage(autoloot_client, settings, fmt::format("{} was not queued because AutoLoot is off for this character.", corpse->GetCleanName()));
		return false;
	}

	GroupSettings group_settings;
	auto group = autoloot_client->GetGroup();
	if (group) {
		group_settings = GetGroupSettings(group->GetID());
		if (NormalizeGroupMode(group_settings.loot_mode) == "none") {
			send_drop_debug();
			DebugMessage(autoloot_client, settings, fmt::format("{} was not queued because group AutoLoot mode is none.", corpse->GetCleanName()));
			return false;
		}
	}

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
	for (const auto loot_slot : loot_slots) {
		auto item_data = corpse->GetItem(loot_slot);
		if (!item_data || !item_data->item_id) {
			continue;
		}

		const auto *item = database.GetItem(item_data->item_id);
		const auto item_name = item ? item->Name : fmt::format("Unknown Item {}", item_data->item_id);
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

		const auto filter_action = GetFilterAction(autoloot_client->CharacterID(), item_data->item_id, settings.filter_mode);
		if (filter_action == "skip") {
			DebugMessage(
				autoloot_client,
				settings,
				fmt::format(
					"{}{} from {} ignored by AutoLoot filter (mode: {}).",
					item_name,
					QuantitySuffix(quantity),
					corpse_name,
					settings.filter_mode
				)
			);
			continue;
		}

		auto recipient = DetermineRecipient(autoloot_client, corpse, group_settings);
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
		entry.group_id = group ? group->GetID() : 0;
		entry.shared = group && group->GroupCount() > 1 && NormalizeGroupMode(group_settings.loot_mode) != "solo";
		entry.no_drop = IsNoDrop(item_data->item_id);
		entry.item_name = item->Name;
		entry.corpse_name = corpse->GetCleanName();
		entry.state = entry.shared && group_settings.need_greed_enabled && entry.no_drop ? "rolling" : "waiting";
		entry.rule = filter_action == "loot" ? "auto" : "ask";
		if (filter_action == "loot" && HasFilter(autoloot_client->CharacterID(), item_data->item_id, "include")) {
			entry.rule = "always";
		}
		entry.created_at = std::time(nullptr);

		m_loot_entries[entry.entry_id] = entry;
		queued = true;
		DebugMessage(
			autoloot_client,
			settings,
			fmt::format(
				"{}{} from {} added to AutoLoot for {} (entry {}, rule: {}, state: {}).",
				entry.item_name,
				QuantitySuffix(entry.quantity),
				entry.corpse_name,
				recipient->GetCleanName(),
				entry.entry_id,
				entry.rule,
				entry.state
			)
		);

		if (filter_action == "loot" && entry.state != "rolling") {
			auto_loot_entries.emplace_back(recipient, entry.entry_id);
		}
	}

	for (const auto &[recipient, entry_id] : auto_loot_entries) {
		if (recipient) {
			LootEntryForClient(recipient, entry_id);
		}
	}

	corpse = entity_list.GetCorpseByID(corpse_id);

	if (queued) {
		if (corpse) {
			LootCoin(corpse, autoloot_client);
			corpse->ResetDecayTimer();
		}

		if (settings.log_enabled) {
			Audit(
				autoloot_client->CharacterID(),
				nearby ? "nearby_queue" : "kill_queue",
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

	client->Message(Chat::White, "AUTOLOOT|snapshot|begin");
	for (const auto &[entry_id, entry] : m_loot_entries) {
		if (!IsEntryVisibleToClient(entry, client)) {
			continue;
		}

		auto corpse = entity_list.GetCorpseByID(entry.corpse_id);
		const bool locked = corpse && (corpse->IsLocked() || corpse->IsBeingLooted() || !corpse->CanPlayerLoot(client->CharacterID()));
		const auto state = locked ? std::string("locked") : entry.state;
		client->Message(
			Chat::White,
			fmt::format(
				"AUTOLOOT|entry|scope={}|id={}|corpse_id={}|slot={}|item_id={}|icon={}|name={}|qty={}|source={}|state={}|rule={}|locked={}|nodrop={}",
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
				entry.no_drop ? 1 : 0
			).c_str()
		);
	}
	client->Message(Chat::White, "AUTOLOOT|snapshot|end");
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

		const auto filter_action = GetFilterAction(client->CharacterID(), entry.item_id, settings.filter_mode);
		if (filter_action == "skip") {
			entry.rule = "never";
		}
		else if (filter_action == "loot" && HasFilter(client->CharacterID(), entry.item_id, "include")) {
			entry.rule = "always";
		}
		else {
			entry.rule = filter_action == "loot" ? "auto" : "ask";
		}
	}
}

void AutoLootManager::SendNativeFilterUpdate(Client *client)
{
	if (!AutoLootEnabled()) {
		return;
	}

	RefreshQueuedRulesForClient(client);
	SendNativeUpdate(client);
	SendNativeFilters(client, "both");
}

void AutoLootManager::HandleLootAction(Client *client, const Seperator *sep)
{
	if (!RequireAutoLootEnabled(client)) {
		return;
	}

	if (!client || !sep || sep->argnum < 3 || !sep->IsNumber(2)) {
		client->Message(Chat::White, "Usage: #autoloot action [Entry ID] [loot|leave|never|alwaysloot]");
		return;
	}

	const uint32 entry_id = Strings::ToUnsignedInt(sep->arg[2]);
	const std::string action = Strings::ToLower(sep->arg[3]);

	if (action == "loot" || action == "alwaysloot") {
		bool filter_changed = false;
		if (action == "alwaysloot") {
			auto iter = m_loot_entries.find(entry_id);
			if (iter != m_loot_entries.end() && IsEntryVisibleToClient(iter->second, client)) {
				SetFilter(client->CharacterID(), iter->second.item_id, "include");
				filter_changed = true;
			}
		}

		LootEntryForClient(client, entry_id);
		if (filter_changed) {
			SendNativeFilterUpdate(client);
		}
		else {
			SendNativeUpdate(client);
		}
		return;
	}

	if (action == "leave" || action == "pass" || action == "no") {
		LeaveEntryForClient(client, entry_id, false);
		SendNativeUpdate(client);
		return;
	}

	if (action == "never") {
		const bool filter_changed = LeaveEntryForClient(client, entry_id, true);
		if (filter_changed) {
			SendNativeFilterUpdate(client);
		}
		else {
			SendNativeUpdate(client);
		}
		return;
	}

	if (action == "need" || action == "greed" || action == "alwaysneed" || action == "alwaysgreed") {
		auto iter = m_loot_entries.find(entry_id);
		if (iter == m_loot_entries.end() || !IsEntryVisibleToClient(iter->second, client)) {
			client->Message(Chat::Red, "That AutoLoot entry is no longer available.");
			SendNativeUpdate(client);
			return;
		}

		iter->second.state = action;
		bool filter_changed = false;
		if (action == "alwaysneed") {
			SetFilter(client->CharacterID(), iter->second.item_id, "include");
			filter_changed = true;
		}
		client->Message(Chat::White, fmt::format("AutoLoot marked {} as {}.", iter->second.item_name, action).c_str());
		if (filter_changed) {
			SendNativeFilterUpdate(client);
		}
		else {
			SendNativeUpdate(client);
		}
		return;
	}

	client->Message(Chat::White, "Usage: #autoloot action [Entry ID] [loot|leave|never|alwaysloot]");
}

void AutoLootManager::HandlePersonalLootCommand(Client *client, const Seperator *sep)
{
	if (!RequireAutoLootEnabled(client)) {
		return;
	}

	if (!client || !sep || sep->argnum < 2) {
		client->Message(Chat::White, "Usage: #autoloot personal [lootall|leaveall]");
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

	client->Message(Chat::White, "Usage: #autoloot personal [lootall|leaveall]");
}

void AutoLootManager::InspectEntryForClient(Client *client, uint32 entry_id)
{
	auto iter = m_loot_entries.find(entry_id);
	if (iter == m_loot_entries.end() || !IsEntryVisibleToClient(iter->second, client)) {
		client->Message(Chat::Red, "That AutoLoot entry is no longer available.");
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

void AutoLootManager::LootEntryForClient(Client *client, uint32 entry_id)
{
	auto iter = m_loot_entries.find(entry_id);
	if (iter == m_loot_entries.end() || !IsEntryVisibleToClient(iter->second, client)) {
		client->Message(Chat::Red, "That AutoLoot entry is no longer available.");
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
		client->Message(Chat::Red, "That AutoLoot entry is no longer available.");
		return false;
	}

	const auto entry = iter->second;
	bool filter_changed = false;
	if (add_never_filter) {
		SetFilter(client->CharacterID(), entry.item_id, "exclude");
		client->Message(Chat::White, fmt::format("AutoLoot will never loot {} for this character.", entry.item_name).c_str());
		filter_changed = true;
	}

	if (auto corpse = entity_list.GetCorpseByID(entry.corpse_id)) {
		corpse->ResetDecayTimer();
	}

	m_loot_entries.erase(iter);
	return filter_changed;
}

void AutoLootManager::FinalizeCorpse(Corpse *corpse, Client *coin_client)
{
	if (!corpse || HasPendingVotes(corpse->GetID())) {
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
			"AutoLoot looted {} from {}.",
			Strings::Money(platinum, gold, silver, copper),
			corpse->GetCleanName()
		).c_str()
	);
}

bool AutoLootManager::IsNoDrop(uint32 item_id)
{
	const auto *item = database.GetItem(item_id);
	return item && item->NoDrop == 0;
}

bool AutoLootManager::HasPendingVotes(uint16 corpse_id) const
{
	for (const auto &[vote_id, vote] : m_pending_votes) {
		if (vote.corpse_id == corpse_id) {
			return true;
		}
	}

	return false;
}

void AutoLootManager::StartNeedGreedVote(Group *group, Corpse *corpse, uint16 loot_slot, uint32 item_id)
{
	if (!group || !corpse || !item_id) {
		return;
	}

	for (const auto &[vote_id, vote] : m_pending_votes) {
		if (vote.corpse_id == corpse->GetID() && vote.loot_slot == loot_slot) {
			return;
		}
	}

	auto clients = GetGroupClients(group);
	clients.erase(
		std::remove_if(
			clients.begin(),
			clients.end(),
			[corpse](Client *client) { return !corpse->CanPlayerLoot(client->CharacterID()); }
		),
		clients.end()
	);

	if (clients.empty()) {
		return;
	}

	const auto *item = database.GetItem(item_id);
	if (!item) {
		return;
	}

	PendingVote vote;
	vote.vote_id = m_next_vote_id++;
	vote.group_id = group->GetID();
	vote.corpse_id = corpse->GetID();
	vote.loot_slot = loot_slot;
	vote.item_id = item_id;
	vote.item_name = item->Name;
	vote.expires_at = std::time(nullptr) + kNeedGreedSeconds;

	for (auto client : clients) {
		vote.votes[client->CharacterID()] = VoteChoice::Unset;
	}

	m_pending_votes[vote.vote_id] = vote;
	corpse->Lock();

	EQ::SayLinkEngine linker;
	linker.SetLinkType(EQ::saylink::SayLinkItemData);
	linker.SetItemData(item);
	linker.GenerateLink();

	const auto need = Saylink::Silent(fmt::format("#needgreed vote {} need", vote.vote_id), "Need");
	const auto greed = Saylink::Silent(fmt::format("#needgreed vote {} greed", vote.vote_id), "Greed");
	const auto pass = Saylink::Silent(fmt::format("#needgreed vote {} pass", vote.vote_id), "Pass");

	for (auto client : clients) {
		client->Message(
			Chat::Yellow,
			fmt::format(
				"Need/Greed {}: {} | {} | {} | {}",
				vote.vote_id,
				linker.Link(),
				need,
				greed,
				pass
			).c_str()
		);
	}
}

void AutoLootManager::CastNeedGreedVote(Client *client, uint32 vote_id, VoteChoice choice)
{
	if (!client) {
		return;
	}

	auto vote_iter = m_pending_votes.find(vote_id);
	if (vote_iter == m_pending_votes.end()) {
		client->Message(Chat::Red, "That Need/Greed vote is no longer active.");
		return;
	}

	auto &vote = vote_iter->second;
	auto member_vote = vote.votes.find(client->CharacterID());
	if (member_vote == vote.votes.end()) {
		client->Message(Chat::Red, "You are not eligible for that Need/Greed vote.");
		return;
	}

	member_vote->second = choice;
	client->Message(Chat::White, fmt::format("Need/Greed vote {} recorded.", vote_id).c_str());

	const bool complete = std::all_of(
		vote.votes.begin(),
		vote.votes.end(),
		[](const auto &entry) { return entry.second != VoteChoice::Unset; }
	);

	if (complete) {
		ProcessVote(vote_id, false);
	}
}

void AutoLootManager::ProcessVote(uint32 vote_id, bool timeout)
{
	auto vote_iter = m_pending_votes.find(vote_id);
	if (vote_iter == m_pending_votes.end()) {
		return;
	}

	auto vote = vote_iter->second;
	auto corpse = entity_list.GetCorpseByID(vote.corpse_id);

	std::vector<uint32> need;
	std::vector<uint32> greed;
	for (const auto &[character_id, choice] : vote.votes) {
		if (choice == VoteChoice::Need) {
			need.push_back(character_id);
		}
		else if (choice == VoteChoice::Greed) {
			greed.push_back(character_id);
		}
	}

	std::vector<uint32> pool = !need.empty() ? need : greed;
	if (!corpse || pool.empty()) {
		const auto message = fmt::format("Need/Greed vote {} ended with no winner for {}.", vote_id, vote.item_name);
		for (const auto &[character_id, choice] : vote.votes) {
			if (auto member = entity_list.GetClientByCharID(character_id)) {
				member->Message(Chat::Yellow, message.c_str());
			}
		}
		m_pending_votes.erase(vote_id);
		if (corpse && !HasPendingVotes(corpse->GetID())) {
			corpse->UnLock();
		}
		return;
	}

	const uint32 winner_character_id = pool[zone ? zone->random.Int(0, static_cast<int>(pool.size() - 1)) : 0];
	auto winner = entity_list.GetClientByCharID(winner_character_id);
	if (!winner) {
		const auto message = fmt::format("Need/Greed vote {} winner is offline; {} remains on the corpse.", vote_id, vote.item_name);
		for (const auto &[character_id, choice] : vote.votes) {
			if (auto member = entity_list.GetClientByCharID(character_id)) {
				member->Message(Chat::Yellow, message.c_str());
			}
		}
		m_pending_votes.erase(vote_id);
		if (!HasPendingVotes(corpse->GetID())) {
			corpse->UnLock();
		}
		return;
	}

	auto result = corpse->AutoLootItem(winner, vote.loot_slot, true);
	const bool transferred = result.IsSuccess();
	if (result.IsSuccess()) {
		const auto message = fmt::format("{} won Need/Greed vote {} for {}.", winner->GetCleanName(), vote_id, vote.item_name);
		for (const auto &[character_id, choice] : vote.votes) {
			if (auto member = entity_list.GetClientByCharID(character_id)) {
				member->Message(Chat::Yellow, message.c_str());
			}
		}
	}
	else {
		const auto message = fmt::format("{} could not receive {}; it remains on the corpse.", winner->GetCleanName(), vote.item_name);
		for (const auto &[character_id, choice] : vote.votes) {
			if (auto member = entity_list.GetClientByCharID(character_id)) {
				member->Message(Chat::Yellow, message.c_str());
			}
		}
	}

	m_pending_votes.erase(vote_id);

	if (!HasPendingVotes(corpse->GetID())) {
		corpse->UnLock();
		if (transferred) {
			FinalizeCorpse(corpse, winner);
		}
		else if (corpse->IsEmpty()) {
			corpse->Delete();
		}
	}
}

void AutoLootManager::ForceProcessVotes(Client *client)
{
	if (!client) {
		return;
	}

	auto group = client->GetGroup();
	if (!group) {
		client->Message(Chat::White, "You are not in a group.");
		return;
	}

	std::vector<uint32> vote_ids;
	for (const auto &[vote_id, vote] : m_pending_votes) {
		if (vote.group_id == group->GetID()) {
			vote_ids.push_back(vote_id);
		}
	}

	for (const auto vote_id : vote_ids) {
		ProcessVote(vote_id, false);
	}

	client->Message(Chat::White, fmt::format("Force processed {} Need/Greed vote{}.", vote_ids.size(), vote_ids.size() == 1 ? "" : "s").c_str());
}

void AutoLootManager::RecoverVotes(Client *client)
{
	std::vector<uint32> stale_votes;
	for (const auto &[vote_id, vote] : m_pending_votes) {
		if (!entity_list.GetCorpseByID(vote.corpse_id)) {
			stale_votes.push_back(vote_id);
		}
	}

	for (const auto vote_id : stale_votes) {
		m_pending_votes.erase(vote_id);
	}

	client->Message(Chat::White, fmt::format("Recovered {} stale Need/Greed vote{}.", stale_votes.size(), stale_votes.size() == 1 ? "" : "s").c_str());
}

bool AutoLootManager::IsAutosellExcluded(uint32 character_id, uint32 item_id)
{
	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT 1 FROM `custom_autoloot_autosell_exclusions` "
			"WHERE `character_id` = {} AND `item_id` = {} LIMIT 1",
			character_id,
			item_id
		)
	);

	return results.Success() && results.RowCount() > 0;
}

void AutoLootManager::SetAutosellExcluded(uint32 character_id, uint32 item_id, bool excluded)
{
	if (excluded) {
		database.QueryDatabase(
			fmt::format(
				"INSERT IGNORE INTO `custom_autoloot_autosell_exclusions` (`character_id`, `item_id`, `created_at`) "
				"VALUES ({}, {}, UNIX_TIMESTAMP())",
				character_id,
				item_id
			)
		);
	}
	else {
		database.QueryDatabase(
			fmt::format(
				"DELETE FROM `custom_autoloot_autosell_exclusions` WHERE `character_id` = {} AND `item_id` = {}",
				character_id,
				item_id
			)
		);
	}
}

std::vector<uint32> AutoLootManager::GetAutosellExclusions(uint32 character_id)
{
	std::vector<uint32> item_ids;
	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `item_id` FROM `custom_autoloot_autosell_exclusions` "
			"WHERE `character_id` = {} ORDER BY `item_id` LIMIT 100",
			character_id
		)
	);

	if (!results.Success()) {
		return item_ids;
	}

	for (auto row = results.begin(); row != results.end(); ++row) {
		item_ids.push_back(row[0] ? Strings::ToUnsignedInt(row[0]) : 0);
	}

	return item_ids;
}

std::vector<AutoLootManager::AutosellEntry> AutoLootManager::BuildAutosellPreview(Client *client, uint64 &total_value)
{
	std::vector<AutosellEntry> entries;
	total_value = 0;

	if (!client) {
		return entries;
	}

	auto &inventory = client->GetInv();
	for (int16 slot_id = EQ::invslot::GENERAL_BEGIN; slot_id <= EQ::invslot::GENERAL_END; ++slot_id) {
		auto bag = inventory.GetItem(slot_id);
		if (!bag || !bag->IsClassBag() || !IsAutosellBag(bag->GetItem()->ID)) {
			continue;
		}

		for (uint8 bag_index = EQ::invbag::SLOT_BEGIN; bag_index <= EQ::invbag::SLOT_END && bag_index < bag->GetItem()->BagSlots; ++bag_index) {
			const int16 item_slot = EQ::InventoryProfile::CalcSlotId(slot_id, bag_index);
			auto inst = inventory.GetItem(item_slot);
			if (!inst || !inst->GetItem()) {
				continue;
			}

			if (IsAutosellProtected(inst) || IsAutosellExcluded(client->CharacterID(), inst->GetItem()->ID)) {
				continue;
			}

			const uint32 quantity = inst->IsStackable() ? std::max<int16>(1, inst->GetCharges()) : 1;
			const uint64 value = static_cast<uint64>(inst->GetItem()->Price) * quantity;

			entries.push_back(
				AutosellEntry{
					.slot_id = item_slot,
					.item_id = inst->GetItem()->ID,
					.quantity = quantity,
					.value = value,
					.item_name = inst->GetItem()->Name
				}
			);
			total_value += value;
		}
	}

	return entries;
}

void AutoLootManager::PreviewAutosell(Client *client)
{
	uint64 total_value = 0;
	auto entries = BuildAutosellPreview(client, total_value);
	if (entries.empty()) {
		client->Message(Chat::White, "AutoSell found no eligible items in bags 45500 through 45505.");
		return;
	}

	AutosellSession session;
	session.session_id = m_next_autosell_session_id++;
	session.expires_at = std::time(nullptr) + kAutosellSessionSeconds;
	session.entries = entries;
	session.total_value = total_value;
	m_autosell_sessions[client->CharacterID()] = session;

	client->Message(Chat::White, fmt::format("AutoSell preview: {} item stack{} for {}.", entries.size(), entries.size() == 1 ? "" : "s", Strings::MoneyShort(total_value)).c_str());
}

void AutoLootManager::ConfirmAutosell(Client *client)
{
	auto session_iter = m_autosell_sessions.find(client->CharacterID());
	if (session_iter == m_autosell_sessions.end() || session_iter->second.expires_at <= std::time(nullptr)) {
		client->Message(Chat::Red, "No active AutoSell preview. Use #autosell preview first.");
		m_autosell_sessions.erase(client->CharacterID());
		return;
	}

	uint64 paid = 0;
	uint32 sold = 0;
	for (const auto &entry : session_iter->second.entries) {
		auto inst = client->GetInv().GetItem(entry.slot_id);
		if (!inst || !inst->GetItem() || inst->GetItem()->ID != entry.item_id || IsAutosellProtected(inst)) {
			continue;
		}

		const uint32 current_quantity = inst->IsStackable() ? std::max<int16>(1, inst->GetCharges()) : 1;
		const uint32 quantity = std::min(entry.quantity, current_quantity);
		if (!quantity) {
			continue;
		}

		const uint64 value = static_cast<uint64>(inst->GetItem()->Price) * quantity;
		client->DeleteItemInInventory(entry.slot_id, inst->IsStackable() ? quantity : 0, true);
		paid += value;
		sold++;
	}

	if (paid) {
		client->AddMoneyToPP(paid, true);
		client->SaveCurrency();
	}

	m_autosell_sessions.erase(client->CharacterID());
	client->Message(Chat::White, fmt::format("AutoSell sold {} item stack{} for {}.", sold, sold == 1 ? "" : "s", Strings::MoneyShort(paid)).c_str());
}

void AutoLootManager::CancelAutosell(Client *client)
{
	m_autosell_sessions.erase(client->CharacterID());
	client->Message(Chat::White, "AutoSell preview cancelled.");
}

void AutoLootManager::HandleAutolootCommand(Client *client, const Seperator *sep)
{
	if (!RequireAutoLootEnabled(client)) {
		return;
	}

	if (!client || !sep) {
		return;
	}

	auto settings = GetCharacterSettings(client->CharacterID());
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
		client->Message(Chat::White, "AUTOLOOT|window|show");
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
			client->Message(Chat::White, "AUTOLOOT|window|show");
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

		client->Message(Chat::White, "Usage: #autoloot native [show|status|snapshot]");
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
			client->Message(Chat::White, "Usage: #autoloot inspect [Entry ID]");
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
		settings.enabled = true;
		if (arguments >= 2 && IsValidFilterMode(sep->arg[2])) {
			settings.filter_mode = NormalizeFilterMode(sep->arg[2]);
		}
		SaveCharacterSettings(client->CharacterID(), settings);
		client->Message(Chat::White, fmt::format("AutoLoot enabled. Filter mode: {}.", settings.filter_mode).c_str());
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "off")) {
		settings.enabled = false;
		SaveCharacterSettings(client->CharacterID(), settings);
		client->Message(Chat::White, "AutoLoot disabled.");
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "mode")) {
		if (arguments < 2 || !IsValidFilterMode(sep->arg[2])) {
			client->Message(Chat::White, "Usage: #autoloot mode [both|include|exclude]");
			return;
		}

		settings.filter_mode = NormalizeFilterMode(sep->arg[2]);
		SaveCharacterSettings(client->CharacterID(), settings);
		client->Message(Chat::White, fmt::format("AutoLoot filter mode set to {}.", settings.filter_mode).c_str());
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "debug") || !strcasecmp(sep->arg[1], "verbose") || !strcasecmp(sep->arg[1], "log")) {
		if (arguments < 2) {
			client->Message(Chat::White, "Usage: #autoloot debug [on|off], #autoloot verbose [on|off], or #autoloot log [on|off]");
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
				"AutoLoot {} {}.",
				!strcasecmp(sep->arg[1], "log") ? "log" : "debug chat",
				enabled ? "enabled" : "disabled"
			).c_str()
		);
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "nearby") || !strcasecmp(sep->arg[1], "aoe")) {
		const float radius = arguments >= 2 && sep->IsNumber(2) ? Strings::ToFloat(sep->arg[2]) : kDefaultNearbyRadius;
		ProcessNearby(client, radius);
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "group")) {
		auto group = client->GetGroup();
		if (!group) {
			client->Message(Chat::White, "You are not in a group.");
			return;
		}

		if (arguments < 2 || !strcasecmp(sep->arg[2], "status")) {
			auto group_settings = GetGroupSettings(group->GetID());
			client->Message(
				Chat::White,
				fmt::format(
					"AutoLoot group mode: {}, Need/Greed: {}, Assigned Character ID: {}.",
					group_settings.loot_mode,
					group_settings.need_greed_enabled ? "on" : "off",
					group_settings.assigned_character_id
				).c_str()
			);
			RefreshWindowIfRequested(this, client, sep);
			return;
		}

		if (!strcasecmp(sep->arg[2], "help")) {
			SendGroupHelp(client);
			return;
		}

		if (!group->IsLeader(client) && client->Admin() < AccountStatus::GMAdmin) {
			client->Message(Chat::Red, "Only the group leader can change AutoLoot group settings.");
			return;
		}

		auto group_settings = GetGroupSettings(group->GetID());
		if (!strcasecmp(sep->arg[2], "needgreed")) {
			if (arguments < 3) {
				client->Message(Chat::White, "Usage: #autoloot group needgreed [on|off]");
				return;
			}

			group_settings.need_greed_enabled = Strings::ToBool(sep->arg[3]);
			SaveGroupSettings(group->GetID(), group_settings);
			client->Message(Chat::White, fmt::format("Need/Greed {}.", group_settings.need_greed_enabled ? "enabled" : "disabled").c_str());
			RefreshWindowIfRequested(this, client, sep);
			return;
		}

		if (!strcasecmp(sep->arg[2], "forceprocess")) {
			ForceProcessVotes(client);
			RefreshWindowIfRequested(this, client, sep);
			return;
		}

		if (!strcasecmp(sep->arg[2], "recover")) {
			RecoverVotes(client);
			RefreshWindowIfRequested(this, client, sep);
			return;
		}

		if (!strcasecmp(sep->arg[2], "assign")) {
			if (arguments < 3) {
				client->Message(Chat::White, "Usage: #autoloot group assign [Character Name]");
				return;
			}

			auto assigned = entity_list.GetClientByName(sep->arg[3]);
			if (!assigned || !group->IsGroupMember(assigned)) {
				client->Message(Chat::Red, "That player is not in your group or is not in this zone.");
				return;
			}

			group_settings.loot_mode = "assigned";
			group_settings.assigned_character_id = assigned->CharacterID();
			SaveGroupSettings(group->GetID(), group_settings);
			client->Message(Chat::White, fmt::format("AutoLoot group assigned looter set to {}.", assigned->GetCleanName()).c_str());
			RefreshWindowIfRequested(this, client, sep);
			return;
		}

		if (!IsValidGroupMode(sep->arg[2])) {
			SendGroupHelp(client);
			return;
		}

		group_settings.loot_mode = NormalizeGroupMode(sep->arg[2]);
		SaveGroupSettings(group->GetID(), group_settings);
		client->Message(Chat::White, fmt::format("AutoLoot group mode set to {}.", group_settings.loot_mode).c_str());
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	SendHelp(client);
}

void AutoLootManager::HandleLootFilterCommand(Client *client, const Seperator *sep)
{
	if (!RequireAutoLootEnabled(client)) {
		return;
	}

	if (!client || !sep) {
		return;
	}

	if (!sep->argnum || !strcasecmp(sep->arg[1], "help")) {
		client->Message(Chat::White, "Usage: #lootfilter keep [Item ID]");
		client->Message(Chat::White, "Usage: #lootfilter ignore [Item ID]");
		client->Message(Chat::White, "Usage: #lootfilter unset [Item ID]");
		client->Message(Chat::White, "Usage: #lootfilter add [Item ID] [keep|ignore]");
		client->Message(Chat::White, "Usage: #lootfilter remove [Item ID]");
		client->Message(Chat::White, "Usage: #lootfilter list [keep|ignore|all]");
		return;
	}

	if (!strcasecmp(sep->arg[1], "mode")) {
		auto settings = GetCharacterSettings(client->CharacterID());
		if (sep->argnum < 2 || !IsValidFilterMode(sep->arg[2])) {
			client->Message(Chat::White, "Usage: #lootfilter mode [both|include|exclude]");
			return;
		}

		settings.filter_mode = NormalizeFilterMode(sep->arg[2]);
		SaveCharacterSettings(client->CharacterID(), settings);
		client->Message(Chat::White, fmt::format("Loot filter mode set to {}.", settings.filter_mode).c_str());
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "native")) {
		if (sep->argnum >= 2 && !strcasecmp(sep->arg[2], "list")) {
			SendNativeStatus(client);
			SendNativeFilters(client, sep->argnum >= 3 ? sep->arg[3] : "both");
			return;
		}

		client->Message(Chat::White, "Usage: #lootfilter native list [keep|ignore|all]");
		return;
	}

	if (!strcasecmp(sep->arg[1], "list")) {
		const auto mode = sep->argnum >= 2 ? NormalizeRuleMode(sep->arg[2]) : "both";
		const auto filters = GetFilters(client->CharacterID(), mode);
		if (filters.empty()) {
			client->Message(Chat::White, "No AutoLoot rules found.");
			return;
		}

		for (const auto &[item_id, filter_mode] : filters) {
			const auto *item = database.GetItem(item_id);
			client->Message(Chat::White, fmt::format("{}: {} ({})", DisplayRuleMode(filter_mode), item ? item->Name : "Unknown Item", item_id).c_str());
		}
		return;
	}

	if (!strcasecmp(sep->arg[1], "add")) {
		if (sep->argnum < 2 || !sep->IsNumber(2)) {
			client->Message(Chat::White, "Usage: #lootfilter add [Item ID] [keep|ignore]");
			return;
		}

		const uint32 item_id = Strings::ToUnsignedInt(sep->arg[2]);
		const auto mode = sep->argnum >= 3 ? NormalizeRuleMode(sep->arg[3]) : "include";
		if (mode != "include" && mode != "exclude") {
			client->Message(Chat::White, "Usage: #lootfilter add [Item ID] [keep|ignore]");
			return;
		}

		const auto *item = database.GetItem(item_id);
		if (!item) {
			client->Message(Chat::Red, "Invalid item ID.");
			return;
		}

		SetFilter(client->CharacterID(), item_id, mode);
		client->Message(Chat::White, fmt::format("Set AutoLoot rule {} for {} ({}).", DisplayRuleMode(mode), item->Name, item_id).c_str());
		SendNativeFilterUpdate(client);
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "remove")) {
		if (sep->argnum >= 2 && sep->IsNumber(2)) {
			const uint32 item_id = Strings::ToUnsignedInt(sep->arg[2]);
			const auto mode = sep->argnum >= 3 ? NormalizeRuleMode(sep->arg[3]) : "both";
			if (!IsValidFilterMode(mode)) {
				client->Message(Chat::White, "Usage: #lootfilter remove [Item ID] [keep|ignore|all]");
				return;
			}

			RemoveFilter(client->CharacterID(), item_id, mode);
			client->Message(Chat::White, fmt::format("Unset AutoLoot rule for item {}.", item_id).c_str());
			SendNativeFilterUpdate(client);
			RefreshWindowIfRequested(this, client, sep);
			return;
		}

		if (sep->argnum < 3 || !IsValidFilterMode(NormalizeRuleMode(sep->arg[2])) || !sep->IsNumber(3)) {
			client->Message(Chat::White, "Usage: #lootfilter remove [keep|ignore|all] [Item ID]");
			return;
		}

		const uint32 item_id = Strings::ToUnsignedInt(sep->arg[3]);
		RemoveFilter(client->CharacterID(), item_id, NormalizeRuleMode(sep->arg[2]));
		client->Message(Chat::White, fmt::format("Unset AutoLoot rule for item {}.", item_id).c_str());
		SendNativeFilterUpdate(client);
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if (!strcasecmp(sep->arg[1], "unset") && sep->argnum >= 2 && sep->IsNumber(2)) {
		const uint32 item_id = Strings::ToUnsignedInt(sep->arg[2]);
		RemoveFilter(client->CharacterID(), item_id, "both");
		client->Message(Chat::White, fmt::format("Unset AutoLoot rule for item {}.", item_id).c_str());
		SendNativeFilterUpdate(client);
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	if ((!strcasecmp(sep->arg[1], "include") || !strcasecmp(sep->arg[1], "exclude") || !strcasecmp(sep->arg[1], "keep") || !strcasecmp(sep->arg[1], "ignore")) && sep->argnum >= 2 && sep->IsNumber(2)) {
		const uint32 item_id = Strings::ToUnsignedInt(sep->arg[2]);
		const auto *item = database.GetItem(item_id);
		if (!item) {
			client->Message(Chat::Red, "Invalid item ID.");
			return;
		}

		const auto mode = NormalizeRuleMode(sep->arg[1]);
		SetFilter(client->CharacterID(), item_id, mode);
		client->Message(Chat::White, fmt::format("Set AutoLoot rule {} for {} ({}).", DisplayRuleMode(mode), item->Name, item_id).c_str());
		SendNativeFilterUpdate(client);
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	client->Message(Chat::White, "Usage: #lootfilter help");
}

void AutoLootManager::HandleAutosellCommand(Client *client, const Seperator *sep)
{
	if (!RequireAutoLootEnabled(client)) {
		return;
	}

	if (!client || !sep) {
		return;
	}

	if (!sep->argnum || !strcasecmp(sep->arg[1], "help")) {
		client->Message(Chat::White, "Usage: #autosell preview");
		client->Message(Chat::White, "Usage: #autosell confirm");
		client->Message(Chat::White, "Usage: #autosell cancel");
		client->Message(Chat::White, "Usage: #autosell exclude [add|remove|list|clear] [Item ID]");
		return;
	}

	if (!strcasecmp(sep->arg[1], "preview")) {
		PreviewAutosell(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "confirm")) {
		ConfirmAutosell(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "cancel")) {
		CancelAutosell(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "exclude")) {
		if (sep->argnum < 2) {
			client->Message(Chat::White, "Usage: #autosell exclude [add|remove|list|clear] [Item ID]");
			return;
		}

		if (!strcasecmp(sep->arg[2], "list")) {
			auto exclusions = GetAutosellExclusions(client->CharacterID());
			if (exclusions.empty()) {
				client->Message(Chat::White, "No AutoSell exclusions found.");
				return;
			}

			for (const auto item_id : exclusions) {
				const auto *item = database.GetItem(item_id);
				client->Message(Chat::White, fmt::format("- {} ({})", item ? item->Name : "Unknown Item", item_id).c_str());
			}
			return;
		}

		if (!strcasecmp(sep->arg[2], "clear")) {
			database.QueryDatabase(fmt::format("DELETE FROM `custom_autoloot_autosell_exclusions` WHERE `character_id` = {}", client->CharacterID()));
			client->Message(Chat::White, "AutoSell exclusions cleared.");
			RefreshWindowIfRequested(this, client, sep);
			return;
		}

		if (sep->argnum < 3 || !sep->IsNumber(3)) {
			client->Message(Chat::White, "Usage: #autosell exclude [add|remove] [Item ID]");
			return;
		}

		const uint32 item_id = Strings::ToUnsignedInt(sep->arg[3]);
		const bool add = !strcasecmp(sep->arg[2], "add");
		const bool remove = !strcasecmp(sep->arg[2], "remove");
		if (!add && !remove) {
			client->Message(Chat::White, "Usage: #autosell exclude [add|remove] [Item ID]");
			return;
		}

		SetAutosellExcluded(client->CharacterID(), item_id, add);
		client->Message(Chat::White, fmt::format("AutoSell exclusion {} for item {}.", add ? "added" : "removed", item_id).c_str());
		RefreshWindowIfRequested(this, client, sep);
		return;
	}

	client->Message(Chat::White, "Usage: #autosell help");
}

void AutoLootManager::HandleNeedGreedCommand(Client *client, const Seperator *sep)
{
	if (!RequireAutoLootEnabled(client)) {
		return;
	}

	if (!client || !sep || sep->argnum < 3 || strcasecmp(sep->arg[1], "vote") || !sep->IsNumber(2)) {
		client->Message(Chat::White, "Usage: #needgreed vote [Vote ID] [need|greed|pass]");
		return;
	}

	CastNeedGreedVote(client, Strings::ToUnsignedInt(sep->arg[2]), ParseVoteChoice(sep->arg[3]));
}

void AutoLootManager::ShowWindow(Client *client)
{
	if (!RequireAutoLootEnabled(client)) {
		return;
	}

	if (!client) {
		return;
	}

	client->Message(Chat::White, "AUTOLOOT|window|show");
	SendNativeUpdate(client);
}

void AutoLootManager::SendStatus(Client *client)
{
	const auto settings = GetCharacterSettings(client->CharacterID(), true);
	client->Message(
		Chat::White,
		fmt::format(
			"AutoLoot: {}, mode: {}, debug: {}, log: {}.",
			settings.enabled ? "on" : "off",
			settings.filter_mode,
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
	const auto include_count = GetFilters(client->CharacterID(), "include").size();
	const auto exclude_count = GetFilters(client->CharacterID(), "exclude").size();

	std::string group_mode = "none";
	std::string assigned_name = "none";
	bool grouped = false;
	bool leader = false;
	bool need_greed_enabled = false;

	auto group = client->GetGroup();
	if (group) {
		grouped = true;
		leader = group->IsLeader(client) || client->Admin() >= AccountStatus::GMAdmin;

		const auto group_settings = GetGroupSettings(group->GetID());
		group_mode = group_settings.loot_mode;
		need_greed_enabled = group_settings.need_greed_enabled;

		if (group_settings.assigned_character_id) {
			assigned_name = database.GetCharNameByID(group_settings.assigned_character_id);
			if (assigned_name.empty()) {
				assigned_name = "unknown";
			}
		}
	}

	const auto status = fmt::format(
		"AUTOLOOT|status|enabled={}|include={}|exclude={}|grouped={}|group_mode={}|assigned={}|leader={}|filter_mode={}|debug={}|log={}|needgreed={}",
		settings.enabled ? 1 : 0,
		include_count,
		exclude_count,
		grouped ? 1 : 0,
		group_mode,
		assigned_name,
		leader ? 1 : 0,
		settings.filter_mode,
		settings.debug_enabled ? 1 : 0,
		settings.log_enabled ? 1 : 0,
		need_greed_enabled ? 1 : 0
	);

	client->Message(Chat::White, status.c_str());
}

void AutoLootManager::SendNativeFilters(Client *client, const std::string &filter_mode)
{
	if (!client) {
		return;
	}

	const auto mode = NormalizeRuleMode(filter_mode);
	client->Message(Chat::White, fmt::format("AUTOLOOT|filters|begin|mode={}", mode).c_str());

	for (const auto &[item_id, filter_mode] : GetFilters(client->CharacterID(), mode)) {
		const auto *item = database.GetItem(item_id);
		client->Message(
			Chat::White,
			fmt::format(
				"AUTOLOOT|filter|mode={}|item_id={}|icon={}|name={}",
				ProtocolValue(filter_mode),
				item_id,
				item ? item->Icon : 0,
				ProtocolValue(item ? item->Name : "Unknown Item")
			).c_str()
		);
	}

	client->Message(Chat::White, fmt::format("AUTOLOOT|filters|end|mode={}", mode).c_str());
}

void AutoLootManager::SendHelp(Client *client)
{
	client->Message(Chat::White, "Usage: #autoloot on [both|include|exclude]");
	client->Message(Chat::White, "Usage: #autoloot off");
	client->Message(Chat::White, "Usage: #autoloot mode [both|include|exclude]");
	client->Message(Chat::White, "Usage: #autoloot debug [on|off] or #autoloot verbose [on|off]");
	client->Message(Chat::White, "Usage: #autoloot log [on|off]");
	client->Message(Chat::White, "Usage: #autoloot inspect [Entry ID]");
	client->Message(Chat::White, "Usage: #autoloot nearby [radius]");
	client->Message(Chat::White, "Usage: #autoloot group [status|help|none|solo|master|robin|killer|assign|needgreed|forceprocess|recover]");
}

void AutoLootManager::SendGroupHelp(Client *client)
{
	client->Message(Chat::White, "Usage: #autoloot group none");
	client->Message(Chat::White, "Usage: #autoloot group solo");
	client->Message(Chat::White, "Usage: #autoloot group master");
	client->Message(Chat::White, "Usage: #autoloot group robin");
	client->Message(Chat::White, "Usage: #autoloot group killer");
	client->Message(Chat::White, "Usage: #autoloot group assign [Character Name]");
	client->Message(Chat::White, "Usage: #autoloot group needgreed [on|off]");
	client->Message(Chat::White, "Usage: #autoloot group forceprocess");
	client->Message(Chat::White, "Usage: #autoloot group recover");
}

void AutoLootManager::Audit(uint32 character_id, const std::string &action, uint32 item_id, uint32 quantity, const std::string &detail)
{
	database.QueryDatabase(
		fmt::format(
			"INSERT INTO `custom_autoloot_audit` (`character_id`, `action`, `item_id`, `quantity`, `detail`, `created_at`) "
			"VALUES ({}, '{}', {}, {}, '{}', UNIX_TIMESTAMP())",
			character_id,
			Strings::Escape(action),
			item_id,
			quantity,
			Strings::Escape(detail)
		)
	);
}

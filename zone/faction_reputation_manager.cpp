/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "faction_reputation_manager.h"

#include "client.h"
#include "entity.h"
#include "mob.h"
#include "npc.h"
#include "string_ids.h"
#include "zone.h"
#include "zonedb.h"

#include "common/faction.h"
#include "common/rulesys.h"
#include "common/seperator.h"
#include "common/strings.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "fmt/format.h"

extern Zone *zone;
extern EntityList entity_list;
extern ZoneDatabase database;
extern ZoneDatabase content_db;

FactionReputationManager faction_reputation_manager;

namespace {

constexpr const char *kPreferenceTable = "custom_faction_window_preferences";

struct FactionPreference {
	bool pinned = false;
	bool hidden = false;
	std::string section;
};

struct ReputationRow {
	int32 faction_id = 0;
	std::string name;
	int32 raw_value = 0;
	int32 modified_value = 0;
	FACTION_VALUE standing = FACTION_INDIFFERENTLY;
	bool touched = false;
	bool target = false;
	bool pinned = false;
	bool hidden = false;
	std::string section = "All";
};

std::string ProtocolValue(std::string value)
{
	for (auto &ch : value) {
		if (ch == '|' || ch == '\r' || ch == '\n') {
			ch = ' ';
		}
	}

	return value;
}

std::string SqlString(const std::string &value)
{
	return Strings::Escape(value);
}

std::string Lower(const std::string &value)
{
	return Strings::ToLower(value);
}

std::string FactionName(int32 faction_id, const std::map<int32, std::string> &names)
{
	const auto iter = names.find(faction_id);
	if (iter != names.end() && !iter->second.empty()) {
		return iter->second;
	}

	auto name = content_db.GetFactionName(faction_id);
	if (!name.empty()) {
		return name;
	}

	return fmt::format("Faction {}", faction_id);
}

std::map<int32, std::string> LoadAllFactionNames()
{
	std::map<int32, std::string> factions;
	auto results = content_db.QueryDatabase("SELECT `id`, `name` FROM `faction_list` WHERE `id` > 0 ORDER BY `name`, `id`");
	if (!results.Success()) {
		results = database.QueryDatabase("SELECT `id`, `name` FROM `faction_list` WHERE `id` > 0 ORDER BY `name`, `id`");
	}

	if (!results.Success()) {
		return factions;
	}

	for (auto row = results.begin(); row != results.end(); ++row) {
		const auto faction_id = Strings::ToInt(row[0]);
		if (faction_id > 0) {
			factions[faction_id] = row[1] ? row[1] : "";
		}
	}

	return factions;
}

std::set<int32> LoadTouchedFactionIds(Client *client)
{
	std::set<int32> faction_ids;
	if (!client || !client->CharacterID()) {
		return faction_ids;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `faction_id` FROM `faction_values` WHERE `char_id` = {}",
			client->CharacterID()
		)
	);

	if (!results.Success()) {
		return faction_ids;
	}

	for (auto row = results.begin(); row != results.end(); ++row) {
		const auto faction_id = Strings::ToInt(row[0]);
		if (faction_id > 0) {
			faction_ids.insert(faction_id);
		}
	}

	return faction_ids;
}

std::map<int32, FactionPreference> LoadPreferences(Client *client)
{
	std::map<int32, FactionPreference> preferences;
	if (!client || !client->CharacterID()) {
		return preferences;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `faction_id`, `pinned`, `hidden`, `section` "
			"FROM `{}` WHERE `character_id` = {}",
			kPreferenceTable,
			client->CharacterID()
		)
	);

	if (!results.Success()) {
		return preferences;
	}

	for (auto row = results.begin(); row != results.end(); ++row) {
		const auto faction_id = Strings::ToInt(row[0]);
		if (faction_id <= 0) {
			continue;
		}

		FactionPreference preference;
		preference.pinned = row[1] ? Strings::ToBool(row[1]) : false;
		preference.hidden = row[2] ? Strings::ToBool(row[2]) : false;
		preference.section = row[3] ? row[3] : "";
		preferences[faction_id] = preference;
	}

	return preferences;
}

bool SavePreference(Client *client, int32 faction_id, bool pinned, bool hidden, const std::string &section)
{
	if (!client || !client->CharacterID() || faction_id <= 0) {
		return false;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"INSERT INTO `{}` "
			"(`character_id`, `faction_id`, `pinned`, `hidden`, `section`, `updated_at`) "
			"VALUES ({}, {}, {}, {}, '{}', UNIX_TIMESTAMP()) "
			"ON DUPLICATE KEY UPDATE "
			"`pinned` = VALUES(`pinned`), `hidden` = VALUES(`hidden`), "
			"`section` = VALUES(`section`), `updated_at` = UNIX_TIMESTAMP()",
			kPreferenceTable,
			client->CharacterID(),
			faction_id,
			pinned ? 1 : 0,
			hidden ? 1 : 0,
			SqlString(section)
		)
	);

	return results.Success();
}

bool DeletePreference(Client *client, int32 faction_id)
{
	if (!client || !client->CharacterID() || faction_id <= 0) {
		return false;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"DELETE FROM `{}` WHERE `character_id` = {} AND `faction_id` = {}",
			kPreferenceTable,
			client->CharacterID(),
			faction_id
		)
	);

	return results.Success();
}

bool ResetPreferences(Client *client)
{
	if (!client || !client->CharacterID()) {
		return false;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"DELETE FROM `{}` WHERE `character_id` = {}",
			kPreferenceTable,
			client->CharacterID()
		)
	);

	return results.Success();
}

int32 TargetFactionId(Client *client)
{
	if (!client || !client->GetTarget() || !client->GetTarget()->IsNPC()) {
		return 0;
	}

	return client->GetTarget()->CastToNPC()->GetPrimaryFaction();
}

int32 ParseFactionId(Client *client, const Seperator *sep, int argument_index)
{
	if (!sep || argument_index < 0) {
		return 0;
	}

	const std::string value = sep->arg[argument_index] ? Lower(sep->arg[argument_index]) : "";
	if (value == "target" || value == "t") {
		return TargetFactionId(client);
	}

	return Strings::ToInt(value);
}

std::string SectionForRow(const ReputationRow &row)
{
	if (row.target) {
		return "Target";
	}

	if (row.hidden) {
		return "Hidden";
	}

	if (row.pinned) {
		return "Pinned";
	}

	if (row.touched || row.raw_value != 0) {
		return "Changed";
	}

	return "All";
}

int SectionRank(const ReputationRow &row)
{
	if (row.target) {
		return 0;
	}

	if (row.pinned) {
		return 1;
	}

	if (row.touched || row.raw_value != 0) {
		return 2;
	}

	if (row.hidden) {
		return 4;
	}

	return 3;
}

bool RowMatchesSearch(const ReputationRow &row, const std::string &search)
{
	if (search.empty()) {
		return true;
	}

	const auto lower_name = Lower(row.name);
	if (lower_name.find(search) != std::string::npos) {
		return true;
	}

	return std::to_string(row.faction_id).find(search) != std::string::npos;
}

std::vector<ReputationRow> BuildRows(Client *client, const std::string &mode, const std::string &search)
{
	std::vector<ReputationRow> rows;
	if (!client) {
		return rows;
	}

	const auto all_factions = LoadAllFactionNames();
	const auto touched_factions = LoadTouchedFactionIds(client);
	const auto preferences = LoadPreferences(client);
	const auto target_faction_id = TargetFactionId(client);
	const bool show_all = RuleB(CustomFeatures, FactionWindowShowAllByDefault);
	const bool hidden_mode = mode == "hidden";
	const std::string search_lower = Lower(search);

	std::set<int32> faction_ids;
	if (show_all || hidden_mode || !search_lower.empty()) {
		for (const auto &entry : all_factions) {
			faction_ids.insert(entry.first);
		}
	} else {
		faction_ids.insert(touched_factions.begin(), touched_factions.end());
		for (const auto &entry : preferences) {
			if (entry.second.pinned) {
				faction_ids.insert(entry.first);
			}
		}
	}

	if (target_faction_id > 0) {
		faction_ids.insert(target_faction_id);
	}

	for (const auto faction_id : faction_ids) {
		if (faction_id <= 0) {
			continue;
		}

		ReputationRow row;
		row.faction_id = faction_id;
		row.name = FactionName(faction_id, all_factions);
		row.raw_value = client->GetCharacterFactionLevel(faction_id);
		row.modified_value = client->GetModCharacterFactionLevel(faction_id);
		row.standing = CalculateFaction(nullptr, row.modified_value);
		row.touched = touched_factions.count(faction_id) != 0;
		row.target = target_faction_id == faction_id;

		const auto preference_iter = preferences.find(faction_id);
		if (preference_iter != preferences.end()) {
			row.pinned = preference_iter->second.pinned;
			row.hidden = preference_iter->second.hidden;
			row.section = preference_iter->second.section;
		}

		if (row.section.empty()) {
			row.section = SectionForRow(row);
		} else if (row.target || row.hidden || row.pinned || row.touched || row.raw_value != 0) {
			row.section = SectionForRow(row);
		}

		if (hidden_mode) {
			if (!row.hidden && !row.target) {
				continue;
			}
		} else if (row.hidden && !row.target) {
			continue;
		}

		if (!RowMatchesSearch(row, search_lower)) {
			continue;
		}

		rows.emplace_back(row);
	}

	std::sort(rows.begin(), rows.end(), [](const ReputationRow &left, const ReputationRow &right) {
		const auto left_rank = SectionRank(left);
		const auto right_rank = SectionRank(right);
		if (left_rank != right_rank) {
			return left_rank < right_rank;
		}

		if (left.modified_value != right.modified_value && left_rank == 2) {
			return left.modified_value > right.modified_value;
		}

		const auto left_name = Lower(left.name);
		const auto right_name = Lower(right.name);
		if (left_name != right_name) {
			return left_name < right_name;
		}

		return left.faction_id < right.faction_id;
	});

	return rows;
}

void SendHelp(Client *client)
{
	client->Message(Chat::White, "Faction reputation commands:");
	client->Message(Chat::White, "#rep refresh - Open or refresh the native faction reputation window.");
	client->Message(Chat::White, "#rep list - Print visible faction standings to chat.");
	client->Message(Chat::White, "#rep search <text> - Search visible factions by name or ID.");
	client->Message(Chat::White, "#rep hidden - Show hidden factions.");
	client->Message(Chat::White, "#rep pin target|<id> - Pin a faction to the top of the window.");
	client->Message(Chat::White, "#rep unpin target|<id> - Remove a faction pin.");
	client->Message(Chat::White, "#rep hide target|<id> - Hide a faction from normal views.");
	client->Message(Chat::White, "#rep show target|<id> - Unhide a faction.");
	client->Message(Chat::White, "#rep resetprefs - Clear your faction pins and hidden rows.");
}

} // namespace

void FactionReputationManager::HandleCommand(Client *client, const Seperator *sep)
{
	if (!client) {
		return;
	}

	if (!RuleB(CustomFeatures, FactionWindowEnabled)) {
		client->Message(Chat::White, "Faction reputation window is disabled on this server.");
		return;
	}

	const std::string action = sep && sep->arg[1] ? Lower(sep->arg[1]) : "";
	if (action == "help") {
		SendHelp(client);
		return;
	}

	if (action == "list" || action == "status" || action == "chat") {
		SendChatList(client, "", "");
		return;
	}

	if (action == "search") {
		const std::string search = sep && sep->argplus[2] ? sep->argplus[2] : "";
		SendNativeSnapshot(client, "search", search);
		return;
	}

	if (action == "hidden") {
		SendNativeSnapshot(client, "hidden", "");
		return;
	}

	if (action == "pin" || action == "unpin" || action == "hide" || action == "show") {
		const auto faction_id = ParseFactionId(client, sep, 2);
		if (faction_id <= 0) {
			client->Message(Chat::White, "Use #rep %s target or #rep %s <faction id>.", action.c_str(), action.c_str());
			return;
		}

		auto preferences = LoadPreferences(client);
		auto preference = preferences[faction_id];
		if (action == "pin") {
			preference.pinned = true;
			preference.hidden = false;
			client->Message(Chat::White, "Pinned faction %d.", faction_id);
		} else if (action == "unpin") {
			preference.pinned = false;
			client->Message(Chat::White, "Unpinned faction %d.", faction_id);
		} else if (action == "hide") {
			preference.pinned = false;
			preference.hidden = true;
			client->Message(Chat::White, "Hid faction %d.", faction_id);
		} else {
			preference.hidden = false;
			client->Message(Chat::White, "Unhid faction %d.", faction_id);
		}

		if (!preference.pinned && !preference.hidden && preference.section.empty()) {
			DeletePreference(client, faction_id);
		} else if (!SavePreference(client, faction_id, preference.pinned, preference.hidden, preference.section)) {
			client->Message(Chat::White, "Unable to save faction window preference. Has the custom database update been applied?");
		}

		SendNativeSnapshot(client, action == "show" ? "hidden" : "", "");
		return;
	}

	if (action == "resetprefs") {
		if (ResetPreferences(client)) {
			client->Message(Chat::White, "Faction window preferences cleared.");
		} else {
			client->Message(Chat::White, "Unable to clear faction window preferences.");
		}

		SendNativeSnapshot(client, "", "");
		return;
	}

	SendNativeSnapshot(client, "", "");
}

void FactionReputationManager::SendNativeSnapshot(Client *client)
{
	SendNativeSnapshot(client, "", "");
}

void FactionReputationManager::SendNativeSnapshot(Client *client, const std::string &mode, const std::string &search)
{
	if (!client) {
		return;
	}

	auto rows = BuildRows(client, mode, search);
	const auto total_count = rows.size();
	const auto target_faction_id = TargetFactionId(client);
	const auto max_rows = RuleI(CustomFeatures, FactionWindowMaxRowsPerSnapshot);
	bool capped = false;
	if (max_rows > 0 && rows.size() > static_cast<size_t>(max_rows)) {
		rows.resize(static_cast<size_t>(max_rows));
		capped = true;
	}

	std::string status;
	if (mode == "hidden") {
		status = "Showing hidden faction rows.";
	} else if (!search.empty()) {
		status = fmt::format("Search results for '{}'.", search);
	} else if (target_faction_id > 0) {
		status = "Target faction included. Use Pin or Hide to customize.";
	} else {
		status = "Showing faction reputation. Target an NPC to highlight its primary faction.";
	}

	if (capped) {
		status = fmt::format("{} Showing {} of {} rows.", status, rows.size(), total_count);
	}

	client->Message(Chat::White, "FACTION|window|clear");
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"FACTION|summary|count={}|total={}|target={}|mode={}|search={}|status={}",
			rows.size(),
			total_count,
			target_faction_id,
			ProtocolValue(mode),
			ProtocolValue(search),
			ProtocolValue(status)
		).c_str()
	);

	for (const auto &row : rows) {
		client->Message(
			Chat::White,
			"%s",
			fmt::format(
				"FACTION|row|id={}|name={}|raw={}|mod={}|standing={}|touched={}|target={}|pinned={}|hidden={}|section={}",
				row.faction_id,
				ProtocolValue(row.name),
				row.raw_value,
				row.modified_value,
				ProtocolValue(FactionValueToString(row.standing)),
				row.touched ? 1 : 0,
				row.target ? 1 : 0,
				row.pinned ? 1 : 0,
				row.hidden ? 1 : 0,
				ProtocolValue(row.section)
			).c_str()
		);
	}

	client->Message(Chat::White, "FACTION|window|show");
}

void FactionReputationManager::SendChatList(Client *client)
{
	SendChatList(client, "", "");
}

void FactionReputationManager::SendChatList(Client *client, const std::string &mode, const std::string &search)
{
	if (!client) {
		return;
	}

	auto rows = BuildRows(client, mode, search);
	const auto max_rows = RuleI(CustomFeatures, FactionWindowMaxRowsPerSnapshot);
	if (max_rows > 0 && rows.size() > static_cast<size_t>(max_rows)) {
		rows.resize(static_cast<size_t>(max_rows));
	}

	if (rows.empty()) {
		client->Message(Chat::White, "No faction reputation rows were found.");
		return;
	}

	client->Message(Chat::White, "Faction reputation:");
	for (const auto &row : rows) {
		client->Message(
			Chat::White,
			"%s",
			fmt::format(
				"{}{}{} [{}]: {} (raw {}, modified {})",
				row.target ? "* " : "",
				row.pinned ? "[Pinned] " : "",
				row.name,
				row.section,
				FactionValueToString(row.standing),
				row.raw_value,
				row.modified_value
			).c_str()
		);
	}
}

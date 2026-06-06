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
#include "common/repositories/faction_list_repository.h"
#include "common/rulesys.h"
#include "common/seperator.h"
#include "common/strings.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

extern Zone *zone;
extern EntityList entity_list;
extern ZoneDatabase database;
extern ZoneDatabase content_db;

FactionReputationManager faction_reputation_manager;

namespace {

struct ReputationRow {
	int32 faction_id = 0;
	std::string name;
	int32 raw_value = 0;
	int32 modified_value = 0;
	FACTION_VALUE standing = FACTION_INDIFFERENTLY;
	bool touched = false;
	bool target = false;
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

std::string FactionName(int32 faction_id)
{
	auto name = content_db.GetFactionName(faction_id);
	if (!name.empty()) {
		return name;
	}

	const auto row = FactionListRepository::FindOne(database, faction_id);
	if (row.id == faction_id && !row.name.empty()) {
		return row.name;
	}

	return fmt::format("Faction {}", faction_id);
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

std::set<int32> LoadCuratedFactionIds()
{
	static const std::vector<std::string> names = {
		"Claws of Veeshan",
		"Yelinak",
		"King Tormax",
		"Kromzek",
		"Kromrif",
		"Coldain",
		"Dain Frostreaver IV",
		"Dragons of Norrath",
		"Ring of Scale",
		"Yelinak's Allies",
	};

	std::vector<std::string> where;
	for (const auto &name : names) {
		where.emplace_back(fmt::format("`name` = '{}'", SqlString(name)));
	}

	where.emplace_back("`name` LIKE '%Veeshan%'");
	where.emplace_back("`name` LIKE '%Yelinak%'");
	where.emplace_back("`name` LIKE '%Tormax%'");
	where.emplace_back("`name` LIKE '%Kromzek%'");
	where.emplace_back("`name` LIKE '%Kromrif%'");
	where.emplace_back("`name` LIKE '%Coldain%'");

	std::set<int32> faction_ids;
	auto results = content_db.QueryDatabase(
		fmt::format(
			"SELECT `id` FROM `faction_list` WHERE {}",
			Strings::Join(where, " OR ")
		)
	);

	if (!results.Success()) {
		results = database.QueryDatabase(
			fmt::format(
				"SELECT `id` FROM `faction_list` WHERE {}",
				Strings::Join(where, " OR ")
			)
		);
	}

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

int32 TargetFactionId(Client *client)
{
	if (!client || !client->GetTarget() || !client->GetTarget()->IsNPC()) {
		return 0;
	}

	return client->GetTarget()->CastToNPC()->GetPrimaryFaction();
}

std::vector<ReputationRow> BuildRows(Client *client)
{
	std::set<int32> touched_factions = LoadTouchedFactionIds(client);
	std::set<int32> faction_ids = touched_factions;

	const auto curated = LoadCuratedFactionIds();
	faction_ids.insert(curated.begin(), curated.end());

	const auto target_faction_id = TargetFactionId(client);
	if (target_faction_id > 0) {
		faction_ids.insert(target_faction_id);
	}

	std::vector<ReputationRow> rows;
	for (const auto faction_id : faction_ids) {
		if (faction_id <= 0) {
			continue;
		}

		ReputationRow row;
		row.faction_id = faction_id;
		row.name = FactionName(faction_id);
		row.raw_value = client->GetCharacterFactionLevel(faction_id);
		row.modified_value = client->GetModCharacterFactionLevel(faction_id);
		row.standing = CalculateFaction(nullptr, row.modified_value);
		row.touched = touched_factions.count(faction_id) != 0;
		row.target = target_faction_id == faction_id;
		rows.emplace_back(row);
	}

	std::sort(rows.begin(), rows.end(), [](const ReputationRow &left, const ReputationRow &right) {
		if (left.target != right.target) {
			return left.target > right.target;
		}

		if (left.touched != right.touched) {
			return left.touched > right.touched;
		}

		return Strings::ToLower(left.name) < Strings::ToLower(right.name);
	});

	return rows;
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

	const std::string action = sep && sep->arg[1] ? Strings::ToLower(sep->arg[1]) : "";
	if (action == "list" || action == "status" || action == "chat") {
		SendChatList(client);
		return;
	}

	if (action == "help") {
		client->Message(Chat::White, "Faction reputation commands:");
		client->Message(Chat::White, "#rep - Open the native faction reputation window.");
		client->Message(Chat::White, "#rep refresh - Refresh the native window.");
		client->Message(Chat::White, "#rep list - Print faction standing in chat.");
		return;
	}

	SendNativeSnapshot(client);
}

void FactionReputationManager::SendNativeSnapshot(Client *client)
{
	if (!client) {
		return;
	}

	const auto rows = BuildRows(client);
	const auto target_faction_id = TargetFactionId(client);

	client->Message(Chat::White, "FACTION|window|clear");
	client->Message(
		Chat::White,
		fmt::format(
			"FACTION|summary|count={}|target={}|status={}",
			rows.size(),
			target_faction_id,
			ProtocolValue(target_faction_id > 0 ? "Target faction included." : "Target an NPC to pin its primary faction.")
		).c_str()
	);

	for (const auto &row : rows) {
		client->Message(
			Chat::White,
			fmt::format(
				"FACTION|row|id={}|name={}|raw={}|mod={}|standing={}|touched={}|target={}",
				row.faction_id,
				ProtocolValue(row.name),
				row.raw_value,
				row.modified_value,
				ProtocolValue(FactionValueToString(row.standing)),
				row.touched ? 1 : 0,
				row.target ? 1 : 0
			).c_str()
		);
	}

	client->Message(Chat::White, "FACTION|window|show");
}

void FactionReputationManager::SendChatList(Client *client)
{
	if (!client) {
		return;
	}

	const auto rows = BuildRows(client);
	if (rows.empty()) {
		client->Message(Chat::White, "No faction reputation rows were found.");
		return;
	}

	client->Message(Chat::White, "Faction reputation:");
	for (const auto &row : rows) {
		client->Message(
			Chat::White,
			fmt::format(
				"{}{}: {} (raw {}, modified {})",
				row.target ? "* " : "",
				row.name,
				FactionValueToString(row.standing),
				row.raw_value,
				row.modified_value
			).c_str()
		);
	}
}

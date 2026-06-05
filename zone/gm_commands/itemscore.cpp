/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "common/item_instance.h"
#include "common/item_power.h"
#include "common/rulesys.h"
#include "common/seperator.h"
#include "common/strings.h"
#include "zone/client.h"
#include "zone/zonedb.h"

#include <algorithm>
#include <memory>
#include <string>

namespace {
	constexpr uint32 kRecalcChunkSize = 1000;

	std::string GetArg(const Seperator *sep, uint16 index)
	{
		if (!sep || index > sep->GetMaxArgNum() || !sep->arg[index]) {
			return {};
		}

		return sep->arg[index];
	}

	void SendItemScoreUsage(Client *c)
	{
		c->Message(Chat::White, "Usage: #itemscore init");
		c->Message(Chat::White, "Usage: #itemscore show <item_id>");
		c->Message(Chat::White, "Usage: #itemscore recalc <item_id|all>");
		c->Message(Chat::White, "Usage: #itemscore explain <item_id>");
		c->Message(Chat::White, "Usage: #itemscore audit [limit]");
		c->Message(Chat::White, "Usage: #itemscore override <item_id> level <1-127>");
		c->Message(Chat::White, "Usage: #itemscore override <item_id> multiplier <value>");
		c->Message(Chat::White, "Usage: #itemscore override <item_id> bonus <points>");
		c->Message(Chat::White, "Usage: #itemscore override <item_id> notes <text>");
		c->Message(Chat::White, "Usage: #itemscore clearoverride <item_id>");
		c->Message(Chat::White, "Usage: #itemscore view <item_id>");
	}

	const EQ::ItemData *GetItem(Client *c, const std::string &item_id_arg, uint32 &item_id)
	{
		if (item_id_arg.empty() || !Strings::IsNumber(item_id_arg)) {
			c->Message(Chat::Yellow, "Specify a numeric item ID.");
			return nullptr;
		}

		item_id = Strings::ToUnsignedInt(item_id_arg);
		const auto *item = database.GetItem(item_id);
		if (!item) {
			c->Message(Chat::Yellow, "Item [{}] was not found in shared item data.", item_id);
			return nullptr;
		}

		return item;
	}

	void SendTransport(Client *c, const EQ::ItemData &item)
	{
		std::string transport;
		if (EQ::ItemPower::TryBuildStoredTransportMessage(database, item, transport)) {
			c->Message(Chat::White, "%s", transport.c_str());
		}
	}

	void SendStoredScore(Client *c, const EQ::ItemData &item)
	{
		EQ::ItemPower::StoredScore stored_score;
		if (!EQ::ItemPower::TryGetStoredScore(database, item.ID, stored_score)) {
			c->Message(Chat::Yellow, "No stored item power score for [{}] {}.", item.ID, database.CreateItemLink(item.ID));
			return;
		}

		const auto role = EQ::ItemPower::BestRoleFromScores(stored_score);
		c->Message(
			Chat::White,
			"Item Power [{}] {}: level [{}], score [{}], role [{}], version [{}], source [{}], updated [{}]",
			item.ID,
			database.CreateItemLink(item.ID),
			stored_score.item_level,
			stored_score.item_score,
			EQ::ItemPower::RoleName(role),
			stored_score.score_version,
			stored_score.source,
			stored_score.updated_at
		);
		c->Message(
			Chat::White,
			"Role scores: tank [{}], melee [{}], caster [{}], healer [{}], hybrid [{}]",
			stored_score.tank_score,
			stored_score.melee_score,
			stored_score.caster_score,
			stored_score.healer_score,
			stored_score.hybrid_score
		);
		SendTransport(c, item);
	}

	bool RecalculateItem(Client *c, const EQ::ItemData &item, bool save_breakdown)
	{
		if (!EQ::ItemPower::EnsureSchema(database)) {
			c->Message(Chat::Red, "Unable to initialize item_power schema.");
			return false;
		}

		auto score = EQ::ItemPower::Calculate(item, save_breakdown);
		EQ::ItemPower::ApplyOverrides(database, score);

		std::string error_message;
		if (!EQ::ItemPower::SaveScore(database, score, &error_message, save_breakdown)) {
			c->Message(Chat::Red, "Failed to save score for item [{}]: {}", item.ID, error_message);
			return false;
		}

		c->Message(
			Chat::White,
			"Recalculated [{}] {}: level [{}], score [{}], role [{}], source [{}]",
			item.ID,
			database.CreateItemLink(item.ID),
			score.item_level,
			score.item_score,
			EQ::ItemPower::RoleName(score.best_role),
			score.source
		);
		SendTransport(c, item);
		return true;
	}

	void ExplainItem(Client *c, const EQ::ItemData &item)
	{
		auto score = EQ::ItemPower::Calculate(item, true);
		EQ::ItemPower::ApplyOverrides(database, score);

		c->Message(
			Chat::White,
			"Computed [{}] {}: level [{}], score [{}], role [{}], slot budget [{} {:.2f}], source [{}]",
			item.ID,
			database.CreateItemLink(item.ID),
			score.item_level,
			score.item_score,
			EQ::ItemPower::RoleName(score.best_role),
			score.slot_budget_name,
			score.slot_budget,
			score.source
		);
		c->Message(
			Chat::White,
			"Role scores: tank [{}], melee [{}], caster [{}], healer [{}], hybrid [{}]",
			score.tank_score,
			score.melee_score,
			score.caster_score,
			score.healer_score,
			score.hybrid_score
		);

		for (const auto &component : score.breakdown) {
			c->Message(Chat::White, "Component [{}]: score [{}] {}", component.component, component.score, component.details);
		}

		for (const auto &warning : score.warnings) {
			c->Message(Chat::Yellow, "Warning: {}", warning);
		}
	}

	void RecalculateAll(Client *c)
	{
		if (!EQ::ItemPower::EnsureSchema(database)) {
			c->Message(Chat::Red, "Unable to initialize item_power schema.");
			return;
		}

		uint32 item_cursor = 0;
		uint32 scored = 0;
		uint32 chunk_count = 0;

		database.TransactionBegin();
		while (const auto *item = database.IterateItems(&item_cursor)) {
			auto score = EQ::ItemPower::Calculate(*item, false);
			EQ::ItemPower::ApplyOverrides(database, score);

			std::string error_message;
			if (!EQ::ItemPower::SaveScore(database, score, &error_message, false, false)) {
				database.TransactionRollback();
				c->Message(Chat::Red, "Recalculation stopped at item [{}]: {}", item->ID, error_message);
				return;
			}

			++scored;
			++chunk_count;

			if (chunk_count >= kRecalcChunkSize) {
				const auto commit = database.TransactionCommit();
				if (!commit.Success()) {
					c->Message(Chat::Red, "Failed to commit item power scores: {}", commit.ErrorMessage());
					return;
				}

				chunk_count = 0;
				database.TransactionBegin();
			}
		}

		const auto commit = database.TransactionCommit();
		if (!commit.Success()) {
			c->Message(Chat::Red, "Failed to commit item power scores: {}", commit.ErrorMessage());
			return;
		}

		c->Message(Chat::White, "Recalculated item power scores for [{}] shared-memory items.", scored);
	}

	void AuditScores(Client *c, uint32 limit)
	{
		if (!EQ::ItemPower::SchemaReady(database)) {
			c->Message(Chat::Yellow, "item_power schema is not ready. Run #itemscore init or database updates first.");
			return;
		}

		auto missing = database.QueryDatabase(
			"SELECT COUNT(*) FROM `items` i LEFT JOIN `item_power` p ON p.`item_id` = i.`id` WHERE p.`item_id` IS NULL"
		);
		if (missing.Success() && missing.RowCount()) {
			auto row = missing.begin();
			c->Message(Chat::White, "Items missing item_power rows: [{}]", row[0] ? row[0] : "0");
		}

		auto stale = database.QueryDatabase(
			fmt::format(
				"SELECT COUNT(*) FROM `item_power` WHERE `score_version` <> {}",
				EQ::ItemPower::ScoreVersion
			)
		);
		if (stale.Success() && stale.RowCount()) {
			auto row = stale.begin();
			c->Message(Chat::White, "Stored rows with stale score versions: [{}]", row[0] ? row[0] : "0");
		}

		auto outliers = database.QueryDatabase(
			fmt::format(
				"SELECT i.`id`, i.`Name`, i.`reqlevel`, i.`reclevel`, p.`item_level`, p.`item_score`, "
				"ABS(CAST(p.`item_level` AS SIGNED) - CAST(GREATEST(i.`reqlevel`, CEIL(i.`reclevel` * 0.85)) AS SIGNED)) AS level_delta "
				"FROM `item_power` p INNER JOIN `items` i ON i.`id` = p.`item_id` "
				"ORDER BY level_delta DESC, p.`item_score` DESC LIMIT {}",
				std::clamp<uint32>(limit, 1, 50)
			)
		);

		if (!outliers.Success()) {
			c->Message(Chat::Red, "Unable to audit item power scores: {}", outliers.ErrorMessage());
			return;
		}

		c->Message(Chat::White, "Largest item level deltas:");
		for (auto row : outliers) {
			c->Message(
				Chat::White,
				"[{}] {} req [{}] rec [{}] ilvl [{}] score [{}] delta [{}]",
				row[0] ? row[0] : "0",
				row[1] ? row[1] : "",
				row[2] ? row[2] : "0",
				row[3] ? row[3] : "0",
				row[4] ? row[4] : "0",
				row[5] ? row[5] : "0",
				row[6] ? row[6] : "0"
			);
		}
	}

	void OverrideScore(Client *c, const Seperator *sep)
	{
		uint32 item_id = 0;
		const auto *item = GetItem(c, GetArg(sep, 2), item_id);
		if (!item) {
			return;
		}

		const auto field = Strings::ToLower(GetArg(sep, 3));
		const auto value = GetArg(sep, 4);
		if (field.empty() || value.empty()) {
			SendItemScoreUsage(c);
			return;
		}

		bool updated = false;
		if (field == "level") {
			if (!Strings::IsNumber(value)) {
				c->Message(Chat::Yellow, "Level override must be numeric.");
				return;
			}

			const auto item_level = static_cast<uint16>(std::clamp<uint32>(Strings::ToUnsignedInt(value), 1, 127));
			updated = EQ::ItemPower::SetLevelOverride(database, item_id, item_level);
		}
		else if (field == "multiplier") {
			const auto multiplier = Strings::ToFloat(value, 0.0f);
			if (multiplier <= 0.0f) {
				c->Message(Chat::Yellow, "Score multiplier must be greater than zero.");
				return;
			}

			updated = EQ::ItemPower::SetMultiplierOverride(database, item_id, multiplier);
		}
		else if (field == "bonus") {
			if (!Strings::IsNumber(value)) {
				c->Message(Chat::Yellow, "Flat score bonus must be numeric.");
				return;
			}

			updated = EQ::ItemPower::SetFlatBonusOverride(database, item_id, Strings::ToInt(value));
		}
		else if (field == "notes") {
			std::string notes = sep->argplus[4] ? sep->argplus[4] : "";
			Strings::Trim(notes);
			updated = EQ::ItemPower::SetOverrideNotes(database, item_id, notes);
		}
		else {
			SendItemScoreUsage(c);
			return;
		}

		if (!updated) {
			c->Message(Chat::Red, "Failed to update item power override for item [{}].", item_id);
			return;
		}

		c->Message(Chat::White, "Updated item power override for [{}] {}.", item_id, database.CreateItemLink(item_id));
		if (field != "notes") {
			RecalculateItem(c, *item, true);
		}
	}
}

void command_itemscore(Client *c, const Seperator *sep)
{
	if (!RuleB(CustomFeatures, GearScoreEnabled)) {
		c->Message(Chat::White, "GearScore is disabled on this server.");
		return;
	}

	const auto action = Strings::ToLower(GetArg(sep, 1));
	if (action.empty() || action == "help") {
		SendItemScoreUsage(c);
		return;
	}

	if (action == "init") {
		if (EQ::ItemPower::EnsureSchema(database)) {
			c->Message(Chat::White, "item_power schema is ready.");
		}
		else {
			c->Message(Chat::Red, "item_power schema initialization failed.");
		}
		return;
	}

	if (action == "audit") {
		const auto limit = GetArg(sep, 2).empty() ? 10 : Strings::ToUnsignedInt(GetArg(sep, 2), 10);
		AuditScores(c, limit);
		return;
	}

	if (action == "recalc" && Strings::ToLower(GetArg(sep, 2)) == "all") {
		RecalculateAll(c);
		return;
	}

	if (action == "override") {
		OverrideScore(c, sep);
		return;
	}

	uint32 item_id = 0;
	const auto *item = GetItem(c, GetArg(sep, 2), item_id);
	if (!item) {
		return;
	}

	if (action == "show") {
		SendStoredScore(c, *item);
	}
	else if (action == "recalc") {
		RecalculateItem(c, *item, true);
	}
	else if (action == "explain") {
		ExplainItem(c, *item);
	}
	else if (action == "clearoverride") {
		if (!EQ::ItemPower::ClearOverride(database, item_id)) {
			c->Message(Chat::Red, "Failed to clear item power override for item [{}].", item_id);
			return;
		}

		c->Message(Chat::White, "Cleared item power override for [{}] {}.", item_id, database.CreateItemLink(item_id));
		RecalculateItem(c, *item, true);
	}
	else if (action == "view") {
		std::unique_ptr<EQ::ItemInstance> inst(database.CreateItem(item, 1));
		if (!inst) {
			c->Message(Chat::Red, "Unable to create a view packet for item [{}].", item_id);
			return;
		}

		c->SendItemPacket(0, inst.get(), ItemPacketViewLink);
	}
	else {
		SendItemScoreUsage(c);
	}
}

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
#pragma once

#include "common/types.h"

#include <string>
#include <vector>

class Database;

namespace EQ
{
	class ItemInstance;
	struct ItemData;

	namespace ItemPower
	{
		constexpr uint16 ScoreVersion = 2;

		enum class Role : uint8 {
			Tank = 0,
			Melee,
			Caster,
			Healer,
			Hybrid
		};

		enum class SearchSort : uint8 {
			Score = 0,
			Level,
			Name,
			Random
		};

		enum class RarityFilterMode : uint8 {
			Any = 0,
			Exact,
			Minimum,
			Untagged
		};

		struct ScoreComponent {
			std::string component;
			int32       score = 0;
			std::string details;
		};

		struct ScoreResult {
			uint32 item_id       = 0;
			uint16 item_level    = 1;
			uint32 item_score    = 0;
			uint32 tank_score    = 0;
			uint32 melee_score   = 0;
			uint32 caster_score  = 0;
			uint32 healer_score  = 0;
			uint32 hybrid_score  = 0;
			uint16 score_version = ScoreVersion;

			Role best_role = Role::Tank;
			std::string source = "computed";

			int32 base_stat_score = 0;
			int32 defense_score   = 0;
			int32 offense_score   = 0;
			int32 sustain_score   = 0;
			int32 weapon_score    = 0;
			int32 effect_score    = 0;

			double slot_budget = 1.0;
			std::string slot_budget_name = "Default";

			std::vector<ScoreComponent> breakdown;
			std::vector<std::string> warnings;
		};

		struct StoredScore {
			uint32 item_id       = 0;
			uint16 item_level    = 1;
			uint32 item_score    = 0;
			uint32 tank_score    = 0;
			uint32 melee_score   = 0;
			uint32 caster_score  = 0;
			uint32 healer_score  = 0;
			uint32 hybrid_score  = 0;
			uint16 score_version = ScoreVersion;
			std::string source;
			std::string updated_at;
		};

		struct SearchFilters {
			bool has_min_score = false;
			bool has_max_score = false;
			uint32 min_score = 0;
			uint32 max_score = 0;

			bool has_min_level = false;
			bool has_max_level = false;
			uint16 min_level = 0;
			uint16 max_level = 0;

			bool has_role = false;
			Role role = Role::Tank;

			RarityFilterMode rarity_mode = RarityFilterMode::Any;
			uint8 rarity = 0;

			bool has_class_mask = false;
			uint32 class_mask = 0;

			bool has_slot_mask = false;
			uint32 slot_mask = 0;

			int16 item_type = -1;
			int16 item_class = -1;
			int16 nodrop = -1;
			int16 norent = -1;

			uint32 limit = 25;
			SearchSort sort = SearchSort::Score;
		};

		struct SearchResult {
			uint32 item_id = 0;
			std::string name;
			uint16 item_level = 0;
			uint32 item_score = 0;
			Role best_role = Role::Tank;
			uint32 tank_score = 0;
			uint32 melee_score = 0;
			uint32 caster_score = 0;
			uint32 healer_score = 0;
			uint32 hybrid_score = 0;
			uint16 score_version = ScoreVersion;
			std::string source;
			uint8 reqlevel = 0;
			uint8 reclevel = 0;
			uint32 classes = 0;
			uint32 slots = 0;
			uint8 itemtype = 0;
			uint8 itemclass = 0;
			uint8 nodrop = 0;
			uint8 norent = 0;
			int32 loregroup = 0;
			uint8 rarity = 0;
			std::string rarity_name;
			bool rarity_tagged = false;
		};

		bool EnsureSchema(Database &db);
		bool SchemaReady(Database &db);

		ScoreResult Calculate(const ItemData &item, bool include_breakdown = true);
		ScoreResult Calculate(const ItemInstance &inst, bool include_breakdown = true);
		bool ApplyOverrides(Database &db, ScoreResult &score);
		bool SaveScore(
			Database &db,
			const ScoreResult &score,
			std::string *error_message = nullptr,
			bool save_breakdown = true,
			bool use_transaction = true
		);
		bool TryGetStoredScore(Database &db, uint32 item_id, StoredScore &score);

		bool SetLevelOverride(Database &db, uint32 item_id, uint16 item_level);
		bool SetMultiplierOverride(Database &db, uint32 item_id, float multiplier);
		bool SetFlatBonusOverride(Database &db, uint32 item_id, int32 flat_bonus);
		bool SetOverrideNotes(Database &db, uint32 item_id, const std::string &notes);
		bool ClearOverride(Database &db, uint32 item_id);

		const char *RoleName(Role role);
		const char *RoleKey(Role role);
		bool ParseRole(const std::string &value, Role &role);
		bool ParseRarity(const std::string &value, uint8 &rarity, bool *untagged = nullptr);
		bool ParseClassMask(const std::string &value, uint32 &class_mask);
		bool ParseSlotMask(const std::string &value, uint32 &slot_mask);
		bool ParseSearchFilters(const std::vector<std::string> &args, SearchFilters &filters, std::string *error_message = nullptr);
		Role BestRoleFromScores(const StoredScore &score);
		bool GetItemPower(Database &db, uint32 item_id, SearchResult &result, std::string *error_message = nullptr);
		bool FindItemPower(Database &db, const SearchFilters &filters, std::vector<SearchResult> &results, std::string *error_message = nullptr);
		uint32 RandomItemPower(Database &db, const SearchFilters &filters, std::string *error_message = nullptr);

		std::string BuildTransportMessage(const ItemData &item, const ScoreResult &score);
		std::string BuildTransportMessage(const ItemData &item, const StoredScore &score);
		bool TryBuildTransportMessage(Database &db, const ItemData &item, std::string &message, bool calculate_if_missing);
		bool TryBuildTransportMessage(Database &db, const ItemInstance &inst, std::string &message, bool calculate_if_missing);
		bool TryBuildStoredTransportMessage(Database &db, const ItemData &item, std::string &message);
	}
}

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
#include "item_power.h"

#include "common/database.h"
#include "common/emu_constants.h"
#include "common/item_data.h"
#include "common/strings.h"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>

namespace {
	constexpr const char *kItemPowerTable = "item_power";
	constexpr const char *kItemPowerOverrideTable = "item_power_override";
	constexpr const char *kItemPowerBreakdownTable = "item_power_breakdown";

	int schema_ready = -1;

	struct SlotBudget {
		int slot;
		const char *name;
		double budget;
	};

	struct RoleWeights {
		double hp = 1.0;
		double mana = 1.0;
		double endurance = 1.0;
		double ac = 1.0;
		double str = 1.0;
		double sta = 1.0;
		double dex = 1.0;
		double agi = 1.0;
		double intel = 1.0;
		double wis = 1.0;
		double cha = 1.0;
		double heroic = 1.0;
		double resist = 1.0;
		double heroic_resist = 1.0;
		double regen = 1.0;
		double mana_regen = 1.0;
		double endurance_regen = 1.0;
		double atk = 1.0;
		double accuracy = 1.0;
		double avoidance = 1.0;
		double combat_effects = 1.0;
		double shielding = 1.0;
		double spell_shielding = 1.0;
		double dot_shielding = 1.0;
		double stun_resist = 1.0;
		double strikethrough = 1.0;
		double spell_damage = 1.0;
		double heal_amount = 1.0;
		double clairvoyance = 1.0;
		double damage_shield = 1.0;
		double ds_mitigation = 1.0;
		double haste = 1.0;
		double weapon = 1.0;
		double effects = 1.0;
	};

	struct RoleScore {
		uint32 total = 0;
		std::map<std::string, double> components;
		std::map<std::string, std::vector<std::string>> details;
	};

	struct SlotBudgetResult {
		double budget = 1.0;
		std::string name = "Default";
	};

	struct OverrideData {
		bool found = false;
		bool has_level = false;
		bool has_multiplier = false;
		bool has_flat_bonus = false;
		uint16 item_level = 0;
		float multiplier = 1.0f;
		int32 flat_bonus = 0;
	};

	uint32 RoundScore(double value)
	{
		if (value <= 0.0) {
			return 0;
		}

		return static_cast<uint32>(std::lround(value));
	}

	double Positive(double value)
	{
		return std::max(0.0, value);
	}

	double SoftCap(double value, double cap)
	{
		value = Positive(value);
		if (cap <= 0.0) {
			return value;
		}

		const double soft_cap = cap * 0.70;
		if (value <= soft_cap) {
			return value;
		}

		return soft_cap + ((value - soft_cap) * 0.35);
	}

	void AddScore(
		RoleScore &score,
		const std::string &component,
		const std::string &label,
		double value,
		double base_weight,
		double role_weight
	)
	{
		const double points = Positive(value) * base_weight * role_weight;
		if (points <= 0.0) {
			return;
		}

		score.components[component] += points;
		score.details[component].push_back(fmt::format("{}={:.1f}", label, points));
	}

	void AddFlatScore(
		RoleScore &score,
		const std::string &component,
		const std::string &label,
		double points
	)
	{
		if (points <= 0.0) {
			return;
		}

		score.components[component] += points;
		score.details[component].push_back(fmt::format("{}={:.1f}", label, points));
	}

	RoleWeights GetRoleWeights(EQ::ItemPower::Role role)
	{
		RoleWeights weights;

		switch (role) {
			case EQ::ItemPower::Role::Tank:
				weights.hp = 1.30;
				weights.ac = 1.50;
				weights.sta = 1.25;
				weights.agi = 1.15;
				weights.avoidance = 1.50;
				weights.shielding = 1.50;
				weights.spell_shielding = 1.20;
				weights.resist = 1.10;
				weights.heroic_resist = 1.10;
				weights.weapon = 0.70;
				weights.mana = 0.35;
				weights.spell_damage = 0.20;
				weights.heal_amount = 0.35;
				break;
			case EQ::ItemPower::Role::Melee:
				weights.weapon = 1.60;
				weights.str = 1.25;
				weights.dex = 1.25;
				weights.haste = 1.40;
				weights.atk = 1.35;
				weights.accuracy = 1.50;
				weights.combat_effects = 1.30;
				weights.strikethrough = 1.25;
				weights.hp = 0.60;
				weights.ac = 0.60;
				weights.mana = 0.10;
				weights.spell_damage = 0.10;
				weights.heal_amount = 0.10;
				weights.clairvoyance = 0.10;
				break;
			case EQ::ItemPower::Role::Caster:
				weights.hp = 0.50;
				weights.ac = 0.50;
				weights.mana = 1.25;
				weights.intel = 1.20;
				weights.wis = 1.20;
				weights.spell_damage = 1.60;
				weights.clairvoyance = 1.30;
				weights.mana_regen = 1.25;
				weights.weapon = 0.10;
				weights.heal_amount = 0.20;
				weights.haste = 0.10;
				weights.effects = 1.25;
				break;
			case EQ::ItemPower::Role::Healer:
				weights.hp = 0.70;
				weights.ac = 0.70;
				weights.mana = 1.35;
				weights.intel = 1.20;
				weights.wis = 1.20;
				weights.heal_amount = 1.60;
				weights.clairvoyance = 1.30;
				weights.mana_regen = 1.35;
				weights.weapon = 0.10;
				weights.spell_damage = 0.30;
				weights.haste = 0.10;
				weights.effects = 1.25;
				break;
			case EQ::ItemPower::Role::Hybrid:
				weights.hp = 1.00;
				weights.mana = 0.75;
				weights.endurance = 0.90;
				weights.ac = 1.00;
				weights.str = 1.10;
				weights.sta = 1.10;
				weights.dex = 1.10;
				weights.agi = 1.00;
				weights.intel = 0.85;
				weights.wis = 0.85;
				weights.weapon = 1.00;
				weights.spell_damage = 0.75;
				weights.heal_amount = 0.75;
				weights.clairvoyance = 0.85;
				break;
		}

		return weights;
	}

	SlotBudgetResult GetSlotBudget(const EQ::ItemData &item)
	{
		if (item.ItemType == EQ::item::ItemTypeAugmentation) {
			return SlotBudgetResult{0.50, "Augment"};
		}

		if (item.IsType2HWeapon()) {
			return SlotBudgetResult{1.90, "Primary 2H"};
		}

		if (item.IsType1HWeapon()) {
			return SlotBudgetResult{1.25, "Primary 1H"};
		}

		if (item.ItemType == EQ::item::ItemTypeBow) {
			return SlotBudgetResult{0.75, "Range Bow"};
		}

		if (item.IsTypeShield()) {
			return SlotBudgetResult{1.15, "Shield"};
		}

		const std::array<SlotBudget, 23> budgets = {{
			{EQ::invslot::slotCharm, "Charm", 1.00},
			{EQ::invslot::slotEar1, "Ear", 0.60},
			{EQ::invslot::slotHead, "Head", 1.00},
			{EQ::invslot::slotFace, "Face", 0.75},
			{EQ::invslot::slotEar2, "Ear", 0.60},
			{EQ::invslot::slotNeck, "Neck", 0.75},
			{EQ::invslot::slotShoulders, "Shoulders", 0.95},
			{EQ::invslot::slotArms, "Arms", 0.95},
			{EQ::invslot::slotBack, "Back", 0.90},
			{EQ::invslot::slotWrist1, "Wrist", 0.65},
			{EQ::invslot::slotWrist2, "Wrist", 0.65},
			{EQ::invslot::slotRange, "Range", 0.75},
			{EQ::invslot::slotHands, "Hands", 0.85},
			{EQ::invslot::slotPrimary, "Primary", 1.25},
			{EQ::invslot::slotSecondary, "Secondary", 1.15},
			{EQ::invslot::slotFinger1, "Finger", 0.60},
			{EQ::invslot::slotFinger2, "Finger", 0.60},
			{EQ::invslot::slotChest, "Chest", 1.35},
			{EQ::invslot::slotLegs, "Legs", 1.20},
			{EQ::invslot::slotFeet, "Feet", 0.85},
			{EQ::invslot::slotWaist, "Waist", 0.85},
			{EQ::invslot::slotPowerSource, "Power Source", 1.00},
			{EQ::invslot::slotAmmo, "Ammo", 0.25}
		}};

		SlotBudgetResult best{1.00, "Default"};
		for (const auto &budget : budgets) {
			if (!(item.Slots & (1u << budget.slot))) {
				continue;
			}

			if (budget.budget > best.budget || best.name == "Default") {
				best.budget = budget.budget;
				best.name = budget.name;
			}
		}

		return best;
	}

	double GetWeaponTypeMultiplier(const EQ::ItemData &item)
	{
		if (item.IsType2HWeapon()) {
			return 1.35;
		}

		if (item.ItemType == EQ::item::ItemTypeBow) {
			return 0.85;
		}

		if (item.IsType1HWeapon()) {
			return 1.00;
		}

		return 0.0;
	}

	double GetEffectScore(const EQ::item::ItemEffect_Struct &effect, double base_score)
	{
		return effect.Effect > 0 ? base_score : 0.0;
	}

	RoleScore CalculateRoleScore(const EQ::ItemData &item, EQ::ItemPower::Role role)
	{
		RoleScore score;
		const RoleWeights weights = GetRoleWeights(role);

		AddScore(score, "base_stats", "hp", item.HP, 0.10, weights.hp);
		AddScore(score, "base_stats", "mana", item.Mana, 0.08, weights.mana);
		AddScore(score, "base_stats", "endurance", item.Endur, 0.08, weights.endurance);

		AddScore(score, "defense", "ac", item.AC, 1.80, weights.ac);

		AddScore(score, "base_stats", "str", item.AStr, 0.35, weights.str);
		AddScore(score, "base_stats", "sta", item.ASta, 0.35, weights.sta);
		AddScore(score, "base_stats", "dex", item.ADex, 0.35, weights.dex);
		AddScore(score, "base_stats", "agi", item.AAgi, 0.35, weights.agi);
		AddScore(score, "base_stats", "int", item.AInt, 0.35, weights.intel);
		AddScore(score, "base_stats", "wis", item.AWis, 0.35, weights.wis);
		AddScore(score, "base_stats", "cha", item.ACha, 0.35, weights.cha);

		const double heroic_stats =
			Positive(item.HeroicStr) + Positive(item.HeroicSta) + Positive(item.HeroicDex) +
			Positive(item.HeroicAgi) + Positive(item.HeroicInt) + Positive(item.HeroicWis) +
			Positive(item.HeroicCha);
		AddScore(score, "base_stats", "heroic_stats", heroic_stats, 1.25, weights.heroic);

		const double normal_resists =
			Positive(item.CR) + Positive(item.DR) + Positive(item.PR) + Positive(item.MR) +
			Positive(item.FR) + Positive(item.SVCorruption);
		AddScore(score, "base_stats", "resists", normal_resists, 0.18, weights.resist);

		const double heroic_resists =
			Positive(item.HeroicCR) + Positive(item.HeroicDR) + Positive(item.HeroicPR) +
			Positive(item.HeroicMR) + Positive(item.HeroicFR) + Positive(item.HeroicSVCorrup);
		AddScore(score, "base_stats", "heroic_resists", heroic_resists, 0.40, weights.heroic_resist);

		AddScore(score, "sustain", "hp_regen", SoftCap(item.Regen, 30), 2.00, weights.regen);
		AddScore(score, "sustain", "mana_regen", SoftCap(item.ManaRegen, 15), 2.50, weights.mana_regen);
		AddScore(score, "sustain", "end_regen", SoftCap(item.EnduranceRegen, 15), 2.50, weights.endurance_regen);
		AddScore(score, "sustain", "damage_shield", SoftCap(item.DamageShield, 30), 1.20, weights.damage_shield);

		AddScore(score, "offense", "atk", SoftCap(item.Attack, 250), 0.25, weights.atk);
		AddScore(score, "offense", "accuracy", SoftCap(item.Accuracy, 150), 0.90, weights.accuracy);
		AddScore(score, "offense", "combat_effects", SoftCap(item.CombatEffects, 100), 0.80, weights.combat_effects);
		AddScore(score, "offense", "strikethrough", SoftCap(item.StrikeThrough, 35), 2.50, weights.strikethrough);
		AddScore(score, "offense", "spell_damage", SoftCap(item.SpellDmg, 250), 0.65, weights.spell_damage);
		AddScore(score, "offense", "heal_amount", SoftCap(item.HealAmt, 250), 0.65, weights.heal_amount);
		AddScore(score, "offense", "clairvoyance", SoftCap(item.Clairvoyance, 250), 0.85, weights.clairvoyance);
		AddScore(score, "offense", "haste", SoftCap(item.Haste, 100), 2.20, weights.haste);

		AddScore(score, "defense", "avoidance", SoftCap(item.Avoidance, 100), 0.90, weights.avoidance);
		AddScore(score, "defense", "shielding", SoftCap(item.Shielding, 35), 4.00, weights.shielding);
		AddScore(score, "defense", "spell_shield", SoftCap(item.SpellShield, 35), 3.00, weights.spell_shielding);
		AddScore(score, "defense", "dot_shield", SoftCap(item.DotShielding, 35), 2.50, weights.dot_shielding);
		AddScore(score, "defense", "stun_resist", SoftCap(item.StunResist, 35), 2.00, weights.stun_resist);
		AddScore(score, "defense", "ds_mitigation", SoftCap(item.DSMitigation, 50), 1.50, weights.ds_mitigation);

		const double weapon_type_multiplier = GetWeaponTypeMultiplier(item);
		if (weapon_type_multiplier > 0.0) {
			const double weapon_damage =
				Positive(item.Damage) +
				Positive(item.ElemDmgAmt) +
				(Positive(item.BaneDmgAmt) * 0.25) +
				(Positive(item.BaneDmgRaceAmt) * 0.25);
			const double weapon_score = (weapon_damage * 10.0 / std::max<int>(item.Delay, 1)) * 12.0 * weapon_type_multiplier;
			AddFlatScore(score, "weapon", "damage_delay", weapon_score * weights.weapon);
		}

		const double effect_score =
			GetEffectScore(item.Worn, 12.0) +
			GetEffectScore(item.Focus, 18.0) +
			GetEffectScore(item.Proc, 10.0) +
			GetEffectScore(item.Click, 6.0) +
			GetEffectScore(item.Scroll, 4.0) +
			GetEffectScore(item.Bard, 14.0);
		AddFlatScore(score, "effects", "placeholder_effects", effect_score * weights.effects);

		double total = 0.0;
		for (const auto &component : score.components) {
			total += component.second;
		}

		score.total = RoundScore(total);
		return score;
	}

	uint16 CalculateItemLevel(uint32 item_score, double slot_budget, uint8 req_level, uint8 rec_level)
	{
		const double budget = std::max(slot_budget, 0.25);
		const double normalized_power = std::max(1.0, static_cast<double>(item_score) / budget);
		const auto computed_level = static_cast<uint16>(
			std::clamp<int>(
				static_cast<int>(std::lround(std::pow(normalized_power, 0.58))),
				1,
				127
			)
		);

		const auto rec_floor = static_cast<uint16>(std::ceil(static_cast<double>(rec_level) * 0.85));
		return std::max<uint16>({computed_level, req_level, rec_floor, 1});
	}

	EQ::ItemPower::Role BestRole(
		uint32 tank_score,
		uint32 melee_score,
		uint32 caster_score,
		uint32 healer_score,
		uint32 hybrid_score
	)
	{
		EQ::ItemPower::Role role = EQ::ItemPower::Role::Tank;
		uint32 best_score = tank_score;

		if (melee_score > best_score) {
			role = EQ::ItemPower::Role::Melee;
			best_score = melee_score;
		}

		if (caster_score > best_score) {
			role = EQ::ItemPower::Role::Caster;
			best_score = caster_score;
		}

		if (healer_score > best_score) {
			role = EQ::ItemPower::Role::Healer;
			best_score = healer_score;
		}

		if (hybrid_score > best_score) {
			role = EQ::ItemPower::Role::Hybrid;
		}

		return role;
	}

	uint32 ScoreForRole(const EQ::ItemPower::ScoreResult &score, EQ::ItemPower::Role role)
	{
		switch (role) {
			case EQ::ItemPower::Role::Tank:
				return score.tank_score;
			case EQ::ItemPower::Role::Melee:
				return score.melee_score;
			case EQ::ItemPower::Role::Caster:
				return score.caster_score;
			case EQ::ItemPower::Role::Healer:
				return score.healer_score;
			case EQ::ItemPower::Role::Hybrid:
				return score.hybrid_score;
		}

		return score.item_score;
	}

	void FillBreakdown(EQ::ItemPower::ScoreResult &score, const RoleScore &role_score)
	{
		const std::array<std::string, 6> order = {{
			"base_stats",
			"defense",
			"offense",
			"sustain",
			"weapon",
			"effects"
		}};

		score.breakdown.clear();
		for (const auto &component : order) {
			const auto score_iter = role_score.components.find(component);
			const int32 component_score = score_iter == role_score.components.end() ? 0 : static_cast<int32>(RoundScore(score_iter->second));

			std::string details;
			const auto detail_iter = role_score.details.find(component);
			if (detail_iter != role_score.details.end()) {
				details = Strings::Join(detail_iter->second, ", ");
			}

			score.breakdown.push_back(EQ::ItemPower::ScoreComponent{
				.component = component,
				.score = component_score,
				.details = details
			});

			if (component == "base_stats") {
				score.base_stat_score = component_score;
			}
			else if (component == "defense") {
				score.defense_score = component_score;
			}
			else if (component == "offense") {
				score.offense_score = component_score;
			}
			else if (component == "sustain") {
				score.sustain_score = component_score;
			}
			else if (component == "weapon") {
				score.weapon_score = component_score;
			}
			else if (component == "effects") {
				score.effect_score = component_score;
			}
		}
	}

	OverrideData GetOverride(Database &db, uint32 item_id)
	{
		OverrideData override_data;
		if (!item_id || !EQ::ItemPower::SchemaReady(db)) {
			return override_data;
		}

		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT `item_level_override`, `score_multiplier`, `flat_score_bonus` "
				"FROM `{}` WHERE `item_id` = {} LIMIT 1",
				kItemPowerOverrideTable,
				item_id
			)
		);

		if (!results.Success() || !results.RowCount()) {
			return override_data;
		}

		auto row = results.begin();
		override_data.found = true;
		if (row[0]) {
			override_data.has_level = true;
			override_data.item_level = static_cast<uint16>(Strings::ToUnsignedInt(row[0]));
		}

		if (row[1]) {
			override_data.has_multiplier = true;
			override_data.multiplier = Strings::ToFloat(row[1], 1.0f);
		}

		if (row[2]) {
			override_data.has_flat_bonus = true;
			override_data.flat_bonus = Strings::ToInt(row[2]);
		}

		return override_data;
	}

	void SetSchemaReady(bool ready)
	{
		schema_ready = ready ? 1 : 0;
	}

	bool UpsertOverrideColumn(Database &db, uint32 item_id, const std::string &assignment)
	{
		if (!item_id || !EQ::ItemPower::EnsureSchema(db)) {
			return false;
		}

		const auto results = db.QueryDatabase(
			fmt::format(
				"INSERT INTO `{}` (`item_id`) VALUES ({}) "
				"ON DUPLICATE KEY UPDATE {}",
				kItemPowerOverrideTable,
				item_id,
				assignment
			)
		);

		return results.Success();
	}
}

bool EQ::ItemPower::EnsureSchema(Database &db)
{
	const auto item_power = db.QueryDatabase(
		"CREATE TABLE IF NOT EXISTS `item_power` ("
		"`item_id` INT UNSIGNED NOT NULL,"
		"`item_level` SMALLINT UNSIGNED NOT NULL,"
		"`item_score` INT UNSIGNED NOT NULL,"
		"`tank_score` INT UNSIGNED NOT NULL DEFAULT 0,"
		"`melee_score` INT UNSIGNED NOT NULL DEFAULT 0,"
		"`caster_score` INT UNSIGNED NOT NULL DEFAULT 0,"
		"`healer_score` INT UNSIGNED NOT NULL DEFAULT 0,"
		"`hybrid_score` INT UNSIGNED NOT NULL DEFAULT 0,"
		"`score_version` SMALLINT UNSIGNED NOT NULL,"
		"`source` ENUM('computed', 'manual', 'generated') NOT NULL DEFAULT 'computed',"
		"`updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
		"PRIMARY KEY (`item_id`),"
		"INDEX `idx_item_power_level` (`item_level`),"
		"INDEX `idx_item_power_score_version` (`score_version`)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"
	);

	const auto override = db.QueryDatabase(
		"CREATE TABLE IF NOT EXISTS `item_power_override` ("
		"`item_id` INT UNSIGNED NOT NULL PRIMARY KEY,"
		"`item_level_override` SMALLINT UNSIGNED NULL,"
		"`score_multiplier` FLOAT NULL,"
		"`flat_score_bonus` INT NULL,"
		"`notes` TEXT NULL,"
		"`updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"
	);

	const auto breakdown = db.QueryDatabase(
		"CREATE TABLE IF NOT EXISTS `item_power_breakdown` ("
		"`item_id` INT UNSIGNED NOT NULL,"
		"`score_version` SMALLINT UNSIGNED NOT NULL,"
		"`component` VARCHAR(64) NOT NULL,"
		"`score` INT NOT NULL,"
		"`details` TEXT NULL,"
		"PRIMARY KEY (`item_id`, `score_version`, `component`),"
		"INDEX `idx_item_power_breakdown_version` (`score_version`)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"
	);

	const bool ready = item_power.Success() && override.Success() && breakdown.Success();
	SetSchemaReady(ready);
	return ready;
}

bool EQ::ItemPower::SchemaReady(Database &db)
{
	if (schema_ready != -1) {
		return schema_ready == 1;
	}

	const bool ready =
		db.DoesTableExist(kItemPowerTable) &&
		db.DoesTableExist(kItemPowerOverrideTable) &&
		db.DoesTableExist(kItemPowerBreakdownTable);
	SetSchemaReady(ready);
	return ready;
}

EQ::ItemPower::ScoreResult EQ::ItemPower::Calculate(const ItemData &item, bool include_breakdown)
{
	ScoreResult result;
	result.item_id = item.ID;
	result.score_version = ScoreVersion;

	const auto slot_budget = GetSlotBudget(item);
	result.slot_budget = slot_budget.budget;
	result.slot_budget_name = slot_budget.name;

	const auto tank = CalculateRoleScore(item, Role::Tank);
	const auto melee = CalculateRoleScore(item, Role::Melee);
	const auto caster = CalculateRoleScore(item, Role::Caster);
	const auto healer = CalculateRoleScore(item, Role::Healer);
	const auto hybrid = CalculateRoleScore(item, Role::Hybrid);

	result.tank_score = tank.total;
	result.melee_score = melee.total;
	result.caster_score = caster.total;
	result.healer_score = healer.total;
	result.hybrid_score = hybrid.total;
	result.best_role = BestRole(result.tank_score, result.melee_score, result.caster_score, result.healer_score, result.hybrid_score);
	result.item_score = ScoreForRole(result, result.best_role);
	result.item_level = CalculateItemLevel(result.item_score, result.slot_budget, item.ReqLevel, item.RecLevel);

	if (item.ReqLevel == 0 && item.RecLevel == 0 && result.item_level >= 20) {
		result.warnings.emplace_back("high score with no required or recommended level");
	}

	if (item.ItemType == EQ::item::ItemTypeCharm) {
		result.warnings.emplace_back("charm scoring is generic in V0");
	}

	if (
		item.Click.Effect > 0 ||
		item.Proc.Effect > 0 ||
		item.Worn.Effect > 0 ||
		item.Focus.Effect > 0 ||
		item.Scroll.Effect > 0 ||
		item.Bard.Effect > 0
	) {
		result.warnings.emplace_back("effect scoring uses V0 placeholder values");
	}

	if (include_breakdown) {
		switch (result.best_role) {
			case Role::Tank:
				FillBreakdown(result, tank);
				break;
			case Role::Melee:
				FillBreakdown(result, melee);
				break;
			case Role::Caster:
				FillBreakdown(result, caster);
				break;
			case Role::Healer:
				FillBreakdown(result, healer);
				break;
			case Role::Hybrid:
				FillBreakdown(result, hybrid);
				break;
		}
	}

	return result;
}

bool EQ::ItemPower::ApplyOverrides(Database &db, ScoreResult &score)
{
	const auto override_data = GetOverride(db, score.item_id);
	if (!override_data.found) {
		score.source = "computed";
		return true;
	}

	score.source = "manual";
	const auto original_item_level = score.item_level;

	if (override_data.has_multiplier && override_data.multiplier > 0.0f) {
		score.tank_score = RoundScore(score.tank_score * override_data.multiplier);
		score.melee_score = RoundScore(score.melee_score * override_data.multiplier);
		score.caster_score = RoundScore(score.caster_score * override_data.multiplier);
		score.healer_score = RoundScore(score.healer_score * override_data.multiplier);
		score.hybrid_score = RoundScore(score.hybrid_score * override_data.multiplier);
		score.breakdown.push_back(ScoreComponent{
			.component = "override",
			.score = 0,
			.details = fmt::format("score_multiplier={:.3f}", override_data.multiplier)
		});
	}

	if (override_data.has_flat_bonus && override_data.flat_bonus != 0) {
		const auto apply_flat_bonus = [&](uint32 value) -> uint32 {
			const int64 adjusted = static_cast<int64>(value) + override_data.flat_bonus;
			return adjusted <= 0 ? 0 : static_cast<uint32>(adjusted);
		};

		score.tank_score = apply_flat_bonus(score.tank_score);
		score.melee_score = apply_flat_bonus(score.melee_score);
		score.caster_score = apply_flat_bonus(score.caster_score);
		score.healer_score = apply_flat_bonus(score.healer_score);
		score.hybrid_score = apply_flat_bonus(score.hybrid_score);
		score.breakdown.push_back(ScoreComponent{
			.component = "override",
			.score = override_data.flat_bonus,
			.details = fmt::format("flat_score_bonus={}", override_data.flat_bonus)
		});
	}

	score.best_role = BestRole(score.tank_score, score.melee_score, score.caster_score, score.healer_score, score.hybrid_score);
	score.item_score = ScoreForRole(score, score.best_role);
	if (!override_data.has_level) {
		score.item_level = std::max<uint16>(original_item_level, CalculateItemLevel(score.item_score, score.slot_budget, 0, 0));
	}

	if (override_data.has_level) {
		score.item_level = override_data.item_level;
		score.breakdown.push_back(ScoreComponent{
			.component = "override",
			.score = override_data.item_level,
			.details = fmt::format("item_level_override={}", override_data.item_level)
		});
	}

	return true;
}

bool EQ::ItemPower::SaveScore(
	Database &db,
	const ScoreResult &score,
	std::string *error_message,
	bool save_breakdown,
	bool use_transaction
)
{
	if (!SchemaReady(db) && !EnsureSchema(db)) {
		if (error_message) {
			*error_message = "item_power schema is not ready";
		}
		return false;
	}

	if (use_transaction) {
		db.TransactionBegin();
	}

	const auto item_power = db.QueryDatabase(
		fmt::format(
			"INSERT INTO `{}` "
			"(`item_id`, `item_level`, `item_score`, `tank_score`, `melee_score`, `caster_score`, `healer_score`, `hybrid_score`, `score_version`, `source`, `updated_at`) "
			"VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, '{}', NOW()) "
			"ON DUPLICATE KEY UPDATE "
			"`item_level` = VALUES(`item_level`), "
			"`item_score` = VALUES(`item_score`), "
			"`tank_score` = VALUES(`tank_score`), "
			"`melee_score` = VALUES(`melee_score`), "
			"`caster_score` = VALUES(`caster_score`), "
			"`healer_score` = VALUES(`healer_score`), "
			"`hybrid_score` = VALUES(`hybrid_score`), "
			"`score_version` = VALUES(`score_version`), "
			"`source` = VALUES(`source`), "
			"`updated_at` = NOW()",
			kItemPowerTable,
			score.item_id,
			score.item_level,
			score.item_score,
			score.tank_score,
			score.melee_score,
			score.caster_score,
			score.healer_score,
			score.hybrid_score,
			score.score_version,
			score.source == "manual" || score.source == "generated" ? score.source : "computed"
		)
	);

	if (!item_power.Success()) {
		if (use_transaction) {
			db.TransactionRollback();
		}
		if (error_message) {
			*error_message = item_power.ErrorMessage();
		}
		return false;
	}

	if (save_breakdown) {
		const auto clear_breakdown = db.QueryDatabase(
			fmt::format(
				"DELETE FROM `{}` WHERE `item_id` = {} AND `score_version` = {}",
				kItemPowerBreakdownTable,
				score.item_id,
				score.score_version
			)
		);

		if (!clear_breakdown.Success()) {
			if (use_transaction) {
				db.TransactionRollback();
			}
			if (error_message) {
				*error_message = clear_breakdown.ErrorMessage();
			}
			return false;
		}

		for (const auto &component : score.breakdown) {
			const auto insert_breakdown = db.QueryDatabase(
				fmt::format(
					"INSERT INTO `{}` (`item_id`, `score_version`, `component`, `score`, `details`) "
					"VALUES ({}, {}, '{}', {}, '{}')",
					kItemPowerBreakdownTable,
					score.item_id,
					score.score_version,
					Strings::Escape(component.component),
					component.score,
					Strings::Escape(component.details)
				)
			);

			if (!insert_breakdown.Success()) {
				if (use_transaction) {
					db.TransactionRollback();
				}
				if (error_message) {
					*error_message = insert_breakdown.ErrorMessage();
				}
				return false;
			}
		}
	}

	if (use_transaction) {
		const auto commit = db.TransactionCommit();
		if (!commit.Success()) {
			if (error_message) {
				*error_message = commit.ErrorMessage();
			}
			return false;
		}
	}

	return true;
}

bool EQ::ItemPower::TryGetStoredScore(Database &db, uint32 item_id, StoredScore &score)
{
	if (!item_id || !SchemaReady(db)) {
		return false;
	}

	auto results = db.QueryDatabase(
		fmt::format(
			"SELECT `item_id`, `item_level`, `item_score`, `tank_score`, `melee_score`, "
			"`caster_score`, `healer_score`, `hybrid_score`, `score_version`, `source`, `updated_at` "
			"FROM `{}` WHERE `item_id` = {} LIMIT 1",
			kItemPowerTable,
			item_id
		)
	);

	if (!results.Success() || !results.RowCount()) {
		return false;
	}

	auto row = results.begin();
	score.item_id = row[0] ? Strings::ToUnsignedInt(row[0]) : 0;
	score.item_level = row[1] ? static_cast<uint16>(Strings::ToUnsignedInt(row[1])) : 1;
	score.item_score = row[2] ? Strings::ToUnsignedInt(row[2]) : 0;
	score.tank_score = row[3] ? Strings::ToUnsignedInt(row[3]) : 0;
	score.melee_score = row[4] ? Strings::ToUnsignedInt(row[4]) : 0;
	score.caster_score = row[5] ? Strings::ToUnsignedInt(row[5]) : 0;
	score.healer_score = row[6] ? Strings::ToUnsignedInt(row[6]) : 0;
	score.hybrid_score = row[7] ? Strings::ToUnsignedInt(row[7]) : 0;
	score.score_version = row[8] ? static_cast<uint16>(Strings::ToUnsignedInt(row[8])) : ScoreVersion;
	score.source = row[9] ? row[9] : "";
	score.updated_at = row[10] ? row[10] : "";

	return score.item_id != 0;
}

bool EQ::ItemPower::SetLevelOverride(Database &db, uint32 item_id, uint16 item_level)
{
	return UpsertOverrideColumn(
		db,
		item_id,
		fmt::format("`item_level_override` = {}", item_level)
	);
}

bool EQ::ItemPower::SetMultiplierOverride(Database &db, uint32 item_id, float multiplier)
{
	return UpsertOverrideColumn(
		db,
		item_id,
		fmt::format("`score_multiplier` = {:.6f}", multiplier)
	);
}

bool EQ::ItemPower::SetFlatBonusOverride(Database &db, uint32 item_id, int32 flat_bonus)
{
	return UpsertOverrideColumn(
		db,
		item_id,
		fmt::format("`flat_score_bonus` = {}", flat_bonus)
	);
}

bool EQ::ItemPower::SetOverrideNotes(Database &db, uint32 item_id, const std::string &notes)
{
	return UpsertOverrideColumn(
		db,
		item_id,
		fmt::format("`notes` = '{}'", Strings::Escape(notes))
	);
}

bool EQ::ItemPower::ClearOverride(Database &db, uint32 item_id)
{
	if (!item_id || !SchemaReady(db)) {
		return false;
	}

	const auto results = db.QueryDatabase(
		fmt::format(
			"DELETE FROM `{}` WHERE `item_id` = {}",
			kItemPowerOverrideTable,
			item_id
		)
	);

	return results.Success();
}

const char *EQ::ItemPower::RoleName(Role role)
{
	switch (role) {
		case Role::Tank:
			return "Tank";
		case Role::Melee:
			return "Melee DPS";
		case Role::Caster:
			return "Caster DPS";
		case Role::Healer:
			return "Healer";
		case Role::Hybrid:
			return "Hybrid";
	}

	return "Unknown";
}

const char *EQ::ItemPower::RoleKey(Role role)
{
	switch (role) {
		case Role::Tank:
			return "tank";
		case Role::Melee:
			return "melee";
		case Role::Caster:
			return "caster";
		case Role::Healer:
			return "healer";
		case Role::Hybrid:
			return "hybrid";
	}

	return "unknown";
}

EQ::ItemPower::Role EQ::ItemPower::BestRoleFromScores(const StoredScore &score)
{
	return BestRole(score.tank_score, score.melee_score, score.caster_score, score.healer_score, score.hybrid_score);
}

std::string EQ::ItemPower::BuildTransportMessage(const ItemData &item, const ScoreResult &score)
{
	return fmt::format(
		"ITEMPOWER|set|item_id={}|ilvl={}|score={}|role={}|version={}|source={}|name={}",
		item.ID,
		score.item_level,
		score.item_score,
		score.item_score > 0 ? RoleKey(score.best_role) : "none",
		score.score_version,
		score.source,
		item.Name
	);
}

std::string EQ::ItemPower::BuildTransportMessage(const ItemData &item, const StoredScore &score)
{
	return fmt::format(
		"ITEMPOWER|set|item_id={}|ilvl={}|score={}|role={}|version={}|source={}|name={}",
		item.ID,
		score.item_level,
		score.item_score,
		score.item_score > 0 ? RoleKey(BestRoleFromScores(score)) : "none",
		score.score_version,
		score.source,
		item.Name
	);
}

bool EQ::ItemPower::TryBuildTransportMessage(Database &db, const ItemData &item, std::string &message, bool calculate_if_missing)
{
	StoredScore score;
	if (!TryGetStoredScore(db, item.ID, score)) {
		if (!calculate_if_missing) {
			return false;
		}

		if (!SchemaReady(db) && !EnsureSchema(db)) {
			return false;
		}

		auto calculated_score = Calculate(item, false);
		if (!ApplyOverrides(db, calculated_score)) {
			return false;
		}

		std::string error_message;
		if (!SaveScore(db, calculated_score, &error_message, false)) {
			return false;
		}

		message = BuildTransportMessage(item, calculated_score);
		return true;
	}

	message = BuildTransportMessage(item, score);
	return true;
}

bool EQ::ItemPower::TryBuildStoredTransportMessage(Database &db, const ItemData &item, std::string &message)
{
	return TryBuildTransportMessage(db, item, message, false);
}

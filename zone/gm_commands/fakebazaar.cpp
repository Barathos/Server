#include "../client.h"
#include "../thj_fake_bazaar.h"
#include "../../common/eq_constants.h"
#include "../../common/mysql_request_row.h"
#include "../../common/mysql_request_result.h"
#include "../../common/repositories/buyer_buy_lines_repository.h"
#include "../../common/repositories/buyer_repository.h"
#include "../../common/repositories/buyer_trade_items_repository.h"
#include "../../common/repositories/character_data_repository.h"
#include "../../common/repositories/trader_repository.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

constexpr const char *THJ_FAKE_BAZAAR_SELLER_PREFIX = "Thjvendor";
constexpr const char *THJ_FAKE_BAZAAR_BUYER_PREFIX  = "Thjbounty";
constexpr uint32      THJ_FAKE_BAZAAR_ZONE_ID       = Zones::BAZAAR;
constexpr uint32      THJ_FAKE_BAZAAR_SERIAL_BASE   = 2000000000;
constexpr uint64      THJ_FAKE_BAZAAR_MAX_PRICE     = 2000000000;
constexpr uint32      THJ_FAKE_BAZAAR_MAX_PER_TRADER = 200;
constexpr const char *THJ_FAKE_BAZAAR_CONFIG_PREFIX  = "THJFB.";
constexpr const char *THJ_FAKE_BAZAAR_LEGACY_CONFIG_PREFIX = "THJFakeBazaar.";
constexpr const char *THJ_FAKE_BAZAAR_ITEM_POOL_TABLE = "thj_fake_bazaar_item_pool";
constexpr size_t      THJ_FAKE_BAZAAR_VARIABLE_NAME_LIMIT = 25;
constexpr const char *THJ_FAKE_BAZAAR_DEFAULT_ITEM_TYPES =
	"0,1,2,3,4,5,8,10,45";
constexpr uint32      THJ_FAKE_BAZAAR_DEFAULT_MAX_ITEM_LEVEL = 65;
constexpr const char *THJ_FAKE_BAZAAR_DEFAULT_EXCLUDED_NAME_PATTERNS =
	"Deprecated%|%Test%|%Defiant%|%Abstruse%|%August %|%Boundless%|"
	"%Castaway%|%Cosgrove%|%Crystalweave%|%Duskwoven%|%Ethermere%|%Fused Coral%|%Glacier Giant%|"
	"%Glorious%|%Illustrious%|%Numinous%|%Regal %|%Shadowspin%|%Snowborn%|%Starglow%|%Transcendent%";
constexpr uint32      THJ_FAKE_BAZAAR_ENCHANTED_ITEM_ID_OFFSET = 1000000;
constexpr uint32      THJ_FAKE_BAZAAR_LEGENDARY_ITEM_ID_OFFSET = 2000000;

struct THJFakeBazaarItem {
	uint32      id;
	std::string name;
	uint32      icon;
	uint32      price;
	uint32      ldon_price;
	bool        stackable;
	uint32      stack_size;
	int32       max_charges;
	uint32      item_class;
	uint32      item_type;
	uint32      slots;
	uint32      bag_slots;
	int32       ac;
	int32       hp;
	int32       mana;
	int32       damage;
	int32       rec_level;
	int32       req_level;
	int32       scroll_effect;
	int32       focus_effect;
	uint32      delay;
	int32       endur;
	int32       primary_stats;
	int32       resists;
	int32       heroic_stats;
	int32       attack;
	int32       regen;
	int32       mana_regen;
	int32       endurance_regen;
	int32       haste;
	int32       damage_shield;
	int32       proc_effect;
	int32       click_effect;
	int32       worn_effect;
	int32       bard_effect;
	uint32      elemental_damage;
	uint32      backstab_damage;
	int32       spell_damage;
	int32       heal_amount;
	int32       combat_effects;
	int32       shielding;
	int32       avoidance;
	int32       accuracy;
	uint32      bag_weight_reduction;
};

struct THJFakeBazaarSeedSettings {
	uint32 seller_items;
	uint32 buyer_lines;
	uint32 seller_count;
	uint32 buyer_count;
	uint32 seller_min_multiplier;
	uint32 seller_max_multiplier;
	uint32 buyer_min_multiplier;
	uint32 buyer_max_multiplier;
	uint32 min_expansion;
	uint32 max_expansion;
	uint32 drops_per_zone;
};

struct THJFakeBazaarPricingSettings {
	uint32 min_base_price;
	uint32 max_price;
	uint32 vendor_weight_percent;
	uint32 level_weight;
	uint32 spell_level_weight;
	uint32 ac_weight;
	uint32 stat_weight;
	uint32 heroic_stat_weight;
	uint32 hp_weight;
	uint32 mana_weight;
	uint32 endurance_weight;
	uint32 resist_weight;
	uint32 attack_weight;
	uint32 regen_weight;
	uint32 haste_weight;
	uint32 damage_shield_weight;
	uint32 spell_damage_weight;
	uint32 heal_amount_weight;
	uint32 mod2_weight;
	uint32 weapon_ratio_weight;
	uint32 weapon_damage_weight;
	uint32 elemental_damage_weight;
	uint32 backstab_damage_weight;
	uint32 bag_slot_weight;
	uint32 bag_weight_reduction_weight;
	uint32 family_upgrade_value_percent;
	uint32 high_impact_premium_max_percent;
	uint32 high_impact_spell_damage_weight;
	uint32 high_impact_heal_amount_weight;
	uint32 high_impact_haste_weight;
	uint32 high_impact_heroic_stat_weight;
	uint32 high_impact_weapon_ratio_weight;
	uint32 high_impact_weapon_damage_weight;
	uint32 buyer_min_base_price;
	uint32 buyer_max_price;
	uint32 buyer_vendor_weight_percent;
	uint32 buyer_bag_slot_weight;
	uint32 buyer_bag_weight_reduction_weight;
	uint32 buyer_stackable_max_price;
	uint32 buyer_mundane_max_price;
	uint32 proc_effect_min_bonus;
	uint32 proc_effect_max_bonus;
	uint32 click_effect_min_bonus;
	uint32 click_effect_max_bonus;
	uint32 worn_effect_min_bonus;
	uint32 worn_effect_max_bonus;
	uint32 focus_effect_min_bonus;
	uint32 focus_effect_max_bonus;
	uint32 bard_effect_min_bonus;
	uint32 bard_effect_max_bonus;
};

struct THJFakeBazaarConfig {
	THJFakeBazaarSeedSettings    seed;
	THJFakeBazaarPricingSettings pricing;
	uint32                       min_item_level;
	uint32                       max_item_level;
	uint32                       allow_global_fallback;
	uint32                       exclude_tradeskill_items;
	uint32                       exclude_merchant_items;
	uint32                       exclude_spell_scrolls;
	uint32                       require_equipment_stats;
	uint32                       minimum_equipment_stat_total;
	uint32                       include_item_variants;
	uint32                       use_cached_item_pool;
	uint32                       allow_live_item_scan;
	uint32                       auto_refresh_enabled;
	uint32                       refresh_on_startup;
	uint32                       refresh_when_empty;
	uint32                       refresh_interval_minutes;
	uint32                       minimum_seller_listings;
	uint32                       minimum_buyer_lines;
	std::string                  allowed_item_types;
	std::string                  included_zones;
	std::string                  excluded_zones;
	std::string                  excluded_name_patterns;
};

struct THJFakeBazaarClearResult {
	int traders;
	int buyer_trade_items;
	int buyer_lines;
	int buyers;
};

struct THJFakeBazaarZoneDropCandidate {
	uint32             zone_id;
	THJFakeBazaarItem item;
};

struct THJFakeBazaarItemPricingContext {
	uint64 exact_base_value = 0;
	uint64 family_base_value = 0;
	uint64 high_impact_value = 0;
	uint64 family_high_impact_value = 0;
	uint32 family_base_item_id = 0;
};

static uint32 THJFakeBazaarRowUInt(const char *value)
{
	return value ? static_cast<uint32>(strtoul(value, nullptr, 10)) : 0;
}

static int32 THJFakeBazaarRowInt(const char *value)
{
	return value ? static_cast<int32>(atoi(value)) : 0;
}

static uint32 THJFakeBazaarPpToCopper(uint32 platinum)
{
	if (!platinum) {
		return 0;
	}

	return static_cast<uint32>(std::clamp<uint64>(
		static_cast<uint64>(platinum) * 1000,
		1,
		THJ_FAKE_BAZAAR_MAX_PRICE
	));
}

static uint32 THJFakeBazaarConfigKeyHash(const std::string &key)
{
	uint32 hash = 2166136261u;
	for (const auto c : key) {
		hash ^= static_cast<uint8>(std::tolower(static_cast<unsigned char>(c)));
		hash *= 16777619u;
	}

	return hash;
}

static std::string THJFakeBazaarConfigVariableName(const std::string &key)
{
	const auto readable_name = fmt::format("{}{}", THJ_FAKE_BAZAAR_CONFIG_PREFIX, key);
	if (readable_name.size() <= THJ_FAKE_BAZAAR_VARIABLE_NAME_LIMIT) {
		return readable_name;
	}

	return fmt::format(
		"{}{:.10}.{:08X}",
		THJ_FAKE_BAZAAR_CONFIG_PREFIX,
		key,
		THJFakeBazaarConfigKeyHash(key)
	);
}

static std::string THJFakeBazaarLegacyConfigVariableName(const std::string &key)
{
	return fmt::format("{}{}", THJ_FAKE_BAZAAR_LEGACY_CONFIG_PREFIX, key);
}

static bool THJFakeBazaarConfigVariableExists(const std::string &variable_name, std::string &value)
{
	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `value` FROM `variables` WHERE `varname` = '{}' LIMIT 1",
			Strings::Escape(variable_name)
		)
	);

	if (!results.Success() || !results.RowCount()) {
		return false;
	}

	auto row = results.begin();
	value = row[0] ? row[0] : "";
	return true;
}

static bool THJFakeBazaarConfigValueExists(const std::string &key, std::string &value)
{
	const auto variable_name = THJFakeBazaarConfigVariableName(key);
	if (THJFakeBazaarConfigVariableExists(variable_name, value)) {
		return true;
	}

	const auto legacy_variable_name = THJFakeBazaarLegacyConfigVariableName(key);
	if (
		legacy_variable_name.size() <= THJ_FAKE_BAZAAR_VARIABLE_NAME_LIMIT &&
		legacy_variable_name != variable_name
	) {
		return THJFakeBazaarConfigVariableExists(legacy_variable_name, value);
	}

	return false;
}

static std::string THJFakeBazaarConfigString(const std::string &key, const std::string &default_value)
{
	std::string value;
	return THJFakeBazaarConfigValueExists(key, value) ? value : default_value;
}

static uint32 THJFakeBazaarConfigUInt(
	const std::string &key,
	uint32 default_value,
	uint32 min_value,
	uint32 max_value
)
{
	std::string value;
	if (!THJFakeBazaarConfigValueExists(key, value)) {
		return default_value;
	}

	return std::clamp(Strings::ToUnsignedInt(value, default_value), min_value, max_value);
}

static uint32 THJFakeBazaarConfigPp(
	const std::string &key,
	uint32 default_value,
	uint32 min_value,
	uint32 max_value
)
{
	return THJFakeBazaarPpToCopper(THJFakeBazaarConfigUInt(key, default_value, min_value, max_value));
}

static uint32 THJFakeBazaarClampedArg(
	const Seperator *sep,
	int arg_index,
	uint32 default_value,
	uint32 min_value,
	uint32 max_value
)
{
	if (sep->argnum < arg_index || !sep->IsNumber(arg_index)) {
		return default_value;
	}

	return std::clamp(Strings::ToUnsignedInt(sep->arg[arg_index], default_value), min_value, max_value);
}

static std::vector<std::string> THJFakeBazaarSplitConfigList(std::string value)
{
	std::replace(value.begin(), value.end(), ';', ',');
	std::replace(value.begin(), value.end(), '|', ',');

	std::vector<std::string> entries;
	for (auto entry : Strings::Split(value, ',')) {
		entry = Strings::Trim(entry);
		if (!entry.empty()) {
			entries.push_back(entry);
		}
	}

	return entries;
}

static std::string THJFakeBazaarRemoveNamePatterns(
	const std::string &patterns,
	const std::vector<std::string> &removed_patterns
)
{
	std::vector<std::string> kept_patterns;
	for (const auto &pattern : THJFakeBazaarSplitConfigList(patterns)) {
		bool remove_pattern = false;
		for (const auto &removed_pattern : removed_patterns) {
			if (Strings::EqualFold(pattern, removed_pattern)) {
				remove_pattern = true;
				break;
			}
		}

		if (!remove_pattern) {
			kept_patterns.push_back(pattern);
		}
	}

	return Strings::Implode("|", kept_patterns);
}

static std::string THJFakeBazaarUIntListSql(const std::string &value, const std::string &default_value)
{
	std::vector<std::string> safe_entries;
	for (const auto &entry : THJFakeBazaarSplitConfigList(value.empty() ? default_value : value)) {
		bool is_number = !entry.empty();
		for (const auto c : entry) {
			if (!std::isdigit(static_cast<unsigned char>(c))) {
				is_number = false;
				break;
			}
		}

		if (is_number) {
			safe_entries.push_back(std::to_string(Strings::ToUnsignedInt(entry)));
		}
	}

	return safe_entries.empty() ? default_value : Strings::Implode(", ", safe_entries);
}

static std::string THJFakeBazaarZoneListSql(const std::string &value)
{
	std::vector<std::string> safe_entries;
	for (const auto &entry : THJFakeBazaarSplitConfigList(value)) {
		bool safe = !entry.empty();
		for (const auto c : entry) {
			if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
				safe = false;
				break;
			}
		}

		if (safe) {
			safe_entries.push_back(fmt::format("'{}'", Strings::Escape(entry)));
		}
	}

	return Strings::Implode(", ", safe_entries);
}

static std::string THJFakeBazaarNamePatternSql(const std::string &table_alias, const std::string &patterns)
{
	std::vector<std::string> clauses;
	const auto name_column = table_alias.empty() ? "Name" : fmt::format("{}.Name", table_alias);
	for (const auto &pattern : THJFakeBazaarSplitConfigList(patterns)) {
		clauses.push_back(fmt::format(
			"AND {0} NOT LIKE '{1}'",
			name_column,
			Strings::Escape(pattern)
		));
	}

	return Strings::Implode("\n\t\t\t\t\t", clauses);
}

static std::string THJFakeBazaarItemVariantJoinSql(const THJFakeBazaarConfig &config)
{
	if (!config.include_item_variants) {
		return "i.id = base_i.id";
	}

	return fmt::format(
		"i.id IN (base_i.id, base_i.id + {}, base_i.id + {})",
		THJ_FAKE_BAZAAR_ENCHANTED_ITEM_ID_OFFSET,
		THJ_FAKE_BAZAAR_LEGENDARY_ITEM_ID_OFFSET
	);
}

static std::string THJFakeBazaarLevelSql(const std::string &table_alias, uint32 min_item_level, uint32 max_item_level)
{
	std::vector<std::string> clauses;
	if (min_item_level) {
		clauses.push_back(fmt::format(
			"AND GREATEST({0}.reclevel, {0}.reqlevel) >= {1}",
			table_alias,
			min_item_level
		));
	}

	if (max_item_level) {
		clauses.push_back(fmt::format(
			"AND ({0}.reclevel = 0 OR {0}.reclevel <= {1}) AND ({0}.reqlevel = 0 OR {0}.reqlevel <= {1})",
			table_alias,
			max_item_level
		));
	}

	return Strings::Implode("\n\t\t\t\t\t", clauses);
}

static std::string THJFakeBazaarRecipeExpansionSql(const std::string &table_alias, const THJFakeBazaarSeedSettings &settings)
{
	const auto item_id_column = table_alias.empty() ? "id" : fmt::format("{}.id", table_alias);
	return fmt::format(
		R"(AND NOT EXISTS (
						SELECT 1
						FROM tradeskill_recipe AS tr
						WHERE tr.learned_by_item_id = {0}
							AND tr.enabled <> 0
							AND (
								(tr.min_expansion <> -1 AND tr.min_expansion > {1})
								OR (tr.max_expansion <> -1 AND tr.max_expansion < {2})
							)
					))",
		item_id_column,
		settings.max_expansion,
		settings.min_expansion
	);
}

static std::string THJFakeBazaarTradeskillSql(const std::string &table_alias, const THJFakeBazaarConfig &config)
{
	if (!config.exclude_tradeskill_items) {
		return "";
	}

	const auto prefix = table_alias.empty() ? "" : fmt::format("{}.", table_alias);
	return fmt::format(
		"AND ({0}tradeskills = 0 OR {0}slots <> 0 OR {0}bagslots > 0 OR {0}scrolleffect BETWEEN 1 AND 64999 OR {0}focuseffect > 0)",
		prefix
	);
}

static std::string THJFakeBazaarMerchantListSql(const std::string &table_alias, const THJFakeBazaarConfig &config)
{
	if (!config.exclude_merchant_items) {
		return "";
	}

	const auto item_id_column = table_alias.empty() ? "id" : fmt::format("{}.id", table_alias);
	return fmt::format(
		R"(AND NOT EXISTS (
						SELECT 1
						FROM merchantlist AS ml
						WHERE ml.item = {0}
					))",
		item_id_column
	);
}

static std::string THJFakeBazaarSpellScrollSql(const std::string &table_alias, const THJFakeBazaarConfig &config)
{
	if (!config.exclude_spell_scrolls) {
		return "";
	}

	const auto prefix = table_alias.empty() ? "" : fmt::format("{}.", table_alias);
	return fmt::format(
		"AND {0}itemtype <> 20 AND ({0}scrolleffect <= 0 OR {0}scrolleffect >= 65000)",
		prefix
	);
}

static std::string THJFakeBazaarEquipmentStatsSql(const std::string &table_alias, const THJFakeBazaarConfig &config)
{
	if (!config.require_equipment_stats) {
		return "";
	}

	const auto prefix = table_alias.empty() ? "" : fmt::format("{}.", table_alias);
	return fmt::format(
		R"(AND (
						{0}slots = 0
						OR {0}bagslots > 0
						OR (
							GREATEST(COALESCE({0}ac, 0), 0)
							+ GREATEST(COALESCE({0}hp, 0), 0)
							+ GREATEST(COALESCE({0}mana, 0), 0)
							+ GREATEST(COALESCE({0}endur, 0), 0)
							+ GREATEST(COALESCE({0}astr, 0), 0)
							+ GREATEST(COALESCE({0}asta, 0), 0)
							+ GREATEST(COALESCE({0}aagi, 0), 0)
							+ GREATEST(COALESCE({0}adex, 0), 0)
							+ GREATEST(COALESCE({0}acha, 0), 0)
							+ GREATEST(COALESCE({0}aint, 0), 0)
							+ GREATEST(COALESCE({0}awis, 0), 0)
							+ GREATEST(COALESCE({0}cr, 0), 0)
							+ GREATEST(COALESCE({0}dr, 0), 0)
							+ GREATEST(COALESCE({0}pr, 0), 0)
							+ GREATEST(COALESCE({0}mr, 0), 0)
							+ GREATEST(COALESCE({0}fr, 0), 0)
							+ GREATEST(COALESCE({0}svcorruption, 0), 0)
							+ GREATEST(COALESCE({0}heroic_str, 0), 0)
							+ GREATEST(COALESCE({0}heroic_int, 0), 0)
							+ GREATEST(COALESCE({0}heroic_wis, 0), 0)
							+ GREATEST(COALESCE({0}heroic_agi, 0), 0)
							+ GREATEST(COALESCE({0}heroic_dex, 0), 0)
							+ GREATEST(COALESCE({0}heroic_sta, 0), 0)
							+ GREATEST(COALESCE({0}heroic_cha, 0), 0)
							+ GREATEST(COALESCE({0}attack, 0), 0)
							+ GREATEST(COALESCE({0}regen, 0), 0)
							+ GREATEST(COALESCE({0}manaregen, 0), 0)
							+ GREATEST(COALESCE({0}enduranceregen, 0), 0)
							+ GREATEST(COALESCE({0}haste, 0), 0)
							+ GREATEST(COALESCE({0}damageshield, 0), 0)
							+ GREATEST(COALESCE({0}spelldmg, 0), 0)
							+ GREATEST(COALESCE({0}healamt, 0), 0)
							+ GREATEST(COALESCE({0}combateffects, 0), 0)
							+ GREATEST(COALESCE({0}shielding, 0), 0)
							+ GREATEST(COALESCE({0}avoidance, 0), 0)
							+ GREATEST(COALESCE({0}accuracy, 0), 0)
							+ GREATEST(COALESCE({0}elemdmgamt, 0), 0)
							+ GREATEST(COALESCE({0}backstabdmg, 0), 0)
							+ CASE WHEN {0}proceffect BETWEEN 1 AND 64999 THEN {1} ELSE 0 END
							+ CASE WHEN {0}clickeffect BETWEEN 1 AND 64999 THEN {1} ELSE 0 END
							+ CASE WHEN {0}worneffect BETWEEN 1 AND 64999 THEN {1} ELSE 0 END
							+ CASE WHEN {0}focuseffect BETWEEN 1 AND 64999 THEN {1} ELSE 0 END
							+ CASE WHEN {0}bardeffect BETWEEN 1 AND 64999 THEN {1} ELSE 0 END
						) >= {1}
					))",
		prefix,
		config.minimum_equipment_stat_total
	);
}

static std::string THJFakeBazaarZoneSql(const THJFakeBazaarConfig &config)
{
	std::vector<std::string> clauses;
	const auto included_zones = THJFakeBazaarZoneListSql(config.included_zones);
	if (!included_zones.empty()) {
		clauses.push_back(fmt::format("AND z.short_name IN ({})", included_zones));
	}

	const auto excluded_zones = THJFakeBazaarZoneListSql(config.excluded_zones);
	if (!excluded_zones.empty()) {
		clauses.push_back(fmt::format("AND z.short_name NOT IN ({})", excluded_zones));
	}

	return Strings::Implode("\n\t\t\t\t\t", clauses);
}

static std::string THJFakeBazaarPoolZoneSql(const THJFakeBazaarConfig &config)
{
	std::vector<std::string> clauses;
	const auto included_zones = THJFakeBazaarZoneListSql(config.included_zones);
	if (!included_zones.empty()) {
		clauses.push_back(fmt::format("AND p.zone_short_name IN ({})", included_zones));
	}

	const auto excluded_zones = THJFakeBazaarZoneListSql(config.excluded_zones);
	if (!excluded_zones.empty()) {
		clauses.push_back(fmt::format("AND p.zone_short_name NOT IN ({})", excluded_zones));
	}

	return Strings::Implode("\n\t\t\t\t\t", clauses);
}

static THJFakeBazaarConfig THJFakeBazaarLoadConfig()
{
	THJFakeBazaarConfig config{};

	config.seed.seller_items          = THJFakeBazaarConfigUInt("SellerItems", 400, 1, 5000);
	config.seed.buyer_lines           = THJFakeBazaarConfigUInt("BuyerLines", 120, 0, 1000);
	config.seed.seller_count          = THJFakeBazaarConfigUInt("SellerCount", 12, 1, 80);
	config.seed.buyer_count           = THJFakeBazaarConfigUInt("BuyerCount", 6, 0, 80);
	config.seed.seller_min_multiplier = THJFakeBazaarConfigUInt("SellerMinMultiplier", 2, 1, 100);
	config.seed.seller_max_multiplier = THJFakeBazaarConfigUInt("SellerMaxMultiplier", 12, 1, 100);
	config.seed.buyer_min_multiplier  = THJFakeBazaarConfigUInt("BuyerMinMultiplier", 2, 1, 200);
	config.seed.buyer_max_multiplier  = THJFakeBazaarConfigUInt("BuyerMaxMultiplier", 8, 1, 200);
	config.seed.min_expansion         = THJFakeBazaarConfigUInt("MinExpansion", 0, 0, 30);
	config.seed.max_expansion         = THJFakeBazaarConfigUInt("MaxExpansion", 4, 0, 30);
	config.seed.drops_per_zone        = THJFakeBazaarConfigUInt("DropsPerZone", 2, 1, 20);

	config.min_item_level             = THJFakeBazaarConfigUInt("MinItemLevel", 0, 0, 255);
	config.max_item_level             = THJFakeBazaarConfigUInt("MaxItemLevel", THJ_FAKE_BAZAAR_DEFAULT_MAX_ITEM_LEVEL, 0, 255);
	config.allow_global_fallback      = THJFakeBazaarConfigUInt("AllowGlobalFallback", 0, 0, 1);
	config.exclude_tradeskill_items   = THJFakeBazaarConfigUInt("ExcludeTradeskillItems", 1, 0, 1);
	config.exclude_merchant_items     = THJFakeBazaarConfigUInt("ExcludeMerchantItems", 1, 0, 1);
	config.exclude_spell_scrolls      = THJFakeBazaarConfigUInt("ExcludeSpellScrolls", 1, 0, 1);
	config.require_equipment_stats    = THJFakeBazaarConfigUInt("RequireEquipmentStats", 1, 0, 1);
	config.minimum_equipment_stat_total = THJFakeBazaarConfigUInt("MinimumEquipmentStatTotal", 1, 0, 1000);
	config.include_item_variants      = THJFakeBazaarConfigUInt("IncludeItemVariants", 1, 0, 1);
	config.use_cached_item_pool       = THJFakeBazaarConfigUInt("UseCachedItemPool", 1, 0, 1);
	config.allow_live_item_scan       = THJFakeBazaarConfigUInt("AllowLiveItemScan", 0, 0, 1);
	config.auto_refresh_enabled       = THJFakeBazaarConfigUInt("AutoRefreshEnabled", 1, 0, 1);
	config.refresh_on_startup         = THJFakeBazaarConfigUInt("RefreshOnStartup", 1, 0, 1);
	config.refresh_when_empty         = THJFakeBazaarConfigUInt("RefreshWhenEmpty", 1, 0, 1);
	config.refresh_interval_minutes   = THJFakeBazaarConfigUInt("RefreshIntervalMinutes", 360, 0, 43200);
	config.minimum_seller_listings    = THJFakeBazaarConfigUInt("MinimumSellerListings", 1, 0, 5000);
	config.minimum_buyer_lines        = THJFakeBazaarConfigUInt("MinimumBuyerLines", 1, 0, 1000);
	config.allowed_item_types         = THJFakeBazaarConfigString("AllowedItemTypes", THJ_FAKE_BAZAAR_DEFAULT_ITEM_TYPES);
	config.included_zones             = THJFakeBazaarConfigString("IncludedZones", "");
	config.excluded_zones             = THJFakeBazaarConfigString("ExcludedZones", "");
	config.excluded_name_patterns     = THJFakeBazaarConfigString("ExcludedNamePatterns", THJ_FAKE_BAZAAR_DEFAULT_EXCLUDED_NAME_PATTERNS);
	if (config.include_item_variants) {
		config.excluded_name_patterns = THJFakeBazaarRemoveNamePatterns(
			config.excluded_name_patterns,
			{"%(Enchanted)", "%(Legendary)"}
		);
	}

	config.pricing.min_base_price              = THJFakeBazaarConfigPp("MinBasePricePP", 5, 1, 2000000);
	config.pricing.max_price                   = THJFakeBazaarConfigPp("MaxPricePP", 2000000, 1, 2000000);
	config.pricing.vendor_weight_percent       = THJFakeBazaarConfigUInt("VendorWeightPercent", 100, 0, 1000);
	config.pricing.level_weight                = THJFakeBazaarConfigUInt("LevelWeightCopper", 1000, 0, 10000000);
	config.pricing.spell_level_weight          = THJFakeBazaarConfigUInt("SpellLevelWeightCopper", 5000, 0, 10000000);
	config.pricing.ac_weight                   = THJFakeBazaarConfigUInt("ACWeightCopper", 4000, 0, 10000000);
	config.pricing.stat_weight                 = THJFakeBazaarConfigUInt("StatWeightCopper", 3000, 0, 10000000);
	config.pricing.heroic_stat_weight          = THJFakeBazaarConfigUInt("HeroicStatWeightCopper", 15000, 0, 10000000);
	config.pricing.hp_weight                   = THJFakeBazaarConfigUInt("HPWeightCopper", 200, 0, 10000000);
	config.pricing.mana_weight                 = THJFakeBazaarConfigUInt("ManaWeightCopper", 200, 0, 10000000);
	config.pricing.endurance_weight            = THJFakeBazaarConfigUInt("EnduranceWeightCopper", 100, 0, 10000000);
	config.pricing.resist_weight               = THJFakeBazaarConfigUInt("ResistWeightCopper", 1500, 0, 10000000);
	config.pricing.attack_weight               = THJFakeBazaarConfigUInt("AttackWeightCopper", 3000, 0, 10000000);
	config.pricing.regen_weight                = THJFakeBazaarConfigUInt("RegenWeightCopper", 25000, 0, 10000000);
	config.pricing.haste_weight                = THJFakeBazaarConfigUInt("HasteWeightCopper", 6000, 0, 10000000);
	config.pricing.damage_shield_weight        = THJFakeBazaarConfigUInt("DamageShieldWeightCopper", 15000, 0, 10000000);
	config.pricing.spell_damage_weight         = THJFakeBazaarConfigUInt("SpellDamageWeightCopper", 8000, 0, 10000000);
	config.pricing.heal_amount_weight          = THJFakeBazaarConfigUInt("HealAmountWeightCopper", 8000, 0, 10000000);
	config.pricing.mod2_weight                 = THJFakeBazaarConfigUInt("Mod2WeightCopper", 7000, 0, 10000000);
	config.pricing.weapon_ratio_weight         = THJFakeBazaarConfigUInt("WeaponRatioWeightCopper", 5000, 0, 10000000);
	config.pricing.weapon_damage_weight        = THJFakeBazaarConfigUInt("WeaponDamageWeightCopper", 2000, 0, 10000000);
	config.pricing.elemental_damage_weight     = THJFakeBazaarConfigUInt("ElementalDamageWeightCopper", 10000, 0, 10000000);
	config.pricing.backstab_damage_weight      = THJFakeBazaarConfigUInt("BackstabDamageWeightCopper", 3000, 0, 10000000);
	config.pricing.bag_slot_weight             = THJFakeBazaarConfigUInt("BagSlotWeightCopper", 50000, 0, 10000000);
	config.pricing.bag_weight_reduction_weight = THJFakeBazaarConfigUInt("BagWeightReductionWeightCopper", 1000, 0, 10000000);
	config.pricing.family_upgrade_value_percent = THJFakeBazaarConfigUInt("FamilyUpgradeValuePercent", 75, 0, 100);
	config.pricing.high_impact_premium_max_percent = THJFakeBazaarConfigUInt("HighImpactPremiumMaxPercent", 100, 0, 300);
	config.pricing.high_impact_spell_damage_weight = THJFakeBazaarConfigUInt("HighImpactSpellDamageWeightCopper", 50000, 0, 10000000);
	config.pricing.high_impact_heal_amount_weight = THJFakeBazaarConfigUInt("HighImpactHealAmountWeightCopper", 50000, 0, 10000000);
	config.pricing.high_impact_haste_weight = THJFakeBazaarConfigUInt("HighImpactHasteWeightCopper", 20000, 0, 10000000);
	config.pricing.high_impact_heroic_stat_weight = THJFakeBazaarConfigUInt("HighImpactHeroicStatWeightCopper", 25000, 0, 10000000);
	config.pricing.high_impact_weapon_ratio_weight = THJFakeBazaarConfigUInt("HighImpactWeaponRatioWeightCopper", 8000, 0, 10000000);
	config.pricing.high_impact_weapon_damage_weight = THJFakeBazaarConfigUInt("HighImpactWeaponDamageWeightCopper", 3000, 0, 10000000);
	config.pricing.buyer_min_base_price        = THJFakeBazaarConfigPp("BuyerMinBasePricePP", 1, 0, 2000000);
	config.pricing.buyer_max_price             = THJFakeBazaarConfigPp("BuyerMaxPricePP", 10000, 1, 2000000);
	config.pricing.buyer_vendor_weight_percent = THJFakeBazaarConfigUInt("BuyerVendorWeightPercent", 500, 0, 10000);
	config.pricing.buyer_bag_slot_weight       = THJFakeBazaarConfigUInt("BuyerBagSlotWeightCopper", 500, 0, 10000000);
	config.pricing.buyer_bag_weight_reduction_weight = THJFakeBazaarConfigUInt("BuyerBagWeightReductionWeightCopper", 100, 0, 10000000);
	config.pricing.buyer_stackable_max_price   = THJFakeBazaarConfigPp("BuyerStackableMaxPricePP", 25, 0, 2000000);
	config.pricing.buyer_mundane_max_price     = THJFakeBazaarConfigPp("BuyerMundaneMaxPricePP", 50, 0, 2000000);
	config.pricing.proc_effect_min_bonus       = THJFakeBazaarConfigPp("ProcEffectMinBonusPP", 250, 0, 2000000);
	config.pricing.proc_effect_max_bonus       = THJFakeBazaarConfigPp("ProcEffectMaxBonusPP", 2000, 0, 2000000);
	config.pricing.click_effect_min_bonus      = THJFakeBazaarConfigPp("ClickEffectMinBonusPP", 100, 0, 2000000);
	config.pricing.click_effect_max_bonus      = THJFakeBazaarConfigPp("ClickEffectMaxBonusPP", 1000, 0, 2000000);
	config.pricing.worn_effect_min_bonus       = THJFakeBazaarConfigPp("WornEffectMinBonusPP", 250, 0, 2000000);
	config.pricing.worn_effect_max_bonus       = THJFakeBazaarConfigPp("WornEffectMaxBonusPP", 2000, 0, 2000000);
	config.pricing.focus_effect_min_bonus      = THJFakeBazaarConfigPp("FocusEffectMinBonusPP", 250, 0, 2000000);
	config.pricing.focus_effect_max_bonus      = THJFakeBazaarConfigPp("FocusEffectMaxBonusPP", 2000, 0, 2000000);
	config.pricing.bard_effect_min_bonus       = THJFakeBazaarConfigPp("BardEffectMinBonusPP", 100, 0, 2000000);
	config.pricing.bard_effect_max_bonus       = THJFakeBazaarConfigPp("BardEffectMaxBonusPP", 1000, 0, 2000000);

	if (config.seed.min_expansion > config.seed.max_expansion) {
		std::swap(config.seed.min_expansion, config.seed.max_expansion);
	}

	if (config.min_item_level > config.max_item_level && config.max_item_level) {
		std::swap(config.min_item_level, config.max_item_level);
	}

	return config;
}

static std::string THJFakeBazaarName(const char *prefix, uint32 index)
{
	std::string suffix;
	index++;

	do {
		const uint32 remainder = (index - 1) % 26;
		suffix.insert(suffix.begin(), static_cast<char>('A' + remainder));
		index = (index - 1) / 26;
	} while (index);

	return fmt::format("{}{}", prefix, suffix);
}

static THJFakeBazaarSeedSettings THJFakeBazaarParseSeedSettings(
	const Seperator *sep,
	const THJFakeBazaarConfig &config
)
{
	THJFakeBazaarSeedSettings settings{};
	settings.seller_items          = THJFakeBazaarClampedArg(sep, 2, config.seed.seller_items, 1, 5000);
	settings.buyer_lines           = THJFakeBazaarClampedArg(sep, 3, config.seed.buyer_lines, 0, 1000);
	settings.seller_count          = THJFakeBazaarClampedArg(sep, 4, config.seed.seller_count, 1, 80);
	settings.buyer_count           = THJFakeBazaarClampedArg(sep, 5, config.seed.buyer_count, 0, 80);
	settings.seller_min_multiplier = THJFakeBazaarClampedArg(sep, 6, config.seed.seller_min_multiplier, 1, 100);
	settings.seller_max_multiplier = THJFakeBazaarClampedArg(sep, 7, config.seed.seller_max_multiplier, 1, 100);
	settings.buyer_min_multiplier  = THJFakeBazaarClampedArg(sep, 8, config.seed.buyer_min_multiplier, 1, 200);
	settings.buyer_max_multiplier  = THJFakeBazaarClampedArg(sep, 9, config.seed.buyer_max_multiplier, 1, 200);
	settings.max_expansion         = THJFakeBazaarClampedArg(sep, 10, config.seed.max_expansion, 0, 30);
	settings.drops_per_zone        = THJFakeBazaarClampedArg(sep, 11, config.seed.drops_per_zone, 1, 20);
	settings.min_expansion         = THJFakeBazaarClampedArg(sep, 12, config.seed.min_expansion, 0, 30);

	if (settings.seller_min_multiplier > settings.seller_max_multiplier) {
		std::swap(settings.seller_min_multiplier, settings.seller_max_multiplier);
	}

	if (settings.buyer_min_multiplier > settings.buyer_max_multiplier) {
		std::swap(settings.buyer_min_multiplier, settings.buyer_max_multiplier);
	}

	if (settings.min_expansion > settings.max_expansion) {
		std::swap(settings.min_expansion, settings.max_expansion);
	}

	const uint32 required_sellers = (settings.seller_items + THJ_FAKE_BAZAAR_MAX_PER_TRADER - 1) / THJ_FAKE_BAZAAR_MAX_PER_TRADER;
	settings.seller_count = std::max(settings.seller_count, std::min<uint32>(required_sellers, 80));
	settings.seller_items = std::min<uint32>(settings.seller_items, settings.seller_count * THJ_FAKE_BAZAAR_MAX_PER_TRADER);

	return settings;
}

static void THJFakeBazaarUsage(Client *c)
{
	c->Message(Chat::White, "#fakebazaar status");
	c->Message(Chat::White, "#fakebazaar clear");
	c->Message(Chat::White, "#fakebazaar refresh [force]");
	c->Message(Chat::White, "#fakebazaar pool status");
	c->Message(
		Chat::White,
		"#fakebazaar seed [seller_items] [buyer_lines] [seller_count] [buyer_count] [seller_mult_min] [seller_mult_max] [buyer_mult_min] [buyer_mult_max] [max_expansion] [drops_per_zone] [min_expansion]"
	);
	c->Message(Chat::White, "#fakebazaar config");
	c->Message(Chat::White, "#fakebazaar set [key] [value]");
}

static CharacterDataRepository::CharacterData THJFakeBazaarEnsureCharacter(const std::string &name, uint32 zone_instance_id)
{
	auto character = CharacterDataRepository::FindByName(database, name);
	if (character.id) {
		bool should_update = false;

		if (character.deleted_at > 0) {
			character.deleted_at = 0;
			should_update = true;
		}

		if (character.zone_id != THJ_FAKE_BAZAAR_ZONE_ID || character.zone_instance != zone_instance_id) {
			character.zone_id       = THJ_FAKE_BAZAAR_ZONE_ID;
			character.zone_instance = zone_instance_id;
			should_update = true;
		}

		if (should_update) {
			CharacterDataRepository::UpdateOne(database, character);
		}

		return character;
	}

	character                = CharacterDataRepository::NewEntity();
	character.account_id     = 0;
	character.name           = name;
	character.zone_id        = THJ_FAKE_BAZAAR_ZONE_ID;
	character.zone_instance  = zone_instance_id;
	character.race           = 1;
	character.class_         = 1;
	character.gender         = 0;
	character.level          = 65;
	character.level2         = 65;
	character.deity          = 396;
	character.birthday       = static_cast<uint32>(time(nullptr));
	character.last_login     = static_cast<uint32>(time(nullptr));
	character.cur_hp         = 1000;
	character.mana           = 1000;
	character.endurance      = 1000;
	character.str            = 100;
	character.sta            = 100;
	character.cha            = 100;
	character.dex            = 100;
	character.int_           = 100;
	character.agi            = 100;
	character.wis            = 100;
	character.hunger_level   = 6000;
	character.thirst_level   = 6000;
	character.mailkey        = Strings::Random(16);
	character.xtargets       = 5;
	character.first_login    = 1;

	return CharacterDataRepository::InsertOne(database, character);
}

static THJFakeBazaarItem THJFakeBazaarItemFromRow(MySQLRequestRow &row, uint32 offset)
{
	THJFakeBazaarItem item{};
	item.id            = THJFakeBazaarRowUInt(row[offset]);
	item.name          = row[offset + 1] ? row[offset + 1] : "";
	item.icon          = THJFakeBazaarRowUInt(row[offset + 2]);
	item.price         = THJFakeBazaarRowUInt(row[offset + 3]);
	item.ldon_price    = THJFakeBazaarRowUInt(row[offset + 4]);
	item.stackable     = THJFakeBazaarRowUInt(row[offset + 5]) != 0;
	item.stack_size    = THJFakeBazaarRowUInt(row[offset + 6]);
	item.max_charges   = THJFakeBazaarRowInt(row[offset + 7]);
	item.item_class    = THJFakeBazaarRowUInt(row[offset + 8]);
	item.item_type     = THJFakeBazaarRowUInt(row[offset + 9]);
	item.slots         = THJFakeBazaarRowUInt(row[offset + 10]);
	item.bag_slots     = THJFakeBazaarRowUInt(row[offset + 11]);
	item.ac            = THJFakeBazaarRowInt(row[offset + 12]);
	item.hp            = THJFakeBazaarRowInt(row[offset + 13]);
	item.mana          = THJFakeBazaarRowInt(row[offset + 14]);
	item.damage        = THJFakeBazaarRowInt(row[offset + 15]);
	item.rec_level     = THJFakeBazaarRowInt(row[offset + 16]);
	item.req_level     = THJFakeBazaarRowInt(row[offset + 17]);
	item.scroll_effect = THJFakeBazaarRowInt(row[offset + 18]);
	item.focus_effect  = THJFakeBazaarRowInt(row[offset + 19]);
	item.delay         = THJFakeBazaarRowUInt(row[offset + 20]);
	item.endur         = THJFakeBazaarRowInt(row[offset + 21]);
	item.primary_stats = THJFakeBazaarRowInt(row[offset + 22]);
	item.resists       = THJFakeBazaarRowInt(row[offset + 23]);
	item.heroic_stats  = THJFakeBazaarRowInt(row[offset + 24]);
	item.attack        = THJFakeBazaarRowInt(row[offset + 25]);
	item.regen         = THJFakeBazaarRowInt(row[offset + 26]);
	item.mana_regen    = THJFakeBazaarRowInt(row[offset + 27]);
	item.endurance_regen = THJFakeBazaarRowInt(row[offset + 28]);
	item.haste         = THJFakeBazaarRowInt(row[offset + 29]);
	item.damage_shield = THJFakeBazaarRowInt(row[offset + 30]);
	item.proc_effect   = THJFakeBazaarRowInt(row[offset + 31]);
	item.click_effect  = THJFakeBazaarRowInt(row[offset + 32]);
	item.worn_effect   = THJFakeBazaarRowInt(row[offset + 33]);
	item.bard_effect   = THJFakeBazaarRowInt(row[offset + 34]);
	item.elemental_damage = THJFakeBazaarRowUInt(row[offset + 35]);
	item.backstab_damage  = THJFakeBazaarRowUInt(row[offset + 36]);
	item.spell_damage  = THJFakeBazaarRowInt(row[offset + 37]);
	item.heal_amount   = THJFakeBazaarRowInt(row[offset + 38]);
	item.combat_effects = THJFakeBazaarRowInt(row[offset + 39]);
	item.shielding     = THJFakeBazaarRowInt(row[offset + 40]);
	item.avoidance     = THJFakeBazaarRowInt(row[offset + 41]);
	item.accuracy      = THJFakeBazaarRowInt(row[offset + 42]);
	item.bag_weight_reduction = THJFakeBazaarRowUInt(row[offset + 43]);

	return item;
}

static std::vector<THJFakeBazaarItem> THJFakeBazaarLoadFallbackItems(
	uint32 item_count,
	const std::unordered_set<uint32> &excluded_item_ids,
	const THJFakeBazaarSeedSettings &settings,
	const THJFakeBazaarConfig &config
)
{
	std::vector<THJFakeBazaarItem> items;
	if (!item_count) {
		return items;
	}

	if (!config.allow_global_fallback) {
		return items;
	}

	std::string excluded_filter;
	if (!excluded_item_ids.empty()) {
		std::vector<std::string> excluded_ids;
		excluded_ids.reserve(excluded_item_ids.size());
		for (const auto item_id : excluded_item_ids) {
			excluded_ids.push_back(std::to_string(item_id));
		}

		excluded_filter = fmt::format("AND id NOT IN ({})", Strings::Implode(", ", excluded_ids));
	}

	auto results = content_db.QueryDatabase(
		fmt::format(
			R"(
				SELECT id, Name, icon, price, ldonprice, stackable, stacksize, maxcharges, itemclass, itemtype,
					slots, bagslots, ac, hp, mana, damage, reclevel, reqlevel, scrolleffect, focuseffect,
					delay, endur, (astr + asta + aagi + adex + acha + aint + awis),
					(cr + dr + pr + mr + fr + svcorruption),
					(heroic_str + heroic_int + heroic_wis + heroic_agi + heroic_dex + heroic_sta + heroic_cha),
					attack, regen, manaregen, enduranceregen, haste, damageshield,
					proceffect, clickeffect, worneffect, bardeffect, elemdmgamt, backstabdmg,
					spelldmg, healamt, combateffects, shielding, avoidance, accuracy, bagwr
				FROM items
				WHERE id > 0
					AND minstatus = 0
					AND nodrop <> 0
					AND norent <> 0
					AND summonedflag = 0
					AND notransfer = 0
					AND Name <> ''
					{0}
					{1}
					{2}
					{3}
					{4}
					{5}
					{6}
					AND (
						slots <> 0
						OR bagslots > 0
						OR itemtype IN ({7})
						OR scrolleffect BETWEEN 1 AND 64999
						OR focuseffect > 0
					)
					AND (
						price > 0
						OR ldonprice > 0
						OR bagslots > 0
						OR slots <> 0
						OR scrolleffect BETWEEN 1 AND 64999
						OR focuseffect > 0
						OR ac > 0
						OR hp > 0
						OR mana > 0
						OR damage > 0
					)
					{8}
				ORDER BY RAND()
				LIMIT {9}
			)",
			THJFakeBazaarNamePatternSql("", config.excluded_name_patterns),
			THJFakeBazaarLevelSql("items", config.min_item_level, config.max_item_level),
			THJFakeBazaarRecipeExpansionSql("", settings),
			THJFakeBazaarTradeskillSql("", config),
			THJFakeBazaarMerchantListSql("", config),
			THJFakeBazaarSpellScrollSql("", config),
			THJFakeBazaarEquipmentStatsSql("", config),
			THJFakeBazaarUIntListSql(config.allowed_item_types, THJ_FAKE_BAZAAR_DEFAULT_ITEM_TYPES),
			excluded_filter,
			item_count
		)
	);

	if (!results.Success() || !results.RowCount()) {
		return items;
	}

	items.reserve(results.RowCount());
	for (auto row = results.begin(); row != results.end(); ++row) {
		auto item = THJFakeBazaarItemFromRow(row, 0);

		if (item.id && !item.name.empty()) {
			items.push_back(item);
		}
	}

	return items;
}

static std::vector<THJFakeBazaarItem> THJFakeBazaarSelectZoneDropItems(
	MySQLRequestResult &results,
	uint32 item_count,
	const THJFakeBazaarSeedSettings &settings,
	std::unordered_set<uint32> &chosen_item_ids
)
{
	std::vector<THJFakeBazaarItem> items;
	if (!item_count || !results.Success() || !results.RowCount()) {
		return items;
	}

	std::random_device random_device;
	std::mt19937       rng(random_device());

	std::vector<THJFakeBazaarZoneDropCandidate> all_candidates;
	std::vector<uint32>                         zone_ids;
	std::unordered_map<uint32, std::vector<THJFakeBazaarItem>> zone_items;
	std::unordered_set<uint32> seen_zone_ids;

	all_candidates.reserve(results.RowCount());

	for (auto row = results.begin(); row != results.end(); ++row) {
		const uint32 zone_id = THJFakeBazaarRowUInt(row[0]);
		auto item = THJFakeBazaarItemFromRow(row, 1);
		if (!zone_id || !item.id || item.name.empty()) {
			continue;
		}

		if (seen_zone_ids.insert(zone_id).second) {
			zone_ids.push_back(zone_id);
		}

		zone_items[zone_id].push_back(item);
		all_candidates.push_back({zone_id, item});
	}

	for (auto &[zone_id, candidates] : zone_items) {
		std::shuffle(candidates.begin(), candidates.end(), rng);
	}

	std::shuffle(zone_ids.begin(), zone_ids.end(), rng);

	std::unordered_map<uint32, size_t> zone_positions;
	items.reserve(item_count);

	for (uint32 pass = 0; pass < settings.drops_per_zone && items.size() < item_count; ++pass) {
		for (const auto zone_id : zone_ids) {
			auto &candidates = zone_items[zone_id];
			auto &position = zone_positions[zone_id];

			while (position < candidates.size() && chosen_item_ids.find(candidates[position].id) != chosen_item_ids.end()) {
				position++;
			}

			if (position >= candidates.size()) {
				continue;
			}

			items.push_back(candidates[position]);
			chosen_item_ids.insert(candidates[position].id);
			position++;

			if (items.size() >= item_count) {
				break;
			}
		}
	}

	std::shuffle(all_candidates.begin(), all_candidates.end(), rng);
	for (const auto &candidate : all_candidates) {
		if (items.size() >= item_count) {
			break;
		}

		if (!chosen_item_ids.insert(candidate.item.id).second) {
			continue;
		}

		items.push_back(candidate.item);
	}

	return items;
}

static bool THJFakeBazaarItemPoolTableExists()
{
	auto results = content_db.QueryDatabase(
		fmt::format("SHOW TABLES LIKE '{}'", THJ_FAKE_BAZAAR_ITEM_POOL_TABLE)
	);

	return results.Success() && results.RowCount() > 0;
}

static uint32 THJFakeBazaarItemPoolCount()
{
	if (!THJFakeBazaarItemPoolTableExists()) {
		return 0;
	}

	auto results = content_db.QueryDatabase(
		fmt::format("SELECT COUNT(*) FROM `{}`", THJ_FAKE_BAZAAR_ITEM_POOL_TABLE)
	);

	if (!results.Success() || !results.RowCount()) {
		return 0;
	}

	auto row = results.begin();
	return THJFakeBazaarRowUInt(row[0]);
}

static std::vector<THJFakeBazaarItem> THJFakeBazaarLoadCachedItems(
	uint32 item_count,
	const THJFakeBazaarSeedSettings &settings,
	const THJFakeBazaarConfig &config
)
{
	std::unordered_set<uint32> chosen_item_ids;
	if (!item_count || !THJFakeBazaarItemPoolTableExists()) {
		return THJFakeBazaarLoadFallbackItems(item_count, chosen_item_ids, settings, config);
	}

	auto results = content_db.QueryDatabase(
		fmt::format(
			R"(
				SELECT p.zone_id, i.id, i.Name, i.icon, i.price, i.ldonprice, i.stackable, i.stacksize,
					i.maxcharges, i.itemclass, i.itemtype, i.slots, i.bagslots, i.ac, i.hp, i.mana,
					i.damage, i.reclevel, i.reqlevel, i.scrolleffect, i.focuseffect,
					i.delay, i.endur, (i.astr + i.asta + i.aagi + i.adex + i.acha + i.aint + i.awis),
					(i.cr + i.dr + i.pr + i.mr + i.fr + i.svcorruption),
					(i.heroic_str + i.heroic_int + i.heroic_wis + i.heroic_agi + i.heroic_dex + i.heroic_sta + i.heroic_cha),
					i.attack, i.regen, i.manaregen, i.enduranceregen, i.haste, i.damageshield,
					i.proceffect, i.clickeffect, i.worneffect, i.bardeffect, i.elemdmgamt, i.backstabdmg,
					i.spelldmg, i.healamt, i.combateffects, i.shielding, i.avoidance, i.accuracy, i.bagwr
				FROM `{11}` AS p
				INNER JOIN items AS i
					ON i.id = p.item_id
				WHERE p.min_expansion <= {1}
					AND p.max_expansion >= {0}
					{2}
					AND i.id > 0
					AND i.minstatus = 0
					AND i.nodrop <> 0
					AND i.norent <> 0
					AND i.summonedflag = 0
					AND i.notransfer = 0
					AND i.Name <> ''
					{3}
					{4}
					{5}
					{6}
					{7}
					{8}
					{9}
					AND (
						i.slots <> 0
						OR i.bagslots > 0
						OR i.itemtype IN ({10})
						OR i.scrolleffect BETWEEN 1 AND 64999
						OR i.focuseffect > 0
					)
					AND (
						i.price > 0
						OR i.ldonprice > 0
						OR i.bagslots > 0
						OR i.slots <> 0
						OR i.scrolleffect BETWEEN 1 AND 64999
						OR i.focuseffect > 0
						OR i.ac > 0
						OR i.hp > 0
						OR i.mana > 0
						OR i.damage > 0
					)
				ORDER BY p.zone_id ASC, p.item_id ASC
			)",
			settings.min_expansion,
			settings.max_expansion,
			THJFakeBazaarPoolZoneSql(config),
			THJFakeBazaarNamePatternSql("i", config.excluded_name_patterns),
			THJFakeBazaarLevelSql("i", config.min_item_level, config.max_item_level),
			THJFakeBazaarRecipeExpansionSql("i", settings),
			THJFakeBazaarTradeskillSql("i", config),
			THJFakeBazaarMerchantListSql("i", config),
			THJFakeBazaarSpellScrollSql("i", config),
			THJFakeBazaarEquipmentStatsSql("i", config),
			THJFakeBazaarUIntListSql(config.allowed_item_types, THJ_FAKE_BAZAAR_DEFAULT_ITEM_TYPES),
			THJ_FAKE_BAZAAR_ITEM_POOL_TABLE
		)
	);

	auto items = THJFakeBazaarSelectZoneDropItems(results, item_count, settings, chosen_item_ids);
	if (items.size() < item_count) {
		auto fallback_items = THJFakeBazaarLoadFallbackItems(
			item_count - static_cast<uint32>(items.size()),
			chosen_item_ids,
			settings,
			config
		);

		items.insert(items.end(), fallback_items.begin(), fallback_items.end());
	}

	return items;
}

static std::vector<THJFakeBazaarItem> THJFakeBazaarLoadLiveZoneDropItems(
	uint32 item_count,
	const THJFakeBazaarSeedSettings &settings,
	const THJFakeBazaarConfig &config
)
{
	std::vector<THJFakeBazaarItem> items;
	if (!item_count) {
		return items;
	}

	auto results = content_db.QueryDatabase(
		fmt::format(
			R"(
				SELECT z.zoneidnumber, i.id, i.Name, i.icon, i.price, i.ldonprice, i.stackable, i.stacksize,
					i.maxcharges, i.itemclass, i.itemtype, i.slots, i.bagslots, i.ac, i.hp, i.mana,
					i.damage, i.reclevel, i.reqlevel, i.scrolleffect, i.focuseffect,
					i.delay, i.endur, (i.astr + i.asta + i.aagi + i.adex + i.acha + i.aint + i.awis),
					(i.cr + i.dr + i.pr + i.mr + i.fr + i.svcorruption),
					(i.heroic_str + i.heroic_int + i.heroic_wis + i.heroic_agi + i.heroic_dex + i.heroic_sta + i.heroic_cha),
					i.attack, i.regen, i.manaregen, i.enduranceregen, i.haste, i.damageshield,
					i.proceffect, i.clickeffect, i.worneffect, i.bardeffect, i.elemdmgamt, i.backstabdmg,
					i.spelldmg, i.healamt, i.combateffects, i.shielding, i.avoidance, i.accuracy, i.bagwr
				FROM `zone` AS z
				INNER JOIN spawn2 AS s2
					ON s2.zone = z.short_name
					AND s2.version = z.version
				INNER JOIN spawnentry AS se
					ON se.spawngroupID = s2.spawngroupID
				INNER JOIN npc_types AS nt
					ON nt.id = se.npcID
					AND nt.loottable_id > 0
				INNER JOIN loottable_entries AS lte
					ON lte.loottable_id = nt.loottable_id
					AND lte.probability > 0
				INNER JOIN lootdrop_entries AS lde
					ON lde.lootdrop_id = lte.lootdrop_id
					AND lde.chance > 0
				INNER JOIN items AS base_i
					ON base_i.id = lde.item_id
				INNER JOIN items AS i
					ON {11}
				WHERE z.version = 0
					AND z.expansion >= {0}
					AND z.expansion <= {1}
					AND (s2.min_expansion = -1 OR s2.min_expansion <= {1})
					AND (s2.max_expansion = -1 OR s2.max_expansion >= {0})
					AND (se.min_expansion = -1 OR se.min_expansion <= {1})
					AND (se.max_expansion = -1 OR se.max_expansion >= {0})
					AND (lde.min_expansion = -1 OR lde.min_expansion <= {1})
					AND (lde.max_expansion = -1 OR lde.max_expansion >= {0})
					{2}
					AND i.id > 0
					AND i.minstatus = 0
					AND i.nodrop <> 0
					AND i.norent <> 0
					AND i.summonedflag = 0
					AND i.notransfer = 0
					AND i.Name <> ''
					{3}
					{4}
					{5}
					{6}
					{7}
					{8}
					{9}
					AND (
						i.slots <> 0
						OR i.bagslots > 0
						OR i.itemtype IN ({10})
						OR i.scrolleffect BETWEEN 1 AND 64999
						OR i.focuseffect > 0
					)
					AND (
						i.price > 0
						OR i.ldonprice > 0
						OR i.bagslots > 0
						OR i.slots <> 0
						OR i.scrolleffect BETWEEN 1 AND 64999
						OR i.focuseffect > 0
						OR i.ac > 0
						OR i.hp > 0
						OR i.mana > 0
						OR i.damage > 0
					)
				ORDER BY z.zoneidnumber ASC, i.id ASC
			)",
			settings.min_expansion,
			settings.max_expansion,
			THJFakeBazaarZoneSql(config),
			THJFakeBazaarNamePatternSql("i", config.excluded_name_patterns),
			THJFakeBazaarLevelSql("i", config.min_item_level, config.max_item_level),
			THJFakeBazaarRecipeExpansionSql("i", settings),
			THJFakeBazaarTradeskillSql("i", config),
			THJFakeBazaarMerchantListSql("i", config),
			THJFakeBazaarSpellScrollSql("i", config),
			THJFakeBazaarEquipmentStatsSql("i", config),
			THJFakeBazaarUIntListSql(config.allowed_item_types, THJ_FAKE_BAZAAR_DEFAULT_ITEM_TYPES),
			THJFakeBazaarItemVariantJoinSql(config)
		)
	);

	std::unordered_set<uint32> chosen_item_ids;
	if (!results.Success() || !results.RowCount()) {
		return THJFakeBazaarLoadFallbackItems(item_count, chosen_item_ids, settings, config);
	}

	items = THJFakeBazaarSelectZoneDropItems(results, item_count, settings, chosen_item_ids);

	if (items.size() < item_count) {
		auto fallback_items = THJFakeBazaarLoadFallbackItems(
			item_count - static_cast<uint32>(items.size()),
			chosen_item_ids,
			settings,
			config
		);

		items.insert(items.end(), fallback_items.begin(), fallback_items.end());
	}

	return items;
}

static std::vector<THJFakeBazaarItem> THJFakeBazaarLoadItems(
	uint32 item_count,
	const THJFakeBazaarSeedSettings &settings,
	const THJFakeBazaarConfig &config
)
{
	if (!item_count) {
		return {};
	}

	if (config.use_cached_item_pool) {
		auto cached_items = THJFakeBazaarLoadCachedItems(item_count, settings, config);
		if (!cached_items.empty() || !config.allow_live_item_scan) {
			return cached_items;
		}
	}

	if (!config.allow_live_item_scan) {
		return {};
	}

	return THJFakeBazaarLoadLiveZoneDropItems(item_count, settings, config);
}

static uint64 THJFakeBazaarPositiveValue(int32 value)
{
	return static_cast<uint64>(std::max(0, value));
}

static bool THJFakeBazaarHasEffect(int32 effect_id)
{
	return effect_id > 0 && effect_id < 65000;
}

static uint64 THJFakeBazaarRandomBonus(uint32 min_value, uint32 max_value, std::mt19937 &rng)
{
	if (!min_value && !max_value) {
		return 0;
	}

	if (min_value > max_value) {
		std::swap(min_value, max_value);
	}

	std::uniform_int_distribution<uint32> distribution(min_value, max_value);
	return distribution(rng);
}

static uint64 THJFakeBazaarVendorValue(const THJFakeBazaarItem &item, uint32 vendor_weight_percent)
{
	return std::max<uint64>(item.price, item.ldon_price) * vendor_weight_percent / 100;
}

static uint64 THJFakeBazaarBaseValue(
	const THJFakeBazaarItem &item,
	const THJFakeBazaarPricingSettings &pricing
)
{
	const uint64 vendor_value = THJFakeBazaarVendorValue(item, pricing.vendor_weight_percent);
	uint64 score_value = 0;

	const uint32 level = static_cast<uint32>(std::max(0, std::max(item.rec_level, item.req_level)));
	score_value += static_cast<uint64>(level) * pricing.level_weight;

	if (item.scroll_effect > 0 || item.item_type == 20) {
		score_value += static_cast<uint64>(std::max<uint32>(1, level)) * pricing.spell_level_weight;
	}

	if (item.bag_slots > 0) {
		score_value += static_cast<uint64>(item.bag_slots) * pricing.bag_slot_weight;
		score_value += static_cast<uint64>(item.bag_weight_reduction) * pricing.bag_weight_reduction_weight;
	}

	if (item.slots != 0 || item.item_class == 0) {
		score_value += THJFakeBazaarPositiveValue(item.ac) * pricing.ac_weight;
		score_value += THJFakeBazaarPositiveValue(item.primary_stats) * pricing.stat_weight;
		score_value += THJFakeBazaarPositiveValue(item.heroic_stats) * pricing.heroic_stat_weight;
		score_value += THJFakeBazaarPositiveValue(item.hp) * pricing.hp_weight;
		score_value += THJFakeBazaarPositiveValue(item.mana) * pricing.mana_weight;
		score_value += THJFakeBazaarPositiveValue(item.endur) * pricing.endurance_weight;
		score_value += THJFakeBazaarPositiveValue(item.resists) * pricing.resist_weight;
		score_value += THJFakeBazaarPositiveValue(item.attack) * pricing.attack_weight;
		score_value += (
			THJFakeBazaarPositiveValue(item.regen) +
			THJFakeBazaarPositiveValue(item.mana_regen) +
			THJFakeBazaarPositiveValue(item.endurance_regen)
		) * pricing.regen_weight;
		score_value += THJFakeBazaarPositiveValue(item.haste) * pricing.haste_weight;
		score_value += THJFakeBazaarPositiveValue(item.damage_shield) * pricing.damage_shield_weight;
		score_value += THJFakeBazaarPositiveValue(item.spell_damage) * pricing.spell_damage_weight;
		score_value += THJFakeBazaarPositiveValue(item.heal_amount) * pricing.heal_amount_weight;
		score_value += (
			THJFakeBazaarPositiveValue(item.combat_effects) +
			THJFakeBazaarPositiveValue(item.shielding) +
			THJFakeBazaarPositiveValue(item.avoidance) +
			THJFakeBazaarPositiveValue(item.accuracy)
		) * pricing.mod2_weight;
	}

	const uint64 weapon_damage = static_cast<uint64>(item.damage) + item.elemental_damage;
	if (weapon_damage > 0 && item.delay > 0) {
		const uint64 ratio_score = weapon_damage * 100 / item.delay;
		score_value += ratio_score * pricing.weapon_ratio_weight;
		score_value += weapon_damage * pricing.weapon_damage_weight;
	}

	score_value += static_cast<uint64>(item.elemental_damage) * pricing.elemental_damage_weight;
	score_value += static_cast<uint64>(item.backstab_damage) * pricing.backstab_damage_weight;

	return std::clamp<uint64>(
		std::max({vendor_value, score_value, static_cast<uint64>(pricing.min_base_price)}),
		1,
		pricing.max_price
	);
}

static uint32 THJFakeBazaarFamilyBaseItemID(uint32 item_id)
{
	if (item_id > THJ_FAKE_BAZAAR_LEGENDARY_ITEM_ID_OFFSET) {
		return item_id - THJ_FAKE_BAZAAR_LEGENDARY_ITEM_ID_OFFSET;
	}

	if (item_id > THJ_FAKE_BAZAAR_ENCHANTED_ITEM_ID_OFFSET) {
		return item_id - THJ_FAKE_BAZAAR_ENCHANTED_ITEM_ID_OFFSET;
	}

	return item_id;
}

static uint64 THJFakeBazaarHighImpactValue(
	const THJFakeBazaarItem &item,
	const THJFakeBazaarPricingSettings &pricing
)
{
	uint64 score_value = 0;
	score_value += THJFakeBazaarPositiveValue(item.spell_damage) * pricing.high_impact_spell_damage_weight;
	score_value += THJFakeBazaarPositiveValue(item.heal_amount) * pricing.high_impact_heal_amount_weight;
	score_value += THJFakeBazaarPositiveValue(item.haste) * pricing.high_impact_haste_weight;
	score_value += THJFakeBazaarPositiveValue(item.heroic_stats) * pricing.high_impact_heroic_stat_weight;

	const uint64 weapon_damage = static_cast<uint64>(item.damage) + item.elemental_damage;
	if (weapon_damage > 0 && item.delay > 0) {
		const uint64 ratio_score = weapon_damage * 100 / item.delay;
		score_value += ratio_score * pricing.high_impact_weapon_ratio_weight;
		score_value += weapon_damage * pricing.high_impact_weapon_damage_weight;
	}

	return std::min<uint64>(score_value, pricing.max_price);
}

static std::unordered_map<uint32, THJFakeBazaarItemPricingContext> THJFakeBazaarBuildPricingContexts(
	const std::vector<THJFakeBazaarItem> &items,
	const THJFakeBazaarPricingSettings &pricing
)
{
	std::unordered_map<uint32, THJFakeBazaarItemPricingContext> contexts;
	if (items.empty()) {
		return contexts;
	}

	std::unordered_set<uint32> family_base_item_ids;
	for (const auto &item : items) {
		if (item.id) {
			family_base_item_ids.insert(THJFakeBazaarFamilyBaseItemID(item.id));
		}
	}

	std::unordered_map<uint32, THJFakeBazaarItemPricingContext> family_contexts;
	std::vector<std::string> family_item_ids;
	family_item_ids.reserve(family_base_item_ids.size() * 3);
	for (const auto family_base_item_id : family_base_item_ids) {
		THJFakeBazaarItemPricingContext family_context{};
		family_context.family_base_item_id = family_base_item_id;
		family_contexts[family_base_item_id] = family_context;
		family_item_ids.push_back(std::to_string(family_base_item_id));
		family_item_ids.push_back(std::to_string(family_base_item_id + THJ_FAKE_BAZAAR_ENCHANTED_ITEM_ID_OFFSET));
		family_item_ids.push_back(std::to_string(family_base_item_id + THJ_FAKE_BAZAAR_LEGENDARY_ITEM_ID_OFFSET));
	}

	auto results = content_db.QueryDatabase(
		fmt::format(
			R"(
				SELECT id, Name, icon, price, ldonprice, stackable, stacksize, maxcharges, itemclass, itemtype,
					slots, bagslots, ac, hp, mana, damage, reclevel, reqlevel, scrolleffect, focuseffect,
					delay, endur, (astr + asta + aagi + adex + acha + aint + awis),
					(cr + dr + pr + mr + fr + svcorruption),
					(heroic_str + heroic_int + heroic_wis + heroic_agi + heroic_dex + heroic_sta + heroic_cha),
					attack, regen, manaregen, enduranceregen, haste, damageshield,
					proceffect, clickeffect, worneffect, bardeffect, elemdmgamt, backstabdmg,
					spelldmg, healamt, combateffects, shielding, avoidance, accuracy, bagwr
				FROM items
				WHERE id IN ({})
					AND minstatus = 0
					AND Name <> ''
			)",
			Strings::Implode(", ", family_item_ids)
		)
	);

	if (results.Success() && results.RowCount()) {
		for (auto row = results.begin(); row != results.end(); ++row) {
			const auto family_item = THJFakeBazaarItemFromRow(row, 0);
			auto &family_context = family_contexts[THJFakeBazaarFamilyBaseItemID(family_item.id)];
			family_context.family_base_value = std::max(
				family_context.family_base_value,
				THJFakeBazaarBaseValue(family_item, pricing)
			);
			family_context.family_high_impact_value = std::max(
				family_context.family_high_impact_value,
				THJFakeBazaarHighImpactValue(family_item, pricing)
			);
		}
	}

	for (const auto &item : items) {
		auto context = family_contexts[THJFakeBazaarFamilyBaseItemID(item.id)];
		context.exact_base_value = THJFakeBazaarBaseValue(item, pricing);
		context.high_impact_value = THJFakeBazaarHighImpactValue(item, pricing);
		context.family_base_value = std::max(context.family_base_value, context.exact_base_value);
		context.family_high_impact_value = std::max(context.family_high_impact_value, context.high_impact_value);
		contexts[item.id] = context;
	}

	return contexts;
}

static uint64 THJFakeBazaarSellerBaseValue(
	const THJFakeBazaarItem &item,
	const THJFakeBazaarPricingSettings &pricing,
	const THJFakeBazaarItemPricingContext *context
)
{
	uint64 base_value = context ? context->exact_base_value : THJFakeBazaarBaseValue(item, pricing);
	const uint64 family_value = context ? context->family_base_value : base_value;

	if (pricing.family_upgrade_value_percent && family_value > base_value) {
		base_value += (family_value - base_value) * pricing.family_upgrade_value_percent / 100;
	}

	const uint64 high_impact_value = context ?
		std::max(context->high_impact_value, context->family_high_impact_value) :
		THJFakeBazaarHighImpactValue(item, pricing);

	if (pricing.high_impact_premium_max_percent && high_impact_value) {
		const uint64 premium_cap = base_value * pricing.high_impact_premium_max_percent / 100;
		base_value += std::min(high_impact_value, premium_cap);
	}

	return std::clamp<uint64>(base_value, 1, pricing.max_price);
}

static uint64 THJFakeBazaarEffectBonus(
	const THJFakeBazaarItem &item,
	const THJFakeBazaarPricingSettings &pricing,
	std::mt19937 &rng
)
{
	uint64 bonus = 0;

	if (THJFakeBazaarHasEffect(item.proc_effect)) {
		bonus += THJFakeBazaarRandomBonus(pricing.proc_effect_min_bonus, pricing.proc_effect_max_bonus, rng);
	}

	if (THJFakeBazaarHasEffect(item.click_effect)) {
		bonus += THJFakeBazaarRandomBonus(pricing.click_effect_min_bonus, pricing.click_effect_max_bonus, rng);
	}

	if (THJFakeBazaarHasEffect(item.worn_effect)) {
		bonus += THJFakeBazaarRandomBonus(pricing.worn_effect_min_bonus, pricing.worn_effect_max_bonus, rng);
	}

	if (THJFakeBazaarHasEffect(item.focus_effect)) {
		bonus += THJFakeBazaarRandomBonus(pricing.focus_effect_min_bonus, pricing.focus_effect_max_bonus, rng);
	}

	if (THJFakeBazaarHasEffect(item.bard_effect)) {
		bonus += THJFakeBazaarRandomBonus(pricing.bard_effect_min_bonus, pricing.bard_effect_max_bonus, rng);
	}

	return bonus;
}

static uint32 THJFakeBazaarRandomPrice(
	const THJFakeBazaarItem &item,
	uint32 min_multiplier,
	uint32 max_multiplier,
	const THJFakeBazaarPricingSettings &pricing,
	const THJFakeBazaarItemPricingContext *context,
	std::mt19937 &rng
)
{
	std::uniform_int_distribution<uint32> multiplier_distribution(min_multiplier, max_multiplier);
	const uint64 total =
		(THJFakeBazaarSellerBaseValue(item, pricing, context) * multiplier_distribution(rng)) +
		THJFakeBazaarEffectBonus(item, pricing, rng);

	return static_cast<uint32>(std::clamp<uint64>(total, 1, pricing.max_price));
}

static bool THJFakeBazaarHasPremiumBountyValue(const THJFakeBazaarItem &item)
{
	return
		item.item_type == 20 ||
		item.rec_level > 0 ||
		item.req_level > 0 ||
		item.ac > 0 ||
		item.hp > 0 ||
		item.mana > 0 ||
		item.endur > 0 ||
		item.primary_stats > 0 ||
		item.resists > 0 ||
		item.heroic_stats > 0 ||
		item.attack > 0 ||
		item.regen > 0 ||
		item.mana_regen > 0 ||
		item.endurance_regen > 0 ||
		item.haste > 0 ||
		item.damage_shield > 0 ||
		item.spell_damage > 0 ||
		item.heal_amount > 0 ||
		item.combat_effects > 0 ||
		item.shielding > 0 ||
		item.avoidance > 0 ||
		item.accuracy > 0 ||
		item.elemental_damage > 0 ||
		item.backstab_damage > 0 ||
		THJFakeBazaarHasEffect(item.proc_effect) ||
		THJFakeBazaarHasEffect(item.click_effect) ||
		THJFakeBazaarHasEffect(item.worn_effect) ||
		THJFakeBazaarHasEffect(item.focus_effect) ||
		THJFakeBazaarHasEffect(item.bard_effect) ||
		THJFakeBazaarHasEffect(item.scroll_effect);
}

static bool THJFakeBazaarIsMundaneBountyItem(const THJFakeBazaarItem &item)
{
	return item.stackable || !THJFakeBazaarHasPremiumBountyValue(item);
}

static uint64 THJFakeBazaarBuyerBaseValue(
	const THJFakeBazaarItem &item,
	const THJFakeBazaarPricingSettings &pricing,
	bool is_mundane_bounty_item
)
{
	const uint64 buyer_vendor_value = THJFakeBazaarVendorValue(item, pricing.buyer_vendor_weight_percent);
	uint64 base_value = 0;

	if (is_mundane_bounty_item) {
		base_value = std::max<uint64>(buyer_vendor_value, pricing.buyer_min_base_price);

		if (item.bag_slots > 0) {
			base_value = std::max<uint64>(
				base_value,
				(static_cast<uint64>(item.bag_slots) * pricing.buyer_bag_slot_weight) +
				(static_cast<uint64>(item.bag_weight_reduction) * pricing.buyer_bag_weight_reduction_weight)
			);
		}
	} else {
		base_value = std::max({
			THJFakeBazaarBaseValue(item, pricing),
			buyer_vendor_value,
			static_cast<uint64>(pricing.buyer_min_base_price)
		});
	}

	return std::clamp<uint64>(base_value, 1, pricing.buyer_max_price);
}

static uint32 THJFakeBazaarRandomBuyerPrice(
	const THJFakeBazaarItem &item,
	uint32 min_multiplier,
	uint32 max_multiplier,
	const THJFakeBazaarPricingSettings &pricing,
	std::mt19937 &rng
)
{
	std::uniform_int_distribution<uint32> multiplier_distribution(min_multiplier, max_multiplier);

	const bool is_mundane_bounty_item = THJFakeBazaarIsMundaneBountyItem(item);
	uint64 total = THJFakeBazaarBuyerBaseValue(item, pricing, is_mundane_bounty_item) * multiplier_distribution(rng);

	if (!is_mundane_bounty_item) {
		total += THJFakeBazaarEffectBonus(item, pricing, rng);
	}

	if (item.stackable && pricing.buyer_stackable_max_price) {
		total = std::min<uint64>(total, pricing.buyer_stackable_max_price);
	} else if (is_mundane_bounty_item && pricing.buyer_mundane_max_price) {
		total = std::min<uint64>(total, pricing.buyer_mundane_max_price);
	}

	return static_cast<uint32>(std::clamp<uint64>(total, 1, pricing.buyer_max_price));
}

static int32 THJFakeBazaarTraderCharges(const THJFakeBazaarItem &item, std::mt19937 &rng)
{
	if (item.stackable) {
		const uint32 max_stack = std::clamp<uint32>(item.stack_size ? item.stack_size : 1, 1, 20);
		std::uniform_int_distribution<uint32> stack_distribution(1, max_stack);
		return static_cast<int32>(stack_distribution(rng));
	}

	return item.max_charges > 0 ? item.max_charges : 1;
}

static int32 THJFakeBazaarBuyerQuantity(const THJFakeBazaarItem &, std::mt19937 &)
{
	return 1;
}

static THJFakeBazaarClearResult THJFakeBazaarClear()
{
	THJFakeBazaarClearResult result{};

	const auto seller_prefix = Strings::Escape(THJ_FAKE_BAZAAR_SELLER_PREFIX);
	const auto buyer_prefix  = Strings::Escape(THJ_FAKE_BAZAAR_BUYER_PREFIX);

	result.traders = TraderRepository::DeleteWhere(
		database,
		fmt::format(
			"`char_id` IN (SELECT `id` FROM `character_data` WHERE `name` LIKE '{}%')",
			seller_prefix
		)
	);

	auto buyers = BuyerRepository::GetWhere(
		database,
		fmt::format("`char_name` LIKE '{}%'", buyer_prefix)
	);

	if (buyers.empty()) {
		return result;
	}

	std::vector<std::string> buyer_ids;
	buyer_ids.reserve(buyers.size());
	for (const auto &buyer : buyers) {
		buyer_ids.push_back(std::to_string(buyer.id));
	}

	const auto buyer_ids_string = Strings::Implode(", ", buyer_ids);
	result.buyer_trade_items = BuyerTradeItemsRepository::DeleteWhere(
		database,
		fmt::format(
			"`buyer_buy_lines_id` IN (SELECT `id` FROM `buyer_buy_lines` WHERE `buyer_id` IN ({}))",
			buyer_ids_string
		)
	);
	result.buyer_lines = BuyerBuyLinesRepository::DeleteWhere(
		database,
		fmt::format("`buyer_id` IN ({})", buyer_ids_string)
	);
	result.buyers = BuyerRepository::DeleteWhere(
		database,
		fmt::format("`id` IN ({})", buyer_ids_string)
	);

	return result;
}

static uint32 THJFakeBazaarSellerCharacterCount()
{
	const auto seller_prefix = Strings::Escape(THJ_FAKE_BAZAAR_SELLER_PREFIX);

	return static_cast<uint32>(CharacterDataRepository::Count(
		database,
		fmt::format("`name` LIKE '{}%'", seller_prefix)
	));
}

static uint32 THJFakeBazaarSellerListingCount()
{
	const auto seller_prefix = Strings::Escape(THJ_FAKE_BAZAAR_SELLER_PREFIX);

	return static_cast<uint32>(TraderRepository::Count(
		database,
		fmt::format(
			"`char_id` IN (SELECT `id` FROM `character_data` WHERE `name` LIKE '{}%')",
			seller_prefix
		)
	));
}

static uint32 THJFakeBazaarBuyerCount()
{
	const auto buyer_prefix = Strings::Escape(THJ_FAKE_BAZAAR_BUYER_PREFIX);

	return static_cast<uint32>(BuyerRepository::Count(
		database,
		fmt::format("`char_name` LIKE '{}%'", buyer_prefix)
	));
}

static uint32 THJFakeBazaarBuyerLineCount()
{
	const auto buyer_prefix = Strings::Escape(THJ_FAKE_BAZAAR_BUYER_PREFIX);

	return static_cast<uint32>(BuyerBuyLinesRepository::Count(
		database,
		fmt::format(
			"`buyer_id` IN (SELECT `id` FROM `buyer` WHERE `char_name` LIKE '{}%')",
			buyer_prefix
		)
	));
}

static void THJFakeBazaarStatus(Client *c)
{
	const auto sellers         = THJFakeBazaarSellerCharacterCount();
	const auto seller_listings = THJFakeBazaarSellerListingCount();
	const auto buyers          = THJFakeBazaarBuyerCount();
	const auto buyer_lines     = THJFakeBazaarBuyerLineCount();
	const auto pool_rows       = THJFakeBazaarItemPoolCount();

	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar status: {} seller characters, {} seller listings, {} system buyers, {} bounty buy lines, {} cached pool rows.",
			sellers,
			seller_listings,
			buyers,
			buyer_lines,
			pool_rows
		).c_str()
	);
}

static void THJFakeBazaarPoolStatus(Client *c)
{
	const auto config = THJFakeBazaarLoadConfig();
	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar item pool: {} cached rows, use_cached_item_pool={}, allow_live_item_scan={}, allow_global_fallback={}.",
			THJFakeBazaarItemPoolCount(),
			config.use_cached_item_pool,
			config.allow_live_item_scan,
			config.allow_global_fallback
		).c_str()
	);
	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar item pool rebuild: last_rebuild_unix={} last_rebuild_rows={}. Rebuild the cache with tools/thj/Rebuild-ThjFakeBazaarItemPool.ps1 outside the zone process.",
			THJFakeBazaarConfigString("PoolLastRebuildUnix", "0"),
			THJFakeBazaarConfigString("PoolLastRebuildRows", "0")
		).c_str()
	);
}

static std::vector<std::string> THJFakeBazaarConfigKeys()
{
	return {
		"SellerItems", "BuyerLines", "SellerCount", "BuyerCount",
		"SellerMinMultiplier", "SellerMaxMultiplier", "BuyerMinMultiplier", "BuyerMaxMultiplier",
		"MinExpansion", "MaxExpansion", "DropsPerZone", "MinItemLevel", "MaxItemLevel",
		"AllowGlobalFallback", "ExcludeTradeskillItems", "ExcludeMerchantItems", "ExcludeSpellScrolls",
		"RequireEquipmentStats", "MinimumEquipmentStatTotal", "IncludeItemVariants",
		"UseCachedItemPool", "AllowLiveItemScan",
		"AutoRefreshEnabled", "RefreshOnStartup", "RefreshWhenEmpty", "RefreshIntervalMinutes",
		"MinimumSellerListings", "MinimumBuyerLines", "LastSeedUnix",
		"PoolLastRebuildUnix", "PoolLastRebuildRows",
		"AllowedItemTypes", "IncludedZones", "ExcludedZones", "ExcludedNamePatterns",
		"MinBasePricePP", "MaxPricePP", "VendorWeightPercent",
		"BuyerMinBasePricePP", "BuyerMaxPricePP", "BuyerVendorWeightPercent",
		"BuyerBagSlotWeightCopper", "BuyerBagWeightReductionWeightCopper",
		"BuyerStackableMaxPricePP", "BuyerMundaneMaxPricePP",
		"LevelWeightCopper", "SpellLevelWeightCopper", "ACWeightCopper", "StatWeightCopper",
		"HeroicStatWeightCopper", "HPWeightCopper", "ManaWeightCopper", "EnduranceWeightCopper",
		"ResistWeightCopper", "AttackWeightCopper", "RegenWeightCopper", "HasteWeightCopper",
		"DamageShieldWeightCopper", "SpellDamageWeightCopper", "HealAmountWeightCopper",
		"Mod2WeightCopper", "WeaponRatioWeightCopper", "WeaponDamageWeightCopper",
		"ElementalDamageWeightCopper", "BackstabDamageWeightCopper", "BagSlotWeightCopper",
		"BagWeightReductionWeightCopper", "FamilyUpgradeValuePercent",
		"HighImpactPremiumMaxPercent", "HighImpactSpellDamageWeightCopper",
		"HighImpactHealAmountWeightCopper", "HighImpactHasteWeightCopper",
		"HighImpactHeroicStatWeightCopper", "HighImpactWeaponRatioWeightCopper",
		"HighImpactWeaponDamageWeightCopper", "ProcEffectMinBonusPP", "ProcEffectMaxBonusPP",
		"ClickEffectMinBonusPP", "ClickEffectMaxBonusPP", "WornEffectMinBonusPP",
		"WornEffectMaxBonusPP", "FocusEffectMinBonusPP", "FocusEffectMaxBonusPP",
		"BardEffectMinBonusPP", "BardEffectMaxBonusPP"
	};
}

static std::string THJFakeBazaarResolveConfigKey(const std::string &key)
{
	for (const auto &known_key : THJFakeBazaarConfigKeys()) {
		if (Strings::EqualFold(key, known_key)) {
			return known_key;
		}
	}

	return "";
}

static bool THJFakeBazaarSetConfigValue(const std::string &key, const std::string &value)
{
	const auto variable_name = THJFakeBazaarConfigVariableName(key);
	const auto escaped_name  = Strings::Escape(variable_name);
	const auto escaped_value = Strings::Escape(value);

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `id` FROM `variables` WHERE `varname` = '{}' LIMIT 1",
			escaped_name
		)
	);

	if (!results.Success()) {
		return false;
	}

	if (results.RowCount()) {
		auto row = results.begin();
		auto update = database.QueryDatabase(
			fmt::format(
				"UPDATE `variables` SET `value` = '{}', `ts` = NOW() WHERE `id` = {}",
				escaped_value,
				THJFakeBazaarRowUInt(row[0])
			)
		);

		database.LoadVariables();
		return update.Success();
	}

	auto insert = database.QueryDatabase(
		fmt::format(
			"INSERT INTO `variables` (`varname`, `value`, `information`, `ts`) VALUES ('{}', '{}', 'THJ fake bazaar configuration', NOW())",
			escaped_name,
			escaped_value
		)
	);

	database.LoadVariables();
	return insert.Success();
}

static void THJFakeBazaarConfigMessage(Client *c)
{
	const auto config = THJFakeBazaarLoadConfig();
	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar seed: seller_items={} buyer_lines={} seller_count={} buyer_count={} seller_mult={}..{} buyer_mult={}..{}.",
			config.seed.seller_items,
			config.seed.buyer_lines,
			config.seed.seller_count,
			config.seed.buyer_count,
			config.seed.seller_min_multiplier,
			config.seed.seller_max_multiplier,
			config.seed.buyer_min_multiplier,
			config.seed.buyer_max_multiplier
		).c_str()
	);
	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar pool: expansion={}..{} drops_per_zone={} item_level={}..{} item_types=[{}].",
			config.seed.min_expansion,
			config.seed.max_expansion,
			config.seed.drops_per_zone,
			config.min_item_level,
			config.max_item_level,
			config.allowed_item_types
		).c_str()
	);
	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar fallback: global_items={} (0 means strict zone-drop pool only).",
			config.allow_global_fallback
		).c_str()
	);
	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar cache: use_cached_item_pool={} allow_live_item_scan={} cached_rows={} pool_last_rebuild={} pool_last_rows={}.",
			config.use_cached_item_pool,
			config.allow_live_item_scan,
			THJFakeBazaarItemPoolCount(),
			THJFakeBazaarConfigString("PoolLastRebuildUnix", "0"),
			THJFakeBazaarConfigString("PoolLastRebuildRows", "0")
		).c_str()
	);
	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar item filters: exclude_tradeskill_items={} exclude_merchant_items={} exclude_spell_scrolls={} require_equipment_stats={} min_equipment_stat_total={} include_item_variants={}.",
			config.exclude_tradeskill_items,
			config.exclude_merchant_items,
			config.exclude_spell_scrolls,
			config.require_equipment_stats,
			config.minimum_equipment_stat_total,
			config.include_item_variants
		).c_str()
	);
	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar automation: enabled={} startup={} empty={} interval={}m minimum_rows={}/{} last_seed={}.",
			config.auto_refresh_enabled,
			config.refresh_on_startup,
			config.refresh_when_empty,
			config.refresh_interval_minutes,
			config.minimum_seller_listings,
			config.minimum_buyer_lines,
			THJFakeBazaarConfigString("LastSeedUnix", "0")
		).c_str()
	);
	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar zones: included=[{}] excluded=[{}] excluded_names=[{}].",
			config.included_zones.empty() ? "all" : config.included_zones,
			config.excluded_zones.empty() ? "none" : config.excluded_zones,
			config.excluded_name_patterns
		).c_str()
	);
	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar pricing: min_base={}pp max={}pp vendor_weight={}%. Proc bonus={}..{}pp.",
			config.pricing.min_base_price / 1000,
			config.pricing.max_price / 1000,
			config.pricing.vendor_weight_percent,
			config.pricing.proc_effect_min_bonus / 1000,
			config.pricing.proc_effect_max_bonus / 1000
		).c_str()
	);
	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar seller premium: family_upgrade={}% high_impact_cap={}% spell_dmg={}pp heal={}pp haste={}pp heroic={}pp weapon_ratio={}pp weapon_dmg={}pp.",
			config.pricing.family_upgrade_value_percent,
			config.pricing.high_impact_premium_max_percent,
			config.pricing.high_impact_spell_damage_weight / 1000,
			config.pricing.high_impact_heal_amount_weight / 1000,
			config.pricing.high_impact_haste_weight / 1000,
			config.pricing.high_impact_heroic_stat_weight / 1000,
			config.pricing.high_impact_weapon_ratio_weight / 1000,
			config.pricing.high_impact_weapon_damage_weight / 1000
		).c_str()
	);
	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar buyer pricing: min_base={}pp max={}pp vendor_weight={}%, stack_cap={}pp mundane_cap={}pp.",
			config.pricing.buyer_min_base_price / 1000,
			config.pricing.buyer_max_price / 1000,
			config.pricing.buyer_vendor_weight_percent,
			config.pricing.buyer_stackable_max_price / 1000,
			config.pricing.buyer_mundane_max_price / 1000
		).c_str()
	);
	c->Message(Chat::White, "Use #fakebazaar set Key Value. Values are stored as compact variables named THJFB.*.");
	c->Message(Chat::White, "Common keys: SellerItems, BuyerLines, MinExpansion, MaxExpansion, IncludedZones, ExcludedZones, AllowedItemTypes, IncludeItemVariants, UseCachedItemPool, AllowLiveItemScan, ExcludeTradeskillItems, ExcludeMerchantItems, ExcludeSpellScrolls, RequireEquipmentStats, MinimumEquipmentStatTotal, AutoRefreshEnabled, RefreshIntervalMinutes, BuyerMundaneMaxPricePP, FamilyUpgradeValuePercent, HighImpactPremiumMaxPercent, HighImpactSpellDamageWeightCopper, HighImpactHealAmountWeightCopper.");
}

static void THJFakeBazaarSetConfig(Client *c, const Seperator *sep)
{
	if (sep->argnum < 3) {
		c->Message(Chat::White, "#fakebazaar set [key] [value]");
		c->Message(Chat::White, "Example: #fakebazaar set IncludedZones unrest,guktop,gukbottom");
		return;
	}

	const auto key = THJFakeBazaarResolveConfigKey(sep->arg[2]);
	if (key.empty()) {
		c->Message(Chat::Red, fmt::format("Unknown fake bazaar config key [{}]. Use #fakebazaar config for common keys.", sep->arg[2]).c_str());
		return;
	}

	const std::string value = sep->argplus[3] ? sep->argplus[3] : "";
	if (!THJFakeBazaarSetConfigValue(key, value)) {
		c->Message(Chat::Red, fmt::format("Could not update fake bazaar config [{}].", key).c_str());
		return;
	}

	c->Message(
		Chat::White,
		fmt::format(
			"Set {} = {} (stored as {}). Run #fakebazaar seed to apply it to new fake bazaar rows.",
			key,
			value,
			THJFakeBazaarConfigVariableName(key)
		).c_str()
	);
}

static THJFakeBazaarActionResult THJFakeBazaarSeedRowsWithSettings(
	THJFakeBazaarSeedSettings settings,
	const THJFakeBazaarConfig &config,
	uint32 seed_zone_instance_id,
	const std::string &reason
)
{
	THJFakeBazaarActionResult result{};
	result.seed_zone_instance_id = seed_zone_instance_id;
	result.min_expansion         = settings.min_expansion;
	result.max_expansion         = settings.max_expansion;
	result.drops_per_zone        = settings.drops_per_zone;

	if (settings.buyer_lines && !settings.buyer_count) {
		settings.buyer_count = 1;
	}

	std::random_device random_device;
	std::mt19937       rng(random_device());

	const uint32 total_items_needed = settings.seller_items + settings.buyer_lines;
	auto seed_items = THJFakeBazaarLoadItems(total_items_needed, settings, config);
	if (seed_items.empty()) {
		result.message = "No eligible items were found for fake bazaar seeding.";
		return result;
	}

	std::shuffle(seed_items.begin(), seed_items.end(), rng);

	if (seed_items.size() < total_items_needed) {
		settings.seller_items = std::min<uint32>(settings.seller_items, static_cast<uint32>(seed_items.size()));
		settings.buyer_lines  = std::min<uint32>(
			settings.buyer_lines,
			static_cast<uint32>(seed_items.size()) - settings.seller_items
		);
	}

	const auto seller_pricing_contexts = THJFakeBazaarBuildPricingContexts(seed_items, config.pricing);

	auto clear_result = THJFakeBazaarClear();
	result.cleared_traders           = clear_result.traders;
	result.cleared_buyers            = clear_result.buyers;
	result.cleared_buyer_lines       = clear_result.buyer_lines;
	result.cleared_buyer_trade_items = clear_result.buyer_trade_items;

	std::vector<uint32> seller_character_ids;
	seller_character_ids.reserve(settings.seller_count);
	for (uint32 i = 0; i < settings.seller_count; ++i) {
		auto character = THJFakeBazaarEnsureCharacter(
			THJFakeBazaarName(THJ_FAKE_BAZAAR_SELLER_PREFIX, i),
			seed_zone_instance_id
		);
		if (character.id) {
			seller_character_ids.push_back(character.id);
		}
	}

	std::vector<BuyerRepository::Buyer> buyers;
	buyers.reserve(settings.buyer_count);
	for (uint32 i = 0; i < settings.buyer_count; ++i) {
		auto character = THJFakeBazaarEnsureCharacter(
			THJFakeBazaarName(THJ_FAKE_BAZAAR_BUYER_PREFIX, i),
			seed_zone_instance_id
		);
		if (!character.id) {
			continue;
		}

		auto buyer                   = BuyerRepository::NewEntity();
		buyer.char_id                = character.id;
		buyer.char_entity_id         = 0;
		buyer.char_name              = character.name;
		buyer.char_zone_id           = THJ_FAKE_BAZAAR_ZONE_ID;
		buyer.char_zone_instance_id  = seed_zone_instance_id;
		buyer.transaction_date       = time(nullptr);
		buyer.welcome_message        = "The house bounty board is buying.";
		buyer                        = BuyerRepository::InsertOne(database, buyer);

		if (buyer.id) {
			buyers.push_back(buyer);
		}
	}

	if (seller_character_ids.empty()) {
		result.message = "Could not create fake bazaar seller characters.";
		return result;
	}

	std::vector<uint32> seller_slot_counts(seller_character_ids.size(), 0);
	std::vector<TraderRepository::Trader> trader_entries;
	trader_entries.reserve(settings.seller_items);

	for (uint32 i = 0; i < settings.seller_items && i < seed_items.size(); ++i) {
		const auto seller_index = i % seller_character_ids.size();
		const auto pricing_context = seller_pricing_contexts.find(seed_items[i].id);
		auto trader             = TraderRepository::NewEntity();
		trader.char_id          = seller_character_ids[seller_index];
		trader.item_id          = seed_items[i].id;
		trader.item_sn          = THJ_FAKE_BAZAAR_SERIAL_BASE + i;
		trader.item_charges     = THJFakeBazaarTraderCharges(seed_items[i], rng);
		trader.item_cost        = THJFakeBazaarRandomPrice(
			seed_items[i],
			settings.seller_min_multiplier,
			settings.seller_max_multiplier,
			config.pricing,
			pricing_context != seller_pricing_contexts.end() ? &pricing_context->second : nullptr,
			rng
		);
		trader.slot_id               = static_cast<uint8>(seller_slot_counts[seller_index]++);
		trader.char_entity_id        = 0;
		trader.char_zone_id          = THJ_FAKE_BAZAAR_ZONE_ID;
		trader.char_zone_instance_id = seed_zone_instance_id;
		trader.active_transaction    = 0;
		trader.listing_date          = time(nullptr);

		trader_entries.push_back(trader);
	}

	const int inserted_traders = trader_entries.empty() ? 0 : TraderRepository::InsertMany(database, trader_entries);

	std::vector<BuyerBuyLinesRepository::BuyerBuyLines> buy_line_entries;
	if (!buyers.empty()) {
		buy_line_entries.reserve(settings.buyer_lines);
		std::vector<uint32> buyer_slot_counts(buyers.size(), 0);

		for (uint32 i = 0; i < settings.buyer_lines; ++i) {
			const uint32 item_index = settings.seller_items + i;
			if (item_index >= seed_items.size()) {
				break;
			}

			const auto buyer_index = i % buyers.size();
			auto buy_line          = BuyerBuyLinesRepository::NewEntity();
			buy_line.buyer_id      = buyers[buyer_index].id;
			buy_line.char_id       = buyers[buyer_index].char_id;
			buy_line.buy_slot_id   = static_cast<int32>(buyer_slot_counts[buyer_index]++);
			buy_line.item_id       = static_cast<int32>(seed_items[item_index].id);
			buy_line.item_qty      = THJFakeBazaarBuyerQuantity(seed_items[item_index], rng);
			buy_line.item_price    = static_cast<int32>(
				THJFakeBazaarRandomBuyerPrice(
					seed_items[item_index],
					settings.buyer_min_multiplier,
					settings.buyer_max_multiplier,
					config.pricing,
					rng
				)
			);
			buy_line.item_icon     = seed_items[item_index].icon;
			buy_line.item_name     = seed_items[item_index].name;

			buy_line_entries.push_back(buy_line);
		}
	}

	const int inserted_buy_lines = buy_line_entries.empty() ? 0 : BuyerBuyLinesRepository::InsertMany(database, buy_line_entries);

	result.success            = inserted_traders > 0 || inserted_buy_lines > 0;
	result.inserted_traders   = inserted_traders;
	result.inserted_buy_lines = inserted_buy_lines;
	result.seller_count       = static_cast<uint32>(seller_character_ids.size());
	result.buyer_count        = static_cast<uint32>(buyers.size());
	result.message            = fmt::format(
		"Seeded fake bazaar rows via {}.",
		reason.empty() ? "manual" : reason
	);

	if (result.success) {
		THJFakeBazaarSetConfigValue("LastSeedUnix", std::to_string(time(nullptr)));
	}

	return result;
}

THJFakeBazaarActionResult THJFakeBazaarClearRows()
{
	THJFakeBazaarActionResult result{};
	auto clear_result = THJFakeBazaarClear();

	result.success                    = true;
	result.cleared_traders            = clear_result.traders;
	result.cleared_buyers             = clear_result.buyers;
	result.cleared_buyer_lines        = clear_result.buyer_lines;
	result.cleared_buyer_trade_items  = clear_result.buyer_trade_items;
	result.message                    = "Cleared fake bazaar rows.";

	return result;
}

THJFakeBazaarActionResult THJFakeBazaarSeedRows(uint32 seed_zone_instance_id, const std::string &reason)
{
	const auto config = THJFakeBazaarLoadConfig();
	return THJFakeBazaarSeedRowsWithSettings(config.seed, config, seed_zone_instance_id, reason);
}

static uint32 THJFakeBazaarLastSeedUnix()
{
	std::string value;
	if (!THJFakeBazaarConfigValueExists("LastSeedUnix", value)) {
		return 0;
	}

	return Strings::ToUnsignedInt(value, 0);
}

static bool THJFakeBazaarTryRefreshLock()
{
	auto results = database.QueryDatabase("SELECT GET_LOCK('THJFakeBazaar.Refresh', 0)");
	if (!results.Success() || !results.RowCount()) {
		return false;
	}

	auto row = results.begin();
	return THJFakeBazaarRowUInt(row[0]) == 1;
}

static void THJFakeBazaarReleaseRefreshLock()
{
	database.QueryDatabase("DO RELEASE_LOCK('THJFakeBazaar.Refresh')");
}

static THJFakeBazaarActionResult THJFakeBazaarSkippedResult(const std::string &message)
{
	THJFakeBazaarActionResult result{};
	result.success = true;
	result.skipped = true;
	result.message = message;
	return result;
}

static THJFakeBazaarActionResult THJFakeBazaarRefreshRowsInternal(
	bool force,
	bool startup_check,
	const std::string &reason
)
{
	if (!THJFakeBazaarTryRefreshLock()) {
		return THJFakeBazaarSkippedResult("Another zone process is already checking fake bazaar refresh.");
	}

	auto config = THJFakeBazaarLoadConfig();
	if (!force && !config.auto_refresh_enabled) {
		THJFakeBazaarReleaseRefreshLock();
		return THJFakeBazaarSkippedResult("Fake bazaar auto-refresh is disabled.");
	}

	const uint32 seller_listings = THJFakeBazaarSellerListingCount();
	const uint32 buyer_lines     = THJFakeBazaarBuyerLineCount();
	const uint32 min_sellers     = config.seed.seller_items ? config.minimum_seller_listings : 0;
	const uint32 min_buyers      = config.seed.buyer_lines ? config.minimum_buyer_lines : 0;

	bool should_seed = force;
	std::string refresh_reason = reason.empty() ? "refresh" : reason;

	if (!should_seed && startup_check && config.refresh_on_startup) {
		if (seller_listings < min_sellers || buyer_lines < min_buyers) {
			should_seed = true;
			refresh_reason = "startup";
		}
	}

	if (!should_seed && config.refresh_when_empty) {
		if (seller_listings < min_sellers || buyer_lines < min_buyers) {
			should_seed = true;
			refresh_reason = "minimum-row-check";
		}
	}

	if (!should_seed && config.refresh_interval_minutes) {
		const uint32 now = static_cast<uint32>(time(nullptr));
		const uint32 last_seed = THJFakeBazaarLastSeedUnix();
		const uint64 interval_seconds = static_cast<uint64>(config.refresh_interval_minutes) * 60;

		if (!last_seed) {
			THJFakeBazaarSetConfigValue("LastSeedUnix", std::to_string(now));
		} else if (static_cast<uint64>(now) >= static_cast<uint64>(last_seed) + interval_seconds) {
			should_seed = true;
			refresh_reason = "timer";
		}
	}

	if (!should_seed) {
		THJFakeBazaarReleaseRefreshLock();
		return THJFakeBazaarSkippedResult("Fake bazaar rows are already populated and refresh is not due.");
	}

	auto result = THJFakeBazaarSeedRowsWithSettings(config.seed, config, 0, refresh_reason);
	THJFakeBazaarReleaseRefreshLock();
	return result;
}

THJFakeBazaarActionResult THJFakeBazaarRefreshRows(bool force, const std::string &reason)
{
	return THJFakeBazaarRefreshRowsInternal(force, false, reason);
}

void THJFakeBazaarProcessAutoRefresh()
{
	static bool startup_check_pending = true;

	const bool startup_check = startup_check_pending;
	startup_check_pending = false;

	auto result = THJFakeBazaarRefreshRowsInternal(false, startup_check, startup_check ? "startup" : "timer");
	if (result.success && !result.skipped) {
		LogInfo("{}", result.message);
	} else if (!result.success && !result.message.empty()) {
		LogError("Fake bazaar auto-refresh failed: {}", result.message);
	}
}

static void THJFakeBazaarSeed(Client *c, const Seperator *sep)
{
	auto config = THJFakeBazaarLoadConfig();
	auto settings = THJFakeBazaarParseSeedSettings(sep, config);
	const uint32 seed_zone_instance_id = c->GetZoneID() == THJ_FAKE_BAZAAR_ZONE_ID ? c->GetInstanceID() : 0;
	auto result = THJFakeBazaarSeedRowsWithSettings(settings, config, seed_zone_instance_id, "command");

	if (!result.success) {
		c->Message(Chat::Red, result.message.c_str());
		return;
	}

	c->Message(
		Chat::White,
		fmt::format(
			"Cleared {} fake seller listings, {} fake buyers, and {} fake bounty lines.",
			result.cleared_traders,
			result.cleared_buyers,
			result.cleared_buyer_lines
		).c_str()
	);
	c->Message(
		Chat::White,
		fmt::format(
			"Seeded {} fake bazaar listings across {} sellers and {} bounty buy lines across {} system buyers.",
			result.inserted_traders,
			result.seller_count,
			result.inserted_buy_lines,
			result.buyer_count
		).c_str()
	);
	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar rows were seeded for Bazaar instance {} using zone drops from expansions {}..{} with up to {} first-pass drops per zone.",
			result.seed_zone_instance_id,
			result.min_expansion,
			result.max_expansion,
			result.drops_per_zone
		).c_str()
	);
}

void command_fakebazaar(Client *c, const Seperator *sep)
{
	if (!sep->argnum) {
		THJFakeBazaarUsage(c);
		return;
	}

	const bool is_seed   = !strcasecmp(sep->arg[1], "seed");
	const bool is_clear  = !strcasecmp(sep->arg[1], "clear");
	const bool is_refresh = !strcasecmp(sep->arg[1], "refresh");
	const bool is_status = !strcasecmp(sep->arg[1], "status");
	const bool is_pool   = !strcasecmp(sep->arg[1], "pool");
	const bool is_config = !strcasecmp(sep->arg[1], "config");
	const bool is_set    = !strcasecmp(sep->arg[1], "set");

	if (is_status) {
		THJFakeBazaarStatus(c);
		return;
	}

	if (is_pool) {
		if (sep->argnum < 2 || !strcasecmp(sep->arg[2], "status")) {
			THJFakeBazaarPoolStatus(c);
			return;
		}

		THJFakeBazaarUsage(c);
		return;
	}

	if (is_config) {
		THJFakeBazaarConfigMessage(c);
		return;
	}

	if (is_set) {
		THJFakeBazaarSetConfig(c, sep);
		return;
	}

	if (is_clear) {
		auto result = THJFakeBazaarClearRows();
		c->Message(
			Chat::White,
			fmt::format(
				"Cleared {} fake seller listings, {} fake buyers, {} fake bounty buy lines, and {} fake compensation rows.",
				result.cleared_traders,
				result.cleared_buyers,
				result.cleared_buyer_lines,
				result.cleared_buyer_trade_items
			).c_str()
		);
		return;
	}

	if (is_refresh) {
		const bool force_refresh =
			sep->argnum >= 2 &&
			(!strcasecmp(sep->arg[2], "force") || THJFakeBazaarRowUInt(sep->arg[2]) != 0);
		auto result = THJFakeBazaarRefreshRows(force_refresh, "command");
		if (!result.success) {
			c->Message(Chat::Red, result.message.c_str());
			return;
		}

		if (result.skipped) {
			c->Message(Chat::White, result.message.c_str());
			return;
		}

		c->Message(
			Chat::White,
			fmt::format(
				"Refreshed fake bazaar rows: {} seller listings across {} sellers and {} bounty buy lines across {} system buyers.",
				result.inserted_traders,
				result.seller_count,
				result.inserted_buy_lines,
				result.buyer_count
			).c_str()
		);
		return;
	}

	if (is_seed) {
		THJFakeBazaarSeed(c, sep);
		return;
	}

	THJFakeBazaarUsage(c);
}

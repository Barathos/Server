#include "../client.h"
#include "../../common/eq_constants.h"
#include "../../common/repositories/buyer_buy_lines_repository.h"
#include "../../common/repositories/buyer_repository.h"
#include "../../common/repositories/buyer_trade_items_repository.h"
#include "../../common/repositories/character_data_repository.h"
#include "../../common/repositories/trader_repository.h"

#include <algorithm>
#include <ctime>
#include <random>
#include <vector>

constexpr const char *THJ_FAKE_BAZAAR_SELLER_PREFIX = "Thjvendor";
constexpr const char *THJ_FAKE_BAZAAR_BUYER_PREFIX  = "Thjbounty";
constexpr uint32      THJ_FAKE_BAZAAR_ZONE_ID       = Zones::BAZAAR;
constexpr uint32      THJ_FAKE_BAZAAR_SERIAL_BASE   = 2000000000;
constexpr uint64      THJ_FAKE_BAZAAR_MAX_PRICE     = 2000000000;
constexpr uint32      THJ_FAKE_BAZAAR_MAX_PER_TRADER = 200;

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
};

struct THJFakeBazaarClearResult {
	int traders;
	int buyer_trade_items;
	int buyer_lines;
	int buyers;
};

static uint32 THJFakeBazaarRowUInt(const char *value)
{
	return value ? static_cast<uint32>(strtoul(value, nullptr, 10)) : 0;
}

static int32 THJFakeBazaarRowInt(const char *value)
{
	return value ? static_cast<int32>(atoi(value)) : 0;
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

static THJFakeBazaarSeedSettings THJFakeBazaarParseSeedSettings(const Seperator *sep)
{
	THJFakeBazaarSeedSettings settings{};
	settings.seller_items          = THJFakeBazaarClampedArg(sep, 2, 400, 1, 5000);
	settings.buyer_lines           = THJFakeBazaarClampedArg(sep, 3, 120, 0, 1000);
	settings.seller_count          = THJFakeBazaarClampedArg(sep, 4, 12, 1, 80);
	settings.buyer_count           = THJFakeBazaarClampedArg(sep, 5, 6, 0, 80);
	settings.seller_min_multiplier = THJFakeBazaarClampedArg(sep, 6, 2, 1, 100);
	settings.seller_max_multiplier = THJFakeBazaarClampedArg(sep, 7, 12, 1, 100);
	settings.buyer_min_multiplier  = THJFakeBazaarClampedArg(sep, 8, 10, 1, 200);
	settings.buyer_max_multiplier  = THJFakeBazaarClampedArg(sep, 9, 30, 1, 200);

	if (settings.seller_min_multiplier > settings.seller_max_multiplier) {
		std::swap(settings.seller_min_multiplier, settings.seller_max_multiplier);
	}

	if (settings.buyer_min_multiplier > settings.buyer_max_multiplier) {
		std::swap(settings.buyer_min_multiplier, settings.buyer_max_multiplier);
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
	c->Message(
		Chat::White,
		"#fakebazaar seed [seller_items=400] [buyer_lines=120] [seller_count=12] [buyer_count=6] [seller_mult_min=2] [seller_mult_max=12] [buyer_mult_min=10] [buyer_mult_max=30]"
	);
}

static CharacterDataRepository::CharacterData THJFakeBazaarEnsureCharacter(const std::string &name)
{
	auto character = CharacterDataRepository::FindByName(database, name);
	if (character.id) {
		if (character.deleted_at > 0) {
			character.deleted_at = 0;
			CharacterDataRepository::UpdateOne(database, character);
		}

		return character;
	}

	character                = CharacterDataRepository::NewEntity();
	character.account_id     = 0;
	character.name           = name;
	character.zone_id        = THJ_FAKE_BAZAAR_ZONE_ID;
	character.zone_instance  = 0;
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

static std::vector<THJFakeBazaarItem> THJFakeBazaarLoadItems(uint32 item_count)
{
	std::vector<THJFakeBazaarItem> items;
	if (!item_count) {
		return items;
	}

	auto results = content_db.QueryDatabase(
		fmt::format(
			R"(
				SELECT id, Name, icon, price, ldonprice, stackable, stacksize, maxcharges, itemclass, itemtype,
					slots, bagslots, ac, hp, mana, damage, reclevel, reqlevel, scrolleffect, focuseffect
				FROM items
				WHERE id > 0
					AND minstatus = 0
					AND nodrop <> 0
					AND norent <> 0
					AND summonedflag = 0
					AND notransfer = 0
					AND Name <> ''
					AND Name NOT LIKE 'Deprecated%'
					AND Name NOT LIKE '%Test%'
					AND (
						itemclass IN (1, 2)
						OR slots <> 0
						OR bagslots > 0
						OR itemtype IN (0, 1, 2, 3, 4, 5, 8, 10, 11, 16, 17, 20, 21, 23, 24, 25, 26, 27, 29, 35, 45, 54, 67)
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
				ORDER BY RAND()
				LIMIT {}
			)",
			item_count
		)
	);

	if (!results.Success() || !results.RowCount()) {
		return items;
	}

	items.reserve(results.RowCount());
	for (auto row = results.begin(); row != results.end(); ++row) {
		THJFakeBazaarItem item{};
		item.id            = THJFakeBazaarRowUInt(row[0]);
		item.name          = row[1] ? row[1] : "";
		item.icon          = THJFakeBazaarRowUInt(row[2]);
		item.price         = THJFakeBazaarRowUInt(row[3]);
		item.ldon_price    = THJFakeBazaarRowUInt(row[4]);
		item.stackable     = THJFakeBazaarRowUInt(row[5]) != 0;
		item.stack_size    = THJFakeBazaarRowUInt(row[6]);
		item.max_charges   = THJFakeBazaarRowInt(row[7]);
		item.item_class    = THJFakeBazaarRowUInt(row[8]);
		item.item_type     = THJFakeBazaarRowUInt(row[9]);
		item.slots         = THJFakeBazaarRowUInt(row[10]);
		item.bag_slots     = THJFakeBazaarRowUInt(row[11]);
		item.ac            = THJFakeBazaarRowInt(row[12]);
		item.hp            = THJFakeBazaarRowInt(row[13]);
		item.mana          = THJFakeBazaarRowInt(row[14]);
		item.damage        = THJFakeBazaarRowInt(row[15]);
		item.rec_level     = THJFakeBazaarRowInt(row[16]);
		item.req_level     = THJFakeBazaarRowInt(row[17]);
		item.scroll_effect = THJFakeBazaarRowInt(row[18]);
		item.focus_effect  = THJFakeBazaarRowInt(row[19]);

		if (item.id && !item.name.empty()) {
			items.push_back(item);
		}
	}

	return items;
}

static uint64 THJFakeBazaarBaseValue(const THJFakeBazaarItem &item)
{
	uint64 base_value = item.price ? item.price : item.ldon_price;

	if (!base_value && (item.scroll_effect > 0 || item.item_type == 20)) {
		const uint32 level = static_cast<uint32>(std::max(1, std::max(item.rec_level, item.req_level)));
		base_value = static_cast<uint64>(level) * 5000;
	}

	if (!base_value && item.bag_slots > 0) {
		base_value = static_cast<uint64>(item.bag_slots) * 25000;
	}

	if (!base_value && item.slots != 0) {
		const uint32 level = static_cast<uint32>(std::max(0, std::max(item.rec_level, item.req_level)));
		base_value =
			(static_cast<uint64>(std::max(0, item.ac)) * 100) +
			(static_cast<uint64>(std::max(0, item.hp)) * 20) +
			(static_cast<uint64>(std::max(0, item.mana)) * 20) +
			(static_cast<uint64>(std::max(0, item.damage)) * 500) +
			(static_cast<uint64>(level) * 1000);
	}

	return std::max<uint64>(base_value, 1000);
}

static uint32 THJFakeBazaarRandomPrice(
	const THJFakeBazaarItem &item,
	uint32 min_multiplier,
	uint32 max_multiplier,
	std::mt19937 &rng
)
{
	std::uniform_int_distribution<uint32> multiplier_distribution(min_multiplier, max_multiplier);
	const uint64 total = THJFakeBazaarBaseValue(item) * multiplier_distribution(rng);

	return static_cast<uint32>(std::clamp<uint64>(total, 1, THJ_FAKE_BAZAAR_MAX_PRICE));
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

static int32 THJFakeBazaarBuyerQuantity(const THJFakeBazaarItem &item, std::mt19937 &rng)
{
	if (item.stackable) {
		const uint32 max_stack = std::clamp<uint32>(item.stack_size ? item.stack_size : 1, 1, 100);
		std::uniform_int_distribution<uint32> stack_distribution(std::min<uint32>(5, max_stack), max_stack);
		return static_cast<int32>(stack_distribution(rng));
	}

	std::uniform_int_distribution<int32> quantity_distribution(1, 3);
	return quantity_distribution(rng);
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

static void THJFakeBazaarStatus(Client *c)
{
	const auto seller_prefix = Strings::Escape(THJ_FAKE_BAZAAR_SELLER_PREFIX);
	const auto buyer_prefix  = Strings::Escape(THJ_FAKE_BAZAAR_BUYER_PREFIX);

	const auto sellers = CharacterDataRepository::Count(
		database,
		fmt::format("`name` LIKE '{}%'", seller_prefix)
	);
	const auto seller_listings = TraderRepository::Count(
		database,
		fmt::format(
			"`char_id` IN (SELECT `id` FROM `character_data` WHERE `name` LIKE '{}%')",
			seller_prefix
		)
	);
	const auto buyers = BuyerRepository::Count(
		database,
		fmt::format("`char_name` LIKE '{}%'", buyer_prefix)
	);
	const auto buyer_lines = BuyerBuyLinesRepository::Count(
		database,
		fmt::format(
			"`buyer_id` IN (SELECT `id` FROM `buyer` WHERE `char_name` LIKE '{}%')",
			buyer_prefix
		)
	);

	c->Message(
		Chat::White,
		fmt::format(
			"Fake bazaar status: {} seller characters, {} seller listings, {} system buyers, {} bounty buy lines.",
			sellers,
			seller_listings,
			buyers,
			buyer_lines
		).c_str()
	);
}

static void THJFakeBazaarSeed(Client *c, const Seperator *sep)
{
	auto settings = THJFakeBazaarParseSeedSettings(sep);
	if (settings.buyer_lines && !settings.buyer_count) {
		settings.buyer_count = 1;
	}

	const uint32 total_items_needed = settings.seller_items + settings.buyer_lines;
	auto seed_items = THJFakeBazaarLoadItems(total_items_needed);
	if (seed_items.empty()) {
		c->Message(Chat::Red, "No eligible items were found for fake bazaar seeding.");
		return;
	}

	if (seed_items.size() < total_items_needed) {
		settings.seller_items = std::min<uint32>(settings.seller_items, static_cast<uint32>(seed_items.size()));
		settings.buyer_lines  = std::min<uint32>(
			settings.buyer_lines,
			static_cast<uint32>(seed_items.size()) - settings.seller_items
		);
	}

	auto clear_result = THJFakeBazaarClear();

	std::random_device random_device;
	std::mt19937       rng(random_device());

	std::vector<uint32> seller_character_ids;
	seller_character_ids.reserve(settings.seller_count);
	for (uint32 i = 0; i < settings.seller_count; ++i) {
		auto character = THJFakeBazaarEnsureCharacter(THJFakeBazaarName(THJ_FAKE_BAZAAR_SELLER_PREFIX, i));
		if (character.id) {
			seller_character_ids.push_back(character.id);
		}
	}

	std::vector<BuyerRepository::Buyer> buyers;
	buyers.reserve(settings.buyer_count);
	for (uint32 i = 0; i < settings.buyer_count; ++i) {
		auto character = THJFakeBazaarEnsureCharacter(THJFakeBazaarName(THJ_FAKE_BAZAAR_BUYER_PREFIX, i));
		if (!character.id) {
			continue;
		}

		auto buyer                   = BuyerRepository::NewEntity();
		buyer.char_id                = character.id;
		buyer.char_entity_id         = 0;
		buyer.char_name              = character.name;
		buyer.char_zone_id           = THJ_FAKE_BAZAAR_ZONE_ID;
		buyer.char_zone_instance_id  = 0;
		buyer.transaction_date       = time(nullptr);
		buyer.welcome_message        = "The house bounty board is buying.";
		buyer                        = BuyerRepository::InsertOne(database, buyer);

		if (buyer.id) {
			buyers.push_back(buyer);
		}
	}

	if (seller_character_ids.empty()) {
		c->Message(Chat::Red, "Could not create fake bazaar seller characters.");
		return;
	}

	std::vector<uint32> seller_slot_counts(seller_character_ids.size(), 0);
	std::vector<TraderRepository::Trader> trader_entries;
	trader_entries.reserve(settings.seller_items);

	for (uint32 i = 0; i < settings.seller_items && i < seed_items.size(); ++i) {
		const auto seller_index = i % seller_character_ids.size();
		auto trader             = TraderRepository::NewEntity();
		trader.char_id          = seller_character_ids[seller_index];
		trader.item_id          = seed_items[i].id;
		trader.item_sn          = THJ_FAKE_BAZAAR_SERIAL_BASE + i;
		trader.item_charges     = THJFakeBazaarTraderCharges(seed_items[i], rng);
		trader.item_cost        = THJFakeBazaarRandomPrice(
			seed_items[i],
			settings.seller_min_multiplier,
			settings.seller_max_multiplier,
			rng
		);
		trader.slot_id               = static_cast<uint8>(seller_slot_counts[seller_index]++);
		trader.char_entity_id        = 0;
		trader.char_zone_id          = THJ_FAKE_BAZAAR_ZONE_ID;
		trader.char_zone_instance_id = 0;
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
				THJFakeBazaarRandomPrice(
					seed_items[item_index],
					settings.buyer_min_multiplier,
					settings.buyer_max_multiplier,
					rng
				)
			);
			buy_line.item_icon     = seed_items[item_index].icon;
			buy_line.item_name     = seed_items[item_index].name;

			buy_line_entries.push_back(buy_line);
		}
	}

	const int inserted_buy_lines = buy_line_entries.empty() ? 0 : BuyerBuyLinesRepository::InsertMany(database, buy_line_entries);

	c->Message(
		Chat::White,
		fmt::format(
			"Cleared {} fake seller listings, {} fake buyers, and {} fake bounty lines.",
			clear_result.traders,
			clear_result.buyers,
			clear_result.buyer_lines
		).c_str()
	);
	c->Message(
		Chat::White,
		fmt::format(
			"Seeded {} fake bazaar listings across {} sellers and {} bounty buy lines across {} system buyers.",
			inserted_traders,
			seller_character_ids.size(),
			inserted_buy_lines,
			buyers.size()
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
	const bool is_status = !strcasecmp(sep->arg[1], "status");

	if (is_status) {
		THJFakeBazaarStatus(c);
		return;
	}

	if (is_clear) {
		auto result = THJFakeBazaarClear();
		c->Message(
			Chat::White,
			fmt::format(
				"Cleared {} fake seller listings, {} fake buyers, {} fake bounty buy lines, and {} fake compensation rows.",
				result.traders,
				result.buyers,
				result.buyer_lines,
				result.buyer_trade_items
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

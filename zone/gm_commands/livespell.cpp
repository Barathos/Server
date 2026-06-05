/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "common/data_bucket.h"
#include "common/item_data.h"
#include "common/ipc_mutex.h"
#include "common/json/json.hpp"
#include "common/repositories/data_buckets_repository.h"
#include "common/repositories/items_repository.h"
#include "common/rulesys.h"
#include "common/spdat.h"
#include "common/strings.h"
#include "zone/client.h"
#include "zone/live_spell_manager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <ctime>
#include <limits>
#include <string>
#include <unordered_set>

using json = nlohmann::json;

namespace {
	constexpr uint16 LiveSpellDefaultID = 42527;
	constexpr uint16 LiveSpellDefaultBaseID = 93; // Burst of Flame in stock spell files
	constexpr int LiveSpellDefaultGem = 0;
	constexpr uint16 LiveSpellReserveMinID = 42500;
	constexpr uint16 LiveSpellReserveMaxID = 42602;
	constexpr uint32 LiveSpellBaseScrollID = 15205;
	constexpr const char *LiveSpellBucketPrefix = "live_spell.spell.";
	constexpr const char *LiveSpellNextScrollBucket = "live_spell.next_scroll_item_id";
	bool gLiveSpellServerLoaded = false;

	bool LiveSpellsEnabled()
	{
		return RuleB(CustomFeatures, LiveSpellsEnabled);
	}

	struct LiveSpellElement {
		const char *key;
		const char *label;
		const char *noun;
		int resist_type;
		int icon;
	};

	struct LiveSpellTarget {
		const char *key;
		const char *label;
		SpellTargetType target_type;
		float aoe_range;
	};

	struct LiveSpellDefinition {
		uint16 spell_id = 0;
		uint16 base_spell_id = LiveSpellDefaultBaseID;
		uint32 scroll_item_id = 0;
		uint32 owner_character_id = 0;
		uint32 class_mask = 0;
		uint32 version = 0;
		std::string name;
		std::string element;
		std::string target;
		int target_type = ST_Target;
		int resist_type = RESIST_FIRE;
		int range = 200;
		int aoe_range = 0;
		int damage = 100;
		int mana = 20;
		int cast_time = 2500;
		int recovery_time = 1500;
		int recast_time = 3000;
		int icon = 28;
		int book_icon = 28;
		int level = 1;
	};

	const std::array<LiveSpellElement, 5> LiveSpellElements = {{
		{"fire", "Fire", "Ember", RESIST_FIRE, 28},
		{"cold", "Cold", "Frost", RESIST_COLD, 31},
		{"magic", "Magic", "Arcane", RESIST_MAGIC, 44},
		{"poison", "Poison", "Venom", RESIST_POISON, 47},
		{"disease", "Disease", "Blight", RESIST_DISEASE, 56},
	}};

	const std::array<LiveSpellTarget, 3> LiveSpellTargets = {{
		{"target", "Single Target", ST_Target, 0.0f},
		{"ae", "Targeted AE", ST_AETarget, 35.0f},
		{"pbae", "Point Blank AE", ST_AECaster, 35.0f},
	}};

	bool IsLiveSpellSlotUsable(Client *c, uint32 spell_id, uint32 base_spell_id);
	uint32 BuildLiveSpellVersion();

	template <size_t Size>
	void CopySpellText(char (&destination)[Size], const std::string &source)
	{
		std::memset(destination, 0, Size);
		std::strncpy(destination, source.c_str(), Size - 1);
	}

	std::string SanitizeLiveSpellText(const std::string &value, const size_t max_length)
	{
		std::string sanitized;
		sanitized.reserve(std::min(value.size(), max_length));

		for (const auto ch : value) {
			if (ch == '|' || ch == '=') {
				continue;
			}

			sanitized.push_back(ch);
			if (sanitized.size() >= max_length) {
				break;
			}
		}

		return sanitized;
	}

	std::string TrimLiveSpellText(std::string value)
	{
		while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
			value.erase(value.begin());
		}

		while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
			value.pop_back();
		}

		return value;
	}

	std::string DecodeLiveSpellCommandText(const std::string &value)
	{
		std::string decoded;
		decoded.reserve(std::min<size_t>(value.size(), 60));

		for (const auto raw_ch : value) {
			const auto ch = raw_ch == '_' ? ' ' : raw_ch;
			const bool alpha = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
			const bool digit = ch >= '0' && ch <= '9';
			const bool separator = ch == ' ' || ch == '-' || ch == '\'';
			if (!alpha && !digit && !separator) {
				continue;
			}

			if (std::isspace(static_cast<unsigned char>(ch))) {
				if (decoded.empty() || decoded.back() == ' ') {
					continue;
				}

				decoded.push_back(' ');
			} else {
				decoded.push_back(ch);
			}

			if (decoded.size() >= 60) {
				break;
			}
		}

		return TrimLiveSpellText(decoded);
	}

	bool HasLiveSpellNamedArgs(const Seperator *sep)
	{
		for (int index = 2; index <= sep->argnum; ++index) {
			if (std::strchr(sep->arg[index], '=')) {
				return true;
			}
		}

		return false;
	}

	std::string GetLiveSpellNamedArg(const Seperator *sep, const std::string &key)
	{
		const auto wanted = Strings::ToLower(key);
		for (int index = 2; index <= sep->argnum; ++index) {
			const std::string token = sep->arg[index];
			const auto separator = token.find('=');
			if (separator == std::string::npos) {
				continue;
			}

			if (Strings::ToLower(token.substr(0, separator)) == wanted) {
				return token.substr(separator + 1);
			}
		}

		return {};
	}

	int GetLiveSpellNamedInt(const Seperator *sep, const std::string &key, const int fallback)
	{
		const auto value = GetLiveSpellNamedArg(sep, key);
		return value.empty() ? fallback : Strings::ToInt(value);
	}

	std::string GetLiveSpellPairValue(const std::string &payload, const std::string &key)
	{
		const auto prefix = key + "=";
		size_t pos = 0;

		while (pos < payload.size()) {
			const auto end = payload.find('|', pos);
			const auto count = end == std::string::npos ? std::string::npos : end - pos;
			const auto part = payload.substr(pos, count);

			if (part.compare(0, prefix.size(), prefix) == 0) {
				return part.substr(prefix.size());
			}

			if (end == std::string::npos) {
				break;
			}

			pos = end + 1;
		}

		return {};
	}

	int GetLiveSpellPairInt(const std::string &payload, const std::string &key, const int fallback = 0)
	{
		const auto value = GetLiveSpellPairValue(payload, key);
		return value.empty() ? fallback : Strings::ToInt(value);
	}

	const LiveSpellElement &ResolveLiveSpellElement(const std::string &element)
	{
		const auto key = Strings::ToLower(element);
		for (const auto &entry : LiveSpellElements) {
			if (key == entry.key) {
				return entry;
			}
		}

		return LiveSpellElements.front();
	}

	const LiveSpellTarget &ResolveLiveSpellTarget(const std::string &target)
	{
		const auto key = Strings::ToLower(target);
		for (const auto &entry : LiveSpellTargets) {
			if (key == entry.key) {
				return entry;
			}
		}

		return LiveSpellTargets.front();
	}

	std::string SerializeLiveSpellDefinition(const LiveSpellDefinition &spell)
	{
		return fmt::format(
			"spell_id={}|base={}|scroll={}|owner={}|class_mask={}|version={}|name={}|element={}|target={}|target_type={}|range={}|aoe_range={}|damage={}|mana={}|cast={}|recovery={}|recast={}|resist={}|icon={}|book_icon={}|level={}",
			spell.spell_id,
			spell.base_spell_id,
			spell.scroll_item_id,
			spell.owner_character_id,
			spell.class_mask,
			spell.version,
			SanitizeLiveSpellText(spell.name, 60),
			SanitizeLiveSpellText(spell.element, 16),
			SanitizeLiveSpellText(spell.target, 16),
			spell.target_type,
			spell.range,
			spell.aoe_range,
			spell.damage,
			spell.mana,
			spell.cast_time,
			spell.recovery_time,
			spell.recast_time,
			spell.resist_type,
			spell.icon,
			spell.book_icon,
			spell.level
		);
	}

	LiveSpellDefinition ParseLiveSpellDefinition(const std::string &payload)
	{
		LiveSpellDefinition spell;
		spell.spell_id = static_cast<uint16>(GetLiveSpellPairInt(payload, "spell_id"));
		spell.base_spell_id = static_cast<uint16>(GetLiveSpellPairInt(payload, "base", LiveSpellDefaultBaseID));
		spell.scroll_item_id = static_cast<uint32>(GetLiveSpellPairInt(payload, "scroll"));
		spell.owner_character_id = static_cast<uint32>(GetLiveSpellPairInt(payload, "owner"));
		spell.class_mask = static_cast<uint32>(GetLiveSpellPairInt(payload, "class_mask"));
		spell.version = static_cast<uint32>(GetLiveSpellPairInt(payload, "version"));
		spell.name = GetLiveSpellPairValue(payload, "name");
		spell.element = GetLiveSpellPairValue(payload, "element");
		spell.target = GetLiveSpellPairValue(payload, "target");
		spell.target_type = GetLiveSpellPairInt(payload, "target_type", ST_Target);
		spell.range = GetLiveSpellPairInt(payload, "range", 200);
		spell.aoe_range = GetLiveSpellPairInt(payload, "aoe_range");
		spell.damage = GetLiveSpellPairInt(payload, "damage", 100);
		spell.mana = GetLiveSpellPairInt(payload, "mana", 20);
		spell.cast_time = GetLiveSpellPairInt(payload, "cast", 2500);
		spell.recovery_time = GetLiveSpellPairInt(payload, "recovery", 1500);
		spell.recast_time = GetLiveSpellPairInt(payload, "recast", 3000);
		spell.resist_type = GetLiveSpellPairInt(payload, "resist", RESIST_FIRE);
		spell.icon = GetLiveSpellPairInt(payload, "icon", 28);
		spell.book_icon = GetLiveSpellPairInt(payload, "book_icon", spell.icon);
		spell.level = GetLiveSpellPairInt(payload, "level", 1);
		return spell;
	}

	void AddLiveSpellDefinition(
		std::vector<LiveSpellDefinition> &definitions,
		std::unordered_set<uint16> &loaded_spell_ids,
		const std::string &payload
	)
	{
		auto definition = ParseLiveSpellDefinition(payload);
		if (!definition.spell_id || loaded_spell_ids.count(definition.spell_id)) {
			return;
		}

		loaded_spell_ids.insert(definition.spell_id);
		definitions.emplace_back(std::move(definition));
	}

	std::vector<LiveSpellDefinition> LoadLiveSpellDefinitions()
	{
		std::vector<LiveSpellDefinition> definitions;
		std::unordered_set<uint16> loaded_spell_ids;

		const auto rows = DataBucketsRepository::GetWhere(
			database,
			fmt::format("`key` LIKE '{}%' ORDER BY `key`", LiveSpellBucketPrefix)
		);

		for (const auto &row : rows) {
			AddLiveSpellDefinition(definitions, loaded_spell_ids, row.value);
		}

		const auto nested_payload = DataBucket::GetData(&database, "live_spell.spell");
		if (!nested_payload.empty()) {
			try {
				const auto spell_payloads = json::parse(nested_payload);
				if (spell_payloads.is_object()) {
					for (const auto &entry : spell_payloads.items()) {
						if (entry.value().is_string()) {
							AddLiveSpellDefinition(definitions, loaded_spell_ids, entry.value().get<std::string>());
						}
					}
				}
				else if (spell_payloads.is_string()) {
					AddLiveSpellDefinition(definitions, loaded_spell_ids, spell_payloads.get<std::string>());
				}
			}
			catch (const json::exception&) {
				AddLiveSpellDefinition(definitions, loaded_spell_ids, nested_payload);
			}
		}

		std::sort(
			definitions.begin(),
			definitions.end(),
			[](const LiveSpellDefinition &lhs, const LiveSpellDefinition &rhs) {
				return lhs.spell_id < rhs.spell_id;
			}
		);

		return definitions;
	}

	bool IsPersistedLiveSpellID(const uint16 spell_id)
	{
		if (!spell_id) {
			return false;
		}

		return !DataBucket::GetData(&database, fmt::format("{}{}", LiveSpellBucketPrefix, spell_id)).empty();
	}

	bool PatchServerLiveSpellDefinition(Client *c, const LiveSpellDefinition &definition)
	{
		if (!IsLiveSpellSlotUsable(c, definition.spell_id, definition.base_spell_id)) {
			return false;
		}

		EQ::IPCMutex mutex("spells");
		mutex.Lock();

		auto *mutable_spells = const_cast<SPDat_Spell_Struct*>(spells);
		mutable_spells[definition.spell_id] = spells[definition.base_spell_id];

		auto &spell = mutable_spells[definition.spell_id];
		spell.id = definition.spell_id;
		CopySpellText(spell.name, definition.name);
		CopySpellText(spell.player_1, "GEN");
		CopySpellText(spell.you_cast, fmt::format("You release {}.", definition.name));
		CopySpellText(spell.other_casts, fmt::format("$n releases {}.", definition.name));
		CopySpellText(spell.cast_on_you, fmt::format("{} hits you.", definition.name));
		CopySpellText(spell.cast_on_other, fmt::format("{} hits $n.", definition.name));
		CopySpellText(spell.spell_fades, fmt::format("{} fades.", definition.name));

		spell.range = static_cast<float>(definition.range);
		spell.aoe_range = static_cast<float>(definition.aoe_range);
		spell.cast_time = definition.cast_time;
		spell.recovery_time = definition.recovery_time;
		spell.recast_time = definition.recast_time;
		spell.buff_duration_formula = 0;
		spell.buff_duration = 0;
		spell.aoe_duration = 0;
		spell.mana = definition.mana;
		spell.good_effect = 0;
		spell.activated = 1;
		spell.resist_type = definition.resist_type;
		spell.target_type = static_cast<SpellTargetType>(definition.target_type);
		spell.new_icon = definition.icon;
		spell.resist_difficulty = 0;

		for (int index = 0; index < EFFECT_COUNT; ++index) {
			spell.base_value[index] = 0;
			spell.limit_value[index] = 0;
			spell.max_value[index] = 0;
			spell.formula[index] = 100;
			spell.effect_id[index] = SpellEffect::Blank;
		}

		spell.effect_id[0] = SpellEffect::CurrentHP;
		spell.base_value[0] = -std::max(1, definition.damage);

		for (auto &level : spell.classes) {
			level = 255;
		}

		for (int class_id = 1; class_id <= Class::PLAYER_CLASS_COUNT; ++class_id) {
			const auto class_bit = static_cast<uint32>(1 << (class_id - 1));
			if (definition.class_mask & class_bit) {
				spell.classes[class_id - 1] = static_cast<uint8>(std::max(1, definition.level));
			}
		}

		mutex.Unlock();
		return true;
	}

	void PatchPersistedLiveSpells()
	{
		for (const auto &definition : LoadLiveSpellDefinitions()) {
			PatchServerLiveSpellDefinition(nullptr, definition);
		}
	}

	std::string BuildLiveSpellClientPayload(const LiveSpellDefinition &definition)
	{
		return fmt::format(
			"LIVESPELL|upsert|id={}|base={}|version={}|name={}|mana={}|cast={}|recovery={}|recast={}|range={}|aoe_range={}|icon={}|book_icon={}|level={}|target_type={}|resist={}|spell_type=0|effect0={}|base0={}|max0=0|calc0=100",
			definition.spell_id,
			definition.base_spell_id,
			definition.version,
			SanitizeLiveSpellText(definition.name, 60),
			definition.mana,
			definition.cast_time,
			definition.recovery_time,
			definition.recast_time,
			definition.range,
			definition.aoe_range,
			definition.icon,
			definition.book_icon,
			definition.level,
			definition.target_type,
			definition.resist_type,
			SpellEffect::CurrentHP,
			-std::max(1, definition.damage)
		);
	}

	void SendLiveSpellPatch(Client *c, const LiveSpellDefinition &definition)
	{
		c->Message(Chat::White, BuildLiveSpellClientPayload(definition).c_str());
	}

	uint16 FindFreeLiveSpellID()
	{
		if (!spells || SPDAT_RECORDS <= 0) {
			return 0;
		}

		const auto definitions = LoadLiveSpellDefinitions();
		std::unordered_set<uint16> used_spell_ids;
		for (const auto &definition : definitions) {
			used_spell_ids.insert(definition.spell_id);
		}

		const uint16 upper_bound = static_cast<uint16>(
			std::min<uint32>(LiveSpellReserveMaxID, SPDAT_RECORDS > 0 ? static_cast<uint32>(SPDAT_RECORDS - 1) : 0)
		);

		for (uint16 spell_id = LiveSpellReserveMinID; spell_id <= upper_bound; ++spell_id) {
			if (used_spell_ids.count(spell_id)) {
				continue;
			}

			if (!spells || spells[spell_id].id == 0 || spells[spell_id].name[0] == '\0') {
				return spell_id;
			}
		}

		return 0;
	}

	uint32 FindFreeLiveScrollItemID()
	{
		const auto minimum = static_cast<uint32>(std::max(1, RuleI(Items, LiveItemMinID)));
		const auto maximum = static_cast<uint32>(std::max(0, RuleI(Items, LiveItemMaxID)));
		const auto definitions = LoadLiveSpellDefinitions();

		std::unordered_set<uint32> used_item_ids;
		for (const auto &definition : definitions) {
			used_item_ids.insert(definition.scroll_item_id);
		}

		auto start = static_cast<uint32>(Strings::ToUnsignedInt(DataBucket::GetData(&database, LiveSpellNextScrollBucket)));
		if (start < minimum) {
			start = minimum;
		}

		for (uint32 item_id = start; item_id <= maximum || maximum == 0; ++item_id) {
			if (maximum && item_id > maximum) {
				break;
			}

			if (used_item_ids.count(item_id)) {
				continue;
			}

			if (database.IsLiveItemID(item_id) && !ItemsRepository::FindOne(database, item_id).id) {
				DataBucket::SetData(&database, LiveSpellNextScrollBucket, std::to_string(item_id + 1));
				return item_id;
			}

			if (maximum == 0 && item_id == std::numeric_limits<uint32>::max()) {
				break;
			}
		}

		return 0;
	}

	bool UpsertLiveSpellScrollItem(Client *c, const LiveSpellDefinition &definition)
	{
		auto base_scroll = ItemsRepository::FindOne(database, LiveSpellBaseScrollID);
		if (!base_scroll.id) {
			const auto matches = ItemsRepository::GetWhere(
				database,
				fmt::format("scrolleffect = {} AND itemtype = {} ORDER BY id LIMIT 1", definition.base_spell_id, static_cast<int>(EQ::item::ItemTypeSpell))
			);
			if (!matches.empty()) {
				base_scroll = matches.front();
			}
		}

		if (!base_scroll.id) {
			base_scroll = ItemsRepository::NewEntity();
			base_scroll.itemclass = EQ::item::ItemClassCommon;
			base_scroll.itemtype = EQ::item::ItemTypeSpell;
			base_scroll.classes = definition.class_mask;
			base_scroll.races = 65535;
			base_scroll.norent = 255;
			base_scroll.nodrop = 0;
			base_scroll.size = EQ::item::ItemSizeSmall;
			base_scroll.icon = 778;
			base_scroll.maxcharges = 1;
		}

		const bool replacing = ItemsRepository::FindOne(database, definition.scroll_item_id).id != 0;

		base_scroll.id = static_cast<int32>(definition.scroll_item_id);
		base_scroll.minstatus = 0;
		base_scroll.Name = fmt::format("Spell: {}", definition.name);
		base_scroll.lore = fmt::format("A living scroll for {}", definition.name);
		base_scroll.comment = fmt::format("Generated live spell scroll for spell {}", definition.spell_id);
		base_scroll.itemclass = EQ::item::ItemClassCommon;
		base_scroll.itemtype = EQ::item::ItemTypeSpell;
		base_scroll.classes = static_cast<int32>(definition.class_mask);
		base_scroll.tradeskills = 0;
		base_scroll.icon = 778;
		base_scroll.scrolleffect = definition.spell_id;
		base_scroll.scrolltype = EQ::item::ItemEffectScroll;
		base_scroll.scrolllevel = 1;
		base_scroll.scrolllevel2 = 1;
		base_scroll.maxcharges = 1;
		base_scroll.norent = 255;
		base_scroll.nodrop = 0;
		base_scroll.updated = std::time(nullptr);

		if (replacing) {
			if (!ItemsRepository::UpdateOne(database, base_scroll)) {
				c->Message(Chat::White, fmt::format("Failed to update live spell scroll item {}.", definition.scroll_item_id).c_str());
				return false;
			}
		} else {
			ItemsRepository::InsertOne(database, base_scroll);
			if (!ItemsRepository::FindOne(database, definition.scroll_item_id).id) {
				c->Message(Chat::White, fmt::format("Failed to create live spell scroll item {}.", definition.scroll_item_id).c_str());
				return false;
			}
		}

		database.ClearLiveItemCache(definition.scroll_item_id);
		return true;
	}

	LiveSpellDefinition BuildLiveSpellDefinition(
		Client *c,
		const std::string &element_key,
		const std::string &target_key,
		const int range,
		const int damage,
		const int recast_time,
		const std::string &custom_name
	)
	{
		const auto &element = ResolveLiveSpellElement(element_key);
		const auto &target = ResolveLiveSpellTarget(target_key);
		LiveSpellDefinition definition;

		definition.spell_id = FindFreeLiveSpellID();
		definition.scroll_item_id = FindFreeLiveScrollItemID();
		definition.owner_character_id = c->CharacterID();
		definition.class_mask = static_cast<uint32>(1 << (c->GetClass() - 1));
		definition.version = BuildLiveSpellVersion();
		definition.element = element.key;
		definition.target = target.key;
		definition.target_type = target.target_type;
		definition.resist_type = element.resist_type;
		definition.icon = element.icon;
		definition.book_icon = element.icon;
		definition.range = std::clamp(range, 25, 300);
		definition.aoe_range = static_cast<int>(target.aoe_range);
		definition.damage = std::clamp(damage, 1, 5000);
		definition.recast_time = std::clamp(recast_time, 1000, 600000);
		definition.cast_time = std::clamp(1500 + (definition.damage / 20), 1500, 6000);
		definition.recovery_time = 1500;
		definition.mana = std::clamp(5 + (definition.damage / 5), 5, 5000);
		definition.level = 1;
		definition.name = DecodeLiveSpellCommandText(custom_name);
		if (definition.name.empty()) {
			definition.name = SanitizeLiveSpellText(
				fmt::format("{} {} {}", c->GetCleanName(), element.noun, target.key == std::string("pbae") ? "Bloom" : "Lash"),
				60
			);
		}

		return definition;
	}

	void SendLiveSpellUsage(Client *c)
	{
		c->Message(Chat::White, "Usage: #livespell dialog");
		c->Message(Chat::White, "Usage: #livespell craft [fire|cold|magic|poison|disease] [target|ae|pbae] [range] [damage] [recast_ms]");
		c->Message(Chat::White, "Usage: #livespell craft element=cold target=ae range=200 damage=100 recast=3000 name=Frost_Burst");
		c->Message(Chat::White, "Usage: #livespell test [spell_id] [base_spell_id] [gem]");
		c->Message(Chat::White, "Usage: #livespell patch [spell_id] [base_spell_id]");
		c->Message(Chat::White, "Usage: #livespell scribe [spell_id] [gem]");
		c->Message(Chat::White, "Usage: #livespell ack [spell_id] [version]");
		c->Message(Chat::White, "Usage: #livespell ready");
	}

	bool IsLiveSpellSlotUsable(Client *c, const uint32 spell_id, const uint32 base_spell_id)
	{
		if (!spells || SPDAT_RECORDS <= 0) {
			if (c) {
				c->Message(Chat::White, "Spell data is not loaded.");
			}
			return false;
		}

		if (spell_id >= static_cast<uint32>(SPDAT_RECORDS)) {
			if (c) {
				c->Message(Chat::White, fmt::format("Spell ID {} is outside loaded server spell records [{}].", spell_id, SPDAT_RECORDS).c_str());
			}
			return false;
		}

		if (base_spell_id >= static_cast<uint32>(SPDAT_RECORDS) || !IsValidSpell(base_spell_id)) {
			if (c) {
				c->Message(Chat::White, fmt::format("Base spell ID {} is not valid.", base_spell_id).c_str());
			}
			return false;
		}

		return true;
	}

	uint32 BuildLiveSpellVersion()
	{
		return static_cast<uint32>(std::time(nullptr));
	}

	bool PatchServerLiveSpell(Client *c, const uint16 spell_id, const uint16 base_spell_id, const uint32 version)
	{
		if (!IsLiveSpellSlotUsable(c, spell_id, base_spell_id)) {
			return false;
		}

		EQ::IPCMutex mutex("spells");
		mutex.Lock();

		auto *mutable_spells = const_cast<SPDat_Spell_Struct*>(spells);
		mutable_spells[spell_id] = spells[base_spell_id];

		auto &spell = mutable_spells[spell_id];
		spell.id = spell_id;
		CopySpellText(spell.name, fmt::format("Live Ember Lash {}", version % 10000));
		CopySpellText(spell.player_1, "GEN");
		CopySpellText(spell.you_cast, "You shape a live ember into a spell.");
		CopySpellText(spell.other_casts, "$n shapes a live ember into a spell.");
		CopySpellText(spell.cast_on_you, "A live ember lashes you.");
		CopySpellText(spell.cast_on_other, "A live ember lashes $n.");
		CopySpellText(spell.spell_fades, "The live ember fades.");
		spell.mana = 5;
		spell.cast_time = 1500;
		spell.recovery_time = 1500;
		spell.recast_time = 3000;
		spell.range = 200.0f;
		spell.new_icon = 28;
		for (uint8 &level : spell.classes) {
			level = 1;
		}

		mutex.Unlock();

		return true;
	}

	void SendLiveSpellPatch(Client *c, const uint16 spell_id, const uint16 base_spell_id, const uint32 version)
	{
		const auto &spell = spells[spell_id];
		c->Message(
			Chat::White,
			fmt::format(
				"LIVESPELL|upsert|id={}|base={}|version={}|name={}|mana={}|cast={}|recovery={}|recast={}|range={:.1f}|icon={}|book_icon={}|level=1",
				spell_id,
				base_spell_id,
				version,
				spell.name,
				spell.mana,
				spell.cast_time,
				spell.recovery_time,
				spell.recast_time,
				spell.range,
				spell.new_icon,
				spell.new_icon
			).c_str()
		);
	}

	void ScribeLiveSpell(Client *c, const uint16 spell_id, const int gem)
	{
		if (!IsValidSpell(spell_id)) {
			c->Message(Chat::White, fmt::format("Spell ID {} is not valid on the server.", spell_id).c_str());
			return;
		}

		int book_slot = c->FindSpellBookSlotBySpellID(spell_id);
		if (book_slot < 0) {
			book_slot = c->GetNextAvailableSpellBookSlot();
		}

		if (book_slot < 0) {
			c->Message(Chat::White, "No free spellbook slot is available.");
			return;
		}

		if (c->FindSpellBookSlotBySpellID(spell_id) < 0) {
			c->ScribeSpell(spell_id, book_slot);
		}

		if (gem >= 0 && gem < EQ::spells::SPELL_GEM_COUNT) {
			c->MemSpell(spell_id, gem);
		}

		c->Message(Chat::White, fmt::format("Live spell {} is scribed in book slot {} and gem {}.", spell_id, book_slot, gem).c_str());
	}
}

namespace LiveSpellManager {
	void EnsureServerLoaded()
	{
		if (!LiveSpellsEnabled()) {
			return;
		}

		if (gLiveSpellServerLoaded) {
			return;
		}

		PatchPersistedLiveSpells();
		gLiveSpellServerLoaded = true;
	}

	void SendClientSync(Client *c)
	{
		if (!LiveSpellsEnabled()) {
			if (c) {
				c->Message(Chat::White, "Live Spells are disabled on this server.");
			}
			return;
		}

		EnsureServerLoaded();

		int sent = 0;
		for (const auto &definition : LoadLiveSpellDefinitions()) {
			SendLiveSpellPatch(c, definition);
			++sent;
		}

		c->Message(Chat::White, fmt::format("LiveSpell DLL reported ready. Synced {} generated spell{}.", sent, sent == 1 ? "" : "s").c_str());
	}

	bool CreateSpellScroll(
		Client *c,
		const std::string &element,
		const std::string &target,
		const int range,
		const int damage,
		const int recast_time,
		const std::string &custom_name
	)
	{
		if (!LiveSpellsEnabled()) {
			c->Message(Chat::White, "Live Spells are disabled on this server.");
			return false;
		}

		EnsureServerLoaded();

		auto definition = BuildLiveSpellDefinition(c, element, target, range, damage, recast_time, custom_name);
		if (!definition.spell_id) {
			c->Message(Chat::White, fmt::format("No free live spell slot is available in the reserve [{}-{}]. Add more blank spell rows and rebuild shared memory.", LiveSpellReserveMinID, LiveSpellReserveMaxID).c_str());
			return false;
		}

		if (!definition.scroll_item_id) {
			c->Message(Chat::White, "No free live scroll item ID is available in the configured live item range.");
			return false;
		}

		if (!UpsertLiveSpellScrollItem(c, definition)) {
			return false;
		}

		DataBucket::SetData(
			&database,
			fmt::format("{}{}", LiveSpellBucketPrefix, definition.spell_id),
			SerializeLiveSpellDefinition(definition)
		);

		if (!PatchServerLiveSpellDefinition(c, definition)) {
			return false;
		}

		SendLiveSpellPatch(c, definition);
		c->SummonItem(definition.scroll_item_id, 1);
		c->Message(
			Chat::White,
			fmt::format(
				"Created {} as spell {} and scroll item {}. Scribe the scroll, memorize it, then cast it normally.",
				definition.name,
				definition.spell_id,
				definition.scroll_item_id
			).c_str()
		);

		return true;
	}

	bool IsLiveSpell(const uint16 spell_id)
	{
		if (!LiveSpellsEnabled()) {
			return false;
		}

		return IsPersistedLiveSpellID(spell_id);
	}
}

void command_livespell(Client *c, const Seperator *sep)
{
	if (!LiveSpellsEnabled()) {
		c->Message(Chat::White, "Live Spells are disabled on this server.");
		return;
	}

	const auto arguments = sep->argnum;
	if (!arguments || !strcasecmp(sep->arg[1], "help")) {
		SendLiveSpellUsage(c);
		return;
	}

	const bool is_test = !strcasecmp(sep->arg[1], "test");
	const bool is_patch = !strcasecmp(sep->arg[1], "patch");
	const bool is_scribe = !strcasecmp(sep->arg[1], "scribe");
	const bool is_ack = !strcasecmp(sep->arg[1], "ack");
	const bool is_ready = !strcasecmp(sep->arg[1], "ready");
	const bool is_dialog = !strcasecmp(sep->arg[1], "dialog");
	const bool is_craft = !strcasecmp(sep->arg[1], "craft") || !strcasecmp(sep->arg[1], "create");

	if (is_ack) {
		const auto spell_id = arguments >= 2 ? Strings::ToUnsignedInt(sep->arg[2]) : 0;
		const auto version = arguments >= 3 ? Strings::ToUnsignedInt(sep->arg[3]) : 0;
		c->Message(Chat::White, fmt::format("LiveSpell DLL ack received for spell {} version {}.", spell_id, version).c_str());
		return;
	}

	if (is_ready) {
		LiveSpellManager::SendClientSync(c);
		return;
	}

	if (is_dialog) {
		c->Message(Chat::White, "Opening the Spell Forge.");
		c->Message(Chat::White, "LIVESPELL|ui|open|elements=fire,cold,magic,poison,disease|targets=target,ae,pbae|max_damage=500|max_range=300|min_recast=1000|max_recast=30000");
		return;
	}

	if (is_craft) {
		std::string element = "fire";
		std::string target = "target";
		int range = 200;
		int damage = 100;
		int recast_time = 3000;
		std::string custom_name;

		if (HasLiveSpellNamedArgs(sep)) {
			const auto named_element = GetLiveSpellNamedArg(sep, "element");
			const auto named_target = GetLiveSpellNamedArg(sep, "target");
			if (!named_element.empty()) {
				element = named_element;
			}

			if (!named_target.empty()) {
				target = named_target;
			}

			range = GetLiveSpellNamedInt(sep, "range", range);
			damage = GetLiveSpellNamedInt(sep, "damage", damage);
			recast_time = GetLiveSpellNamedInt(sep, "recast", recast_time);
			custom_name = DecodeLiveSpellCommandText(GetLiveSpellNamedArg(sep, "name"));
		} else {
			element = arguments >= 2 ? std::string(sep->arg[2]) : element;
			target = arguments >= 3 ? std::string(sep->arg[3]) : target;
			range = (arguments >= 4 && sep->IsNumber(4)) ? Strings::ToInt(sep->arg[4]) : range;
			damage = (arguments >= 5 && sep->IsNumber(5)) ? Strings::ToInt(sep->arg[5]) : damage;
			recast_time = (arguments >= 6 && sep->IsNumber(6)) ? Strings::ToInt(sep->arg[6]) : recast_time;
		}

		LiveSpellManager::CreateSpellScroll(c, element, target, range, damage, recast_time, custom_name);
		return;
	}

	if (c->Admin() < AccountStatus::GMMgmt) {
		c->Message(Chat::White, "Only GMs can run live spell prototype patch/test commands.");
		return;
	}

	if (!is_test && !is_patch && !is_scribe) {
		SendLiveSpellUsage(c);
		return;
	}

	const auto spell_id = static_cast<uint16>(
		(arguments >= 2 && sep->IsNumber(2)) ? Strings::ToUnsignedInt(sep->arg[2]) : LiveSpellDefaultID
	);
	const auto base_spell_id = static_cast<uint16>(
		(arguments >= 3 && sep->IsNumber(3)) ? Strings::ToUnsignedInt(sep->arg[3]) : LiveSpellDefaultBaseID
	);
	const auto gem = (arguments >= 4 && sep->IsNumber(4)) ? Strings::ToInt(sep->arg[4]) : LiveSpellDefaultGem;

	if (is_scribe) {
		ScribeLiveSpell(c, spell_id, gem);
		return;
	}

	const auto version = BuildLiveSpellVersion();
	if (!PatchServerLiveSpell(c, spell_id, base_spell_id, version)) {
		return;
	}

	SendLiveSpellPatch(c, spell_id, base_spell_id, version);
	c->Message(Chat::White, fmt::format("Patched server and client transport for live spell {} from base {}.", spell_id, base_spell_id).c_str());

	if (is_test) {
		ScribeLiveSpell(c, spell_id, gem);
	}
}

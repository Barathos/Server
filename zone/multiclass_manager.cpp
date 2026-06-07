/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "multiclass_manager.h"

#include "common/classes.h"
#include "common/eq_packet_structs.h"
#include "common/item_data.h"
#include "common/item_instance.h"
#include "common/deity.h"
#include "common/races.h"
#include "common/rulesys.h"
#include "common/seperator.h"
#include "common/skill_caps.h"
#include "common/spdat.h"
#include "common/strings.h"
#include "zone/client.h"
#include "zone/entity.h"
#include "zone/npc.h"
#include "zone/string_ids.h"
#include "zone/zone.h"
#include "zone/zonedb.h"

#include "fmt/format.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <ctime>
#include <limits>
#include <vector>

MulticlassManager multiclass_manager;

namespace {

bool MulticlassEnabled()
{
	return RuleB(CustomFeatures, MulticlassEnabled);
}

bool RequireMulticlassEnabled(Client *client)
{
	if (MulticlassEnabled()) {
		return true;
	}

	if (client) {
		client->Message(Chat::White, "Multiclass is disabled on this server.");
	}

	return false;
}

uint32 ToUInt(const char *value)
{
	return value ? Strings::ToUnsignedInt(value) : 0;
}

bool IsPetClass(uint8 class_id)
{
	return class_id == Class::Magician || class_id == Class::Necromancer || class_id == Class::Beastlord;
}

constexpr const char *PetClassOriginVariable = "pet_bag_origin_class";

uint8 GetPetOriginClass(Mob *pet)
{
	if (!pet || !pet->IsNPC()) {
		return Class::None;
	}

	auto *pet_npc = pet->CastToNPC();
	const auto spell_id = pet_npc->GetPetSpellID();
	if (spell_id) {
		for (uint8 class_id = Class::Warrior; class_id <= Class::Berserker; ++class_id) {
			if (IsPetClass(class_id) && GetSpellLevel(spell_id, class_id) < std::numeric_limits<uint8>::max()) {
				return class_id;
			}
		}
	}

	if (pet_npc->EntityVariableExists(PetClassOriginVariable)) {
		const auto class_id = static_cast<uint8>(Strings::ToInt(pet_npc->GetEntityVariable(PetClassOriginVariable)));
		if (IsPetClass(class_id)) {
			return class_id;
		}
	}

	return Class::None;
}

bool IsPetHealthAction(const std::string &action_name)
{
	return action_name == "health" ||
		action_name == "healthreport" ||
		action_name == "hp" ||
		action_name == "stats" ||
		action_name == "inventory" ||
		action_name == "inv";
}

bool IsTankClass(uint8 class_id)
{
	return class_id == Class::Warrior || class_id == Class::Paladin || class_id == Class::ShadowKnight;
}

bool IsHealerClass(uint8 class_id)
{
	return class_id == Class::Cleric || class_id == Class::Druid || class_id == Class::Shaman;
}

bool IsIntCasterClass(uint8 class_id)
{
	switch (class_id) {
		case Class::ShadowKnight:
		case Class::Bard:
		case Class::Necromancer:
		case Class::Wizard:
		case Class::Magician:
		case Class::Enchanter:
			return true;
		default:
			return false;
	}
}

bool IsWisCasterClass(uint8 class_id)
{
	switch (class_id) {
		case Class::Cleric:
		case Class::Paladin:
		case Class::Ranger:
		case Class::Druid:
		case Class::Shaman:
		case Class::Beastlord:
			return true;
		default:
			return false;
	}
}

bool IsBardClass(uint8 class_id)
{
	return class_id == Class::Bard;
}

uint8 GetBestSpellLevelForSlots(const MulticlassManager::ClassSlots &class_slots, uint16 spell_id)
{
	if (!IsValidSpell(spell_id)) {
		return 255;
	}

	uint8 best_level = 255;
	for (const auto class_id : class_slots) {
		if (!IsPlayerClass(class_id)) {
			continue;
		}

		best_level = std::min(best_level, spells[spell_id].classes[class_id - 1]);
	}

	return best_level;
}

void RefreshClientAfterMulticlassProfileChange(Client *client)
{
	if (!client) {
		return;
	}

	client->CalcBonuses();
	client->SendManaUpdate();
	client->SendEnduranceUpdate();

	static const int16 inventory_ranges[][2] = {
		{EQ::invslot::POSSESSIONS_BEGIN, EQ::invslot::POSSESSIONS_END},
		{EQ::invbag::GENERAL_BAGS_BEGIN, EQ::invbag::GENERAL_BAGS_END},
		{EQ::invbag::CURSOR_BAG_BEGIN, EQ::invbag::CURSOR_BAG_END}
	};

	for (const auto &range : inventory_ranges) {
		for (int16 slot_id = range[0]; slot_id <= range[1]; ++slot_id) {
			const auto *inst = client->GetInv().GetItem(slot_id);
			if (inst) {
				client->SendItemPacket(slot_id, inst, ItemPacketType::ItemPacketTrade);
			}
		}
	}

	for (uint8 material_slot = EQ::textures::textureBegin; material_slot <= EQ::textures::LastTexture; ++material_slot) {
		client->SendWearChange(material_slot);
	}

	multiclass_manager.SendNativeSpellLevelSnapshot(client);
}

bool IsStrikerClass(uint8 class_id)
{
	return class_id == Class::Monk || class_id == Class::Rogue || class_id == Class::Berserker;
}

bool ShouldAutoSeedSkill(EQ::skills::SkillType skill_id)
{
	if (skill_id < EQ::skills::Skill1HBlunt || skill_id > EQ::skills::HIGHEST_SKILL) {
		return false;
	}

	if (EQ::skills::IsTradeskill(skill_id) || EQ::skills::IsSpecializedSkill(skill_id)) {
		return false;
	}

	return true;
}

bool IsBlockedMelodyControlSong(uint16 spell_id)
{
	return
		IsCharmSpell(spell_id) ||
		IsFearSpell(spell_id) ||
		IsMesmerizeSpell(spell_id) ||
		IsStunSpell(spell_id) ||
		IsEffectInSpell(spell_id, SpellEffect::Root) ||
		IsEffectInSpell(spell_id, SpellEffect::Lull);
}

Mob *ResolveMelodyTarget(Client *client, uint16 spell_id)
{
	if (!client || !IsValidSpell(spell_id)) {
		return nullptr;
	}

	if (IsBeneficialSpell(spell_id)) {
		return client;
	}

	auto *target = client->GetTarget();
	if (!target || target == client) {
		return nullptr;
	}

	if (!client->IsAttackAllowed(target)) {
		return nullptr;
	}

	return target;
}

std::array<uint8, 3> ProfileSlots(const MulticlassManager::Profile &profile)
{
	return {profile.class_slot_1, profile.class_slot_2, profile.class_slot_3};
}

void AddUnique(std::vector<std::string> &values, const std::string &value)
{
	if (std::find(values.begin(), values.end(), value) == values.end()) {
		values.push_back(value);
	}
}

void AddRoleTags(std::vector<std::string> &roles, uint8 class_id)
{
	if (IsTankClass(class_id)) {
		AddUnique(roles, "Vanguard");
	}

	if (IsHealerClass(class_id)) {
		AddUnique(roles, "Healer");
	}

	if (IsPetClass(class_id)) {
		AddUnique(roles, "Pet");
	}

	if (IsIntCasterClass(class_id)) {
		AddUnique(roles, "Arcane");
	}

	if (IsWisCasterClass(class_id)) {
		AddUnique(roles, "Primal");
	}

	if (IsBardClass(class_id)) {
		AddUnique(roles, "Bardic");
	}

	if (IsStrikerClass(class_id)) {
		AddUnique(roles, "Striker");
	}
}

std::string JoinValues(const std::vector<std::string> &values, const std::string &delimiter)
{
	std::string joined;
	for (const auto &value : values) {
		if (!joined.empty()) {
			joined += delimiter;
		}

		joined += value;
	}

	return joined;
}

std::string PetOrderName(uint8 pet_order)
{
	switch (pet_order) {
		case 0:
			return "follow";
		case 1:
			return "sit";
		case 2:
			return "guard";
		case 3:
			return "feign";
		default:
			return "unknown";
	}
}

std::string NormalizeLookupText(const std::string &value)
{
	std::string normalized;
	normalized.reserve(value.size());
	for (const auto c : value) {
		const auto value_char = static_cast<unsigned char>(c);
		if (std::isalnum(value_char)) {
			normalized.push_back(static_cast<char>(std::tolower(value_char)));
		}
	}

	return normalized;
}

std::array<uint8, 3> SortedProfileSlots(const MulticlassManager::Profile &profile)
{
	auto slots = ProfileSlots(profile);
	std::sort(slots.begin(), slots.end());
	return slots;
}

std::string TrioKey(const MulticlassManager::Profile &profile)
{
	const auto slots = SortedProfileSlots(profile);
	return fmt::format("{}-{}-{}", slots[0], slots[1], slots[2]);
}

struct TrioTemplate {
	const char *key;
	const char *name;
	const char *summary;
	const char *bonus_path;
};

const TrioTemplate *FindExactTrioTemplate(const MulticlassManager::Profile &profile)
{
	static const std::array<TrioTemplate, 10> templates = {{
		{"11-13-15", "Grand Menagerie", "Three bonded pet lines fighting as one stable. Pet scaling and command strength are the core fantasy.", "Menagerie"},
		{"1-12-13", "Warbound Arcanum", "A battle shell with summoned pressure and arcane burst. Built to stand still and make the field burn.", "Arcanum"},
		{"9-12-16", "Spellblade Compact", "A caster-root melee killer with Rogue precision, Berserker violence, and Wizard finishers.", "Spellblade"},
		{"2-3-8", "Radiant Hymn", "A holy frontline chorus: plate, healing, and sustained Bard support in one identity.", "Hymn"},
		{"8-9-14", "Velvet Conspiracy", "Charm, song, and knives. This trio wins by controlling the room before the room notices.", "Conspiracy"},
		{"1-9-12", "Arcane Duelist Pact", "A duelist shell that mixes weapon pressure, Rogue openings, and Wizard punishment.", "Duelist"},
		{"2-6-10", "Verdant Synod", "Three healing traditions braided together for recovery, cures, and long-form survival.", "Synod"},
		{"1-2-3", "Aegis Covenant", "The pure defensive oath: Warrior durability, Cleric recovery, and Paladin protection.", "Aegis"},
		{"5-8-11", "Dirge of the Grave", "Shadow magic, undead pressure, and Bard cadence sharpened into a grim support engine.", "Dirge"},
		{"4-6-15", "Wildcall Pact", "Ranger, Druid, and Beastlord form a roaming primal kit with tracking, pet pressure, and nature magic.", "Wildcall"},
	}};

	const auto key = TrioKey(profile);
	for (const auto &entry : templates) {
		if (key == entry.key) {
			return &entry;
		}
	}

	return nullptr;
}

uint8 UnlockedResonanceTiers(uint8 level)
{
	return std::min<uint8>(6, level / 10);
}

std::array<std::string, 6> BuildBonusTierNames(const MulticlassManager::Profile &profile)
{
	if (const auto *exact = FindExactTrioTemplate(profile)) {
		const std::string path = exact->bonus_path;
		if (path == "Menagerie") {
			return {"Bonded Leashes", "Pack Savagery", "Triune Ward", "Shared Command", "Alpha Chorus", "Grand Menagerie"};
		}
		if (path == "Arcanum") {
			return {"Arcane Bulwark", "Summoner's Guard", "Battlecaster's Focus", "Elemental Reprisal", "War-Mage Tempo", "Arcanum Ascendant"};
		}
		if (path == "Spellblade") {
			return {"Hidden Edge", "Volatile Footwork", "Spellsteel Accuracy", "Rupture Window", "Executioner's Spark", "Compact Fulfilled"};
		}
		if (path == "Hymn") {
			return {"Aegis Verse", "Merciful Cadence", "Consecrated Guard", "Bright Refrain", "Unbroken Chorus", "Radiant Hymn"};
		}
		if (path == "Conspiracy") {
			return {"Soft Step", "Mesmeric Rhythm", "Knife in the Song", "Velvet Escape", "Mindbreak Flourish", "Perfect Conspiracy"};
		}
		if (path == "Duelist") {
			return {"Arcane Guard", "Opening Cut", "Riposte Spark", "Duelist's Read", "Killing Tempo", "Pact of the Last Word"};
		}
		if (path == "Synod") {
			return {"Green Accord", "Cleansing Root", "Patient Waters", "Triune Recovery", "Living Bastion", "Verdant Synod"};
		}
		if (path == "Aegis") {
			return {"Shield Oath", "Interlocking Plate", "Mercy Under Fire", "Blessed Rampart", "Last Stand Prayer", "Aegis Covenant"};
		}
		if (path == "Dirge") {
			return {"Grave Note", "Dread Cadence", "Bone Chorus", "Umbral Recovery", "Funeral Engine", "Dirge of the Grave"};
		}
		if (path == "Wildcall") {
			return {"Trail Sense", "Pack Step", "Living Thorn", "Primal Recovery", "Wildfire Hunt", "Wildcall Pact"};
		}
	}

	uint8 pet_classes = 0;
	bool has_tank = false;
	bool has_healer = false;
	bool has_bard = false;
	bool has_striker = false;
	bool has_int_caster = false;
	bool has_wis_caster = false;

	for (const auto class_id : ProfileSlots(profile)) {
		if (!IsPlayerClass(class_id)) {
			continue;
		}

		pet_classes += IsPetClass(class_id) ? 1 : 0;
		has_tank = has_tank || IsTankClass(class_id);
		has_healer = has_healer || IsHealerClass(class_id);
		has_bard = has_bard || IsBardClass(class_id);
		has_striker = has_striker || IsStrikerClass(class_id);
		has_int_caster = has_int_caster || IsIntCasterClass(class_id);
		has_wis_caster = has_wis_caster || IsWisCasterClass(class_id);
	}

	if (pet_classes > 1) {
		return {"Bonded Petcraft", "Shared Instinct", "Pack Ward", "Triple Command", "Alpha Pressure", "Menagerie Mastery"};
	}
	if (has_bard) {
		return {"Opening Verse", "Resonant Step", "Songguard", "Crescendo", "Echoing Tools", "Chorus of Three"};
	}
	if (has_tank && has_healer) {
		return {"Steady Oath", "Field Mending", "Guarded Recovery", "Line Holder", "Saving Hand", "Covenant Prime"};
	}
	if (has_striker && (has_int_caster || has_wis_caster)) {
		return {"Cutting Spark", "Quickened Hands", "Spellstep", "Exploit Weakness", "Lethal Focus", "Striker's Revelation"};
	}
	if (has_healer && (has_int_caster || has_wis_caster)) {
		return {"Twin Font", "Clear Mind", "Merciful Focus", "Harmonic Recovery", "Grace Under Fire", "Concord Ascendant"};
	}

	return {"Cross Training", "Shared Reserves", "Field Focus", "Class Harmony", "Trio Momentum", "Triad Ascendant"};
}

}

void MulticlassManager::HandleCommand(Client *client, const Seperator *sep)
{
	if (!client || !sep) {
		return;
	}

	if (!RequireMulticlassEnabled(client)) {
		return;
	}

	if (sep->argnum < 1) {
		SendStatus(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "help")) {
		SendHelp(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "status")) {
		SendStatus(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "refresh") || !strcasecmp(sep->arg[1], "itemrefresh")) {
		Client *target_client = client;
		if (client->GetTarget() && client->GetTarget()->IsClient()) {
			target_client = client->GetTarget()->CastToClient();
		}

		RefreshAlternateAdvancementTable(target_client);
		RefreshClientAfterMulticlassProfileChange(target_client);
		SendNativeSnapshot(target_client, "Inventory item usability refreshed.", false, false);
		client->Message(Chat::White, "%s", fmt::format("Refreshed Multiclass item presentation for {}.", target_client->GetCleanName()).c_str());
		return;
	}

	if (!strcasecmp(sep->arg[1], "itemcheck") || !strcasecmp(sep->arg[1], "equipcheck")) {
		Client *target_client = client;
		if (client->GetTarget() && client->GetTarget()->IsClient()) {
			target_client = client->GetTarget()->CastToClient();
		}

		SendItemCheck(client, target_client, sep->arg[2], sep->arg[3], sep->arg[4]);
		return;
	}

	if (!strcasecmp(sep->arg[1], "diag") || !strcasecmp(sep->arg[1], "diagnostics")) {
		SendDiagnostics(client, sep->arg[2]);
		return;
	}

	if (!strcasecmp(sep->arg[1], "reweave")) {
		Client *target_client = client;
		if (client->GetTarget() && client->GetTarget()->IsClient()) {
			target_client = client->GetTarget()->CastToClient();
		}

		if (!strcasecmp(sep->arg[2], "grant")) {
			const auto grant_count = static_cast<uint8>(std::min<uint32>(255, Strings::ToUnsignedInt(sep->arg[3])));
			if (!grant_count || !EnsureProfile(target_client)) {
				client->Message(Chat::White, "Usage: #multiclass reweave grant <count> while targeting the character.");
				return;
			}

			auto profile = LoadProfile(target_client->CharacterID());
			profile.reweaves_available = std::min<uint8>(255, profile.reweaves_available + grant_count);
			if (SaveProfile(profile, "admin_reweave_grant")) {
				client->Message(
					Chat::White,
					"%s",
					fmt::format(
						"Granted {} reweave{} to {}. Available: {}.",
						grant_count,
						grant_count == 1 ? "" : "s",
						target_client->GetCleanName(),
						profile.reweaves_available
					).c_str()
				);
				SendNativeSnapshot(target_client, "A reweave has been granted.", false, false);
			} else {
				client->Message(Chat::White, "Unable to grant Multiclass reweaves.");
			}
			return;
		}

		std::string status;
		const auto slot_id = static_cast<uint8>(Strings::ToUnsignedInt(sep->arg[2]));
		const auto class_id = ParseClassId(sep->arg[3]);
		if (!ReweaveProfileSlot(target_client, slot_id, class_id, "admin_reweave", status)) {
			client->Message(Chat::White, "%s", status.c_str());
			return;
		}

		client->Message(Chat::White, "%s", status.c_str());
		RefreshAlternateAdvancementTable(target_client);
		SendNativeSnapshot(target_client, status, false, false);
		return;
	}

	if (!strcasecmp(sep->arg[1], "set")) {
		Client *target_client = client;
		if (client->GetTarget() && client->GetTarget()->IsClient()) {
			target_client = client->GetTarget()->CastToClient();
		}

		const auto class_slot_1 = ParseClassId(sep->arg[2]);
		const auto class_slot_2 = ParseClassId(sep->arg[3]);
		const auto class_slot_3 = ParseClassId(sep->arg[4]);
		if (!class_slot_1 || !class_slot_2 || !class_slot_3) {
			client->Message(Chat::White, "Usage: #multiclass set <class1> <class2> <class3>. Classes may be ids or names.");
			return;
		}

		if (SetProfile(target_client, class_slot_1, class_slot_2, class_slot_3, "admin")) {
			client->Message(
				Chat::White,
				"%s",
				fmt::format(
					"Multiclass trio set for {}: {} / {} / {}.",
					target_client->GetCleanName(),
					ClassName(class_slot_1, target_client->GetLevel()),
					ClassName(class_slot_2, target_client->GetLevel()),
					ClassName(class_slot_3, target_client->GetLevel())
				).c_str()
			);
			SendStatus(client);
			RefreshAlternateAdvancementTable(target_client);
			SendNativeSnapshot(target_client, "Trio updated. Spellbook levels refreshed.", false, false);
		} else {
			client->Message(Chat::White, "Unable to set Multiclass trio profile.");
		}
		return;
	}

	if (
		!strcasecmp(sep->arg[1], "window") ||
		!strcasecmp(sep->arg[1], "ui") ||
		!strcasecmp(sep->arg[1], "native")
	) {
		SendNativeSnapshot(client);
		return;
	}

	SendHelp(client);
}

void MulticlassManager::HandleNativeCommand(Client *client, const Seperator *sep)
{
	if (!client || !sep) {
		return;
	}

	if (!RequireMulticlassEnabled(client)) {
		return;
	}

	if (sep->argnum < 1) {
		RefreshAlternateAdvancementTable(client);
		RefreshClientAfterMulticlassProfileChange(client);
		SendNativeSnapshot(client, "Inventory item usability refreshed.");
		return;
	}

	if (
		!strcasecmp(sep->arg[1], "status") ||
		!strcasecmp(sep->arg[1], "open") ||
		!strcasecmp(sep->arg[1], "window")
	) {
		RefreshAlternateAdvancementTable(client);
		SendNativeSnapshot(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "refresh") || !strcasecmp(sep->arg[1], "itemrefresh")) {
		RefreshAlternateAdvancementTable(client);
		RefreshClientAfterMulticlassProfileChange(client);
		SendNativeSnapshot(client, "Inventory item usability refreshed.");
		return;
	}

	if (!strcasecmp(sep->arg[1], "itemcheck") || !strcasecmp(sep->arg[1], "equipcheck")) {
		SendItemCheck(client, client, sep->arg[2], sep->arg[3], sep->arg[4]);
		return;
	}

	if (!strcasecmp(sep->arg[1], "pets") || !strcasecmp(sep->arg[1], "petwindow") || !strcasecmp(sep->arg[1], "petui")) {
		SendNativeSnapshot(client, "Pet console refreshed.", false, true);
		return;
	}

	if (!strcasecmp(sep->arg[1], "melody") || !strcasecmp(sep->arg[1], "bardmelody") || !strcasecmp(sep->arg[1], "songui")) {
		std::string status;
		bool show_melody_window = false;
		HandleNativeMelodyCommand(client, sep, status, show_melody_window);
		SendNativeSnapshot(client, status, false, false);
		SendNativeBardMelody(client, show_melody_window);
		return;
	}

	if (
		!strcasecmp(sep->arg[1], "disc") ||
		!strcasecmp(sep->arg[1], "discs") ||
		!strcasecmp(sep->arg[1], "discipline") ||
		!strcasecmp(sep->arg[1], "disciplines") ||
		!strcasecmp(sep->arg[1], "combat") ||
		!strcasecmp(sep->arg[1], "combatability")
	) {
		std::string status;
		bool show_discipline_window = false;
		HandleNativeDisciplineCommand(client, sep, status, show_discipline_window);
		SendNativeSnapshot(client, status, false, false);
		SendNativeDisciplines(client, show_discipline_window, status);
		return;
	}

	if (!strcasecmp(sep->arg[1], "choose") || !strcasecmp(sep->arg[1], "lock")) {
		const auto class_slot_2 = ParseClassId(sep->arg[2]);
		const auto class_slot_3 = ParseClassId(sep->arg[3]);
		std::string status;
		if (!SetProfileFromNative(client, class_slot_2, class_slot_3, status)) {
			SendNativeSnapshot(client, status);
			return;
		}

		RefreshAlternateAdvancementTable(client);
		SendNativeSnapshot(client, "Trio locked. Camp once if the stock client needs to rebuild class windows.");
		return;
	}

	if (!strcasecmp(sep->arg[1], "reweave")) {
		std::string status;
		const auto slot_id = static_cast<uint8>(Strings::ToUnsignedInt(sep->arg[2]));
		const auto class_id = ParseClassId(sep->arg[3]);
		if (!ReweaveProfileSlot(client, slot_id, class_id, "native_reweave", status)) {
			SendNativeSnapshot(client, status);
			return;
		}

		RefreshAlternateAdvancementTable(client);
		SendNativeSnapshot(client, status);
		return;
	}

	if (!strcasecmp(sep->arg[1], "help")) {
		SendNativeSnapshot(client, "Choose two different added classes, then lock the trio.");
		return;
	}

	if (!strcasecmp(sep->arg[1], "pet") || !strcasecmp(sep->arg[1], "petcmd")) {
		std::string status;
		HandleNativePetCommand(client, sep, status);
		SendNativeSnapshot(client, status, false, true);
		return;
	}

	SendNativeSnapshot(client, "Unsupported Multiclass UI request.");
}

bool MulticlassManager::EnsureProfile(Client *client)
{
	if (!MulticlassEnabled() || !client || !client->CharacterID() || !IsPlayerClass(client->GetClass()) || !SchemaAvailable()) {
		return false;
	}

	const uint32 character_id = client->CharacterID();
	const uint8 class_id = client->GetClass();
	const auto class_name = Strings::Escape(ClassName(class_id, client->GetLevel()));
	const auto trio_name = Strings::Escape(fmt::format("{} Triad", class_name));

	auto results = database.QueryDatabase(
		fmt::format(
			"INSERT INTO `custom_multiclass_profiles` "
			"(`character_id`, `class_slot_1`, `class_slot_2`, `class_slot_3`, `trio_name`, `resonance_key`, `multiple_pets_enabled`, `locked`, `reweaves_available`, `created_at`, `updated_at`) "
			"VALUES ({}, {}, 0, 0, '{}', '', 1, 0, 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP()) "
			"ON DUPLICATE KEY UPDATE "
			"`class_slot_1` = IF(`class_slot_1` = 0, VALUES(`class_slot_1`), `class_slot_1`), "
			"`trio_name` = IF(`trio_name` = '', VALUES(`trio_name`), `trio_name`), "
			"`updated_at` = VALUES(`updated_at`)",
			character_id,
			class_id,
			trio_name
		)
	);

	if (results.Success()) {
		profile_cache_.erase(character_id);
		return true;
	}

	return false;
}

MulticlassManager::Profile MulticlassManager::LoadProfile(uint32 character_id)
{
	Profile profile;
	if (!MulticlassEnabled() || !character_id) {
		return profile;
	}

	const auto cached_profile = profile_cache_.find(character_id);
	if (cached_profile != profile_cache_.end()) {
		return cached_profile->second;
	}

	if (!SchemaAvailable()) {
		return profile;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `character_id`, `class_slot_1`, `class_slot_2`, `class_slot_3`, `trio_name`, `resonance_key`, "
			"`multiple_pets_enabled`, `locked`, `reweaves_available` "
			"FROM `custom_multiclass_profiles` WHERE `character_id` = {} LIMIT 1",
			character_id
		)
	);

	if (!results.Success() || !results.RowCount()) {
		if (results.Success()) {
			profile.character_id = character_id;
			profile_cache_[character_id] = profile;
		}

		return profile;
	}

	auto row = results.begin();
	profile.character_id = ToUInt(row[0]);
	profile.class_slot_1 = static_cast<uint8>(ToUInt(row[1]));
	profile.class_slot_2 = static_cast<uint8>(ToUInt(row[2]));
	profile.class_slot_3 = static_cast<uint8>(ToUInt(row[3]));
	profile.trio_name = row[4] ? row[4] : "";
	profile.resonance_key = row[5] ? row[5] : "";
	profile.multiple_pets_enabled = ToUInt(row[6]) != 0;
	profile.locked = ToUInt(row[7]) != 0;
	profile.reweaves_available = static_cast<uint8>(ToUInt(row[8]));

	if (profile.trio_name.empty()) {
		profile.trio_name = BuildTrioName(profile);
	}

	profile_cache_[character_id] = profile;
	return profile;
}

bool MulticlassManager::SetProfile(Client *client, uint8 class_slot_1, uint8 class_slot_2, uint8 class_slot_3, const std::string &source)
{
	if (
		!MulticlassEnabled() ||
		!client ||
		!client->CharacterID() ||
		!IsPlayerClass(class_slot_1) ||
		!IsPlayerClass(class_slot_2) ||
		!IsPlayerClass(class_slot_3) ||
		!SchemaAvailable()
	) {
		return false;
	}

	if (class_slot_1 == class_slot_2 || class_slot_1 == class_slot_3 || class_slot_2 == class_slot_3) {
		return false;
	}

	Profile profile;
	profile.character_id = client->CharacterID();
	profile.class_slot_1 = class_slot_1;
	profile.class_slot_2 = class_slot_2;
	profile.class_slot_3 = class_slot_3;
	profile.multiple_pets_enabled = true;
	profile.locked = true;
	profile.reweaves_available = 0;
	profile.trio_name = BuildTrioName(profile);
	profile.resonance_key = TrioKey(profile);

	if (!SaveProfile(profile, source)) {
		return false;
	}

	AuditProfile(profile, source);
	SeedEligibleSkills(client, true);
	RefreshClientAfterMulticlassProfileChange(client);
	return true;
}

bool MulticlassManager::HasClass(uint32 character_id, uint8 class_id)
{
	if (!MulticlassEnabled() || !IsPlayerClass(class_id)) {
		return false;
	}

	const auto profile = LoadProfile(character_id);
	return profile.class_slot_1 == class_id || profile.class_slot_2 == class_id || profile.class_slot_3 == class_id;
}

bool MulticlassManager::HasClass(const Client *client, uint8 class_id)
{
	if (!client || !IsPlayerClass(class_id)) {
		return false;
	}

	if (!MulticlassEnabled()) {
		return client->GetClass() == class_id;
	}

	const auto slots = GetClassSlots(client);
	return slots[0] == class_id || slots[1] == class_id || slots[2] == class_id;
}

bool MulticlassManager::HasMultiplePetProfile(uint32 character_id)
{
	if (!MulticlassEnabled()) {
		return false;
	}

	const auto profile = LoadProfile(character_id);
	if (!profile.multiple_pets_enabled) {
		return false;
	}

	const std::array<uint8, 3> slots = {profile.class_slot_1, profile.class_slot_2, profile.class_slot_3};
	uint8 pet_classes = 0;
	for (const auto class_id : slots) {
		if (IsPetClass(class_id)) {
			pet_classes++;
		}
	}

	return pet_classes > 1;
}

uint8 MulticlassManager::GetPetRosterLimit(const Client *client)
{
	if (!MulticlassEnabled() || !client || !client->CharacterID()) {
		return 1;
	}

	const auto profile = LoadProfile(client->CharacterID());
	if (!profile.multiple_pets_enabled || !profile.locked) {
		return 1;
	}

	uint8 pet_classes = 0;
	for (const auto class_id : GetClassSlots(client)) {
		pet_classes += IsPetClass(class_id) ? 1 : 0;
	}

	if (!pet_classes) {
		return 1;
	}

	return std::min<uint8>(3, pet_classes);
}

std::vector<Mob *> MulticlassManager::GetPetRoster(const Client *client)
{
	std::vector<Mob *> pets;
	if (!client) {
		return pets;
	}

	for (const auto &entry : entity_list.GetNPCList()) {
		auto *npc = entry.second;
		if (
			!npc ||
			npc->GetOwnerID() != client->GetID() ||
			!npc->IsPet() ||
			!npc->GetPetSpellID()
		) {
			continue;
		}

		pets.push_back(npc);
	}

	std::sort(
		pets.begin(),
		pets.end(),
		[](const Mob *left, const Mob *right) {
			return left && right && left->GetID() < right->GetID();
		}
	);

	return pets;
}

std::vector<Mob *> MulticlassManager::GetSecondaryPetRoster(Client *client)
{
	std::vector<Mob *> pets;
	if (!MulticlassEnabled() || !client || GetPetRosterLimit(client) <= 1) {
		return pets;
	}

	auto *primary_pet = client->GetPet();
	for (auto *pet : GetPetRoster(client)) {
		if (!pet || pet == primary_pet || pet->IsCharmed()) {
			continue;
		}

		pets.push_back(pet);
	}

	return pets;
}

bool MulticlassManager::IsPetRosterMember(const Client *client, Mob *pet)
{
	if (
		!MulticlassEnabled() ||
		!client ||
		!pet ||
		!client->CharacterID() ||
		GetPetRosterLimit(client) <= 1 ||
		pet->GetOwnerID() != client->GetID() ||
		!pet->IsNPC() ||
		!pet->IsPet()
	) {
		return false;
	}

	auto *npc = pet->CastToNPC();
	return npc && npc->GetPetSpellID();
}

bool MulticlassManager::CanCreateAdditionalPet(Client *client)
{
	if (!MulticlassEnabled() || !client) {
		return false;
	}

	auto roster = GetPetRoster(client);
	auto *current_pet = client->GetPet();
	if (
		current_pet &&
		std::find(roster.begin(), roster.end(), current_pet) == roster.end()
	) {
		return false;
	}

	return roster.size() < GetPetRosterLimit(client);
}

bool MulticlassManager::RegisterPet(Client *client, Mob *pet)
{
	if (!MulticlassEnabled() || !client || !pet || !client->CharacterID()) {
		return false;
	}

	auto roster = GetPetRoster(client);
	if (roster.empty()) {
		roster.push_back(pet);
	}

	auto focused_pet = GetFocusedPet(client);
	if (!focused_pet || roster.size() <= 1) {
		focused_pet_ids_[client->CharacterID()] = pet->GetID();
		focused_pet = pet;
	}

	if (focused_pet && focused_pet->GetOwnerID() == client->GetID()) {
		client->SetPetID(focused_pet->GetID());
	} else {
		client->SetPetID(pet->GetID());
	}

	return true;
}

bool MulticlassManager::GetPetFollowPosition(const Client *client, const Mob *pet, glm::vec4 &position)
{
	if (
		!MulticlassEnabled() ||
		!client ||
		!pet ||
		!client->CharacterID() ||
		GetPetRosterLimit(client) <= 1
	) {
		return false;
	}

	const auto roster = GetPetRoster(client);
	if (roster.size() <= 1) {
		return false;
	}

	size_t roster_index = roster.size();
	for (size_t index = 0; index < roster.size(); ++index) {
		if (roster[index] == pet) {
			roster_index = index;
			break;
		}
	}

	if (roster_index == roster.size() || roster_index == 0) {
		return false;
	}

	const float side_offset = roster_index == 1 ? -24.0f : 24.0f;
	const float back_offset = -12.0f;
	const float heading = (client->GetHeading() / 512.0f) * 6.283184f;
	const float forward_x = std::sin(heading);
	const float forward_y = std::cos(heading);
	const float right_x = std::cos(heading);
	const float right_y = -std::sin(heading);

	position = client->GetPosition();
	position.x += (right_x * side_offset) + (forward_x * back_offset);
	position.y += (right_y * side_offset) + (forward_y * back_offset);
	return true;
}

MulticlassManager::ClassSlots MulticlassManager::GetClassSlots(const Client *client)
{
	ClassSlots slots = {0, 0, 0};
	if (!client || !IsPlayerClass(client->GetClass())) {
		return slots;
	}

	if (!MulticlassEnabled()) {
		slots[0] = client->GetClass();
		return slots;
	}

	const auto profile = LoadProfile(client->CharacterID());
	slots[0] = IsPlayerClass(profile.class_slot_1) ? profile.class_slot_1 : client->GetClass();
	slots[1] = IsPlayerClass(profile.class_slot_2) ? profile.class_slot_2 : 0;
	slots[2] = IsPlayerClass(profile.class_slot_3) ? profile.class_slot_3 : 0;

	return slots;
}

uint32 MulticlassManager::GetClassMask(const Client *client)
{
	uint32 mask = 0;
	for (const auto class_id : GetClassSlots(client)) {
		if (IsPlayerClass(class_id)) {
			mask |= GetPlayerClassBit(class_id);
		}
	}

	return mask;
}

uint32 MulticlassManager::GetAAClassMask(const Client *client)
{
	uint32 mask = 0;
	for (const auto class_id : GetClassSlots(client)) {
		if (IsPlayerClass(class_id)) {
			mask |= (1 << class_id);
		}
	}

	return mask;
}

uint8 MulticlassManager::GetBestSpellLevel(const Client *client, uint16 spell_id)
{
	if (!client || !IsValidSpell(spell_id)) {
		return 255;
	}

	return GetBestSpellLevelForSlots(GetClassSlots(client), spell_id);
}

bool MulticlassManager::CanUseSpell(const Client *client, uint16 spell_id)
{
	const auto best_level = GetBestSpellLevel(client, spell_id);
	return best_level != 255 && client && client->GetLevel() >= best_level;
}

bool MulticlassManager::CanUseItem(const Client *client, const EQ::ItemInstance *inst)
{
	if (!client || !inst) {
		return false;
	}

	const auto *item = inst->GetItem();
	if (!item) {
		return false;
	}

	if (!(item->Races & GetPlayerRaceBit(client->GetBaseRace()))) {
		return false;
	}

	return (item->Classes & GetClassMask(client)) != 0;
}

std::string MulticlassManager::BuildItemUseReport(const Client *client, const EQ::ItemInstance *inst, int16 equipment_slot)
{
	if (!client) {
		return "Multiclass item check failed: no client context.";
	}

	const auto *client_name = client->GetName();
	if (!inst) {
		return fmt::format("Multiclass item check for {} failed: no item instance.", client_name);
	}

	const auto *item = inst->GetItem();
	if (!item) {
		return fmt::format("Multiclass item check for {} failed: item data is unavailable.", client_name);
	}

	const auto profile = LoadProfile(client->CharacterID());
	const auto slots = GetClassSlots(client);
	const auto race_mask = GetPlayerRaceBit(client->GetBaseRace());
	const auto deity_mask = Deity::GetBitmask(client->GetDeity());
	const auto class_mask = GetClassMask(client);
	const bool race_ok = (item->Races & race_mask) != 0;
	const bool deity_ok = item->Deity == 0 || (item->Deity & deity_mask) != 0;
	const bool class_ok = (item->Classes & class_mask) != 0;
	const bool level_ok = item->ReqLevel == 0 || client->GetLevel() >= item->ReqLevel;
	const bool slot_check = EQ::ValueWithin(equipment_slot, EQ::invslot::EQUIPMENT_BEGIN, EQ::invslot::EQUIPMENT_END);
	const bool slot_ok = !slot_check || inst->IsSlotAllowed(equipment_slot);
	const uint32 presentation_classes = (race_ok && class_ok) ? (item->Classes | class_mask) : item->Classes;

	std::vector<std::string> failures;
	if (!race_ok) {
		failures.emplace_back("base race is not on the item");
	}
	if (!deity_ok) {
		failures.emplace_back("deity is not on the item");
	}
	if (!class_ok) {
		failures.emplace_back("none of the trio classes are on the item");
	}
	if (!level_ok) {
		failures.emplace_back(fmt::format("requires level {}", item->ReqLevel));
	}
	if (!slot_ok) {
		failures.emplace_back("item cannot go in that equipment slot");
	}

	const std::string result = failures.empty() ? "ALLOW" : fmt::format("BLOCK: {}", JoinValues(failures, ", "));
	const std::string slot_text = slot_check ? fmt::format("slot {}", equipment_slot) : "slot not checked";

	return fmt::format(
		"Multiclass item check for {}: {} ({}) on {} -> {}. Character race {}({}) mask 0x{:X}; deity {}({}) mask 0x{:X}; trio [{} / {} / {}], locked {}, class mask 0x{:X}. Item races 0x{:X}, deity 0x{:X}, classes 0x{:X}, presentation classes 0x{:X}, req level {}, slots 0x{:X}.",
		client_name,
		item->Name,
		item->ID,
		slot_text,
		result,
		GetRaceIDName(client->GetBaseRace()),
		client->GetBaseRace(),
		race_mask,
		Deity::GetName(client->GetDeity()),
		client->GetDeity(),
		deity_mask,
		ClassName(slots[0], client->GetLevel()),
		ClassName(slots[1], client->GetLevel()),
		ClassName(slots[2], client->GetLevel()),
		profile.locked ? "yes" : "no",
		class_mask,
		item->Races,
		item->Deity,
		item->Classes,
		presentation_classes,
		item->ReqLevel,
		item->Slots
	);
}

bool MulticlassManager::CanHaveSkill(const Client *client, EQ::skills::SkillType skill_id)
{
	return GetBestSkillCap(client, skill_id, RuleI(Character, MaxLevel)) > 0;
}

uint16 MulticlassManager::GetBestSkillCap(const Client *client, EQ::skills::SkillType skill_id, uint8 level)
{
	if (!client) {
		return 0;
	}

	uint16 best_cap = 0;
	for (const auto class_id : GetClassSlots(client)) {
		if (!IsPlayerClass(class_id)) {
			continue;
		}

		const auto normalized_skill = NormalizeSkillForClass(client, class_id, skill_id);
		best_cap = std::max(best_cap, static_cast<uint16>(SkillCaps::Instance()->GetSkillCap(class_id, normalized_skill, level).cap));
	}

	return best_cap;
}

uint8 MulticlassManager::GetBestSkillTrainLevel(const Client *client, EQ::skills::SkillType skill_id)
{
	if (!client) {
		return 0;
	}

	uint8 best_train_level = 0;
	for (const auto class_id : GetClassSlots(client)) {
		if (!IsPlayerClass(class_id)) {
			continue;
		}

		const auto normalized_skill = NormalizeSkillForClass(client, class_id, skill_id);
		const auto train_level = SkillCaps::Instance()->GetSkillTrainLevel(class_id, normalized_skill, RuleI(Character, MaxLevel));
		if (train_level && (!best_train_level || train_level < best_train_level)) {
			best_train_level = train_level;
		}
	}

	return best_train_level;
}

uint16 MulticlassManager::SeedEligibleSkills(Client *client, bool notify)
{
	if (!MulticlassEnabled() || !client || !client->CharacterID() || !SchemaAvailable() || !EnsureProfile(client)) {
		return 0;
	}

	const auto profile = LoadProfile(client->CharacterID());
	if (
		!profile.locked ||
		!IsPlayerClass(profile.class_slot_1) ||
		!IsPlayerClass(profile.class_slot_2) ||
		!IsPlayerClass(profile.class_slot_3)
	) {
		return 0;
	}

	std::vector<std::string> seeded_names;
	for (const auto &skill_entry : EQ::skills::GetSkillTypeMap()) {
		const auto skill_id = skill_entry.first;
		if (!ShouldAutoSeedSkill(skill_id) || client->GetRawSkill(skill_id) > 0) {
			continue;
		}

		const auto train_level = GetBestSkillTrainLevel(client, skill_id);
		if (!train_level || client->GetLevel() < train_level) {
			continue;
		}

		if (!GetBestSkillCap(client, skill_id, client->GetLevel())) {
			continue;
		}

		client->SetSkill(skill_id, 1);
		seeded_names.emplace_back(skill_entry.second);
	}

	if (notify && !seeded_names.empty()) {
		std::vector<std::string> preview;
		const auto preview_count = std::min<size_t>(seeded_names.size(), 6);
		for (size_t i = 0; i < preview_count; ++i) {
			preview.emplace_back(seeded_names[i]);
		}

		const auto extra_count = seeded_names.size() - preview_count;
		client->Message(
			Chat::Skills,
			"%s",
			fmt::format(
				"Multiclass unlocked {} class skill{}: {}{}.",
				seeded_names.size(),
				seeded_names.size() == 1 ? "" : "s",
				JoinValues(preview, ", "),
				extra_count ? fmt::format(", and {} more", extra_count) : ""
			).c_str()
		);
	}

	return static_cast<uint16>(seeded_names.size());
}

void MulticlassManager::ApplyTrioBonuses(Client *client, StatBonuses *bonuses)
{
	if (!MulticlassEnabled() || !client || !bonuses || !client->CharacterID() || !SchemaAvailable()) {
		return;
	}

	const auto profile = LoadProfile(client->CharacterID());
	if (
		!profile.locked ||
		!IsPlayerClass(profile.class_slot_1) ||
		!IsPlayerClass(profile.class_slot_2) ||
		!IsPlayerClass(profile.class_slot_3)
	) {
		return;
	}

	const auto tiers = UnlockedResonanceTiers(client->GetLevel());
	if (!tiers) {
		return;
	}

	uint8 pet_classes = 0;
	bool has_tank = false;
	bool has_healer = false;
	bool has_int_caster = false;
	bool has_wis_caster = false;
	bool has_bard = false;
	bool has_striker = false;

	for (const auto class_id : ProfileSlots(profile)) {
		if (!IsPlayerClass(class_id)) {
			continue;
		}

		pet_classes += IsPetClass(class_id) ? 1 : 0;
		has_tank = has_tank || IsTankClass(class_id);
		has_healer = has_healer || IsHealerClass(class_id);
		has_int_caster = has_int_caster || IsIntCasterClass(class_id);
		has_wis_caster = has_wis_caster || IsWisCasterClass(class_id);
		has_bard = has_bard || IsBardClass(class_id);
		has_striker = has_striker || IsStrikerClass(class_id);
	}

	const int level = client->GetLevel();
	const int tier_value = tiers;

	bonuses->HP += level * tier_value * 2;
	bonuses->ATK += tier_value * 3;
	bonuses->AC += tier_value * 3;

	if (has_int_caster || has_wis_caster || has_bard) {
		bonuses->Mana += level * tier_value * 2;
		bonuses->ManaRegen += tier_value;
		bonuses->ChannelChanceSpells += tier_value;
	}

	if (has_tank || has_striker) {
		bonuses->Endurance += level * tier_value * 2;
		bonuses->EnduranceRegen += tier_value;
	}

	if (has_tank) {
		bonuses->HP += level * tier_value * 2;
		bonuses->AC += tier_value * 6;
		bonuses->MeleeMitigation += tier_value;
		bonuses->ShieldBlock += tier_value;
	}

	if (has_healer) {
		bonuses->HealAmt += tier_value * 4;
		bonuses->HealRate += tier_value;
		bonuses->CriticalHealChance += tier_value;
		bonuses->ManaRegen += tier_value;
	}

	if (has_int_caster) {
		bonuses->SpellDmg += tier_value * 4;
		bonuses->CriticalSpellChance += tier_value;
		bonuses->Mana += level * tier_value;
	}

	if (has_wis_caster) {
		bonuses->ManaRegen += tier_value;
		bonuses->WIS += tier_value * 2;
		bonuses->HealAmt += tier_value * 2;
	}

	if (has_striker) {
		bonuses->ATK += tier_value * 5;
		bonuses->HitChance += tier_value * 10;
		bonuses->CriticalHitChance[EQ::skills::HIGHEST_SKILL + 1] += tier_value;
		bonuses->SkillDamageAmount[EQ::skills::SkillBackstab] += tier_value * 2;
		bonuses->SkillDamageAmount[EQ::skills::SkillFrenzy] += tier_value * 2;
		bonuses->SkillDamageAmount2[EQ::skills::SkillBackstab] += tier_value;
		bonuses->SkillDamageAmount2[EQ::skills::SkillFrenzy] += tier_value;
	}

	if (pet_classes) {
		bonuses->PetMaxHP += pet_classes * tier_value * 2;
		bonuses->PetCriticalHit += pet_classes * tier_value;
		bonuses->Pet_Add_Atk += pet_classes * tier_value * 5;
		bonuses->PetMeleeMitigation += pet_classes * tier_value * 2;
	}

	if (has_bard) {
		bonuses->singingMod += tier_value;
		bonuses->brassMod += tier_value;
		bonuses->percussionMod += tier_value;
		bonuses->windMod += tier_value;
		bonuses->stringedMod += tier_value;
		bonuses->songModCap += tier_value;
		bonuses->SongRange += tier_value * 5;
	}

	if (const auto *exact = FindExactTrioTemplate(profile)) {
		const std::string path = exact->bonus_path;
		if (path == "Menagerie") {
			bonuses->PetMaxHP += tier_value * 4;
			bonuses->PetCriticalHit += tier_value * 2;
			bonuses->Pet_Add_Atk += tier_value * 8;
			if (tiers >= 4) {
				bonuses->GivePetGroupTarget = true;
			}
		} else if (path == "Arcanum") {
			bonuses->PercentMaxHPChange += tier_value * 50;
			bonuses->SpellDmg += tier_value * 3;
			bonuses->DamageShield += tier_value * 2;
		} else if (path == "Spellblade") {
			bonuses->SkillDamageAmount[EQ::skills::SkillBackstab] += tier_value * 3;
			bonuses->SkillDamageAmount[EQ::skills::SkillFrenzy] += tier_value * 3;
			bonuses->CriticalSpellChance += tier_value;
			bonuses->ChannelChanceSpells += tier_value * 2;
		} else if (path == "Hymn") {
			bonuses->HealRate += tier_value;
			bonuses->MeleeMitigation += tier_value;
			bonuses->SongRange += tier_value * 5;
		} else if (path == "Conspiracy") {
			bonuses->AvoidMeleeChance += tier_value * 2;
			bonuses->DodgeChance += tier_value;
			bonuses->ChannelChanceSpells += tier_value * 2;
			bonuses->SpellDmg += tier_value * 2;
		} else if (path == "Duelist") {
			bonuses->RiposteChance += tier_value;
			bonuses->DodgeChance += tier_value;
			bonuses->SpellDmg += tier_value * 2;
			bonuses->HitChance += tier_value * 6;
		} else if (path == "Synod") {
			bonuses->HealRate += tier_value * 2;
			bonuses->ManaRegen += tier_value * 2;
			bonuses->CriticalHealChance += tier_value;
		} else if (path == "Aegis") {
			bonuses->AC += tier_value * 8;
			bonuses->MeleeMitigation += tier_value * 2;
			bonuses->HPRegen += tier_value;
		} else if (path == "Dirge") {
			bonuses->SpellDmg += tier_value * 3;
			bonuses->Pet_Add_Atk += tier_value * 5;
			bonuses->ManaRegen += tier_value;
		} else if (path == "Wildcall") {
			bonuses->PetCriticalHit += tier_value;
			bonuses->HitChance += tier_value * 5;
			bonuses->HPRegen += tier_value;
			bonuses->ManaRegen += tier_value;
		}
	}
}

std::string MulticlassManager::BuildTrioBonusSummary(Client *client)
{
	if (!client || !client->CharacterID() || !SchemaAvailable() || !EnsureProfile(client)) {
		return "Resonance bonuses: profile unavailable.";
	}

	const auto profile = LoadProfile(client->CharacterID());
	if (!profile.locked) {
		return "Resonance bonuses unlock every 10 levels after the trio is locked.";
	}

	const auto tiers = UnlockedResonanceTiers(client->GetLevel());
	const auto tier_names = BuildBonusTierNames(profile);

	std::vector<std::string> active;
	for (uint8 i = 0; i < tiers && i < tier_names.size(); ++i) {
		active.emplace_back(fmt::format("L{} {}", (i + 1) * 10, tier_names[i]));
	}

	if (active.empty()) {
		return fmt::format("Resonance bonuses: 0/6 active. Next: L10 {}.", tier_names[0]);
	}

	std::string summary = fmt::format("Resonance bonuses: {}/6 active: {}", tiers, JoinValues(active, ", "));
	if (tiers < tier_names.size()) {
		summary += fmt::format(". Next: L{} {}", (tiers + 1) * 10, tier_names[tiers]);
	}

	return summary + ".";
}

bool MulticlassManager::IsIntCaster(const Client *client)
{
	for (const auto class_id : GetClassSlots(client)) {
		if (IsIntCasterClass(class_id)) {
			return true;
		}
	}

	return false;
}

bool MulticlassManager::IsWisCaster(const Client *client)
{
	for (const auto class_id : GetClassSlots(client)) {
		if (IsWisCasterClass(class_id)) {
			return true;
		}
	}

	return false;
}

bool MulticlassManager::IsBard(const Client *client)
{
	for (const auto class_id : GetClassSlots(client)) {
		if (IsBardClass(class_id)) {
			return true;
		}
	}

	return false;
}

uint8 MulticlassManager::GetPrimaryIntCasterClass(const Client *client)
{
	for (const auto class_id : GetClassSlots(client)) {
		if (IsIntCasterClass(class_id)) {
			return class_id;
		}
	}

	return client ? client->GetClass() : Class::None;
}

uint8 MulticlassManager::GetPrimaryWisCasterClass(const Client *client)
{
	for (const auto class_id : GetClassSlots(client)) {
		if (IsWisCasterClass(class_id)) {
			return class_id;
		}
	}

	return client ? client->GetClass() : Class::None;
}

uint8 MulticlassManager::GetClientPresentationClass(const Client *client)
{
	if (!client || !IsPlayerClass(client->GetClass())) {
		return Class::None;
	}

	if (IsCasterClass(client->GetClass()) || IsSpellFighterClass(client->GetClass())) {
		return client->GetClass();
	}

	for (const auto class_id : GetClassSlots(client)) {
		if (IsCasterClass(class_id) || IsSpellFighterClass(class_id)) {
			return class_id;
		}
	}

	return client->GetClass();
}

void MulticlassManager::SendNativeSpellLevelSnapshot(Client *client)
{
	if (!client) {
		return;
	}

	SendNativeSpellLevelPatch(client, GetClientPresentationClass(client));
}

void MulticlassManager::SendNativeSpellLevelForSpell(Client *client, uint16 spell_id)
{
	SendNativeSpellLevelPatchRow(client, spell_id, GetClientPresentationClass(client));
}

bool MulticlassManager::SendNativeSpellLevelPatchRow(Client *client, uint16 spell_id, uint8 presentation_class)
{
	if (!client || !IsPlayerClass(presentation_class) || !IsValidSpell(spell_id)) {
		return false;
	}

	const auto best_level = GetBestSpellLevel(client, spell_id);
	if (!best_level || best_level == 255) {
		return false;
	}

	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"MULTICLASS|spell_level|id={}|level={}|presentation={}",
			spell_id,
			best_level,
			presentation_class
		).c_str()
	);

	return true;
}

void MulticlassManager::SendHelp(Client *client)
{
	client->Message(Chat::White, "Multiclass admin/native bridge commands:");
	client->Message(Chat::White, "#multiclass status - Show the target Multiclass trio profile.");
	client->Message(Chat::White, "#multiclass set <class1> <class2> <class3> - Admin-set the target trio.");
	client->Message(Chat::White, "#multiclass reweave grant <count> - Admin-grant rare one-slot reweaves to the target.");
	client->Message(Chat::White, "#multiclass reweave <slot 2|3> <class> - Admin-test a one-slot reweave.");
	client->Message(Chat::White, "#multiclass diag - Show capability masks and caster/skill diagnostics.");
	client->Message(Chat::White, "#multiclass itemcheck [cursor|slot <slot>|item <id>] [equip_slot] - Explain item equip eligibility for the target.");
	client->Message(Chat::White, "#multiclass refresh - Resend Multiclass item/spell/AA presentation to the target.");
	client->Message(Chat::White, "#multiclass native - Send the native window snapshot contract.");
	client->Message(Chat::White, "#mc itemcheck [cursor|slot <slot>|item <id>] [equip_slot] - Player-facing item eligibility check.");
	client->Message(Chat::White, "#mc refresh - Resend inventory item usability to this client.");
	client->Message(Chat::White, "#mc pets - Open or refresh the standalone native pet console.");
	client->Message(Chat::White, "#mc petcmd <pet-id|active|all> <attack|back|follow|guard|health|hp|stats|inventory> - Macro-friendly pet control.");
	client->Message(Chat::White, "#mc disc - Open or refresh the native Multiclass discipline bridge.");
	client->Message(Chat::White, "Player flow should go through the native Multiclass UI, not typed commands.");
}

void MulticlassManager::SendStatus(Client *client)
{
	if (!SchemaAvailable()) {
		client->Message(Chat::White, "Multiclass tables are not installed. Run world database:updates for this feature.");
		return;
	}

	Client *target_client = client;
	if (client->GetTarget() && client->GetTarget()->IsClient()) {
		target_client = client->GetTarget()->CastToClient();
	}

	if (!EnsureProfile(target_client)) {
		client->Message(Chat::White, "Multiclass profile is unavailable.");
		return;
	}

	const auto profile = LoadProfile(target_client->CharacterID());
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"Multiclass profile for {}: {} / {} / {} [{}]. Multiple pets: {}. Locked: {}. Reweaves: {}.",
			target_client->GetCleanName(),
			ClassName(profile.class_slot_1, target_client->GetLevel()),
			ClassName(profile.class_slot_2, target_client->GetLevel()),
			ClassName(profile.class_slot_3, target_client->GetLevel()),
			profile.trio_name,
			profile.multiple_pets_enabled ? "enabled" : "disabled",
			profile.locked ? "yes" : "no",
			profile.reweaves_available
		).c_str()
	);
}

void MulticlassManager::SendDiagnostics(Client *client, const char *topic)
{
	if (!client) {
		return;
	}

	Client *target_client = client;
	if (client->GetTarget() && client->GetTarget()->IsClient()) {
		target_client = client->GetTarget()->CastToClient();
	}

	if (!EnsureProfile(target_client)) {
		client->Message(Chat::White, "Multiclass profile is unavailable.");
		return;
	}

	const auto topic_value = topic ? Strings::ToLower(topic) : "";
	if (!topic_value.empty() && topic_value != "all") {
		if (topic_value == "skills" || topic_value == "skill") {
			uint16 eligible_count = 0;
			uint16 active_count = 0;
			uint16 seedable_count = 0;
			std::vector<std::string> missing_preview;

			for (const auto &skill_entry : EQ::skills::GetSkillTypeMap()) {
				const auto skill_id = skill_entry.first;
				if (!ShouldAutoSeedSkill(skill_id)) {
					continue;
				}

				const auto train_level = GetBestSkillTrainLevel(target_client, skill_id);
				const auto cap = GetBestSkillCap(target_client, skill_id, target_client->GetLevel());
				if (!train_level || target_client->GetLevel() < train_level || !cap) {
					continue;
				}

				++eligible_count;
				if (target_client->GetRawSkill(skill_id) > 0) {
					++active_count;
				} else {
					++seedable_count;
					if (missing_preview.size() < 8) {
						missing_preview.emplace_back(skill_entry.second);
					}
				}
			}

			client->Message(
				Chat::White,
				"%s",
				fmt::format(
					"Multiclass skill diagnostics for {}: {} active / {} eligible; {} ready to seed.",
					target_client->GetCleanName(),
					active_count,
					eligible_count,
					seedable_count
				).c_str()
			);
			if (!missing_preview.empty()) {
				client->Message(Chat::White, "%s", fmt::format("Ready preview: {}.", JoinValues(missing_preview, ", ")).c_str());
			}
			return;
		}

		if (topic_value == "spells" || topic_value == "spell") {
			uint16 scribed_count = 0;
			uint16 usable_count = 0;
			uint16 off_presentation_count = 0;
			const auto presentation_class = GetClientPresentationClass(target_client);
			for (uint32 slot_id = 0; slot_id < EQ::spells::SPELLBOOK_SIZE; ++slot_id) {
				const auto spell_id = target_client->GetPP().spell_book[slot_id];
				if (!IsValidSpell(spell_id)) {
					continue;
				}

				++scribed_count;
				const auto best_level = GetBestSpellLevel(target_client, spell_id);
				if (best_level != 255 && target_client->GetLevel() >= best_level) {
					++usable_count;
					if (IsPlayerClass(presentation_class) && spells[spell_id].classes[presentation_class - 1] == 255) {
						++off_presentation_count;
					}
				}
			}

			client->Message(
				Chat::White,
				"%s",
				fmt::format(
					"Multiclass spell diagnostics for {}: {} scribed, {} usable by trio, {} require native off-presentation display patches.",
					target_client->GetCleanName(),
					scribed_count,
					usable_count,
					off_presentation_count
				).c_str()
			);
			return;
		}

		if (topic_value == "discs" || topic_value == "disc" || topic_value == "disciplines") {
			uint16 learned_count = 0;
			uint16 usable_count = 0;
			for (uint32 slot_id = 0; slot_id < MAX_PP_DISCIPLINES; ++slot_id) {
				const auto spell_id = target_client->GetPP().disciplines.values[slot_id];
				if (!IsValidSpell(spell_id)) {
					continue;
				}

				++learned_count;
				if (CanUseSpell(target_client, spell_id)) {
					++usable_count;
				}
			}

			client->Message(
				Chat::White,
				"%s",
				fmt::format(
					"Multiclass discipline diagnostics for {}: {} learned, {} usable by trio.",
					target_client->GetCleanName(),
					learned_count,
					usable_count
				).c_str()
			);
			return;
		}

		if (topic_value == "aa" || topic_value == "aas") {
			uint16 visible_count = 0;
			uint16 learned_count = 0;
			uint16 first_rank_purchase_count = 0;
			uint16 grant_only_count = 0;
			const auto aa_class_mask = GetAAClassMask(target_client);
			for (const auto &aa_entry : zone->aa_abilities) {
				auto *ability = aa_entry.second.get();
				if (!ability || !ability->first || !(ability->classes & aa_class_mask)) {
					continue;
				}

				if (!target_client->CanUseAlternateAdvancementRank(ability->first)) {
					continue;
				}

				++visible_count;
				if (ability->grant_only) {
					++grant_only_count;
				}

				uint32 charges = 0;
				if (target_client->GetAA(ability->first_rank_id, &charges)) {
					++learned_count;
				} else if (target_client->CanPurchaseAlternateAdvancementRank(ability->first, false, true)) {
					++first_rank_purchase_count;
				}
			}

			client->Message(
				Chat::White,
				"%s",
				fmt::format(
					"Multiclass AA diagnostics for {}: AA class mask {}, normal class mask {}.",
					target_client->GetCleanName(),
					aa_class_mask,
					GetClassMask(target_client)
				).c_str()
			);
			client->Message(
				Chat::White,
				"%s",
				fmt::format(
					"AA window candidates: {} visible definitions, {} learned/profile entries, {} first ranks purchasable, {} grant-only.",
					visible_count,
					learned_count,
					first_rank_purchase_count,
					grant_only_count
				).c_str()
			);
			client->Message(
				Chat::White,
				"%s",
				fmt::format(
					"Learned AA packet guard: {} / {} profile entries used. Current first goal is visibility; full buyout needs separate cap testing.",
					learned_count,
					MAX_PP_AA_ARRAY
				).c_str()
			);
			return;
		}

		if (topic_value == "melody" || topic_value == "bard" || topic_value == "songs") {
			client->Message(Chat::White, "%s", BuildBardMelodySummary(target_client).c_str());
			if (IsBard(target_client)) {
				const auto slots = LoadBardMelody(target_client);
				for (size_t i = 0; i < slots.size(); ++i) {
					const auto spell_id = slots[i];
					client->Message(
						Chat::White,
						"%s",
						fmt::format(
							"Melody slot {}: {}{}",
							i + 1,
							IsValidSpell(spell_id) ? spells[spell_id].name : "-",
							IsValidSpell(spell_id) ? fmt::format(" ({})", spell_id) : ""
						).c_str()
					);
				}
			}
			return;
		}

		if (topic_value == "bonuses" || topic_value == "bonus" || topic_value == "resonance") {
			client->Message(Chat::White, "%s", BuildTrioBonusSummary(target_client).c_str());
			const auto profile = LoadProfile(target_client->CharacterID());
			const auto metadata = BuildTrioMetadata(profile);
			client->Message(Chat::White, "%s", fmt::format("Resonance identity: {}. Roles: {}.", metadata.resonance, metadata.roles).c_str());
			return;
		}

		if (topic_value == "pets" || topic_value == "pet") {
			const auto roster = GetPetRoster(target_client);
			client->Message(
				Chat::White,
				"%s",
				fmt::format(
					"Multiclass pet diagnostics for {}: {}/{} active roster pets. Focus: {}.",
					target_client->GetCleanName(),
					roster.size(),
					GetPetRosterLimit(target_client),
					GetFocusedPet(target_client) ? GetFocusedPet(target_client)->GetCleanName() : "-"
				).c_str()
			);
			for (auto *pet : roster) {
				if (!pet || !pet->IsNPC()) {
					continue;
				}
				client->Message(
					Chat::White,
					"%s",
					fmt::format(
						"Pet {} [{}]: order {}, hp {:.0f}, mana {:.0f}, taunt {}, hold {}, nocast {}.",
						pet->GetCleanName(),
						pet->GetID(),
						PetOrderName(pet->GetPetOrder()),
						pet->GetHPRatio(),
						pet->GetManaRatio(),
						pet->CastToNPC()->IsTaunting() ? "on" : "off",
						(pet->IsHeld() || pet->IsGHeld()) ? "on" : "off",
						pet->IsNoCast() ? "on" : "off"
					).c_str()
				);
			}
			return;
		}

		if (topic_value == "items" || topic_value == "item") {
			const auto class_mask = GetClassMask(target_client);
			uint16 usable_equipped = 0;
			uint16 blocked_equipped = 0;
			for (int16 slot_id = EQ::invslot::EQUIPMENT_BEGIN; slot_id <= EQ::invslot::EQUIPMENT_END; ++slot_id) {
				const auto *inst = target_client->GetInv().GetItem(slot_id);
				if (!inst) {
					continue;
				}

				if (CanUseItem(target_client, inst)) {
					++usable_equipped;
				} else {
					++blocked_equipped;
				}
			}

			client->Message(
				Chat::White,
				"%s",
				fmt::format(
					"Multiclass item diagnostics for {}: class mask {}, equipped usable {}, equipped blocked {}.",
					target_client->GetCleanName(),
					class_mask,
					usable_equipped,
					blocked_equipped
				).c_str()
			);
			return;
		}

		client->Message(Chat::White, "#multiclass diag topics: skills, spells, discs, aa, melody, bonuses, pets, items, all.");
		return;
	}

	const auto slots = GetClassSlots(target_client);
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"Multiclass diagnostics for {}: slots [{} / {} / {}], class mask {}, AA mask {}.",
			target_client->GetCleanName(),
			ClassName(slots[0], target_client->GetLevel()),
			ClassName(slots[1], target_client->GetLevel()),
			ClassName(slots[2], target_client->GetLevel()),
			GetClassMask(target_client),
			GetAAClassMask(target_client)
		).c_str()
	);
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"Client presentation class: {}. Base server class remains {}.",
			ClassName(GetClientPresentationClass(target_client), target_client->GetLevel()),
			ClassName(target_client->GetClass(), target_client->GetLevel())
		).c_str()
	);
	client->Message(Chat::White, "%s", BuildTrioBonusSummary(target_client).c_str());
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"Caster flags: INT={}, WIS={}, Bard={}. Skill samples: Meditate cap {}, Dual Wield cap {}, Defense cap {}.",
			IsIntCaster(target_client) ? "yes" : "no",
			IsWisCaster(target_client) ? "yes" : "no",
			IsBard(target_client) ? "yes" : "no",
			GetBestSkillCap(target_client, EQ::skills::SkillMeditate, target_client->GetLevel()),
			GetBestSkillCap(target_client, EQ::skills::SkillDualWield, target_client->GetLevel()),
			GetBestSkillCap(target_client, EQ::skills::SkillDefense, target_client->GetLevel())
		).c_str()
	);
}

void MulticlassManager::SendItemCheck(Client *requester, Client *target_client, const char *mode, const char *value, const char *slot_value)
{
	if (!requester || !target_client) {
		return;
	}

	const std::string mode_value = mode && mode[0] ? Strings::ToLower(mode) : "cursor";
	const std::string value_text = value && value[0] ? value : "";
	const std::string slot_text = slot_value && slot_value[0] ? slot_value : "";
	int16 equipment_slot = -1;
	if (Strings::IsNumber(slot_text)) {
		equipment_slot = static_cast<int16>(Strings::ToInt(slot_text, -1));
	}

	const EQ::ItemInstance *inst = nullptr;
	EQ::ItemInstance *created_inst = nullptr;
	std::string source;

	if (mode_value == "cursor") {
		inst = target_client->GetInv().GetItem(EQ::invslot::slotCursor);
		source = "cursor";
		if (Strings::IsNumber(value_text)) {
			equipment_slot = static_cast<int16>(Strings::ToInt(value_text, -1));
		}
	} else if (mode_value == "slot") {
		if (!Strings::IsNumber(value_text)) {
			requester->Message(Chat::White, "Usage: #mc itemcheck slot <slot_id> [equip_slot]");
			return;
		}

		const auto slot_id = static_cast<int16>(Strings::ToInt(value_text, -1));
		if (!target_client->IsValidSlot(slot_id)) {
			requester->Message(Chat::White, "%s", fmt::format("Slot {} is not valid for {}.", slot_id, target_client->GetCleanName()).c_str());
			return;
		}

		inst = target_client->GetInv().GetItem(slot_id);
		source = fmt::format("slot {}", slot_id);
		if (equipment_slot < 0 && EQ::ValueWithin(slot_id, EQ::invslot::EQUIPMENT_BEGIN, EQ::invslot::EQUIPMENT_END)) {
			equipment_slot = slot_id;
		}
	} else if (mode_value == "item" || mode_value == "id") {
		if (!Strings::IsNumber(value_text)) {
			requester->Message(Chat::White, "Usage: #mc itemcheck item <item_id> [equip_slot]");
			return;
		}

		const auto item_id = Strings::ToUnsignedInt(value_text);
		created_inst = database.CreateItem(item_id);
		inst = created_inst;
		source = fmt::format("item {}", item_id);
	} else if (Strings::IsNumber(mode_value)) {
		const auto number = Strings::ToUnsignedInt(mode_value);
		if (target_client->IsValidSlot(number) && target_client->GetInv().GetItem(static_cast<int16>(number))) {
			inst = target_client->GetInv().GetItem(static_cast<int16>(number));
			source = fmt::format("slot {}", number);
			if (Strings::IsNumber(value_text)) {
				equipment_slot = static_cast<int16>(Strings::ToInt(value_text, -1));
			} else if (EQ::ValueWithin(static_cast<int16>(number), EQ::invslot::EQUIPMENT_BEGIN, EQ::invslot::EQUIPMENT_END)) {
				equipment_slot = static_cast<int16>(number);
			}
		} else {
			created_inst = database.CreateItem(number);
			inst = created_inst;
			source = fmt::format("item {}", number);
			if (Strings::IsNumber(value_text)) {
				equipment_slot = static_cast<int16>(Strings::ToInt(value_text, -1));
			}
		}
	} else {
		requester->Message(Chat::White, "Usage: #mc itemcheck [cursor [equip_slot] | slot <slot_id> [equip_slot] | item <item_id> [equip_slot] | <item_id> [equip_slot]]");
		return;
	}

	if (!inst) {
		requester->Message(Chat::White, "%s", fmt::format("No item found for {} on {}.", source, target_client->GetCleanName()).c_str());
		safe_delete(created_inst);
		return;
	}

	const auto report = BuildItemUseReport(target_client, inst, equipment_slot);
	requester->Message(Chat::White, "%s", fmt::format("Checking {} for {}.", source, target_client->GetCleanName()).c_str());
	requester->Message(Chat::White, "%s", report.c_str());
	if (requester != target_client) {
		target_client->Message(Chat::Yellow, "%s", report.c_str());
	}

	safe_delete(created_inst);
}

void MulticlassManager::SendNativeSnapshot(Client *client, const std::string &status, bool show_window, bool show_pet_window)
{
	if (!SchemaAvailable()) {
		client->Message(Chat::White, "Multiclass tables are not installed. Run world database:updates for this feature.");
		return;
	}

	if (!EnsureProfile(client)) {
		client->Message(Chat::White, "Multiclass native snapshot is unavailable.");
		return;
	}

	const auto profile = LoadProfile(client->CharacterID());
	const auto presentation_class = GetClientPresentationClass(client);
	const auto metadata = BuildTrioMetadata(profile);
	const auto pet_roster = GetPetRoster(client);
	auto *focused_pet = GetFocusedPet(client);
	const auto focused_pet_id = focused_pet ? focused_pet->GetID() : 0;
	const auto current_mana = std::max<int64>(0, client->GetMana());
	const auto max_mana = std::max<int64>(0, client->GetMaxMana());
	const auto current_endurance = std::max<int64>(0, client->GetEndurance());
	const auto max_endurance = std::max<int64>(0, client->GetMaxEndurance());
	const auto seeded_skills = SeedEligibleSkills(client, true);
	std::string status_text = status.empty() ?
		(profile.locked ? "Trio locked." : "Choose two added classes to lock your trio.") :
		status;
	if (seeded_skills > 0 && status.empty()) {
		status_text = fmt::format("Unlocked {} Multiclass skill{}.", seeded_skills, seeded_skills == 1 ? "" : "s");
	}

	client->Message(Chat::White, "MULTICLASS|window|clear");
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"MULTICLASS|profile|name={}|resonance={}|class1={}|class1_name={}|class2={}|class2_name={}|class3={}|class3_name={}|presentation={}|presentation_name={}|base={}|base_name={}|pets={}|locked={}|reweaves={}",
			ProtocolValue(profile.trio_name),
			ProtocolValue(profile.resonance_key.empty() ? TrioKey(profile) : profile.resonance_key),
			profile.class_slot_1,
			ProtocolValue(ClassName(profile.class_slot_1, client->GetLevel())),
			profile.class_slot_2,
			ProtocolValue(ClassName(profile.class_slot_2, client->GetLevel())),
			profile.class_slot_3,
			ProtocolValue(ClassName(profile.class_slot_3, client->GetLevel())),
			presentation_class,
			ProtocolValue(ClassName(presentation_class, client->GetLevel())),
			client->GetClass(),
			ProtocolValue(ClassName(client->GetClass(), client->GetLevel())),
			profile.multiple_pets_enabled ? 1 : 0,
			profile.locked ? 1 : 0,
			profile.reweaves_available
		).c_str()
	);
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"MULTICLASS|vitals|mana={}|max_mana={}|endurance={}|max_endurance={}|class_mask={}|aa_mask={}|presentation={}|base={}",
			current_mana,
			max_mana,
			current_endurance,
			max_endurance,
			GetClassMask(client),
			GetAAClassMask(client),
			presentation_class,
			client->GetClass()
		).c_str()
	);
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"MULTICLASS|trio|roles={}|resonance={}|summary={}",
			ProtocolValue(metadata.roles),
			ProtocolValue(metadata.resonance),
			ProtocolValue(metadata.summary)
		).c_str()
	);
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"MULTICLASS|skills|summary={}|unlocked={}",
			ProtocolValue(BuildSkillSummary(client)),
			seeded_skills
		).c_str()
	);
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"MULTICLASS|bonuses|summary={}",
			ProtocolValue(BuildTrioBonusSummary(client))
		).c_str()
	);
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"MULTICLASS|selection|can_choose={}|status={}",
			profile.locked ? 0 : 1,
			ProtocolValue(status_text)
		).c_str()
	);
	SendNativeSpellLevelPatch(client, presentation_class);
	SendNativeBardMelody(client, false);
	SendNativeDisciplines(client, false);
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"MULTICLASS|pet_roster|mode=live|count={}|limit={}|focus={}|policy={}|control={}",
			pet_roster.size(),
			GetPetRosterLimit(client),
			focused_pet_id,
			ProtocolValue(metadata.pet_policy),
			ProtocolValue(metadata.pet_control)
		).c_str()
	);
	for (auto *pet : pet_roster) {
		if (!pet || !pet->IsNPC()) {
			continue;
		}

		auto *pet_npc = pet->CastToNPC();
		auto *target = pet->GetTarget();
		client->Message(
			Chat::White,
			"%s",
			fmt::format(
				"MULTICLASS|pet|id={}|name={}|hp={:.0f}|mana={:.0f}|target={}|taunt={}|hold={}|spellhold={}|order={}|focused={}",
				pet->GetID(),
				ProtocolValue(pet->GetCleanName()),
				pet->GetHPRatio(),
				pet->GetManaRatio(),
				ProtocolValue(target ? target->GetCleanName() : "-"),
				pet_npc->IsTaunting() ? 1 : 0,
				(pet->IsHeld() || pet->IsGHeld()) ? 1 : 0,
				pet->IsNoCast() ? 1 : 0,
				ProtocolValue(PetOrderName(pet->GetPetOrder())),
				pet->GetID() == focused_pet_id ? 1 : 0
			).c_str()
		);
	}
	if (show_window) {
		client->Message(Chat::White, "MULTICLASS|window|show");
	}

	if (show_pet_window) {
		client->Message(Chat::White, "MULTICLASS|pet_window|show");
	}
}

void MulticlassManager::SendNativeSpellLevelPatch(Client *client, uint8 presentation_class)
{
	if (!client || !IsPlayerClass(presentation_class)) {
		return;
	}

	uint32 patched_count = 0;
	const auto class_slots = GetClassSlots(client);
	const auto max_spell_id = std::min<uint32>(SPDAT_RECORDS, static_cast<uint32>(std::numeric_limits<uint16>::max()) + 1);
	client->Message(Chat::White, "MULTICLASS|spell_levels|begin");
	for (uint32 spell_id_value = 2; spell_id_value < max_spell_id; ++spell_id_value) {
		const auto spell_id = static_cast<uint16>(spell_id_value);
		if (!IsValidSpell(spell_id)) {
			continue;
		}

		const auto best_level = GetBestSpellLevelForSlots(class_slots, spell_id);
		if (!best_level || best_level == 255) {
			continue;
		}

		client->Message(
			Chat::White,
			"%s",
			fmt::format(
				"MULTICLASS|spell_level|id={}|level={}|presentation={}",
				spell_id,
				best_level,
				presentation_class
			).c_str()
		);
		patched_count++;
	}

	client->Message(
		Chat::White,
		"%s",
		fmt::format("MULTICLASS|spell_levels|end|count={}", patched_count).c_str()
	);
}

bool MulticlassManager::SetProfileFromNative(Client *client, uint8 class_slot_2, uint8 class_slot_3, std::string &status)
{
	if (!client || !client->CharacterID()) {
		status = "Character profile is unavailable.";
		return false;
	}

	if (!SchemaAvailable() || !EnsureProfile(client)) {
		status = "Multiclass tables are unavailable.";
		return false;
	}

	auto profile = LoadProfile(client->CharacterID());
	if (profile.locked) {
		status = "This trio is already locked.";
		return false;
	}

	const auto class_slot_1 = IsPlayerClass(profile.class_slot_1) ? profile.class_slot_1 : client->GetClass();
	if (!IsPlayerClass(class_slot_1) || !IsPlayerClass(class_slot_2) || !IsPlayerClass(class_slot_3)) {
		status = "Choose two valid added classes.";
		return false;
	}

	if (class_slot_1 == class_slot_2 || class_slot_1 == class_slot_3 || class_slot_2 == class_slot_3) {
		status = "Choose three different classes.";
		return false;
	}

	if (!SetProfile(client, class_slot_1, class_slot_2, class_slot_3, "native")) {
		status = "Unable to lock the selected trio.";
		return false;
	}

	return true;
}

bool MulticlassManager::ReweaveProfileSlot(Client *client, uint8 class_slot, uint8 class_id, const std::string &source, std::string &status)
{
	if (!client || !client->CharacterID()) {
		status = "Character profile is unavailable.";
		return false;
	}

	if (!SchemaAvailable() || !EnsureProfile(client)) {
		status = "Multiclass tables are unavailable.";
		return false;
	}

	if (!IsPlayerClass(class_id) || class_slot < 2 || class_slot > 3) {
		status = "Reweave usage: choose slot 2 or 3 and a valid class. Slot 1 stays tied to the character's base identity for now.";
		return false;
	}

	auto profile = LoadProfile(client->CharacterID());
	if (!profile.locked) {
		status = "Lock a trio before reweaving it.";
		return false;
	}

	if (!profile.reweaves_available && source != "admin_reweave") {
		status = "No reweaves are available.";
		return false;
	}

	if (source != "admin_reweave" && (!zone || (!zone->IsCity() && !zone->CanBind()))) {
		status = "Reweaves are only available in cities or bind-safe sanctuaries.";
		return false;
	}

	if (
		class_id == profile.class_slot_1 ||
		(class_slot != 2 && class_id == profile.class_slot_2) ||
		(class_slot != 3 && class_id == profile.class_slot_3)
	) {
		status = "Choose a class that is not already in the trio.";
		return false;
	}

	const auto previous_class = class_slot == 2 ? profile.class_slot_2 : profile.class_slot_3;
	if (class_slot == 2) {
		profile.class_slot_2 = class_id;
	} else {
		profile.class_slot_3 = class_id;
	}

	if (source != "admin_reweave" && profile.reweaves_available > 0) {
		--profile.reweaves_available;
	}

	profile.trio_name = BuildTrioName(profile);
	profile.resonance_key = TrioKey(profile);
	profile.multiple_pets_enabled = true;

	if (!SaveProfile(profile, source)) {
		status = "Unable to save the reweave.";
		return false;
	}

	AuditProfile(profile, source);
	SeedEligibleSkills(client, true);
	RefreshClientAfterMulticlassProfileChange(client);

	status = fmt::format(
		"Rewove slot {} from {} to {}. Reweaves left: {}.",
		class_slot,
		ClassName(previous_class, client->GetLevel()),
		ClassName(class_id, client->GetLevel()),
		profile.reweaves_available
	);
	return true;
}

Mob *MulticlassManager::GetFocusedPet(Client *client)
{
	if (!client || !client->CharacterID()) {
		return nullptr;
	}

	const auto roster = GetPetRoster(client);
	if (roster.empty()) {
		focused_pet_ids_.erase(client->CharacterID());
		return nullptr;
	}

	const auto focused_pet_id = focused_pet_ids_[client->CharacterID()];
	auto *focused_pet = entity_list.GetMob(focused_pet_id);
	if (focused_pet && focused_pet->GetOwnerID() == client->GetID() && focused_pet->IsPet()) {
		return focused_pet;
	}

	focused_pet_ids_[client->CharacterID()] = roster.front()->GetID();
	client->SetPetID(roster.front()->GetID());
	return roster.front();
}

bool MulticlassManager::SetFocusedPet(Client *client, uint16 pet_id)
{
	if (!client || !client->CharacterID() || !pet_id) {
		return false;
	}

	for (auto *pet : GetPetRoster(client)) {
		if (pet && pet->GetID() == pet_id) {
			focused_pet_ids_[client->CharacterID()] = pet_id;
			client->SetPetID(pet_id);
			return true;
		}
	}

	return false;
}

Mob *MulticlassManager::GetPetForClass(Client *client, uint8 class_id, const std::vector<Mob *> &roster, std::string &status)
{
	if (!client || !IsPetClass(class_id)) {
		status = "That class does not have a persistent pet command target.";
		return nullptr;
	}

	if (!HasClass(client, class_id)) {
		status = fmt::format("Your trio does not include {}.", ClassName(class_id, client->GetLevel()));
		return nullptr;
	}

	for (auto *pet : roster) {
		if (pet && GetPetOriginClass(pet) == class_id) {
			return pet;
		}
	}

	std::vector<uint8> pet_classes;
	for (const auto slot_class_id : GetClassSlots(client)) {
		if (IsPetClass(slot_class_id)) {
			pet_classes.emplace_back(slot_class_id);
		}
	}

	const auto class_slot = std::find(pet_classes.begin(), pet_classes.end(), class_id);
	if (class_slot != pet_classes.end()) {
		const auto roster_index = static_cast<size_t>(std::distance(pet_classes.begin(), class_slot));
		if (roster_index < roster.size() && roster[roster_index]) {
			return roster[roster_index];
		}
	}

	status = fmt::format("No active {} pet was found.", ClassName(class_id, client->GetLevel()));
	return nullptr;
}

bool MulticlassManager::ApplyStockPetCommand(Client *client, Mob *pet, uint32 command, Mob *target, std::string &status)
{
	if (!client || !pet || !pet->IsNPC()) {
		status = "Select an active Multiclass pet first.";
		return false;
	}

	const auto pet_id = pet->GetID();
	const auto original_pet_id = client->GetPetID();

	client->SetPetID(pet_id);

	EQApplicationPacket app(OP_PetCommands, sizeof(PetCommand_Struct));
	auto *pet_command = reinterpret_cast<PetCommand_Struct *>(app.pBuffer);
	pet_command->command = command;
	pet_command->target = target ? target->GetID() : 0;
	client->Handle_OP_PetCommands(&app);

	if (command == PET_GETLOST) {
		focused_pet_ids_.erase(client->CharacterID());
		if (original_pet_id && original_pet_id != pet_id && entity_list.GetMob(original_pet_id)) {
			client->SetPetID(original_pet_id);
		} else {
			client->SetPetID(0);
			GetFocusedPet(client);
		}
		status = "Focused pet dismissed.";
		return true;
	}

	if (original_pet_id && original_pet_id != pet_id && entity_list.GetMob(original_pet_id)) {
		client->SetPetID(original_pet_id);
	}

	status = "Pet command sent.";
	return true;
}

bool MulticlassManager::ApplyPetAction(Client *client, Mob *pet, const std::string &action, std::string &status)
{
	if (!client || !pet || !pet->IsNPC()) {
		status = "Select an active Multiclass pet first.";
		return false;
	}

	auto *pet_npc = pet->CastToNPC();
	const auto action_name = Strings::ToLower(action);

	if (action_name == "attack" || action_name == "qattack") {
		auto *target = client->GetTarget();
		if (!target || target == client) {
			status = "Target an enemy before sending pets to attack.";
			return false;
		}

		if (target->IsMezzed()) {
			status = "Pets will not wake a mezzed target.";
			return false;
		}

		if (pet->IsFeared() || !pet->IsAttackAllowed(target)) {
			status = "One or more pets cannot attack that target.";
			return false;
		}

		ApplyStockPetCommand(client, pet, action_name == "qattack" ? PET_QATTACK : PET_ATTACK, target, status);
		status = fmt::format("Sent {} to attack {}.", pet->GetCleanName(), target->GetCleanName());
		return true;
	}

	if (action_name == "back" || action_name == "backoff") {
		ApplyStockPetCommand(client, pet, PET_BACKOFF, nullptr, status);
		status = "Pet attack cleared.";
		return true;
	}

	if (action_name == "follow" || action_name == "guardme") {
		ApplyStockPetCommand(client, pet, action_name == "guardme" ? PET_GUARDME : PET_FOLLOWME, nullptr, status);
		status = "Pet follow mode set.";
		return true;
	}

	if (action_name == "guard" || action_name == "guardhere") {
		ApplyStockPetCommand(client, pet, PET_GUARDHERE, nullptr, status);
		status = "Pet guard spot set.";
		return true;
	}

	if (action_name == "taunt") {
		ApplyStockPetCommand(client, pet, PET_TAUNT, nullptr, status);
		status = pet_npc->IsTaunting() ? "Focused pet taunt enabled." : "Focused pet taunt disabled.";
		return true;
	}

	if (action_name == "hold") {
		pet->SetHeld(!pet->IsHeld());
		if (pet->IsHeld()) {
			pet->SetGHeld(false);
			client->MessageString(Chat::PetResponse, PET_HOLD_SET_ON);
			pet->SayString(client, Chat::PetResponse, PET_NOW_HOLDING);
		} else {
			client->MessageString(Chat::PetResponse, PET_HOLD_SET_OFF);
		}
		client->SetPetCommandState(PET_BUTTON_HOLD, pet->IsHeld() ? 1 : 0);
		client->SetPetCommandState(PET_BUTTON_GHOLD, 0);
		status = pet->IsHeld() ? "Focused pet hold enabled." : "Focused pet hold disabled.";
		return true;
	}

	if (action_name == "spellhold" || action_name == "nocast") {
		pet->SetNoCast(!pet->IsNoCast());
		client->MessageString(Chat::PetResponse, pet->IsNoCast() ? PET_NOT_CASTING : PET_CASTING);
		client->MessageString(Chat::PetResponse, pet->IsNoCast() ? PET_SPELLHOLD_SET_ON : PET_SPELLHOLD_SET_OFF);
		client->SetPetCommandState(PET_BUTTON_SPELLHOLD, pet->IsNoCast() ? 1 : 0);
		status = pet->IsNoCast() ? "Focused pet spell hold enabled." : "Focused pet spell hold disabled.";
		return true;
	}

	if (action_name == "sit") {
		const bool should_sit = pet->GetPetOrder() != 1;
		pet->SetPetOrder(should_sit ? 1 : 0);
		pet->SetAppearance(should_sit ? eaSitting : eaStanding);
		status = should_sit ? "Focused pet is sitting." : "Focused pet is standing.";
		return true;
	}

	if (action_name == "dismiss" || action_name == "getlost") {
		return ApplyStockPetCommand(client, pet, PET_GETLOST, nullptr, status);
	}

	if (IsPetHealthAction(action_name)) {
		ApplyStockPetCommand(client, pet, PET_HEALTHREPORT, nullptr, status);
		status = "Pet health reported.";
		return true;
	}

	status = "Unsupported pet action.";
	return false;
}

bool MulticlassManager::HandleNativePetCommand(Client *client, const Seperator *sep, std::string &status)
{
	if (!client || !sep) {
		status = "Multiclass pet command is unavailable.";
		return false;
	}

	const auto roster = GetPetRoster(client);
	if (roster.empty()) {
		status = "No active Multiclass pets yet.";
		return false;
	}

	if (sep->argnum < 2 || !strcasecmp(sep->arg[2], "refresh")) {
		status = "Pet roster refreshed.";
		return true;
	}

	std::string action = sep->arg[2] ? sep->arg[2] : "";
	auto action_name = Strings::ToLower(action);

	const auto requested_class_id = ParseClassId(action.c_str());
	if (IsPetClass(requested_class_id)) {
		const std::string class_action = sep->arg[3] ? sep->arg[3] : "";
		if (class_action.empty()) {
			status = "Usage: /pet <nec|mag|bst> <action>";
			return false;
		}

		auto *class_pet = GetPetForClass(client, requested_class_id, roster, status);
		if (!class_pet) {
			return false;
		}

		SetFocusedPet(client, class_pet->GetID());

		std::string action_status;
		if (!ApplyPetAction(client, class_pet, class_action, action_status)) {
			status = fmt::format("{} pet: {}", ClassName(requested_class_id, client->GetLevel()), action_status);
			return false;
		}

		status = fmt::format("{} pet: {}", ClassName(requested_class_id, client->GetLevel()), action_status);
		return true;
	}

	if (action_name == "focus") {
		const auto pet_id = static_cast<uint16>(Strings::ToUnsignedInt(sep->arg[3]));
		if (!SetFocusedPet(client, pet_id)) {
			status = "Unable to focus that pet.";
			return false;
		}

		auto *pet = GetFocusedPet(client);
		status = pet ? fmt::format("Focused {}.", pet->GetCleanName()) : "Pet focus updated.";
		return true;
	}

	std::vector<Mob *> targets;
	auto target_arg = sep->arg[3] ? Strings::ToLower(sep->arg[3]) : "";
	if ((action_name == "active" || action_name == "focused" || action_name == "all" || Strings::IsNumber(action_name)) && sep->arg[3] && sep->arg[3][0]) {
		target_arg = action_name;
		action = sep->arg[3];
		action_name = Strings::ToLower(action);
	}

	const bool target_requested = !target_arg.empty();
	const bool all_pets = target_arg == "all" ||
		(!target_requested && (
			action_name == "attack" ||
			action_name == "qattack" ||
			action_name == "back" ||
			action_name == "backoff" ||
			action_name == "follow" ||
			action_name == "guard" ||
			action_name == "guardme" ||
			IsPetHealthAction(action_name)
		));

	if (target_arg != "all" && Strings::IsNumber(target_arg)) {
		const auto pet_id = static_cast<uint16>(Strings::ToUnsignedInt(target_arg.c_str()));
		for (auto *pet : roster) {
			if (pet && pet->GetID() == pet_id) {
				SetFocusedPet(client, pet_id);
				targets.push_back(pet);
				break;
			}
		}
	} else if (all_pets) {
		targets = roster;
	} else {
		auto *focused_pet = GetFocusedPet(client);
		if (focused_pet) {
			targets.push_back(focused_pet);
		}
	}

	if (targets.empty()) {
		status = "Select an active Multiclass pet first.";
		return false;
	}

	uint8 applied = 0;
	std::string last_status;
	for (auto *pet : targets) {
		if (ApplyPetAction(client, pet, action, last_status)) {
			++applied;
		}
	}

	if (applied > 1) {
		status = fmt::format("Applied {} to {} pets.", action, applied);
	} else if (applied == 1) {
		status = last_status;
	} else {
		status = last_status.empty() ? "Pet command did not apply." : last_status;
	}

	return applied > 0;
}

bool MulticlassManager::HandleNativeMelodyCommand(Client *client, const Seperator *sep, std::string &status, bool &show_melody_window)
{
	show_melody_window = true;
	if (!client || !sep) {
		status = "Bard Melody is unavailable.";
		return false;
	}

	if (!IsBard(client)) {
		status = "Bard Melody requires Bard in your trio.";
		return false;
	}

	if (!MelodySchemaAvailable()) {
		status = "Bard Melody tables are unavailable.";
		return false;
	}

	const std::string action = sep->arg[2] ? Strings::ToLower(sep->arg[2]) : "";
	if (action.empty() || action == "open" || action == "refresh" || action == "status") {
		status = BuildBardMelodySummary(client);
		return true;
	}

	auto slots = LoadBardMelody(client);
	if (action == "clear") {
		if (sep->argnum >= 3 && Strings::IsNumber(sep->arg[3])) {
			const int slot_id = Strings::ToInt(sep->arg[3]);
			if (!EQ::ValueWithin(slot_id, 1, static_cast<int>(slots.size()))) {
				status = "Choose a Bard Melody slot from 1 to 4.";
				return false;
			}

			slots[slot_id - 1] = 0;
			SaveBardMelody(client, slots);
			status = fmt::format("Cleared Bard Melody slot {}.", slot_id);
			return true;
		}

		slots = {};
		SaveBardMelody(client, slots);
		status = "Cleared Bard Melody.";
		return true;
	}

	if (action == "set") {
		const int slot_id = Strings::ToInt(sep->arg[3]);
		const uint16 spell_id = static_cast<uint16>(Strings::ToUnsignedInt(sep->arg[4]));
		if (!EQ::ValueWithin(slot_id, 1, static_cast<int>(slots.size()))) {
			status = "Choose a Bard Melody slot from 1 to 4.";
			return false;
		}

		std::string reason;
		if (!IsAllowedBardMelodySong(client, spell_id, &reason)) {
			status = reason.empty() ? "That song cannot be used in Bard Melody." : reason;
			return false;
		}

		slots[slot_id - 1] = spell_id;
		SaveBardMelody(client, slots);
		status = fmt::format("Bard Melody slot {} set to {}.", slot_id, spells[spell_id].name);
		return true;
	}

	status = "Bard Melody supports open, refresh, set <slot> <spell_id>, and clear [slot].";
	return false;
}

bool MulticlassManager::HandleNativeDisciplineCommand(Client *client, const Seperator *sep, std::string &status, bool &show_discipline_window)
{
	show_discipline_window = true;
	if (!client || !sep) {
		status = "Multiclass disciplines are unavailable.";
		return false;
	}

	const std::string action = sep->arg[2] ? Strings::ToLower(sep->arg[2]) : "";
	if (action.empty() || action == "open" || action == "refresh" || action == "status" || action == "list" || action == "window") {
		status = BuildDisciplineSummary(client);
		return true;
	}

	const char *lookup_text = nullptr;
	if (action == "use") {
		lookup_text = sep->argplus[3];
	} else {
		lookup_text = sep->argplus[2];
	}

	if (!lookup_text || !lookup_text[0]) {
		status = "Select a learned discipline first.";
		return false;
	}

	std::string lookup = lookup_text;
	Strings::Trim(lookup);
	if (lookup.empty()) {
		status = "Select a learned discipline first.";
		return false;
	}

	uint16 spell_id = 0;
	const auto learned_disciplines = client->GetLearnedDisciplines();
	if (Strings::IsNumber(lookup)) {
		const auto requested_id = static_cast<uint16>(Strings::ToUnsignedInt(lookup));
		for (const auto learned_id : learned_disciplines) {
			if (learned_id == requested_id && IsValidSpell(learned_id) && IsDiscipline(learned_id)) {
				spell_id = requested_id;
				break;
			}
		}
	} else {
		const auto wanted = NormalizeLookupText(lookup);
		if (!wanted.empty()) {
			for (const auto learned_id : learned_disciplines) {
				if (!IsValidSpell(learned_id) || !IsDiscipline(learned_id)) {
					continue;
				}

				if (NormalizeLookupText(spells[learned_id].name) == wanted) {
					spell_id = static_cast<uint16>(learned_id);
					break;
				}
			}

			if (!spell_id) {
				for (const auto learned_id : learned_disciplines) {
					if (!IsValidSpell(learned_id) || !IsDiscipline(learned_id)) {
						continue;
					}

					const auto normalized_name = NormalizeLookupText(spells[learned_id].name);
					if (normalized_name.find(wanted) != std::string::npos) {
						spell_id = static_cast<uint16>(learned_id);
						break;
					}
				}
			}
		}
	}

	if (!IsValidSpell(spell_id) || !IsDiscipline(spell_id)) {
		status = "That discipline is not learned on this character.";
		return false;
	}

	const auto target_id = client->GetTarget() ? client->GetTarget()->GetID() : client->GetID();
	if (!client->UseDiscipline(spell_id, target_id)) {
		status = fmt::format("{} was not ready.", spells[spell_id].name);
		return false;
	}

	status = fmt::format("Used {}.", spells[spell_id].name);
	return true;
}

bool MulticlassManager::MelodySchemaAvailable()
{
	if (!MulticlassEnabled()) {
		return false;
	}

	if (melody_schema_available_) {
		return true;
	}

	auto results = database.QueryDatabase("SHOW TABLES LIKE 'custom_multiclass_bard_melody'");
	melody_schema_available_ = results.Success() && results.RowCount() > 0;
	return melody_schema_available_;
}

MulticlassManager::BardMelodySlots MulticlassManager::LoadBardMelody(Client *client)
{
	BardMelodySlots slots = {};
	if (!client || !client->CharacterID() || !MelodySchemaAvailable()) {
		return slots;
	}

	const auto cached = bard_melody_cache_.find(client->CharacterID());
	if (cached != bard_melody_cache_.end()) {
		return cached->second;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `slot_id`, `spell_id` FROM `custom_multiclass_bard_melody` "
			"WHERE `character_id` = {} AND `slot_id` BETWEEN 1 AND 4",
			client->CharacterID()
		)
	);

	if (results.Success()) {
		for (auto row = results.begin(); row != results.end(); ++row) {
			const int slot_id = Strings::ToInt(row[0]);
			const uint16 spell_id = static_cast<uint16>(Strings::ToUnsignedInt(row[1]));
			if (EQ::ValueWithin(slot_id, 1, static_cast<int>(slots.size()))) {
				slots[slot_id - 1] = spell_id;
			}
		}
	}

	bard_melody_cache_[client->CharacterID()] = slots;
	return slots;
}

bool MulticlassManager::SaveBardMelody(Client *client, const BardMelodySlots &slots)
{
	if (!client || !client->CharacterID() || !MelodySchemaAvailable()) {
		return false;
	}

	auto delete_results = database.QueryDatabase(
		fmt::format(
			"DELETE FROM `custom_multiclass_bard_melody` WHERE `character_id` = {}",
			client->CharacterID()
		)
	);

	if (!delete_results.Success()) {
		return false;
	}

	std::vector<std::string> values;
	for (size_t i = 0; i < slots.size(); ++i) {
		if (!slots[i]) {
			continue;
		}

		values.emplace_back(
			fmt::format(
				"({}, {}, {}, UNIX_TIMESTAMP())",
				client->CharacterID(),
				i + 1,
				slots[i]
			)
		);
	}

	if (!values.empty()) {
		auto insert_results = database.QueryDatabase(
			fmt::format(
				"INSERT INTO `custom_multiclass_bard_melody` (`character_id`, `slot_id`, `spell_id`, `updated_at`) VALUES {}",
				Strings::Join(values, ",")
			)
		);

		if (!insert_results.Success()) {
			return false;
		}
	}

	bard_melody_cache_[client->CharacterID()] = slots;
	return true;
}

bool MulticlassManager::IsAllowedBardMelodySong(Client *client, uint16 spell_id, std::string *reason)
{
	if (!client || !IsBard(client)) {
		if (reason) {
			*reason = "Bard Melody requires Bard in your trio.";
		}
		return false;
	}

	if (!IsValidSpell(spell_id) || !IsBardSong(spell_id)) {
		if (reason) {
			*reason = "Choose a scribed Bard song.";
		}
		return false;
	}

	bool scribed = false;
	for (const auto scribed_spell : client->GetScribedSpells()) {
		if (scribed_spell == spell_id) {
			scribed = true;
			break;
		}
	}

	if (!scribed) {
		if (reason) {
			*reason = "That Bard song is not in your spellbook.";
		}
		return false;
	}

	const auto best_level = GetBestSpellLevel(client, spell_id);
	if (best_level == 255 || client->GetLevel() < best_level) {
		if (reason) {
			*reason = "Your trio cannot use that Bard song yet.";
		}
		return false;
	}

	if (IsBlockedMelodyControlSong(spell_id)) {
		if (reason) {
			*reason = "Charm, fear, mez, stun, root, and lull songs are disabled for Bard Melody until separately validated.";
		}
		return false;
	}

	return true;
}

void MulticlassManager::RefreshAlternateAdvancementTable(Client *client)
{
	if (!client) {
		return;
	}

	client->SendClearPlayerAA();
	client->SendAlternateAdvancementTable();
	client->SendAlternateAdvancementPoints();
	client->SendAlternateAdvancementStats();
}

bool MulticlassManager::HasActiveBardMelody(Client *client)
{
	if (!MulticlassEnabled() || !client || !IsBard(client) || !MelodySchemaAvailable()) {
		return false;
	}

	const auto slots = LoadBardMelody(client);
	for (const auto spell_id : slots) {
		if (spell_id && IsAllowedBardMelodySong(client, spell_id, nullptr)) {
			return true;
		}
	}

	return false;
}

std::string MulticlassManager::BuildBardMelodySummary(Client *client)
{
	if (!client || !IsBard(client)) {
		return "Bard Melody: Bard not in trio.";
	}

	const auto slots = LoadBardMelody(client);
	uint8 active = 0;
	for (const auto spell_id : slots) {
		if (IsValidSpell(spell_id)) {
			active++;
		}
	}

	return fmt::format("Bard Melody: {}/4 song slots selected.", active);
}

std::string MulticlassManager::BuildDisciplineSummary(Client *client)
{
	if (!client) {
		return "Disciplines: unavailable.";
	}

	uint16 learned_count = 0;
	uint16 ready_count = 0;
	for (const auto spell_id : client->GetLearnedDisciplines()) {
		if (!IsValidSpell(spell_id) || !IsDiscipline(spell_id)) {
			continue;
		}

		++learned_count;
		const auto best_level = GetBestSpellLevel(client, static_cast<uint16>(spell_id));
		const auto timer = client->GetDisciplineTimer(spells[spell_id].timer_id);
		if (best_level != 255 && client->GetLevel() >= best_level && timer == 0) {
			++ready_count;
		}
	}

	if (!learned_count) {
		return "Disciplines: none learned.";
	}

	return fmt::format("Disciplines: {}/{} ready.", ready_count, learned_count);
}

void MulticlassManager::SendNativeDisciplines(Client *client, bool show_discipline_window, const std::string &status)
{
	if (!client) {
		return;
	}

	struct DisciplineRow {
		uint16 slot = 0;
		uint16 spell_id = 0;
		uint8 level = 255;
		uint32 timer = 0;
		uint32 timer_total = 0;
		bool ready = false;
		std::string state;
	};

	std::vector<DisciplineRow> rows;
	uint16 usable_count = 0;
	uint16 ready_count = 0;

	for (uint32 slot_id = 0; slot_id < MAX_PP_DISCIPLINES; ++slot_id) {
		const auto spell_id = client->GetPP().disciplines.values[slot_id];
		if (!IsValidSpell(spell_id) || !IsDiscipline(spell_id)) {
			continue;
		}

		DisciplineRow row;
		row.slot = static_cast<uint16>(slot_id + 1);
		row.spell_id = static_cast<uint16>(spell_id);
		row.level = GetBestSpellLevel(client, row.spell_id);
		row.timer = client->GetDisciplineTimer(spells[spell_id].timer_id);
		row.timer_total = spells[spell_id].recast_time > 0 ? spells[spell_id].recast_time / 1000 : 0;
		if (row.timer > 0) {
			row.timer_total = std::max(row.timer_total, row.timer);
		}

		const bool usable = row.level != 255 && client->GetLevel() >= row.level;
		if (usable) {
			++usable_count;
		}

		row.ready = usable && row.timer == 0;
		if (row.ready) {
			++ready_count;
			row.state = "Ready";
		} else if (row.level == 255) {
			row.state = "Trio blocked";
		} else if (client->GetLevel() < row.level) {
			row.state = fmt::format("Level {}", row.level);
		} else if (row.timer > 0) {
			row.state = "Reuse";
		} else {
			row.state = "Blocked";
		}

		rows.emplace_back(row);
	}

	const auto status_text = !status.empty() ? status : BuildDisciplineSummary(client);
	client->Message(Chat::White, "MULTICLASS|disciplines|clear");
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"MULTICLASS|disciplines|summary|status={}|count={}|usable={}|ready={}",
			ProtocolValue(status_text),
			rows.size(),
			usable_count,
			ready_count
		).c_str()
	);

	for (const auto &row : rows) {
		client->Message(
			Chat::White,
			"%s",
			fmt::format(
				"MULTICLASS|discipline|slot={}|id={}|name={}|level={}|timer={}|total={}|ready={}|state={}",
				row.slot,
				row.spell_id,
				ProtocolValue(spells[row.spell_id].name),
				row.level == 255 ? 0 : row.level,
				row.timer,
				row.timer_total,
				row.ready ? 1 : 0,
				ProtocolValue(row.state)
			).c_str()
		);
	}

	client->Message(Chat::White, "%s", fmt::format("MULTICLASS|disciplines|end|count={}", rows.size()).c_str());
	if (show_discipline_window) {
		client->Message(Chat::White, "MULTICLASS|discipline_window|show");
	}
}

void MulticlassManager::SendNativeBardMelody(Client *client, bool show_melody_window)
{
	if (!client) {
		return;
	}

	const bool has_bard = IsBard(client);
	const auto slots = has_bard ? LoadBardMelody(client) : BardMelodySlots{};
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"MULTICLASS|melody|has_bard={}|status={}",
			has_bard ? 1 : 0,
			ProtocolValue(BuildBardMelodySummary(client))
		).c_str()
	);

	for (size_t i = 0; i < slots.size(); ++i) {
		const auto spell_id = slots[i];
		client->Message(
			Chat::White,
			"%s",
			fmt::format(
				"MULTICLASS|melody_slot|slot={}|id={}|name={}|level={}|state={}",
				i + 1,
				spell_id,
				ProtocolValue(IsValidSpell(spell_id) ? spells[spell_id].name : "-"),
				IsValidSpell(spell_id) ? GetBestSpellLevel(client, spell_id) : 0,
				IsValidSpell(spell_id) ? "selected" : "empty"
			).c_str()
		);
	}

	uint16 candidate_count = 0;
	if (has_bard) {
		for (const auto scribed_spell : client->GetScribedSpells()) {
			if (scribed_spell < 0 || scribed_spell > std::numeric_limits<uint16>::max()) {
				continue;
			}

			const auto spell_id = static_cast<uint16>(scribed_spell);
			if (!IsValidSpell(spell_id) || !IsBardSong(spell_id)) {
				continue;
			}

			std::string reason;
			const bool allowed = IsAllowedBardMelodySong(client, spell_id, &reason);
			client->Message(
				Chat::White,
				"%s",
				fmt::format(
					"MULTICLASS|melody_song|id={}|name={}|level={}|allowed={}|reason={}",
					spell_id,
					ProtocolValue(spells[spell_id].name),
					GetBestSpellLevel(client, spell_id),
					allowed ? 1 : 0,
					ProtocolValue(reason)
				).c_str()
			);
			candidate_count++;
		}
	}

	client->Message(Chat::White, "%s", fmt::format("MULTICLASS|melody_songs|end|count={}", candidate_count).c_str());
	if (show_melody_window) {
		client->Message(Chat::White, "MULTICLASS|melody_window|show");
	}
}

void MulticlassManager::ProcessBardMelody(Client *client)
{
	if (!MulticlassEnabled() || !client || client->IsDead() || !IsBard(client) || !MelodySchemaAvailable()) {
		return;
	}

	const auto now = static_cast<uint32>(std::time(nullptr));
	auto &next_pulse = bard_melody_next_pulse_[client->CharacterID()];
	if (next_pulse && now < next_pulse) {
		return;
	}

	next_pulse = now + 6;

	const auto slots = LoadBardMelody(client);
	for (const auto spell_id : slots) {
		if (!spell_id || !IsAllowedBardMelodySong(client, spell_id, nullptr)) {
			continue;
		}

		auto *target = ResolveMelodyTarget(client, spell_id);
		if (!target) {
			continue;
		}

		client->ApplyBardPulse(spell_id, target, EQ::spells::CastingSlot::Item);
	}
}

bool MulticlassManager::SchemaAvailable()
{
	if (!MulticlassEnabled()) {
		return false;
	}

	if (schema_available_) {
		return true;
	}

	auto results = database.QueryDatabase("SHOW TABLES LIKE 'custom_multiclass_profiles'");
	schema_available_ = results.Success() && results.RowCount() > 0;
	return schema_available_;
}

bool MulticlassManager::SaveProfile(const Profile &profile, const std::string &source)
{
	if (!profile.character_id) {
		return false;
	}

	const auto trio_name = Strings::Escape(profile.trio_name);
	const auto resonance_key = Strings::Escape(profile.resonance_key);
	const auto source_value = Strings::Escape(source);

	auto results = database.QueryDatabase(
		fmt::format(
			"INSERT INTO `custom_multiclass_profiles` "
			"(`character_id`, `class_slot_1`, `class_slot_2`, `class_slot_3`, `trio_name`, `resonance_key`, `multiple_pets_enabled`, `locked`, `reweaves_available`, `created_at`, `updated_at`) "
			"VALUES ({}, {}, {}, {}, '{}', '{}', {}, {}, {}, UNIX_TIMESTAMP(), UNIX_TIMESTAMP()) "
			"ON DUPLICATE KEY UPDATE "
			"`class_slot_1` = VALUES(`class_slot_1`), "
			"`class_slot_2` = VALUES(`class_slot_2`), "
			"`class_slot_3` = VALUES(`class_slot_3`), "
			"`trio_name` = VALUES(`trio_name`), "
			"`resonance_key` = VALUES(`resonance_key`), "
			"`multiple_pets_enabled` = VALUES(`multiple_pets_enabled`), "
			"`locked` = VALUES(`locked`), "
			"`reweaves_available` = VALUES(`reweaves_available`), "
			"`updated_at` = UNIX_TIMESTAMP()",
			profile.character_id,
			profile.class_slot_1,
			profile.class_slot_2,
			profile.class_slot_3,
			trio_name,
			resonance_key,
			profile.multiple_pets_enabled ? 1 : 0,
			profile.locked ? 1 : 0,
			profile.reweaves_available
		)
	);

	if (!results.Success()) {
		return false;
	}

	profile_cache_[profile.character_id] = profile;

	auto audit_results = database.QueryDatabase(
		fmt::format(
			"INSERT INTO `custom_multiclass_profile_audit` "
			"(`character_id`, `class_slot`, `class_id`, `action`, `detail`, `created_at`) "
			"VALUES ({}, 0, 0, 'save', '{}', UNIX_TIMESTAMP())",
			profile.character_id,
			source_value
		)
	);
	return audit_results.Success();
}

bool MulticlassManager::AuditProfile(const Profile &profile, const std::string &source)
{
	const auto detail = Strings::Escape(source);
	auto results = database.QueryDatabase(
		fmt::format(
			"INSERT INTO `custom_multiclass_profile_audit` "
			"(`character_id`, `class_slot`, `class_id`, `action`, `detail`, `created_at`) "
			"VALUES "
			"({}, 1, {}, 'set_slot', '{}', UNIX_TIMESTAMP()), "
			"({}, 2, {}, 'set_slot', '{}', UNIX_TIMESTAMP()), "
			"({}, 3, {}, 'set_slot', '{}', UNIX_TIMESTAMP())",
			profile.character_id,
			profile.class_slot_1,
			detail,
			profile.character_id,
			profile.class_slot_2,
			detail,
			profile.character_id,
			profile.class_slot_3,
			detail
		)
	);

	return results.Success();
}

std::string MulticlassManager::ClassName(uint8 class_id, uint8 level)
{
	if (!IsPlayerClass(class_id)) {
		return "Unchosen";
	}

	const char *class_name = GetClassIDName(class_id, level);
	return class_name ? class_name : "Unknown";
}

std::string MulticlassManager::BuildTrioName(const Profile &profile)
{
	if (const auto *exact = FindExactTrioTemplate(profile)) {
		return exact->name;
	}

	const std::array<uint8, 3> slots = {profile.class_slot_1, profile.class_slot_2, profile.class_slot_3};
	uint8 chosen = 0;
	uint8 pet_classes = 0;
	bool has_tank = false;
	bool has_healer = false;

	for (const auto class_id : slots) {
		if (!IsPlayerClass(class_id)) {
			continue;
		}

		chosen++;
		pet_classes += IsPetClass(class_id) ? 1 : 0;
		has_tank = has_tank || IsTankClass(class_id);
		has_healer = has_healer || IsHealerClass(class_id);
	}

	if (!chosen) {
		return "Unchosen Triad";
	}

	if (pet_classes >= 3) {
		return "Grand Menagerie";
	}

	if (pet_classes == 2) {
		return "Twin Menagerie";
	}

	if (has_tank && has_healer && chosen >= 3) {
		return "Warbound Covenant";
	}

	return fmt::format("{} Triad", ClassName(profile.class_slot_1));
}

MulticlassManager::TrioMetadata MulticlassManager::BuildTrioMetadata(const Profile &profile)
{
	TrioMetadata metadata;
	std::vector<std::string> roles;

	uint8 chosen = 0;
	uint8 pet_classes = 0;
	bool has_tank = false;
	bool has_healer = false;
	bool has_int_caster = false;
	bool has_bard = false;
	bool has_striker = false;

	for (const auto class_id : ProfileSlots(profile)) {
		if (!IsPlayerClass(class_id)) {
			continue;
		}

		++chosen;
		AddRoleTags(roles, class_id);
		pet_classes += IsPetClass(class_id) ? 1 : 0;
		has_tank = has_tank || IsTankClass(class_id);
		has_healer = has_healer || IsHealerClass(class_id);
		has_int_caster = has_int_caster || IsIntCasterClass(class_id);
		has_bard = has_bard || IsBardClass(class_id);
		has_striker = has_striker || IsStrikerClass(class_id);
	}

	metadata.roles = roles.empty() ? "Unchosen" : JoinValues(roles, " / ");

	if (const auto *exact = FindExactTrioTemplate(profile)) {
		metadata.resonance = exact->name;
		metadata.summary = exact->summary;
	} else if (pet_classes >= 3) {
		metadata.resonance = "Grand Menagerie";
	} else if (pet_classes == 2) {
		metadata.resonance = "Twin Menagerie";
	} else if (has_tank && pet_classes && has_int_caster) {
		metadata.resonance = "Warbound Arcanum";
	} else if (has_tank && has_healer) {
		metadata.resonance = "Warbound Covenant";
	} else if (has_healer && has_int_caster) {
		metadata.resonance = "Concord of Mind and Spirit";
	} else if (has_bard) {
		metadata.resonance = "Chorus of Three";
	} else if (has_striker && has_int_caster) {
		metadata.resonance = "Spellblade Compact";
	} else if (!profile.trio_name.empty()) {
		metadata.resonance = profile.trio_name;
	} else {
		metadata.resonance = BuildTrioName(profile);
	}

	if (!profile.locked) {
		metadata.summary = "Pick two added classes. The server locks the trio and applies capability routing after confirmation.";
	} else if (!metadata.summary.empty()) {
		// Exact trio templates provide their own summary.
	} else if (pet_classes > 1) {
		metadata.summary = fmt::format(
			"Multi-pet trio. Summon up to {} owned pets in-zone, then use this window for focus and broadcast commands.",
			std::min<uint8>(3, pet_classes)
		);
	} else if (has_tank && pet_classes && has_int_caster) {
		metadata.summary = "Tough base with pet pressure and burst casting. Spellbook presentation is enabled for caster controls.";
	} else if (has_tank && has_healer) {
		metadata.summary = "Durable support trio with defensive skills, healing, and melee staying power.";
	} else if (has_healer && has_int_caster) {
		metadata.summary = "Hybrid caster support trio with healing, control, and offensive spell access.";
	} else if (pet_classes == 1) {
		metadata.summary = "Pet-capable trio. The native pet console controls the active owned pet and keeps stock pet controls as compatibility only.";
	} else if (chosen) {
		metadata.summary = "Fixed trio identity. Class gates route through the Multiclass capability layer.";
	} else {
		metadata.summary = "Choose your second and third classes to define this character's trio identity.";
	}

	metadata.pet_policy = profile.multiple_pets_enabled ? "native-ui" : "single-pet";
	metadata.pet_control = profile.multiple_pets_enabled ?
		"Attack/Back/Follow/Guard affect all pets; Taunt/Hold/Cast/Dismiss affect the focused pet." :
		"Single-pet profile; native pet console still displays pet state.";

	return metadata;
}

std::string MulticlassManager::BuildSkillSummary(Client *client)
{
	if (!client || !SchemaAvailable() || !EnsureProfile(client)) {
		return "Skills: profile unavailable.";
	}

	const auto profile = LoadProfile(client->CharacterID());
	if (!profile.locked) {
		return "Skills: lock your trio to unlock off-class tools.";
	}

	uint16 eligible_count = 0;
	uint16 active_count = 0;
	uint16 ready_unseeded_count = 0;
	for (const auto &skill_entry : EQ::skills::GetSkillTypeMap()) {
		const auto skill_id = skill_entry.first;
		if (!ShouldAutoSeedSkill(skill_id)) {
			continue;
		}

		const auto train_level = GetBestSkillTrainLevel(client, skill_id);
		if (!train_level || client->GetLevel() < train_level || !GetBestSkillCap(client, skill_id, client->GetLevel())) {
			continue;
		}

		++eligible_count;
		if (client->GetRawSkill(skill_id) > 0) {
			++active_count;
		} else {
			++ready_unseeded_count;
		}
	}

	return fmt::format(
		"Skills: {}/{} active{}; tradeskills and specializations stay content-based.",
		active_count,
		eligible_count,
		ready_unseeded_count ? fmt::format(", {} ready to seed", ready_unseeded_count) : ""
	);
}

uint8 MulticlassManager::ParseClassId(const char *value)
{
	if (!value || !value[0]) {
		return Class::None;
	}

	if (Strings::IsNumber(value)) {
		const auto class_id = static_cast<uint8>(Strings::ToUnsignedInt(value));
		return IsPlayerClass(class_id) ? class_id : Class::None;
	}

	const auto wanted = Strings::ToLower(value);
	for (uint8 class_id = Class::Warrior; class_id <= Class::Berserker; ++class_id) {
		auto class_name = Strings::ToLower(GetClassIDName(class_id));
		class_name = Strings::Replace(class_name, " ", "");
		if (
			wanted == Strings::ToLower(GetClassIDName(class_id)) ||
			wanted == class_name ||
			wanted == Strings::ToLower(GetPlayerClassAbbreviation(class_id))
		) {
			return class_id;
		}
	}

	if (wanted == "sk" || wanted == "shadowknight") {
		return Class::ShadowKnight;
	}

	return Class::None;
}

std::string MulticlassManager::ProtocolValue(std::string value)
{
	for (auto &c : value) {
		if (c == '|' || c == '\r' || c == '\n' || c == '\t') {
			c = ' ';
		}
	}

	return value;
}

EQ::skills::SkillType MulticlassManager::NormalizeSkillForClass(const Client *client, uint8 class_id, EQ::skills::SkillType skill_id)
{
	if (
		client &&
		client->ClientVersion() < EQ::versions::ClientVersion::RoF2 &&
		class_id == Class::Berserker &&
		skill_id == EQ::skills::Skill1HPiercing
	) {
		return EQ::skills::Skill2HPiercing;
	}

	return skill_id;
}

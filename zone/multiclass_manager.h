/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#pragma once

#include "common/skills.h"
#include "common/types.h"

#include "glm/vec4.hpp"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

class Client;
class Mob;
class Seperator;
struct StatBonuses;

namespace EQ {
	class ItemInstance;
}

class MulticlassManager {
public:
	using ClassSlots = std::array<uint8, 3>;
	using BardMelodySlots = std::array<uint16, 4>;

	struct Profile {
		uint32 character_id = 0;
		uint8 class_slot_1 = 0;
		uint8 class_slot_2 = 0;
		uint8 class_slot_3 = 0;
		std::string trio_name;
		std::string resonance_key;
		bool multiple_pets_enabled = true;
		bool locked = false;
		uint8 reweaves_available = 0;
	};

	void HandleCommand(Client *client, const Seperator *sep);
	void HandleNativeCommand(Client *client, const Seperator *sep);
	bool EnsureProfile(Client *client);
	Profile LoadProfile(uint32 character_id);
	bool SetProfile(Client *client, uint8 class_slot_1, uint8 class_slot_2, uint8 class_slot_3, const std::string &source);
	bool HasClass(uint32 character_id, uint8 class_id);
	bool HasClass(const Client *client, uint8 class_id);
	bool HasMultiplePetProfile(uint32 character_id);
	ClassSlots GetClassSlots(const Client *client);
	uint32 GetClassMask(const Client *client);
	uint32 GetAAClassMask(const Client *client);
	uint8 GetBestSpellLevel(const Client *client, uint16 spell_id);
	bool CanUseSpell(const Client *client, uint16 spell_id);
	bool CanUseItem(const Client *client, const EQ::ItemInstance *inst);
	std::string BuildItemUseReport(const Client *client, const EQ::ItemInstance *inst, int16 equipment_slot = -1);
	bool CanHaveSkill(const Client *client, EQ::skills::SkillType skill_id);
	uint16 GetBestSkillCap(const Client *client, EQ::skills::SkillType skill_id, uint8 level);
	uint8 GetBestSkillTrainLevel(const Client *client, EQ::skills::SkillType skill_id);
	uint16 SeedEligibleSkills(Client *client, bool notify = true);
	void ApplyTrioBonuses(Client *client, StatBonuses *bonuses);
	std::string BuildTrioBonusSummary(Client *client);
	bool IsIntCaster(const Client *client);
	bool IsWisCaster(const Client *client);
	bool IsBard(const Client *client);
	uint8 GetPrimaryIntCasterClass(const Client *client);
	uint8 GetPrimaryWisCasterClass(const Client *client);
	uint8 GetClientPresentationClass(const Client *client);
	uint8 GetPetRosterLimit(const Client *client);
	std::vector<Mob *> GetPetRoster(const Client *client);
	std::vector<Mob *> GetSecondaryPetRoster(Client *client);
	bool IsPetRosterMember(const Client *client, Mob *pet);
	bool CanCreateAdditionalPet(Client *client);
	bool RegisterPet(Client *client, Mob *pet);
	bool SetFocusedPet(Client *client, uint16 pet_id);
	bool GetPetFollowPosition(const Client *client, const Mob *pet, glm::vec4 &position);
	bool HasActiveBardMelody(Client *client);
	void ProcessBardMelody(Client *client);
	void SendNativeSpellLevelSnapshot(Client *client);
	void SendNativeSpellLevelForSpell(Client *client, uint16 spell_id);

private:
	void SendHelp(Client *client);
	void SendStatus(Client *client);
	void SendDiagnostics(Client *client, const char *topic = nullptr);
	void SendItemCheck(Client *requester, Client *target_client, const char *mode, const char *value, const char *slot_value = nullptr);
	void SendNativeSnapshot(Client *client, const std::string &status = "", bool show_window = true, bool show_pet_window = false);
	bool SendNativeSpellLevelPatchRow(Client *client, uint16 spell_id, uint8 presentation_class);
	void SendNativeSpellLevelPatch(Client *client, uint8 presentation_class);

	struct TrioMetadata {
		std::string roles;
		std::string resonance;
		std::string summary;
		std::string pet_policy;
		std::string pet_control;
	};

	bool SetProfileFromNative(Client *client, uint8 class_slot_2, uint8 class_slot_3, std::string &status);
	bool ReweaveProfileSlot(Client *client, uint8 class_slot, uint8 class_id, const std::string &source, std::string &status);
	bool HandleNativePetCommand(Client *client, const Seperator *sep, std::string &status);
	bool HandleNativeMelodyCommand(Client *client, const Seperator *sep, std::string &status, bool &show_melody_window);
	bool HandleNativeDisciplineCommand(Client *client, const Seperator *sep, std::string &status, bool &show_discipline_window);
	bool ApplyStockPetCommand(Client *client, Mob *pet, uint32 command, Mob *target, std::string &status);
	bool ApplyPetAction(Client *client, Mob *pet, const std::string &action, std::string &status);
	Mob *GetFocusedPet(Client *client);
	Mob *GetPetForClass(Client *client, uint8 class_id, const std::vector<Mob *> &roster, std::string &status);
	bool SchemaAvailable();
	bool MelodySchemaAvailable();
	BardMelodySlots LoadBardMelody(Client *client);
	bool SaveBardMelody(Client *client, const BardMelodySlots &slots);
	void SendNativeBardMelody(Client *client, bool show_melody_window = false);
	void SendNativeDisciplines(Client *client, bool show_discipline_window = false, const std::string &status = "");
	void RefreshAlternateAdvancementTable(Client *client);
	bool IsAllowedBardMelodySong(Client *client, uint16 spell_id, std::string *reason = nullptr);
	std::string BuildBardMelodySummary(Client *client);
	std::string BuildDisciplineSummary(Client *client);
	bool SaveProfile(const Profile &profile, const std::string &source);
	bool AuditProfile(const Profile &profile, const std::string &source);
	std::string ClassName(uint8 class_id, uint8 level = 0);
	std::string BuildTrioName(const Profile &profile);
	TrioMetadata BuildTrioMetadata(const Profile &profile);
	std::string BuildSkillSummary(Client *client);
	uint8 ParseClassId(const char *value);
	std::string ProtocolValue(std::string value);
	EQ::skills::SkillType NormalizeSkillForClass(const Client *client, uint8 class_id, EQ::skills::SkillType skill_id);

	bool schema_available_ = false;
	bool melody_schema_available_ = false;
	std::unordered_map<uint32, Profile> profile_cache_;
	std::unordered_map<uint32, uint16> focused_pet_ids_;
	std::unordered_map<uint32, BardMelodySlots> bard_melody_cache_;
	std::unordered_map<uint32, uint32> bard_melody_next_pulse_;
};

extern MulticlassManager multiclass_manager;

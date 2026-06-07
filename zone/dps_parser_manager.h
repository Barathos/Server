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

#include <map>
#include <set>
#include <string>

class Client;
class Mob;
class Seperator;

class DpsParserManager {
public:
	void HandleCommand(Client *client, const Seperator *sep);
	void RecordDamage(Mob *target, Mob *attacker, int64 damage, uint16 spell_id, EQ::skills::SkillType skill_used);
	void RecordHeal(Mob *target, Mob *caster, uint64 amount, uint16 spell_id);
	void SendSnapshot(Client *client, bool show_window);
	void Reset(Client *client);
	void SetLive(Client *client, bool enabled);
	bool IsLive(Client *client) const;

private:
	struct ActorStats {
		uint32 actor_key = 0;
		uint32 owner_character_id = 0;
		std::string actor_name;
		std::string source_name;
		uint64 damage = 0;
		uint64 healing = 0;
		uint64 incoming = 0;
	};

	struct Encounter {
		uint32 id = 0;
		uint16 target_entity_id = 0;
		std::string target_name;
		uint32 start_ms = 0;
		uint32 last_ms = 0;
		bool ended = false;
		std::map<uint32, ActorStats> actors;
		std::set<uint32> participant_character_ids;
	};

	uint32 Now() const;
	uint32 Elapsed(uint32 start, uint32 end) const;
	uint32 EncounterKey(Mob *target, Mob *attacker) const;
	Encounter &GetOrCreateEncounter(Mob *target, Mob *attacker);
	Encounter *FindRelevantEncounter(Mob *target, Mob *source);
	void PruneExpired();
	void MaybeSendLiveUpdates(const Encounter &encounter);
	bool IsRelevant(Client *client, const Encounter &encounter) const;
	Client *OwnerClient(Mob *mob) const;
	ActorStats MakeActorStats(Mob *mob) const;
	void SendEncounter(Client *client, const Encounter *encounter, bool show_window);
	void SendClear(Client *client, const std::string &status, bool show_window);
	std::string Escape(std::string value) const;

	std::map<uint32, Encounter> m_encounters;
	std::map<uint16, uint32> m_active_by_target;
	std::set<uint32> m_live_character_ids;
	uint32 m_next_encounter_id = 1;
	uint32 m_last_live_snapshot_ms = 0;
};

extern DpsParserManager dps_parser_manager;

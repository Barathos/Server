/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "dps_parser_manager.h"

#include "bot.h"
#include "client.h"
#include "entity.h"
#include "merc.h"
#include "mob.h"
#include "npc.h"

#include "common/rulesys.h"
#include "common/seperator.h"
#include "common/strings.h"
#include "common/timer.h"

#include "fmt/format.h"

#include <algorithm>
#include <list>
#include <vector>

extern EntityList entity_list;

DpsParserManager dps_parser_manager;

void DpsParserManager::HandleCommand(Client *client, const Seperator *sep)
{
	if (!client) {
		return;
	}

	if (!RuleB(CustomFeatures, DpsParserEnabled)) {
		client->Message(Chat::White, "DPS parser is disabled on this server.");
		return;
	}

	const std::string action = sep && sep->arg[1] ? Strings::ToLower(sep->arg[1]) : "";
	if (action == "help") {
		client->Message(Chat::White, "DPS parser commands:");
		client->Message(Chat::White, "#dps - Open or refresh the native DPS parser.");
		client->Message(Chat::White, "#dps summary - Print your current encounter summary.");
		client->Message(Chat::White, "#dps reset - Clear active DPS parser encounters in this zone.");
		client->Message(Chat::White, "#dps live on|off - Toggle automatic native DPS parser updates.");
		return;
	}

	if (action == "reset") {
		Reset(client);
		return;
	}

	if (action == "live") {
		const std::string value = sep && sep->arg[2] ? Strings::ToLower(sep->arg[2]) : "";
		const bool enabled = value != "off" && value != "0" && value != "false";
		SetLive(client, enabled);
		client->Message(Chat::White, "DPS parser live updates are %s.", enabled ? "on" : "off");
		SendSnapshot(client, true);
		return;
	}

	if (action == "status") {
		client->Message(Chat::White, "DPS parser is enabled. Live updates are %s.", IsLive(client) ? "on" : "off");
		SendSnapshot(client, true);
		return;
	}

	SendSnapshot(client, action != "summary");
}

void DpsParserManager::RecordDamage(Mob *target, Mob *attacker, int64 damage, uint16 spell_id, EQ::skills::SkillType skill_used)
{
	if (!RuleB(CustomFeatures, DpsParserEnabled) || !target || damage <= 0) {
		return;
	}

	PruneExpired();

	auto &encounter = GetOrCreateEncounter(target, attacker);
	encounter.last_ms = Now();

	const auto attacker_stats = MakeActorStats(attacker);
	const auto target_owner = OwnerClient(target);
	const auto attacker_owner = OwnerClient(attacker);

	if (attacker_stats.actor_key > 0 && attacker_owner) {
		auto &stats = encounter.actors[attacker_stats.actor_key];
		if (stats.actor_key == 0) {
			stats = attacker_stats;
		}
		stats.damage += static_cast<uint64>(damage);
		encounter.participant_character_ids.insert(attacker_owner->CharacterID());
	}

	if (target_owner) {
		encounter.participant_character_ids.insert(target_owner->CharacterID());

		if (attacker && attacker->IsNPC() && !attacker->IsPet()) {
			auto incoming_stats = MakeActorStats(attacker);
			if (incoming_stats.actor_key > 0) {
				auto &stats = encounter.actors[incoming_stats.actor_key];
				if (stats.actor_key == 0) {
					stats = incoming_stats;
				}
				stats.incoming += static_cast<uint64>(damage);
			}
		}
	}

	MaybeSendLiveUpdates(encounter);
}

void DpsParserManager::RecordHeal(Mob *target, Mob *caster, uint64 amount, uint16 spell_id)
{
	if (!RuleB(CustomFeatures, DpsParserEnabled) || !target || !caster || amount == 0) {
		return;
	}

	PruneExpired();

	auto *encounter = FindRelevantEncounter(target, caster);
	if (!encounter) {
		encounter = &GetOrCreateEncounter(target, caster);
	}

	encounter->last_ms = Now();

	const auto caster_stats = MakeActorStats(caster);
	const auto caster_owner = OwnerClient(caster);
	const auto target_owner = OwnerClient(target);
	if (caster_stats.actor_key > 0 && caster_owner) {
		auto &stats = encounter->actors[caster_stats.actor_key];
		if (stats.actor_key == 0) {
			stats = caster_stats;
		}
		stats.healing += amount;
		encounter->participant_character_ids.insert(caster_owner->CharacterID());
	}

	if (target_owner) {
		encounter->participant_character_ids.insert(target_owner->CharacterID());
	}

	MaybeSendLiveUpdates(*encounter);
}

void DpsParserManager::SendSnapshot(Client *client, bool show_window)
{
	if (!client) {
		return;
	}

	if (!RuleB(CustomFeatures, DpsParserEnabled)) {
		SendClear(client, "DPS parser is disabled on this server.", show_window);
		return;
	}

	PruneExpired();

	const Encounter *best = nullptr;
	for (const auto &entry : m_encounters) {
		const auto &encounter = entry.second;
		if (!IsRelevant(client, encounter)) {
			continue;
		}

		if (!best || encounter.last_ms > best->last_ms) {
			best = &encounter;
		}
	}

	SendEncounter(client, best, show_window);
}

void DpsParserManager::Reset(Client *client)
{
	m_encounters.clear();
	m_active_by_target.clear();
	m_last_live_snapshot_ms = 0;

	if (client) {
		SendClear(client, "DPS parser reset.", true);
		client->Message(Chat::White, "DPS parser reset.");
	}
}

void DpsParserManager::SetLive(Client *client, bool enabled)
{
	if (!client || !client->CharacterID()) {
		return;
	}

	if (enabled) {
		m_live_character_ids.insert(client->CharacterID());
	} else {
		m_live_character_ids.erase(client->CharacterID());
	}
}

bool DpsParserManager::IsLive(Client *client) const
{
	return client && m_live_character_ids.count(client->CharacterID()) != 0;
}

uint32 DpsParserManager::Now() const
{
	return Timer::GetCurrentTime();
}

uint32 DpsParserManager::Elapsed(uint32 start, uint32 end) const
{
	return end >= start ? end - start : 0;
}

uint32 DpsParserManager::EncounterKey(Mob *target, Mob *attacker) const
{
	if (target && target->IsNPC() && !target->IsPet()) {
		return target->GetID();
	}

	if (attacker && attacker->IsNPC() && !attacker->IsPet()) {
		return attacker->GetID();
	}

	return target ? target->GetID() : 0;
}

DpsParserManager::Encounter &DpsParserManager::GetOrCreateEncounter(Mob *target, Mob *attacker)
{
	const auto now = Now();
	const auto target_key = EncounterKey(target, attacker);
	auto active_iter = m_active_by_target.find(static_cast<uint16>(target_key));
	if (active_iter != m_active_by_target.end()) {
		auto encounter_iter = m_encounters.find(active_iter->second);
		if (encounter_iter != m_encounters.end()) {
			auto &encounter = encounter_iter->second;
			if (!encounter.ended && Elapsed(encounter.last_ms, now) <= static_cast<uint32>(RuleI(CustomFeatures, DpsParserEncounterTimeoutMS))) {
				return encounter;
			}
			encounter.ended = true;
		}
	}

	Encounter encounter;
	encounter.id = m_next_encounter_id++;
	encounter.target_entity_id = static_cast<uint16>(target_key);
	encounter.target_name = target ? target->GetCleanName() : "Encounter";
	encounter.start_ms = now;
	encounter.last_ms = now;

	auto inserted = m_encounters.emplace(encounter.id, encounter);
	m_active_by_target[static_cast<uint16>(target_key)] = encounter.id;
	return inserted.first->second;
}

DpsParserManager::Encounter *DpsParserManager::FindRelevantEncounter(Mob *target, Mob *source)
{
	const auto target_owner = OwnerClient(target);
	const auto source_owner = OwnerClient(source);

	Encounter *best = nullptr;
	for (auto &entry : m_encounters) {
		auto &encounter = entry.second;
		if (encounter.ended) {
			continue;
		}

		if (
			(target && encounter.target_entity_id == target->GetID()) ||
			(source && encounter.target_entity_id == source->GetID()) ||
			(target_owner && encounter.participant_character_ids.count(target_owner->CharacterID()) != 0) ||
			(source_owner && encounter.participant_character_ids.count(source_owner->CharacterID()) != 0)
		) {
			if (!best || encounter.last_ms > best->last_ms) {
				best = &encounter;
			}
		}
	}

	return best;
}

void DpsParserManager::PruneExpired()
{
	const auto now = Now();
	const auto timeout = static_cast<uint32>(RuleI(CustomFeatures, DpsParserEncounterTimeoutMS));
	std::vector<uint32> erase_ids;

	for (auto &entry : m_encounters) {
		auto &encounter = entry.second;
		if (!encounter.ended && Elapsed(encounter.last_ms, now) > timeout) {
			encounter.ended = true;
		}

		if (encounter.ended && Elapsed(encounter.last_ms, now) > 60000) {
			erase_ids.push_back(entry.first);
		}
	}

	for (const auto encounter_id : erase_ids) {
		for (auto iter = m_active_by_target.begin(); iter != m_active_by_target.end();) {
			if (iter->second == encounter_id) {
				iter = m_active_by_target.erase(iter);
			} else {
				++iter;
			}
		}

		m_encounters.erase(encounter_id);
	}
}

void DpsParserManager::MaybeSendLiveUpdates(const Encounter &encounter)
{
	const auto now = Now();
	const auto interval = static_cast<uint32>(std::max(250, RuleI(CustomFeatures, DpsParserSnapshotIntervalMS)));
	if (Elapsed(m_last_live_snapshot_ms, now) < interval) {
		return;
	}

	m_last_live_snapshot_ms = now;

	std::list<Client *> clients;
	entity_list.GetClientList(clients);
	for (auto *client : clients) {
		if (!client || !client->Connected() || !IsLive(client) || !IsRelevant(client, encounter)) {
			continue;
		}

		SendEncounter(client, &encounter, false);
	}
}

bool DpsParserManager::IsRelevant(Client *client, const Encounter &encounter) const
{
	if (!client || !client->CharacterID()) {
		return false;
	}

	if (encounter.participant_character_ids.count(client->CharacterID()) != 0) {
		return true;
	}

	for (const auto character_id : encounter.participant_character_ids) {
		auto *participant = entity_list.GetClientByCharID(character_id);
		if (participant && entity_list.IsInSameGroupOrRaidGroup(client, participant)) {
			return true;
		}
	}

	return client->GetTarget() && client->GetTarget()->GetID() == encounter.target_entity_id;
}

Client *DpsParserManager::OwnerClient(Mob *mob) const
{
	if (!mob) {
		return nullptr;
	}

	Mob *owner = mob->GetUltimateOwner();
	if (!owner) {
		owner = mob->GetOwnerOrSelf();
	}

	if (owner && owner->IsClient()) {
		return owner->CastToClient();
	}

	if (mob->IsClient()) {
		return mob->CastToClient();
	}

	return nullptr;
}

DpsParserManager::ActorStats DpsParserManager::MakeActorStats(Mob *mob) const
{
	ActorStats stats;
	if (!mob) {
		return stats;
	}

	auto *owner = OwnerClient(mob);
	if (owner) {
		stats.actor_key = owner->CharacterID();
		stats.owner_character_id = owner->CharacterID();
		stats.actor_name = owner->GetCleanName();
		stats.source_name = mob == owner ? owner->GetCleanName() : mob->GetCleanName();
		return stats;
	}

	stats.actor_key = 1000000 + mob->GetID();
	stats.actor_name = mob->GetCleanName();
	stats.source_name = mob->GetCleanName();
	return stats;
}

void DpsParserManager::SendEncounter(Client *client, const Encounter *encounter, bool show_window)
{
	if (!client) {
		return;
	}

	if (!encounter) {
		SendClear(client, "No active DPS parser encounter.", show_window);
		return;
	}

	std::vector<ActorStats> rows;
	uint64 total_damage = 0;
	uint64 total_healing = 0;
	uint64 total_incoming = 0;

	for (const auto &entry : encounter->actors) {
		rows.push_back(entry.second);
		total_damage += entry.second.damage;
		total_healing += entry.second.healing;
		total_incoming += entry.second.incoming;
	}

	std::sort(rows.begin(), rows.end(), [](const ActorStats &left, const ActorStats &right) {
		const auto left_total = left.damage + left.healing + left.incoming;
		const auto right_total = right.damage + right.healing + right.incoming;
		if (left_total != right_total) {
			return left_total > right_total;
		}

		return Strings::ToLower(left.actor_name) < Strings::ToLower(right.actor_name);
	});

	const auto elapsed_ms = std::max<uint32>(Elapsed(encounter->start_ms, encounter->last_ms), 1000);
	const auto elapsed_seconds = static_cast<double>(elapsed_ms) / 1000.0;

	client->Message(Chat::White, "DPS|window|clear");
	client->Message(
		Chat::White,
		"%s",
		fmt::format(
			"DPS|summary|id={}|target_id={}|target={}|elapsed={}|damage={}|healing={}|incoming={}|ended={}|status={}",
			encounter->id,
			encounter->target_entity_id,
			Escape(encounter->target_name),
			elapsed_ms,
			total_damage,
			total_healing,
			total_incoming,
			encounter->ended ? 1 : 0,
			Escape(encounter->ended ? "Encounter ended." : "Encounter active.")
		).c_str()
	);

	for (const auto &row : rows) {
		const auto dps = static_cast<uint64>(static_cast<double>(row.damage) / elapsed_seconds);
		const auto hps = static_cast<uint64>(static_cast<double>(row.healing) / elapsed_seconds);
		const auto damage_pct = total_damage > 0 ? static_cast<uint32>((row.damage * 100) / total_damage) : 0;
		client->Message(
			Chat::White,
			"%s",
			fmt::format(
				"DPS|row|actor_id={}|owner_id={}|actor={}|source={}|damage={}|healing={}|incoming={}|dps={}|hps={}|pct={}",
				row.actor_key,
				row.owner_character_id,
				Escape(row.actor_name),
				Escape(row.source_name),
				row.damage,
				row.healing,
				row.incoming,
				dps,
				hps,
				damage_pct
			).c_str()
		);
	}

	if (show_window) {
		client->Message(Chat::White, "DPS|window|show");
	}
}

void DpsParserManager::SendClear(Client *client, const std::string &status, bool show_window)
{
	if (!client) {
		return;
	}

	client->Message(Chat::White, "DPS|window|clear");
	client->Message(
		Chat::White,
		"%s",
		fmt::format("DPS|summary|id=0|target_id=0|target=|elapsed=0|damage=0|healing=0|incoming=0|ended=1|status={}", Escape(status)).c_str()
	);

	if (show_window) {
		client->Message(Chat::White, "DPS|window|show");
	}
}

std::string DpsParserManager::Escape(std::string value) const
{
	for (auto &ch : value) {
		if (ch == '|' || ch == '\r' || ch == '\n') {
			ch = ' ';
		}
	}

	return value;
}

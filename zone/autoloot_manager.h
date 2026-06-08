/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#pragma once

#include "common/advanced_loot.h"
#include "common/types.h"

#include <ctime>
#include <map>
#include <set>
#include <string>
#include <vector>

class Client;
class Corpse;
class Group;
class Mob;
class Seperator;

class AutoLootManager {
public:
	using VoteChoice = EQ::AdvancedLoot::VoteChoice;
	using LootFilterDecision = EQ::AdvancedLoot::LootFilterDecision;

	void Process();
	void ProcessCorpseDeath(Corpse *corpse, Mob *killer);
	void ShowWindow(Client *client);

	void HandleAdvancedLootCommand(Client *client, const Seperator *sep);
	bool IsManualLootLocked(Corpse *corpse, uint16 loot_slot, std::string *reason = nullptr) const;

private:
	struct CharacterSettings {
		bool use_advanced_looting = true;
		bool apply_filters = true;
		bool auto_split_coin = true;
		bool confirm_remove_filter = true;
		bool auto_remove_looted_lore = true;
		bool auto_show_loot_window = true;
		bool show_new_items_only = false;
		bool auto_loot_all = false;
		bool master_looter_candidate = true;
		bool debug_enabled = false;
		bool log_enabled = false;
	};

	struct FilterEntry {
		uint32 item_id = 0;
		LootFilterDecision decision = LootFilterDecision::Unset;
		bool auto_ask_roll = false;
	};

	struct LootEntry {
		uint32 entry_id = 0;
		uint16 corpse_id = 0;
		uint16 loot_slot = 0;
		uint32 item_id = 0;
		uint32 icon_id = 0;
		uint32 quantity = 0;
		uint32 owner_character_id = 0;
		uint32 master_looter_character_id = 0;
		uint32 group_id = 0;
		bool shared = false;
		bool no_drop = false;
		bool dynamic_instance = false;
		bool auto_roll = false;
		bool free_grab = false;
		bool assigned_from_shared = false;
		std::string item_name;
		std::string corpse_name;
		std::string state = "waiting";
		std::string rule = "-";
		std::map<uint32, VoteChoice> votes;
		time_t created_at = 0;
		time_t vote_started_at = 0;
	};

	struct LootSource {
		Client *client = nullptr;
		std::string killer_name;
		std::string owner_name;
		std::string source_type;
	};

	CharacterSettings GetCharacterSettings(uint32 character_id, bool create_enabled = false);
	void SaveCharacterSettings(uint32 character_id, const CharacterSettings &settings);
	void DebugMessage(Client *client, const CharacterSettings &settings, const std::string &message);

	FilterEntry GetFilter(uint32 character_id, uint32 item_id);
	void SetFilter(uint32 character_id, uint32 item_id, LootFilterDecision decision, bool auto_ask_roll);
	void RemoveFilter(uint32 character_id, uint32 item_id);
	std::vector<FilterEntry> GetFilters(uint32 character_id);

	LootSource ResolveLootSource(Mob *killer);
	Client *ResolveLootClient(Mob *killer);
	Client *FindAutoLootClient(Client *resolved_client, Corpse *corpse);
	Client *DetermineMasterLooter(Group *group, Corpse *corpse, Client *fallback);
	std::vector<Client *> GetGroupClients(Group *group);
	std::vector<Client *> GetGroupClientsByID(uint32 group_id);

	bool ProcessCorpse(Corpse *corpse, Client *resolved_client, const LootSource *source = nullptr);
	bool QueueCorpseEntries(Corpse *corpse, Client *resolved_client, const LootSource *source = nullptr);
	bool HasQueuedEntry(uint16 corpse_id, uint16 loot_slot) const;
	bool IsEntryVisibleToClient(const LootEntry &entry, Client *client) const;
	void PruneLootEntries();
	void RefreshQueuedRulesForClient(Client *client);
	void SendNativeSnapshot(Client *client);
	void SendNativeUpdate(Client *client);
	void SendNativeFilterUpdate(Client *client);
	void HandleLootAction(Client *client, const Seperator *sep);
	void HandleAdvancedLootFilterCommand(Client *client, const Seperator *sep);
	void HandleSharedLootAction(Client *client, uint32 entry_id, const std::string &action, const Seperator *sep);
	void HandlePersonalLootCommand(Client *client, const Seperator *sep);
	void InspectEntryForClient(Client *client, uint32 entry_id);
	void SendManageInfo(Client *client, uint32 entry_id);
	void RecordSharedVote(Client *client, uint32 entry_id, VoteChoice choice, bool set_always_rule);
	void ResolveSharedVote(uint32 entry_id, bool timeout);
	std::vector<Client *> GetEligibleSharedLootClients(const LootEntry &entry, Corpse *corpse);
	void SendSharedLootUpdate(const std::vector<Client *> &clients);
	void LootEntryForClient(Client *client, uint32 entry_id);
	bool LeaveEntryForClient(Client *client, uint32 entry_id, bool add_never_filter);
	void FinalizeCorpse(Corpse *corpse, Client *coin_client);
	void LootCoin(Corpse *corpse, Client *client);
	void MaybeAutoShowLootWindow(Client *client, bool has_new_items);

	void SendStatus(Client *client);
	void SendNativeStatus(Client *client);
	void SendNativeFilters(Client *client);
	void SendHelp(Client *client);
	void SendGroupHelp(Client *client);
	void Audit(uint32 character_id, const std::string &action, uint32 item_id = 0, uint32 quantity = 0, const std::string &detail = "");

	std::map<uint32, LootEntry> m_loot_entries;
	std::map<uint32, uint32> m_group_master_looters;
	std::map<uint32, std::map<uint32, time_t>> m_pending_filter_removals;
	uint32 m_next_loot_entry_id = 1;
	time_t m_last_process = 0;
};

extern AutoLootManager auto_loot_manager;

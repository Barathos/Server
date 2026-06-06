/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#pragma once

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
	enum class VoteChoice {
		Unset,
		Need,
		Greed,
		Pass
	};

	void Process();
	void ProcessCorpseDeath(Corpse *corpse, Mob *killer);
	void ProcessNearby(Client *client, float radius);
	void ShowWindow(Client *client);

	void HandleAutolootCommand(Client *client, const Seperator *sep);
	void HandleLootFilterCommand(Client *client, const Seperator *sep);
	void HandleAutosellCommand(Client *client, const Seperator *sep);
	void HandleNeedGreedCommand(Client *client, const Seperator *sep);

private:
	struct CharacterSettings {
		bool enabled = false;
		std::string filter_mode = "both";
		bool debug_enabled = false;
		bool log_enabled = false;
	};

	struct GroupSettings {
		std::string loot_mode = "solo";
		uint32 assigned_character_id = 0;
		uint32 round_robin_index = 0;
		bool need_greed_enabled = false;
	};

	struct PendingVote {
		uint32 vote_id = 0;
		uint32 group_id = 0;
		uint16 corpse_id = 0;
		uint16 loot_slot = 0;
		uint32 item_id = 0;
		std::string item_name;
		std::map<uint32, VoteChoice> votes;
		time_t expires_at = 0;
	};

	struct AutosellEntry {
		int16 slot_id = 0;
		uint32 item_id = 0;
		uint32 quantity = 0;
		uint64 value = 0;
		std::string item_name;
	};

	struct AutosellSession {
		uint32 session_id = 0;
		time_t expires_at = 0;
		std::vector<AutosellEntry> entries;
		uint64 total_value = 0;
	};

	struct LootEntry {
		uint32 entry_id = 0;
		uint16 corpse_id = 0;
		uint16 loot_slot = 0;
		uint32 item_id = 0;
		uint32 icon_id = 0;
		uint32 quantity = 0;
		uint32 owner_character_id = 0;
		uint32 group_id = 0;
		bool shared = false;
		bool no_drop = false;
		std::string item_name;
		std::string corpse_name;
		std::string state = "waiting";
		std::string rule = "-";
		time_t created_at = 0;
	};

	CharacterSettings GetCharacterSettings(uint32 character_id, bool create_enabled = false);
	void SaveCharacterSettings(uint32 character_id, const CharacterSettings &settings);
	GroupSettings GetGroupSettings(uint32 group_id);
	void SaveGroupSettings(uint32 group_id, const GroupSettings &settings);

	bool ShouldLootItem(uint32 character_id, uint32 item_id, const std::string &filter_mode);
	std::string GetFilterAction(uint32 character_id, uint32 item_id, const std::string &filter_mode);
	bool HasFilter(uint32 character_id, uint32 item_id, const std::string &filter_mode);
	void SetFilter(uint32 character_id, uint32 item_id, const std::string &filter_mode);
	void RemoveFilter(uint32 character_id, uint32 item_id, const std::string &filter_mode);
	std::vector<std::pair<uint32, std::string>> GetFilters(uint32 character_id, const std::string &filter_mode);

	Client *ResolveLootClient(Mob *killer);
	Client *FindAutoLootClient(Client *resolved_client, Corpse *corpse);
	Client *DetermineRecipient(Client *resolved_client, Corpse *corpse, const GroupSettings &settings);
	std::vector<Client *> GetGroupClients(Group *group);

	bool ProcessCorpse(Corpse *corpse, Client *resolved_client, bool nearby);
	bool QueueCorpseEntries(Corpse *corpse, Client *resolved_client, bool nearby);
	bool HasQueuedEntry(uint16 corpse_id, uint16 loot_slot) const;
	bool IsEntryVisibleToClient(const LootEntry &entry, Client *client) const;
	void PruneLootEntries();
	void RefreshQueuedRulesForClient(Client *client);
	void SendNativeSnapshot(Client *client);
	void SendNativeUpdate(Client *client);
	void SendNativeFilterUpdate(Client *client);
	void HandleLootAction(Client *client, const Seperator *sep);
	void HandlePersonalLootCommand(Client *client, const Seperator *sep);
	void InspectEntryForClient(Client *client, uint32 entry_id);
	void LootEntryForClient(Client *client, uint32 entry_id);
	bool LeaveEntryForClient(Client *client, uint32 entry_id, bool add_never_filter);
	void FinalizeCorpse(Corpse *corpse, Client *coin_client);
	void LootCoin(Corpse *corpse, Client *client);

	bool IsNoDrop(uint32 item_id);
	bool HasPendingVotes(uint16 corpse_id) const;
	void StartNeedGreedVote(Group *group, Corpse *corpse, uint16 loot_slot, uint32 item_id);
	void CastNeedGreedVote(Client *client, uint32 vote_id, VoteChoice choice);
	void ProcessVote(uint32 vote_id, bool timeout);
	void ForceProcessVotes(Client *client);
	void RecoverVotes(Client *client);

	bool IsAutosellExcluded(uint32 character_id, uint32 item_id);
	void SetAutosellExcluded(uint32 character_id, uint32 item_id, bool excluded);
	std::vector<uint32> GetAutosellExclusions(uint32 character_id);
	std::vector<AutosellEntry> BuildAutosellPreview(Client *client, uint64 &total_value);
	void PreviewAutosell(Client *client);
	void ConfirmAutosell(Client *client);
	void CancelAutosell(Client *client);

	void SendStatus(Client *client);
	void SendNativeStatus(Client *client);
	void SendNativeFilters(Client *client, const std::string &filter_mode);
	void SendHelp(Client *client);
	void SendGroupHelp(Client *client);
	void Audit(uint32 character_id, const std::string &action, uint32 item_id = 0, uint32 quantity = 0, const std::string &detail = "");

	std::map<uint32, PendingVote> m_pending_votes;
	std::map<uint32, AutosellSession> m_autosell_sessions;
	std::map<uint32, LootEntry> m_loot_entries;
	uint32 m_next_vote_id = 1;
	uint32 m_next_autosell_session_id = 1;
	uint32 m_next_loot_entry_id = 1;
	time_t m_last_process = 0;
};

extern AutoLootManager auto_loot_manager;

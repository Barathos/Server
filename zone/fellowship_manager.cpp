/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "fellowship_manager.h"

#include "client.h"
#include "entity.h"
#include "position.h"
#include "string_ids.h"
#include "zone.h"
#include "zonedb.h"

#include "common/eq_packet.h"
#include "common/eqemu_logsys.h"
#include "common/opcodemgr.h"
#include "common/rulesys.h"
#include "common/seperator.h"
#include "common/strings.h"
#include "common/timer.h"

#include "fmt/format.h"

#include <algorithm>
#include <ctime>
#include <map>
#include <optional>
#include <string>
#include <vector>

extern EntityList entity_list;
extern Zone *zone;
extern ZoneDatabase database;

FellowshipManager fellowship_manager;

namespace {

constexpr const char *kFellowshipTable = "custom_fellowships";
constexpr const char *kMemberTable = "custom_fellowship_members";
constexpr const char *kCampfireTable = "custom_fellowship_campfires";
constexpr uint32 kLeaderRank = 1;
constexpr uint32 kMemberRank = 0;
constexpr uint32 kInviteExpirationSeconds = 300;

struct Membership {
	uint32 fellowship_id = 0;
	uint32 leader_character_id = 0;
	std::string fellowship_name;
	std::string motd;
	uint32 rank = 0;
	bool sharing_enabled = false;
	uint32 member_count = 0;
};

struct Campfire {
	uint32 fellowship_id = 0;
	uint32 zone_id = 0;
	uint32 instance_id = 0;
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float heading = 0.0f;
	std::string campfire_type;
	uint32 created_by_character_id = 0;
	uint32 created_at = 0;
	uint32 expires_at = 0;
};

struct PendingInvite {
	uint32 fellowship_id = 0;
	uint32 leader_character_id = 0;
	uint32 created_at = 0;
};

std::map<uint32, PendingInvite> pending_invites;

uint32 ToUInt(const char *value)
{
	return value ? Strings::ToUnsignedInt(value) : 0;
}

uint64 ToUInt64(const char *value)
{
	return value ? Strings::ToUnsignedBigInt(value) : 0;
}

float ToFloat(const char *value)
{
	return value ? Strings::ToFloat(value) : 0.0f;
}

std::string SqlString(std::string value)
{
	return fmt::format("'{}'", Strings::Escape(value));
}

std::string CleanFellowshipName(std::string value)
{
	Strings::Trim(value);
	if (value.size() > 64) {
		value.resize(64);
	}

	return value;
}

std::string CleanMotd(std::string value)
{
	Strings::Trim(value);
	if (value.size() > 1024) {
		value.resize(1024);
	}

	return value;
}

bool FeatureEnabled(Client *client)
{
	if (!RuleB(CustomFeatures, FellowshipsEnabled)) {
		if (client) {
			client->Message(Chat::White, "Fellowships are disabled on this server.");
		}

		return false;
	}

	return true;
}

std::optional<Membership> LoadMembership(uint32 character_id)
{
	if (!character_id) {
		return std::nullopt;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT f.`id`, f.`leader_character_id`, f.`name`, f.`motd`, "
			"m.`rank`, m.`sharing_enabled`, "
			"(SELECT COUNT(*) FROM `{}` mc WHERE mc.`fellowship_id` = f.`id`) "
			"FROM `{}` m "
			"INNER JOIN `{}` f ON f.`id` = m.`fellowship_id` "
			"WHERE m.`character_id` = {} LIMIT 1",
			kMemberTable,
			kMemberTable,
			kFellowshipTable,
			character_id
		)
	);

	if (!results.Success() || results.RowCount() == 0) {
		return std::nullopt;
	}

	auto row = results.begin();
	Membership membership;
	membership.fellowship_id = ToUInt(row[0]);
	membership.leader_character_id = ToUInt(row[1]);
	membership.fellowship_name = row[2] ? row[2] : "";
	membership.motd = row[3] ? row[3] : "";
	membership.rank = ToUInt(row[4]);
	membership.sharing_enabled = row[5] ? Strings::ToBool(row[5]) : false;
	membership.member_count = ToUInt(row[6]);
	return membership;
}

std::optional<Membership> RequireMembership(Client *client)
{
	if (!client) {
		return std::nullopt;
	}

	auto membership = LoadMembership(client->CharacterID());
	if (!membership) {
		client->Message(Chat::White, "You are not in a fellowship.");
		return std::nullopt;
	}

	return membership;
}

bool IsLeader(Client *client, const Membership &membership)
{
	return client && client->CharacterID() == membership.leader_character_id;
}

bool RequireLeader(Client *client, const Membership &membership)
{
	if (!IsLeader(client, membership)) {
		if (client) {
			client->Message(Chat::White, "Only the fellowship leader can do that.");
		}

		return false;
	}

	return true;
}

void TouchMember(Client *client)
{
	if (!client || !client->CharacterID() || !zone) {
		return;
	}

	database.QueryDatabase(
		fmt::format(
			"UPDATE `{}` "
			"SET `character_name` = {}, `level` = {}, `class_id` = {}, "
			"`last_zone_id` = {}, `last_instance_id` = {}, `last_online` = UNIX_TIMESTAMP(), "
			"`updated_at` = UNIX_TIMESTAMP() "
			"WHERE `character_id` = {}",
			kMemberTable,
			SqlString(client->GetCleanName()),
			static_cast<uint32>(client->GetLevel()),
			static_cast<uint32>(client->GetClass()),
			zone->GetZoneID(),
			zone->GetInstanceID(),
			client->CharacterID()
		)
	);
}

void SendHelp(Client *client)
{
	if (!client) {
		return;
	}

	client->Message(Chat::White, "Fellowship commands:");
	client->Message(Chat::White, "#fellowshipdebug status - Show your fellowship and campfire state.");
	client->Message(Chat::White, "#fellowshipdebug create <name> - Create a fellowship.");
	client->Message(Chat::White, "#fellowshipdebug invite - Invite your player target.");
	client->Message(Chat::White, "#fellowshipdebug accept - Accept a pending fellowship invite.");
	client->Message(Chat::White, "#fellowshipdebug leave - Leave your fellowship.");
	client->Message(Chat::White, "#fellowshipdebug disband - Disband your fellowship as leader.");
	client->Message(Chat::White, "#fellowshipdebug remove <member name> - Remove a member as leader.");
	client->Message(Chat::White, "#fellowshipdebug motd <message> - Set the fellowship message.");
	client->Message(Chat::White, "#fellowshipdebug share - Toggle your fellowship vitality sharing flag.");
	client->Message(Chat::White, "#fellowshipdebug camp create|destroy|port - Manage the basic stored campfire.");
}

std::optional<Campfire> LoadCampfire(uint32 fellowship_id, bool active_only = true)
{
	if (!fellowship_id) {
		return std::nullopt;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `fellowship_id`, `zone_id`, `instance_id`, `x`, `y`, `z`, `heading`, "
			"`campfire_type`, `created_by_character_id`, `created_at`, `expires_at` "
			"FROM `{}` WHERE `fellowship_id` = {} {} LIMIT 1",
			kCampfireTable,
			fellowship_id,
			active_only ? "AND `expires_at` > UNIX_TIMESTAMP()" : ""
		)
	);

	if (!results.Success() || results.RowCount() == 0) {
		return std::nullopt;
	}

	auto row = results.begin();
	Campfire campfire;
	campfire.fellowship_id = ToUInt(row[0]);
	campfire.zone_id = ToUInt(row[1]);
	campfire.instance_id = ToUInt(row[2]);
	campfire.x = ToFloat(row[3]);
	campfire.y = ToFloat(row[4]);
	campfire.z = ToFloat(row[5]);
	campfire.heading = ToFloat(row[6]);
	campfire.campfire_type = row[7] ? row[7] : "";
	campfire.created_by_character_id = ToUInt(row[8]);
	campfire.created_at = ToUInt(row[9]);
	campfire.expires_at = ToUInt(row[10]);
	return campfire;
}

void DeleteExpiredCampfires()
{
	database.QueryDatabase(
		fmt::format(
			"DELETE FROM `{}` WHERE `expires_at` <= UNIX_TIMESTAMP()",
			kCampfireTable
		)
	);
}

std::vector<uint32> LoadFellowshipCharacterIds(uint32 fellowship_id)
{
	std::vector<uint32> character_ids;
	if (!fellowship_id) {
		return character_ids;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `character_id` FROM `{}` WHERE `fellowship_id` = {}",
			kMemberTable,
			fellowship_id
		)
	);

	if (!results.Success()) {
		return character_ids;
	}

	for (auto row = results.begin(); row != results.end(); ++row) {
		const auto character_id = ToUInt(row[0]);
		if (character_id) {
			character_ids.push_back(character_id);
		}
	}

	return character_ids;
}

void NotifyOnlineMembers(uint32 fellowship_id, const std::string &message)
{
	const auto member_ids = LoadFellowshipCharacterIds(fellowship_id);
	if (member_ids.empty()) {
		return;
	}

	for (const auto &[entity_id, client] : entity_list.GetClientList()) {
		if (!client || !client->CharacterID()) {
			continue;
		}

		if (std::find(member_ids.begin(), member_ids.end(), client->CharacterID()) != member_ids.end()) {
			client->Message(Chat::White, "%s", message.c_str());
		}
	}
}

uint32 CountNearbyOnlineMembers(uint32 fellowship_id, Client *center)
{
	if (!fellowship_id || !center) {
		return 0;
	}

	const auto member_ids = LoadFellowshipCharacterIds(fellowship_id);
	if (member_ids.empty()) {
		return 0;
	}

	const auto radius = std::max(1, RuleI(CustomFeatures, FellowshipCampfireNearbyRadius));
	const auto radius_squared = static_cast<float>(radius * radius);
	uint32 count = 0;

	for (const auto &[entity_id, client] : entity_list.GetClientList()) {
		if (!client || !client->Connected() || !client->CharacterID()) {
			continue;
		}

		if (std::find(member_ids.begin(), member_ids.end(), client->CharacterID()) == member_ids.end()) {
			continue;
		}

		if (DistanceSquaredNoZ(center->GetPosition(), client->GetPosition()) <= radius_squared) {
			++count;
		}
	}

	return count;
}

bool AddMember(uint32 fellowship_id, Client *client, uint32 rank)
{
	if (!fellowship_id || !client || !client->CharacterID() || !zone) {
		return false;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"INSERT INTO `{}` "
			"(`fellowship_id`, `character_id`, `character_name`, `rank`, `sharing_enabled`, "
			"`level`, `class_id`, `last_zone_id`, `last_instance_id`, `last_online`, "
			"`vitality_exp`, `vitality_aa_exp`, `joined_at`, `updated_at`) "
			"VALUES ({}, {}, {}, {}, 0, {}, {}, {}, {}, UNIX_TIMESTAMP(), 0, 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP())",
			kMemberTable,
			fellowship_id,
			client->CharacterID(),
			SqlString(client->GetCleanName()),
			rank,
			static_cast<uint32>(client->GetLevel()),
			static_cast<uint32>(client->GetClass()),
			zone->GetZoneID(),
			zone->GetInstanceID()
		)
	);

	return results.Success();
}

bool CreateFellowship(Client *client, const std::string &name)
{
	if (!FeatureEnabled(client) || !client) {
		return false;
	}

	if (LoadMembership(client->CharacterID())) {
		client->Message(Chat::White, "You are already in a fellowship.");
		return false;
	}

	const auto clean_name = CleanFellowshipName(name.empty() ? fmt::format("{}'s Fellowship", client->GetCleanName()) : name);
	if (clean_name.empty()) {
		client->Message(Chat::White, "Usage: #fellowshipdebug create <name>");
		return false;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"INSERT INTO `{}` (`leader_character_id`, `name`, `motd`, `created_at`, `updated_at`) "
			"VALUES ({}, {}, '', UNIX_TIMESTAMP(), UNIX_TIMESTAMP())",
			kFellowshipTable,
			client->CharacterID(),
			SqlString(clean_name)
		)
	);

	if (!results.Success()) {
		client->Message(Chat::White, "Fellowship creation failed.");
		return false;
	}

	const auto fellowship_id = results.LastInsertedID();
	if (!AddMember(fellowship_id, client, kLeaderRank)) {
		database.QueryDatabase(
			fmt::format("DELETE FROM `{}` WHERE `id` = {}", kFellowshipTable, fellowship_id)
		);
		client->Message(Chat::White, "Fellowship creation failed while adding the leader.");
		return false;
	}

	client->Message(Chat::White, "Created fellowship '%s'.", clean_name.c_str());
	return true;
}

bool InviteTarget(Client *client)
{
	if (!FeatureEnabled(client) || !client) {
		return false;
	}

	auto membership = RequireMembership(client);
	if (!membership || !RequireLeader(client, *membership)) {
		return false;
	}

	if (membership->member_count >= static_cast<uint32>(std::max(1, RuleI(CustomFeatures, FellowshipMaxMembers)))) {
		client->Message(Chat::White, "Your fellowship is full.");
		return false;
	}

	auto *target = client->GetTarget();
	if (!target || !target->IsClient()) {
		client->Message(Chat::White, "Target a player to invite.");
		return false;
	}

	auto *target_client = target->CastToClient();
	if (!target_client || target_client == client) {
		client->Message(Chat::White, "Target another player to invite.");
		return false;
	}

	if (LoadMembership(target_client->CharacterID())) {
		client->Message(Chat::White, "%s is already in a fellowship.", target_client->GetCleanName());
		return false;
	}

	pending_invites[target_client->CharacterID()] = {
		membership->fellowship_id,
		client->CharacterID(),
		Timer::GetTimeSeconds()
	};

	client->Message(Chat::White, "Invited %s to fellowship '%s'.", target_client->GetCleanName(), membership->fellowship_name.c_str());
	target_client->Message(Chat::White, "%s invited you to fellowship '%s'. Use #fellowshipdebug accept to join.", client->GetCleanName(), membership->fellowship_name.c_str());
	return true;
}

bool AcceptInvite(Client *client)
{
	if (!FeatureEnabled(client) || !client) {
		return false;
	}

	if (LoadMembership(client->CharacterID())) {
		client->Message(Chat::White, "You are already in a fellowship.");
		return false;
	}

	auto invite_iter = pending_invites.find(client->CharacterID());
	if (invite_iter == pending_invites.end()) {
		client->Message(Chat::White, "You do not have a pending fellowship invite.");
		return false;
	}

	const auto invite = invite_iter->second;
	pending_invites.erase(invite_iter);

	if (Timer::GetTimeSeconds() > invite.created_at + kInviteExpirationSeconds) {
		client->Message(Chat::White, "Your fellowship invite has expired.");
		return false;
	}

	auto leader_membership = LoadMembership(invite.leader_character_id);
	if (!leader_membership || leader_membership->fellowship_id != invite.fellowship_id) {
		client->Message(Chat::White, "That fellowship invite is no longer valid.");
		return false;
	}

	if (leader_membership->member_count >= static_cast<uint32>(std::max(1, RuleI(CustomFeatures, FellowshipMaxMembers)))) {
		client->Message(Chat::White, "That fellowship is now full.");
		return false;
	}

	if (!AddMember(invite.fellowship_id, client, kMemberRank)) {
		client->Message(Chat::White, "Joining the fellowship failed.");
		return false;
	}

	NotifyOnlineMembers(invite.fellowship_id, fmt::format("{} has joined the fellowship.", client->GetCleanName()));
	return true;
}

bool LeaveFellowship(Client *client)
{
	if (!FeatureEnabled(client) || !client) {
		return false;
	}

	auto membership = RequireMembership(client);
	if (!membership) {
		return false;
	}

	if (IsLeader(client, *membership) && membership->member_count > 1) {
		client->Message(Chat::White, "The leader must transfer leadership or disband before leaving. Leadership transfer is not wired yet.");
		return false;
	}

	database.QueryDatabase(
		fmt::format(
			"DELETE FROM `{}` WHERE `character_id` = {}",
			kMemberTable,
			client->CharacterID()
		)
	);

	if (membership->member_count <= 1) {
		database.QueryDatabase(fmt::format("DELETE FROM `{}` WHERE `fellowship_id` = {}", kCampfireTable, membership->fellowship_id));
		database.QueryDatabase(fmt::format("DELETE FROM `{}` WHERE `id` = {}", kFellowshipTable, membership->fellowship_id));
		client->Message(Chat::White, "You left and deleted the empty fellowship.");
		return true;
	}

	NotifyOnlineMembers(membership->fellowship_id, fmt::format("{} has left the fellowship.", client->GetCleanName()));
	return true;
}

bool DisbandFellowship(Client *client)
{
	if (!FeatureEnabled(client) || !client) {
		return false;
	}

	auto membership = RequireMembership(client);
	if (!membership || !RequireLeader(client, *membership)) {
		return false;
	}

	NotifyOnlineMembers(membership->fellowship_id, fmt::format("Fellowship '{}' has been disbanded.", membership->fellowship_name));
	database.QueryDatabase(fmt::format("DELETE FROM `{}` WHERE `fellowship_id` = {}", kCampfireTable, membership->fellowship_id));
	database.QueryDatabase(fmt::format("DELETE FROM `{}` WHERE `fellowship_id` = {}", kMemberTable, membership->fellowship_id));
	database.QueryDatabase(fmt::format("DELETE FROM `{}` WHERE `id` = {}", kFellowshipTable, membership->fellowship_id));
	return true;
}

bool RemoveMember(Client *client, const std::string &member_name)
{
	if (!FeatureEnabled(client) || !client) {
		return false;
	}

	auto membership = RequireMembership(client);
	if (!membership || !RequireLeader(client, *membership)) {
		return false;
	}

	auto clean_name = member_name;
	Strings::Trim(clean_name);
	if (clean_name.empty()) {
		client->Message(Chat::White, "Usage: #fellowshipdebug remove <member name>");
		return false;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `character_id`, `character_name` FROM `{}` "
			"WHERE `fellowship_id` = {} AND LOWER(`character_name`) = LOWER({}) LIMIT 1",
			kMemberTable,
			membership->fellowship_id,
			SqlString(clean_name)
		)
	);

	if (!results.Success() || results.RowCount() == 0) {
		client->Message(Chat::White, "No fellowship member named '%s' was found.", clean_name.c_str());
		return false;
	}

	auto row = results.begin();
	const auto character_id = ToUInt(row[0]);
	const std::string character_name = row[1] ? row[1] : clean_name;
	if (character_id == membership->leader_character_id) {
		client->Message(Chat::White, "The leader cannot be removed. Use #fellowshipdebug disband.");
		return false;
	}

	database.QueryDatabase(
		fmt::format(
			"DELETE FROM `{}` WHERE `fellowship_id` = {} AND `character_id` = {}",
			kMemberTable,
			membership->fellowship_id,
			character_id
		)
	);

	NotifyOnlineMembers(membership->fellowship_id, fmt::format("{} was removed from the fellowship.", character_name));
	return true;
}

bool SetMotd(Client *client, const std::string &message)
{
	if (!FeatureEnabled(client) || !client) {
		return false;
	}

	auto membership = RequireMembership(client);
	if (!membership || !RequireLeader(client, *membership)) {
		return false;
	}

	const auto motd = CleanMotd(message);
	database.QueryDatabase(
		fmt::format(
			"UPDATE `{}` SET `motd` = {}, `updated_at` = UNIX_TIMESTAMP() WHERE `id` = {}",
			kFellowshipTable,
			SqlString(motd),
			membership->fellowship_id
		)
	);

	NotifyOnlineMembers(membership->fellowship_id, fmt::format("Fellowship message updated: {}", motd.empty() ? "(empty)" : motd));
	return true;
}

bool ToggleSharing(Client *client)
{
	if (!FeatureEnabled(client) || !client) {
		return false;
	}

	auto membership = RequireMembership(client);
	if (!membership) {
		return false;
	}

	const auto enabled = !membership->sharing_enabled;
	database.QueryDatabase(
		fmt::format(
			"UPDATE `{}` SET `sharing_enabled` = {}, `updated_at` = UNIX_TIMESTAMP() WHERE `character_id` = {}",
			kMemberTable,
			enabled ? 1 : 0,
			client->CharacterID()
		)
	);

	client->Message(Chat::White, "Fellowship vitality sharing is now %s.", enabled ? "enabled" : "disabled");
	return true;
}

bool CreateCampfire(Client *client)
{
	if (!FeatureEnabled(client) || !client || !zone) {
		return false;
	}

	auto membership = RequireMembership(client);
	if (!membership) {
		return false;
	}

	DeleteExpiredCampfires();
	if (LoadCampfire(membership->fellowship_id)) {
		client->Message(Chat::White, "Your fellowship already has a campfire. Destroy it before creating a new one.");
		return false;
	}

	TouchMember(client);
	const auto required = static_cast<uint32>(std::max(1, RuleI(CustomFeatures, FellowshipCampfireRequiredNearbyMembers)));
	const auto nearby = CountNearbyOnlineMembers(membership->fellowship_id, client);
	if (nearby < required) {
		client->Message(Chat::White, "You need %u nearby fellowship members to create a campfire. Nearby now: %u.", required, nearby);
		return false;
	}

	const auto duration = static_cast<uint32>(std::max(60, RuleI(CustomFeatures, FellowshipCampfireDurationSeconds)));
	auto results = database.QueryDatabase(
		fmt::format(
			"INSERT INTO `{}` "
			"(`fellowship_id`, `zone_id`, `instance_id`, `x`, `y`, `z`, `heading`, `campfire_type`, "
			"`created_by_character_id`, `created_at`, `expires_at`) "
			"VALUES ({}, {}, {}, {:.6f}, {:.6f}, {:.6f}, {:.6f}, 'honor', {}, UNIX_TIMESTAMP(), UNIX_TIMESTAMP() + {}) "
			"ON DUPLICATE KEY UPDATE "
			"`zone_id` = VALUES(`zone_id`), `instance_id` = VALUES(`instance_id`), "
			"`x` = VALUES(`x`), `y` = VALUES(`y`), `z` = VALUES(`z`), `heading` = VALUES(`heading`), "
			"`campfire_type` = VALUES(`campfire_type`), `created_by_character_id` = VALUES(`created_by_character_id`), "
			"`created_at` = VALUES(`created_at`), `expires_at` = VALUES(`expires_at`)",
			kCampfireTable,
			membership->fellowship_id,
			zone->GetZoneID(),
			zone->GetInstanceID(),
			client->GetX(),
			client->GetY(),
			client->GetZ(),
			client->GetHeading(),
			client->CharacterID(),
			duration
		)
	);

	if (!results.Success()) {
		client->Message(Chat::White, "Campfire creation failed.");
		return false;
	}

	NotifyOnlineMembers(membership->fellowship_id, fmt::format("{} created a fellowship campfire.", client->GetCleanName()));
	return true;
}

bool DestroyCampfire(Client *client)
{
	if (!FeatureEnabled(client) || !client) {
		return false;
	}

	auto membership = RequireMembership(client);
	if (!membership) {
		return false;
	}

	auto campfire = LoadCampfire(membership->fellowship_id, false);
	if (!campfire) {
		client->Message(Chat::White, "Your fellowship does not have a campfire.");
		return false;
	}

	database.QueryDatabase(
		fmt::format(
			"DELETE FROM `{}` WHERE `fellowship_id` = {}",
			kCampfireTable,
			membership->fellowship_id
		)
	);

	NotifyOnlineMembers(membership->fellowship_id, fmt::format("{} destroyed the fellowship campfire.", client->GetCleanName()));
	return true;
}

bool TeleportToCampfire(Client *client)
{
	if (!FeatureEnabled(client) || !client) {
		return false;
	}

	auto membership = RequireMembership(client);
	if (!membership) {
		return false;
	}

	DeleteExpiredCampfires();
	auto campfire = LoadCampfire(membership->fellowship_id);
	if (!campfire) {
		client->Message(Chat::White, "Your fellowship does not have an active campfire.");
		return false;
	}

	client->Message(Chat::White, "Traveling to your fellowship campfire.");
	client->MovePC(campfire->zone_id, campfire->instance_id, campfire->x, campfire->y, campfire->z, campfire->heading, 0, ZoneSolicited);
	return true;
}

void ShowStatus(Client *client)
{
	if (!FeatureEnabled(client) || !client) {
		return;
	}

	auto membership = LoadMembership(client->CharacterID());
	if (!membership) {
		client->Message(Chat::White, "You are not in a fellowship.");
		return;
	}

	TouchMember(client);
	DeleteExpiredCampfires();
	client->Message(Chat::White, "Fellowship: %s (ID %u)", membership->fellowship_name.c_str(), membership->fellowship_id);
	client->Message(Chat::White, "Leader character ID: %u, members: %u/%u, sharing: %s",
		membership->leader_character_id,
		membership->member_count,
		static_cast<uint32>(std::max(1, RuleI(CustomFeatures, FellowshipMaxMembers))),
		membership->sharing_enabled ? "on" : "off"
	);

	if (!membership->motd.empty()) {
		client->Message(Chat::White, "MOTD: %s", membership->motd.c_str());
	}

	auto campfire = LoadCampfire(membership->fellowship_id);
	if (!campfire) {
		client->Message(Chat::White, "Campfire: none active.");
		return;
	}

	const auto now = static_cast<uint32>(std::time(nullptr));
	const auto remaining = campfire->expires_at > now ? campfire->expires_at - now : 0;
	client->Message(
		Chat::White,
		"Campfire: zone %u instance %u at %.2f %.2f %.2f, %u seconds remaining.",
		campfire->zone_id,
		campfire->instance_id,
		campfire->x,
		campfire->y,
		campfire->z,
		remaining
	);
}

void HandleCampCommand(Client *client, const Seperator *sep)
{
	if (!sep || sep->argnum < 2 || !sep->arg[2][0] || !strcasecmp(sep->arg[2], "status")) {
		ShowStatus(client);
		return;
	}

	if (!strcasecmp(sep->arg[2], "create")) {
		CreateCampfire(client);
		return;
	}

	if (!strcasecmp(sep->arg[2], "destroy")) {
		DestroyCampfire(client);
		return;
	}

	if (
		!strcasecmp(sep->arg[2], "port") ||
		!strcasecmp(sep->arg[2], "teleport") ||
		!strcasecmp(sep->arg[2], "travel")
	) {
		TeleportToCampfire(client);
		return;
	}

	SendHelp(client);
}

} // namespace

void FellowshipManager::HandleCommand(Client *client, const Seperator *sep)
{
	if (!client) {
		return;
	}

	if (!sep || sep->argnum < 1 || !sep->arg[1][0] || !strcasecmp(sep->arg[1], "status")) {
		ShowStatus(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "help")) {
		SendHelp(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "create")) {
		CreateFellowship(client, sep->argnum >= 2 ? sep->argplus[2] : "");
		return;
	}

	if (!strcasecmp(sep->arg[1], "invite")) {
		InviteTarget(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "accept")) {
		AcceptInvite(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "leave")) {
		LeaveFellowship(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "disband")) {
		DisbandFellowship(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "remove")) {
		RemoveMember(client, sep->argnum >= 2 ? sep->argplus[2] : "");
		return;
	}

	if (!strcasecmp(sep->arg[1], "motd")) {
		SetMotd(client, sep->argnum >= 2 ? sep->argplus[2] : "");
		return;
	}

	if (!strcasecmp(sep->arg[1], "share")) {
		ToggleSharing(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "camp") || !strcasecmp(sep->arg[1], "campfire")) {
		HandleCampCommand(client, sep);
		return;
	}

	SendHelp(client);
}

void FellowshipManager::LogDiscoveryPacket(Client *client, const EQApplicationPacket *app, const char *context) const
{
	if (
		!RuleB(CustomFeatures, FellowshipsEnabled) ||
		!RuleB(CustomFeatures, FellowshipOpcodeDiscoveryEnabled) ||
		!client ||
		!app
	) {
		return;
	}

	LogInfo(
		"Fellowship opcode discovery [{}] character [{}] emu [{}] protocol [{:#06x}] size [{}] {}",
		context ? context : "unknown",
		client->GetCleanName(),
		OpcodeManager::EmuToName(app->GetOpcode()),
		app->GetProtocolOpcode(),
		app->Size(),
		DumpPacketToString(app)
	);
}

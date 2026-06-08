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
#include <cstring>
#include <ctime>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

extern EntityList entity_list;
extern Zone *zone;
extern ZoneDatabase database;

FellowshipManager fellowship_manager;

namespace {

constexpr const char *kFellowshipTable = "custom_fellowships";
constexpr const char *kMemberTable = "custom_fellowship_members";
constexpr const char *kCampfireTable = "custom_fellowship_campfires";
constexpr const char *kInviteTable = "custom_fellowship_invites";
constexpr uint32 kLeaderRank = 1;
constexpr uint32 kMemberRank = 0;
constexpr uint32 kInviteExpirationSeconds = 300;
constexpr uint32 kClientActionCreate = 1;
constexpr uint32 kClientActionAcceptInvite = 4;
constexpr uint32 kClientActionInviteTarget = 5;
constexpr uint32 kCreatePacketWireSize = 1078;
constexpr uint32 kFellowshipInvitePacketSize = 16;
constexpr uint32 kFellowshipInviteEntityIdOffset = 0x0c;
constexpr uint32 kFellowshipStatePacketSize = 0x9e4;
constexpr uint32 kFellowshipStateNameOffset = 0x008;
constexpr uint32 kFellowshipStateMotdOffset = 0x048;
constexpr uint32 kFellowshipStateMembersOffset = 0x448;
constexpr uint32 kFellowshipStateMemberListOffset = 0x44c;
constexpr uint32 kFellowshipStateMemberSize = 0x54;
constexpr uint32 kFellowshipStateSyncOffset = 0x83c;
constexpr uint32 kFellowshipStatePlayerHandlesOffset = 0x840;
constexpr uint32 kFellowshipStatePlayerHandleSize = 0x20;
constexpr uint32 kFellowshipStateExpSharingOffset = 0x9c0;
constexpr uint32 kFellowshipStateExpCappedOffset = 0x9cc;
constexpr uint32 kFellowshipStateOfflineOffset = 0x9d8;
constexpr uint32 kFellowshipMaxClientMembers = 12;
constexpr uint32 kFellowshipStateWithCampfirePacketSize = 0xa04;
constexpr uint32 kFellowshipActionPacketSize = 1076;
constexpr uint32 kFellowshipProbeDefaultIntervalMs = 2500;
constexpr uint32 kFellowshipProbeMinIntervalMs = 1000;
constexpr uint32 kFellowshipProbeMaxIntervalMs = 10000;

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

struct FellowshipMemberState {
	uint32 character_id = 0;
	uint32 entity_id = 0;
	std::string character_name;
	uint32 rank = 0;
	bool sharing_enabled = false;
	uint32 level = 0;
	uint32 class_id = 0;
	uint32 zone_id = 0;
	uint32 instance_id = 0;
	uint32 last_online = 0;
};

struct FellowshipStateContext {
	Membership membership;
	std::string leader_name;
	std::vector<FellowshipMemberState> members;
};

enum class FellowshipProbeLayout {
	MemoryUnknownOne,
	MemoryUnknownZero,
	MemoryWithTimestamp,
	MemoryWithoutLeadingUnknown,
	MemoryWithCampfireTail,
	ActionCreateAck,
	ActionStateFixed,
	CampfireOnly,
};

struct FellowshipProbeDefinition {
	const char *label = "";
	EmuOpcode opcode = OP_Unknown;
	uint16 opcode_bypass = 0;
	FellowshipProbeLayout layout = FellowshipProbeLayout::MemoryUnknownOne;
};

struct FellowshipProbeSession {
	uint32 next_probe = 0;
	uint32 interval_ms = kFellowshipProbeDefaultIntervalMs;
	bool active = false;
	Timer timer;
};

std::map<uint32, PendingInvite> pending_invites;
std::map<uint32, FellowshipProbeSession> fellowship_probe_sessions;

const std::vector<FellowshipProbeDefinition> &GetFellowshipProbes()
{
	static const std::vector<FellowshipProbeDefinition> probes = {
		{ "rof2-showeq-20130417-memory-one", OP_Fellowship, 0, FellowshipProbeLayout::MemoryUnknownOne },
		{ "rof2-showeq-20130417-memory-zero", OP_Fellowship, 0, FellowshipProbeLayout::MemoryUnknownZero },
		{ "rof2-showeq-20130417-memory-timestamp", OP_Fellowship, 0, FellowshipProbeLayout::MemoryWithTimestamp },
		{ "create-action-opcode-memory-one", OP_FellowshipUpdate, 0, FellowshipProbeLayout::MemoryUnknownOne },
		{ "rof-showeq-20121128-memory-one", OP_Unknown, 0x23ad, FellowshipProbeLayout::MemoryUnknownOne },
		{ "rof-showeq-20121212-memory-one", OP_Unknown, 0x40fd, FellowshipProbeLayout::MemoryUnknownOne },
		{ "rof-showeq-20130313-memory-one", OP_Unknown, 0x7eb8, FellowshipProbeLayout::MemoryUnknownOne },
		{ "rof-showeq-20120817-memory-one", OP_Unknown, 0x584f, FellowshipProbeLayout::MemoryUnknownOne },
		{ "rof-showeq-20121023-memory-one", OP_Unknown, 0x5545, FellowshipProbeLayout::MemoryUnknownOne },
		{ "rof2-memory-no-leading-unknown", OP_Fellowship, 0, FellowshipProbeLayout::MemoryWithoutLeadingUnknown },
		{ "rof2-memory-with-campfire-tail", OP_Fellowship, 0, FellowshipProbeLayout::MemoryWithCampfireTail },
		{ "rof-20121212-memory-with-campfire-tail", OP_Unknown, 0x40fd, FellowshipProbeLayout::MemoryWithCampfireTail },
		{ "rof2-action-create-ack", OP_Fellowship, 0, FellowshipProbeLayout::ActionCreateAck },
		{ "rof2-action-state-fixed", OP_Fellowship, 0, FellowshipProbeLayout::ActionStateFixed },
		{ "create-action-opcode-create-ack", OP_FellowshipUpdate, 0, FellowshipProbeLayout::ActionCreateAck },
		{ "create-action-opcode-state-fixed", OP_FellowshipUpdate, 0, FellowshipProbeLayout::ActionStateFixed },
	};

	return probes;
}

uint32 ReadUInt32OrZero(const EQApplicationPacket *app, uint32 offset)
{
	if (!app || !app->pBuffer || app->size < offset + sizeof(uint32)) {
		return 0;
	}

	return app->ReadUInt32(offset);
}

uint32 CountNonZeroBytes(const EQApplicationPacket *app)
{
	if (!app || !app->pBuffer) {
		return 0;
	}

	uint32 count = 0;
	for (uint32 offset = 0; offset < app->size; ++offset) {
		if (app->pBuffer[offset] != 0) {
			++count;
		}
	}

	return count;
}

std::string DescribeNonZeroOffsets(const EQApplicationPacket *app, uint32 max_offsets = 24)
{
	if (!app || !app->pBuffer || app->size == 0) {
		return "none";
	}

	std::vector<std::string> offsets;
	uint32 total = 0;

	for (uint32 offset = 0; offset < app->size; ++offset) {
		const auto value = app->pBuffer[offset];
		if (value == 0) {
			continue;
		}

		++total;
		if (offsets.size() < max_offsets) {
			offsets.emplace_back(fmt::format("{}=0x{:02x}", offset, static_cast<uint32>(value)));
		}
	}

	if (offsets.empty()) {
		return "none";
	}

	if (total > offsets.size()) {
		offsets.emplace_back(fmt::format("...{} more", total - static_cast<uint32>(offsets.size())));
	}

	return Strings::Join(offsets, ",");
}

void WriteUInt16(uint8 *buffer, uint32 offset, uint16 value)
{
	std::memcpy(buffer + offset, &value, sizeof(value));
}

void WriteUInt8(uint8 *buffer, uint32 offset, uint8 value)
{
	std::memcpy(buffer + offset, &value, sizeof(value));
}

void WriteUInt32(uint8 *buffer, uint32 offset, uint32 value)
{
	std::memcpy(buffer + offset, &value, sizeof(value));
}

void WriteFloat(uint8 *buffer, uint32 offset, float value)
{
	std::memcpy(buffer + offset, &value, sizeof(value));
}

void WriteFixedString(uint8 *buffer, uint32 offset, uint32 length, const std::string &value)
{
	if (!length) {
		return;
	}

	const auto copy_length = std::min<uint32>(length - 1, static_cast<uint32>(value.size()));
	if (copy_length) {
		std::memcpy(buffer + offset, value.data(), copy_length);
	}
}

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

void DeletePendingInvite(uint32 target_character_id)
{
	if (!target_character_id) {
		return;
	}

	pending_invites.erase(target_character_id);
	database.QueryDatabase(
		fmt::format(
			"DELETE FROM `{}` WHERE `target_character_id` = {}",
			kInviteTable,
			target_character_id
		)
	);
}

void DeleteExpiredPendingInvites()
{
	const auto now = Timer::GetTimeSeconds();
	for (auto invite_iter = pending_invites.begin(); invite_iter != pending_invites.end();) {
		if (now > invite_iter->second.created_at + kInviteExpirationSeconds) {
			invite_iter = pending_invites.erase(invite_iter);
			continue;
		}

		++invite_iter;
	}

	database.QueryDatabase(
		fmt::format(
			"DELETE FROM `{}` WHERE `expires_at` > 0 AND `expires_at` < UNIX_TIMESTAMP()",
			kInviteTable
		)
	);
}

bool StorePendingInvite(uint32 fellowship_id, Client *inviter, Client *target)
{
	if (!fellowship_id || !inviter || !target || !inviter->CharacterID() || !target->CharacterID()) {
		return false;
	}

	DeleteExpiredPendingInvites();
	pending_invites[target->CharacterID()] = {
		fellowship_id,
		inviter->CharacterID(),
		Timer::GetTimeSeconds()
	};

	auto results = database.QueryDatabase(
		fmt::format(
			"REPLACE INTO `{}` "
			"(`target_character_id`, `fellowship_id`, `leader_character_id`, `inviter_character_id`, `created_at`, `expires_at`) "
			"VALUES ({}, {}, {}, {}, UNIX_TIMESTAMP(), UNIX_TIMESTAMP() + {})",
			kInviteTable,
			target->CharacterID(),
			fellowship_id,
			inviter->CharacterID(),
			inviter->CharacterID(),
			kInviteExpirationSeconds
		)
	);

	return results.Success();
}

std::optional<PendingInvite> LoadPendingInvite(uint32 target_character_id)
{
	if (!target_character_id) {
		return std::nullopt;
	}

	DeleteExpiredPendingInvites();

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `fellowship_id`, `leader_character_id`, `created_at` "
			"FROM `{}` WHERE `target_character_id` = {} AND (`expires_at` = 0 OR `expires_at` >= UNIX_TIMESTAMP()) LIMIT 1",
			kInviteTable,
			target_character_id
		)
	);

	if (results.Success() && results.RowCount() > 0) {
		auto row = results.begin();
		return PendingInvite{
			ToUInt(row[0]),
			ToUInt(row[1]),
			ToUInt(row[2])
		};
	}

	auto invite_iter = pending_invites.find(target_character_id);
	if (invite_iter == pending_invites.end()) {
		return std::nullopt;
	}

	if (Timer::GetTimeSeconds() > invite_iter->second.created_at + kInviteExpirationSeconds) {
		pending_invites.erase(invite_iter);
		return std::nullopt;
	}

	return invite_iter->second;
}

bool HasPendingInvite(uint32 target_character_id)
{
	return LoadPendingInvite(target_character_id).has_value();
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

std::vector<FellowshipMemberState> LoadMemberStates(uint32 fellowship_id)
{
	std::vector<FellowshipMemberState> members;
	if (!fellowship_id) {
		return members;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `character_id`, `character_name`, `rank`, `sharing_enabled`, `level`, `class_id`, "
			"`last_zone_id`, `last_instance_id`, `last_online` "
			"FROM `{}` WHERE `fellowship_id` = {} "
			"ORDER BY `rank` DESC, `joined_at` ASC LIMIT {}",
			kMemberTable,
			fellowship_id,
			kFellowshipMaxClientMembers
		)
	);

	if (!results.Success()) {
		return members;
	}

	for (auto row = results.begin(); row != results.end(); ++row) {
		FellowshipMemberState member;
		member.character_id = ToUInt(row[0]);
		member.character_name = row[1] ? row[1] : "";
		if (auto *member_client = entity_list.GetClientByName(member.character_name.c_str())) {
			member.entity_id = member_client->GetID();
		}
		member.rank = ToUInt(row[2]);
		member.sharing_enabled = row[3] ? Strings::ToBool(row[3]) : false;
		member.level = ToUInt(row[4]);
		member.class_id = ToUInt(row[5]);
		member.zone_id = ToUInt(row[6]);
		member.instance_id = ToUInt(row[7]);
		member.last_online = ToUInt(row[8]);
		members.push_back(member);
	}

	return members;
}

std::optional<FellowshipStateContext> LoadFellowshipStateContext(Client *client)
{
	if (!client || !client->CharacterID()) {
		return std::nullopt;
	}

	auto membership = LoadMembership(client->CharacterID());
	if (!membership) {
		return std::nullopt;
	}

	auto members = LoadMemberStates(membership->fellowship_id);
	if (members.empty()) {
		return std::nullopt;
	}

	std::string leader_name = client->GetCleanName();
	for (const auto &member : members) {
		if (member.character_id == membership->leader_character_id) {
			leader_name = member.character_name;
			break;
		}
	}

	FellowshipStateContext context;
	context.membership = *membership;
	context.leader_name = leader_name;
	context.members = std::move(members);
	return context;
}

std::unique_ptr<EQApplicationPacket> BuildFellowshipMemoryStatePacket(
	const FellowshipStateContext &context,
	EmuOpcode opcode,
	uint16 opcode_bypass,
	uint32 unknown0,
	bool write_timestamp,
	bool include_campfire_tail = false
)
{
	auto outapp = std::make_unique<EQApplicationPacket>(
		opcode,
		include_campfire_tail ? kFellowshipStateWithCampfirePacketSize : kFellowshipStatePacketSize
	);
	if (opcode_bypass) {
		outapp->SetOpcodeBypass(opcode_bypass);
	}

	std::memset(outapp->pBuffer, 0, outapp->size);
	WriteUInt32(outapp->pBuffer, 0x000, unknown0);
	WriteUInt32(outapp->pBuffer, 0x004, context.membership.fellowship_id);
	WriteFixedString(outapp->pBuffer, kFellowshipStateNameOffset, 0x40, context.membership.fellowship_name);
	WriteFixedString(outapp->pBuffer, kFellowshipStateMotdOffset, 0x400, context.membership.motd);
	WriteUInt32(outapp->pBuffer, kFellowshipStateMembersOffset, static_cast<uint32>(context.members.size()));
	if (write_timestamp) {
		WriteUInt32(outapp->pBuffer, kFellowshipStateSyncOffset, Timer::GetTimeSeconds());
	}

	for (uint32 index = 0; index < context.members.size(); ++index) {
		const auto &member = context.members[index];
		const auto member_offset = kFellowshipStateMemberListOffset + (index * kFellowshipStateMemberSize);
		WriteUInt32(outapp->pBuffer, member_offset + 0x00, member.character_id);
		WriteFixedString(outapp->pBuffer, member_offset + 0x04, 0x40, member.character_name);
		WriteUInt32(outapp->pBuffer, member_offset + 0x44, member.zone_id);
		WriteUInt32(outapp->pBuffer, member_offset + 0x48, member.level);
		WriteUInt32(outapp->pBuffer, member_offset + 0x4c, member.class_id);
		WriteUInt32(outapp->pBuffer, member_offset + 0x50, 0);
		WriteFixedString(
			outapp->pBuffer,
			kFellowshipStatePlayerHandlesOffset + (index * kFellowshipStatePlayerHandleSize),
			kFellowshipStatePlayerHandleSize,
			member.character_name
		);
		WriteUInt8(outapp->pBuffer, kFellowshipStateExpSharingOffset + index, member.sharing_enabled ? 1 : 0);
		WriteUInt8(outapp->pBuffer, kFellowshipStateExpCappedOffset + index, 0);
		WriteUInt8(outapp->pBuffer, kFellowshipStateOfflineOffset + index, 0);
	}

	if (include_campfire_tail) {
		WriteFloat(outapp->pBuffer, 0x9e4, 0.0f); // campfire Y
		WriteFloat(outapp->pBuffer, 0x9e8, 0.0f); // campfire X
		WriteFloat(outapp->pBuffer, 0x9ec, 0.0f); // campfire Z
		WriteUInt16(outapp->pBuffer, 0x9f0, 0);    // campfire zone
		WriteUInt16(outapp->pBuffer, 0x9f2, 0);    // campfire instance
		WriteUInt32(outapp->pBuffer, 0x9f4, 0);    // campfire timestamp
		WriteUInt32(outapp->pBuffer, 0x9f8, 0);
		WriteUInt32(outapp->pBuffer, 0x9fc, 0);    // campfire type?
		WriteUInt32(outapp->pBuffer, 0xa00, 0);    // campfire active
	}

	return outapp;
}

std::unique_ptr<EQApplicationPacket> BuildFellowshipMemoryWithoutLeadingUnknownPacket(
	const FellowshipStateContext &context,
	EmuOpcode opcode,
	uint16 opcode_bypass
)
{
	auto source = BuildFellowshipMemoryStatePacket(context, opcode, opcode_bypass, 1, false);
	auto outapp = std::make_unique<EQApplicationPacket>(opcode, kFellowshipStatePacketSize - sizeof(uint32));
	if (opcode_bypass) {
		outapp->SetOpcodeBypass(opcode_bypass);
	}

	std::memcpy(outapp->pBuffer, source->pBuffer + sizeof(uint32), outapp->size);
	return outapp;
}

std::unique_ptr<EQApplicationPacket> BuildFellowshipActionPacket(
	const FellowshipStateContext &context,
	EmuOpcode opcode,
	uint16 opcode_bypass,
	uint32 action,
	bool include_fixed_state
)
{
	auto outapp = std::make_unique<EQApplicationPacket>(opcode, kFellowshipActionPacketSize);
	if (opcode_bypass) {
		outapp->SetOpcodeBypass(opcode_bypass);
	}

	std::memset(outapp->pBuffer, 0, outapp->size);
	WriteUInt32(outapp->pBuffer, 0x000, action);
	WriteUInt32(outapp->pBuffer, 0x004, context.membership.fellowship_id);
	WriteUInt32(outapp->pBuffer, 0x008, static_cast<uint32>(context.members.size()));

	if (!include_fixed_state) {
		return outapp;
	}

	const auto &first_member = context.members.front();
	WriteFixedString(outapp->pBuffer, 0x00c, 0x40, context.membership.fellowship_name);
	WriteFixedString(outapp->pBuffer, 0x04c, 0x40, context.leader_name);
	WriteFixedString(outapp->pBuffer, 0x08c, 0x40, first_member.character_name);
	WriteUInt32(outapp->pBuffer, 0x0cc, first_member.level);
	WriteUInt32(outapp->pBuffer, 0x0d0, first_member.class_id);
	WriteUInt32(outapp->pBuffer, 0x0d4, first_member.zone_id);
	WriteUInt32(outapp->pBuffer, 0x0d8, first_member.instance_id);
	WriteUInt32(outapp->pBuffer, 0x0dc, first_member.sharing_enabled ? 1 : 0);
	WriteFixedString(outapp->pBuffer, 0x100, 0x300, context.membership.motd);
	return outapp;
}

void SendFellowshipInvitePopup(Client *inviter, Client *target)
{
	if (!inviter || !target) {
		return;
	}

	auto outapp = std::make_unique<EQApplicationPacket>(OP_FellowshipInvite, kFellowshipInvitePacketSize);
	std::memset(outapp->pBuffer, 0, outapp->size);
	WriteUInt32(outapp->pBuffer, kFellowshipInviteEntityIdOffset, inviter->GetID());

	if (RuleB(CustomFeatures, FellowshipOpcodeDiscoveryEnabled)) {
		LogInfo(
			"Sending fellowship invite popup inviter [{}] target [{}] inviter_entity_id [{}] size [{}] {}",
			inviter->GetCleanName(),
			target->GetCleanName(),
			inviter->GetID(),
			outapp->size,
			DumpPacketToString(outapp.get())
		);
	}

	target->QueuePacket(outapp.get());
}

std::unique_ptr<EQApplicationPacket> BuildFellowshipProbePacket(
	const FellowshipStateContext &context,
	const FellowshipProbeDefinition &probe
)
{
	switch (probe.layout) {
	case FellowshipProbeLayout::MemoryUnknownZero:
		return BuildFellowshipMemoryStatePacket(context, probe.opcode, probe.opcode_bypass, 0, false);
	case FellowshipProbeLayout::MemoryWithTimestamp:
		return BuildFellowshipMemoryStatePacket(context, probe.opcode, probe.opcode_bypass, 1, true);
	case FellowshipProbeLayout::MemoryWithoutLeadingUnknown:
		return BuildFellowshipMemoryWithoutLeadingUnknownPacket(context, probe.opcode, probe.opcode_bypass);
	case FellowshipProbeLayout::MemoryWithCampfireTail:
		return BuildFellowshipMemoryStatePacket(context, probe.opcode, probe.opcode_bypass, 1, true, true);
	case FellowshipProbeLayout::ActionCreateAck:
		return BuildFellowshipActionPacket(context, probe.opcode, probe.opcode_bypass, 1, false);
	case FellowshipProbeLayout::ActionStateFixed:
		return BuildFellowshipActionPacket(context, probe.opcode, probe.opcode_bypass, 2, true);
	case FellowshipProbeLayout::CampfireOnly:
		return BuildFellowshipActionPacket(context, probe.opcode, probe.opcode_bypass, 0, false);
	case FellowshipProbeLayout::MemoryUnknownOne:
	default:
		return BuildFellowshipMemoryStatePacket(context, probe.opcode, probe.opcode_bypass, 1, false);
	}
}

const char *GetFellowshipProbeLayoutName(FellowshipProbeLayout layout)
{
	switch (layout) {
	case FellowshipProbeLayout::MemoryUnknownZero:
		return "memory_unknown0_zero";
	case FellowshipProbeLayout::MemoryWithTimestamp:
		return "memory_with_timestamp";
	case FellowshipProbeLayout::MemoryWithoutLeadingUnknown:
		return "memory_without_leading_unknown";
	case FellowshipProbeLayout::MemoryWithCampfireTail:
		return "memory_with_campfire_tail";
	case FellowshipProbeLayout::ActionCreateAck:
		return "action_create_ack";
	case FellowshipProbeLayout::ActionStateFixed:
		return "action_state_fixed";
	case FellowshipProbeLayout::CampfireOnly:
		return "campfire_only";
	case FellowshipProbeLayout::MemoryUnknownOne:
	default:
		return "memory_unknown0_one";
	}
}

bool SendFellowshipProbe(Client *client, FellowshipProbeSession &session, bool manual)
{
	if (!client || !client->CharacterID()) {
		return false;
	}

	if (!RuleB(CustomFeatures, FellowshipOpcodeDiscoveryEnabled)) {
		client->Message(Chat::White, "Fellowship opcode discovery is disabled.");
		session.active = false;
		return false;
	}

	const auto context = LoadFellowshipStateContext(client);
	if (!context) {
		client->Message(Chat::White, "No fellowship state is available to probe.");
		session.active = false;
		return false;
	}

	const auto &probes = GetFellowshipProbes();
	if (session.next_probe >= probes.size()) {
		client->Message(Chat::White, "Fellowship probe set is complete. Use #fellowshipdebug probe reset to run it again.");
		session.active = false;
		return false;
	}

	const auto probe_index = session.next_probe++;
	const auto &probe = probes[probe_index];
	auto outapp = BuildFellowshipProbePacket(*context, probe);
	const auto protocol_opcode = probe.opcode_bypass;
	const auto layout_name = GetFellowshipProbeLayoutName(probe.layout);

	LogInfo(
		"Fellowship probe character [{}] index [{}]/[{}] label [{}] emu [{}] bypass [{:#06x}] payload_size [{}] wire_size [{}] layout [{}] manual [{}] {}",
		client->GetCleanName(),
		probe_index + 1,
		probes.size(),
		probe.label,
		OpcodeManager::EmuToName(outapp->GetOpcode()),
		protocol_opcode,
		outapp->size,
		outapp->Size(),
		layout_name,
		manual ? 1 : 0,
		DumpPacketToString(outapp.get())
	);

	client->Message(
		Chat::White,
		"Fellowship probe %u/%u: %s",
		probe_index + 1,
		static_cast<uint32>(probes.size()),
		probe.label
	);

	client->QueuePacket(outapp.get());
	session.timer.Start(session.interval_ms);
	return true;
}

void SendFellowshipState(Client *client)
{
	const auto context = LoadFellowshipStateContext(client);
	if (!context) {
		return;
	}

	auto outapp = BuildFellowshipMemoryStatePacket(*context, OP_Fellowship, 0, 1, true, true);

	if (RuleB(CustomFeatures, FellowshipOpcodeDiscoveryEnabled)) {
		LogInfo(
			"Sending fellowship state packet character [{}] fellowship_id [{}] members [{}] leader [{}] size [{}] {}",
			client->GetCleanName(),
			context->membership.fellowship_id,
			context->members.size(),
			context->leader_name,
			outapp->size,
			DumpPacketToString(outapp.get())
		);
	}

	client->QueuePacket(outapp.get());
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
	client->Message(Chat::White, "#fellowshipdebug probe start|stop|next|reset|status|list [interval_ms|probe_number] - Send controlled fellowship packet probes.");
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
			SendFellowshipState(client);
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
		SendFellowshipState(client);
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
	SendFellowshipState(client);
	return true;
}

bool InviteTarget(Client *client, uint32 target_entity_id = 0)
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

	Client *target_client = nullptr;
	if (target_entity_id) {
		target_client = entity_list.GetClientByID(static_cast<uint16>(target_entity_id));
	}

	if (!target_client) {
		auto *target = client->GetTarget();
		if (!target || !target->IsClient()) {
			client->Message(Chat::White, "Target a player to invite.");
			return false;
		}

		target_client = target->CastToClient();
	}

	if (!target_client || target_client == client) {
		client->Message(Chat::White, "Target another player to invite.");
		return false;
	}

	if (LoadMembership(target_client->CharacterID())) {
		client->Message(Chat::White, "%s is already in a fellowship.", target_client->GetCleanName());
		return false;
	}

	if (!StorePendingInvite(membership->fellowship_id, client, target_client)) {
		client->Message(Chat::White, "Creating the fellowship invite failed.");
		return false;
	}

	SendFellowshipInvitePopup(client, target_client);
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

	auto invite = LoadPendingInvite(client->CharacterID());
	if (!invite) {
		client->Message(Chat::White, "You do not have a pending fellowship invite.");
		return false;
	}

	if (Timer::GetTimeSeconds() > invite->created_at + kInviteExpirationSeconds) {
		DeletePendingInvite(client->CharacterID());
		client->Message(Chat::White, "Your fellowship invite has expired.");
		return false;
	}

	auto leader_membership = LoadMembership(invite->leader_character_id);
	if (!leader_membership || leader_membership->fellowship_id != invite->fellowship_id) {
		DeletePendingInvite(client->CharacterID());
		client->Message(Chat::White, "That fellowship invite is no longer valid.");
		return false;
	}

	if (leader_membership->member_count >= static_cast<uint32>(std::max(1, RuleI(CustomFeatures, FellowshipMaxMembers)))) {
		DeletePendingInvite(client->CharacterID());
		client->Message(Chat::White, "That fellowship is now full.");
		return false;
	}

	if (!AddMember(invite->fellowship_id, client, kMemberRank)) {
		client->Message(Chat::White, "Joining the fellowship failed.");
		return false;
	}

	DeletePendingInvite(client->CharacterID());
	NotifyOnlineMembers(invite->fellowship_id, fmt::format("{} has joined the fellowship.", client->GetCleanName()));
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

void HandleProbeCommand(Client *client, const Seperator *sep)
{
	if (!FeatureEnabled(client) || !client) {
		return;
	}

	if (!RuleB(CustomFeatures, FellowshipOpcodeDiscoveryEnabled)) {
		client->Message(Chat::White, "Fellowship opcode discovery is disabled.");
		return;
	}

	auto &session = fellowship_probe_sessions[client->CharacterID()];

	if (!sep || sep->argnum < 2 || !sep->arg[2][0] || !strcasecmp(sep->arg[2], "status")) {
		client->Message(
			Chat::White,
			"Fellowship probe status: %s next %u/%u interval %u ms.",
			session.active ? "running" : "stopped",
			std::min<uint32>(session.next_probe + 1, static_cast<uint32>(GetFellowshipProbes().size())),
			static_cast<uint32>(GetFellowshipProbes().size()),
			session.interval_ms
		);
		return;
	}

	if (!strcasecmp(sep->arg[2], "list")) {
		const auto &probes = GetFellowshipProbes();
		for (uint32 index = 0; index < probes.size(); ++index) {
			client->Message(Chat::White, "Probe %u/%u: %s", index + 1, static_cast<uint32>(probes.size()), probes[index].label);
		}
		return;
	}

	if (!strcasecmp(sep->arg[2], "stop")) {
		session.active = false;
		session.timer.Disable();
		client->Message(Chat::White, "Fellowship probe stopped.");
		return;
	}

	if (!strcasecmp(sep->arg[2], "reset")) {
		const auto &probes = GetFellowshipProbes();
		session.active = false;
		session.next_probe = 0;
		if (sep->argnum >= 3 && sep->arg[3][0] && Strings::IsNumber(sep->arg[3])) {
			const auto requested_probe = Strings::ToUnsignedInt(sep->arg[3]);
			if (requested_probe >= 1 && requested_probe <= probes.size()) {
				session.next_probe = requested_probe - 1;
			}
		}
		session.timer.Disable();
		client->Message(
			Chat::White,
			"Fellowship probe reset to packet %u/%u.",
			std::min<uint32>(session.next_probe + 1, static_cast<uint32>(probes.size())),
			static_cast<uint32>(probes.size())
		);
		return;
	}

	if (!strcasecmp(sep->arg[2], "next")) {
		session.active = false;
		session.timer.Disable();
		SendFellowshipProbe(client, session, true);
		return;
	}

	if (!strcasecmp(sep->arg[2], "start")) {
		if (sep->argnum >= 3 && sep->arg[3][0] && Strings::IsNumber(sep->arg[3])) {
			session.interval_ms = std::clamp(
				Strings::ToUnsignedInt(sep->arg[3]),
				kFellowshipProbeMinIntervalMs,
				kFellowshipProbeMaxIntervalMs
			);
		}
		else if (!session.interval_ms) {
			session.interval_ms = kFellowshipProbeDefaultIntervalMs;
		}

		session.active = true;
		client->Message(Chat::White, "Fellowship probe started. Keep the Fellowship window open. Use #fellowshipdebug probe stop if the client acts strange.");
		SendFellowshipProbe(client, session, true);
		return;
	}

	client->Message(Chat::White, "Usage: #fellowshipdebug probe start|stop|next|reset|status|list [interval_ms]");
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

	if (!strcasecmp(sep->arg[1], "probe")) {
		HandleProbeCommand(client, sep);
		return;
	}

	SendHelp(client);
}

void FellowshipManager::HandleClientPacket(Client *client, const EQApplicationPacket *app) const
{
	if (!RuleB(CustomFeatures, FellowshipsEnabled) || !client || !app) {
		return;
	}

	const auto action = ReadUInt32OrZero(app, 0);
	const auto field_04 = ReadUInt32OrZero(app, 4);
	const auto field_08 = ReadUInt32OrZero(app, 8);
	const auto field_12 = ReadUInt32OrZero(app, 12);
	const auto is_fellowship_action = app->GetOpcode() == OP_FellowshipUpdate;
	const auto is_invite_response = app->GetOpcode() == OP_FellowshipInvite;
	const auto has_pending_invite = HasPendingInvite(client->CharacterID());
	const auto possible_create = is_fellowship_action && action == kClientActionCreate && app->Size() == kCreatePacketWireSize;
	const auto possible_accept = is_fellowship_action &&
		(
			action == kClientActionAcceptInvite ||
			(has_pending_invite && action != 0 && action != kClientActionCreate && action != kClientActionInviteTarget)
		);
	const auto possible_invite_response_accept = is_invite_response && has_pending_invite;

	if (RuleB(CustomFeatures, FellowshipOpcodeDiscoveryEnabled)) {
		LogInfo(
			"Fellowship client packet character [{}] emu [{}] protocol [{:#06x}] payload_size [{}] wire_size [{}] action [{}] field_04 [{}] field_08 [{}] field_12 [{}] pending_invite [{}] non_zero_bytes [{}] non_zero_offsets [{}]{}{}{} {}",
			client->GetCleanName(),
			OpcodeManager::EmuToName(app->GetOpcode()),
			app->GetProtocolOpcode(),
			app->size,
			app->Size(),
			action,
			field_04,
			field_08,
			field_12,
			has_pending_invite ? "yes" : "no",
			CountNonZeroBytes(app),
			DescribeNonZeroOffsets(app),
			possible_create ? " possible_create" : "",
			possible_accept ? " possible_accept" : "",
			possible_invite_response_accept ? " possible_invite_response_accept" : "",
			DumpPacketToString(app)
		);
	}

	if (possible_create) {
		CreateFellowship(client, "");
	} else if (possible_accept || possible_invite_response_accept) {
		AcceptInvite(client);
	} else if (is_fellowship_action && action == kClientActionInviteTarget) {
		InviteTarget(client, field_04);
	}
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

void FellowshipManager::ProcessClient(Client *client) const
{
	if (
		!RuleB(CustomFeatures, FellowshipsEnabled) ||
		!RuleB(CustomFeatures, FellowshipOpcodeDiscoveryEnabled) ||
		!client ||
		!client->Connected() ||
		!client->CharacterID()
	) {
		return;
	}

	auto session_iter = fellowship_probe_sessions.find(client->CharacterID());
	if (session_iter == fellowship_probe_sessions.end() || !session_iter->second.active) {
		return;
	}

	auto &session = session_iter->second;
	if (session.timer.Check()) {
		SendFellowshipProbe(client, session, false);
	}
}

void FellowshipManager::SendClientState(Client *client) const
{
	if (!RuleB(CustomFeatures, FellowshipsEnabled)) {
		return;
	}

	SendFellowshipState(client);
}

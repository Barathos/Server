/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "achievement_manager.h"

#include "common/seperator.h"
#include "common/skills.h"
#include "common/strings.h"
#include "zone/client.h"
#include "zone/npc.h"
#include "zone/zone.h"
#include "zone/zonedb.h"

#include "fmt/format.h"

#include <cstring>

AchievementManager achievement_manager;

namespace {
	uint32 ToUInt(const char *value)
	{
		return value ? Strings::ToUnsignedInt(value) : 0;
	}

	std::string ObjectiveDisplayName(const std::string &type, const std::string &target_name, uint32 required_count)
	{
		if (!target_name.empty()) {
			return target_name;
		}

		if (Strings::EqualFold(type, "level")) {
			return fmt::format("Reach level {}", required_count);
		}

		return type;
	}

	std::string ProtocolValue(std::string value, size_t maximum_length = 0)
	{
		for (auto &c : value) {
			if (c == '|' || c == '\r' || c == '\n' || c == '\t') {
				c = ' ';
			}
		}

		if (maximum_length && value.size() > maximum_length) {
			if (maximum_length > 3) {
				value.resize(maximum_length - 3);
				value.append("...");
			}
			else {
				value.resize(maximum_length);
			}
		}

		return value;
	}
}

void AchievementManager::HandleCommand(Client *client, const Seperator *sep)
{
	if (!client || !sep) {
		return;
	}

	if (sep->argnum < 1) {
		SendNativeWindow(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "help")) {
		SendHelp(client);
		return;
	}

	if (
		!strcasecmp(sep->arg[1], "window") ||
		!strcasecmp(sep->arg[1], "ui") ||
		!strcasecmp(sep->arg[1], "open") ||
		!strcasecmp(sep->arg[1], "panel")
	) {
		SendNativeWindow(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "native")) {
		auto number_arg = [sep](int argument_index) -> uint32 {
			return sep->argnum >= argument_index && sep->IsNumber(argument_index) ?
				Strings::ToUnsignedInt(sep->arg[argument_index]) :
				0;
		};

		if (sep->argnum < 2) {
			SendNativeWindow(client);
			return;
		}

		if (
			!strcasecmp(sep->arg[2], "show") ||
			!strcasecmp(sep->arg[2], "open") ||
			!strcasecmp(sep->arg[2], "window") ||
			!strcasecmp(sep->arg[2], "status") ||
			!strcasecmp(sep->arg[2], "snapshot") ||
			!strcasecmp(sep->arg[2], "refresh")
		) {
			SendNativeWindow(client, number_arg(3), number_arg(4));
			return;
		}

		if (!strcasecmp(sep->arg[2], "category")) {
			if (sep->argnum < 3 || !sep->IsNumber(3)) {
				client->Message(Chat::White, "Usage: #ach native category [category_id]");
				return;
			}

			SendNativeCategory(client, Strings::ToUnsignedInt(sep->arg[3]), number_arg(4));
			return;
		}

		if (!strcasecmp(sep->arg[2], "detail")) {
			if (sep->argnum < 3 || !sep->IsNumber(3)) {
				client->Message(Chat::White, "Usage: #ach native detail [achievement_id]");
				return;
			}

			client->Message(Chat::White, "ACH|objectives|clear");
			SendNativeDetail(client, Strings::ToUnsignedInt(sep->arg[3]));
			client->Message(Chat::White, "ACH|window|show");
			return;
		}

		if (!strcasecmp(sep->arg[2], "check")) {
			RecheckAutomatic(client);
			SendNativeWindow(client, number_arg(3), number_arg(4));
			return;
		}

		client->Message(Chat::White, "Usage: #ach native [show|refresh|category|detail|check]");
		return;
	}

	if (!strcasecmp(sep->arg[1], "status")) {
		SendStatus(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "categories") || !strcasecmp(sep->arg[1], "list")) {
		SendCategories(client);
		return;
	}

	if (!strcasecmp(sep->arg[1], "category")) {
		if (sep->argnum < 2 || !sep->IsNumber(2)) {
			client->Message(Chat::White, "Usage: #ach category [category_id]");
			return;
		}

		SendCategory(client, Strings::ToUnsignedInt(sep->arg[2]));
		return;
	}

	if (!strcasecmp(sep->arg[1], "detail")) {
		if (sep->argnum < 2 || !sep->IsNumber(2)) {
			client->Message(Chat::White, "Usage: #ach detail [achievement_id]");
			return;
		}

		SendDetail(client, Strings::ToUnsignedInt(sep->arg[2]));
		return;
	}

	if (!strcasecmp(sep->arg[1], "check")) {
		RecheckAutomatic(client);
		SendStatus(client);
		return;
	}

	SendHelp(client);
}

void AchievementManager::ProcessLevel(Client *client)
{
	if (!client) {
		return;
	}

	const uint32 character_id = client->CharacterID();
	const uint32 level = client->GetLevel();

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `id`, `achievement_id`, `required_count` "
			"FROM `custom_achievement_objectives` "
			"WHERE `objective_type` = 'level' AND `required_count` <= {}",
			level
		)
	);

	if (!results.Success()) {
		return;
	}

	for (auto row = results.begin(); row != results.end(); ++row) {
		const uint32 objective_id = ToUInt(row[0]);
		const uint32 achievement_id = ToUInt(row[1]);
		const uint32 required_count = ToUInt(row[2]);

		UpdateObjectiveProgress(character_id, objective_id, level, level >= required_count);
		TryCompleteAchievement(client, achievement_id);
	}
}

void AchievementManager::ProcessZoneVisit(Client *client)
{
	if (!client || !zone) {
		return;
	}

	ProcessMatchedObjectives(
		client,
		fmt::format(
			"o.`objective_type` = 'zone_visit' AND o.`zone_id` = {}",
			zone->GetZoneID()
		),
		1,
		true
	);
}

void AchievementManager::ProcessTaskComplete(Client *client, uint32 task_id)
{
	if (!client || !task_id) {
		return;
	}

	ProcessMatchedObjectives(
		client,
		fmt::format(
			"o.`objective_type` = 'task_complete' AND o.`target_id` = {}",
			task_id
		),
		1,
		true
	);
}

void AchievementManager::ProcessSkill(Client *client, uint32 skill_id, uint32 value)
{
	if (!client) {
		return;
	}

	ProcessMatchedObjectives(
		client,
		fmt::format(
			"o.`objective_type` = 'skill' AND o.`target_id` = {}",
			skill_id
		),
		value,
		true
	);
}

void AchievementManager::ProcessKill(Client *client, NPC *npc)
{
	if (!client || !npc || !zone) {
		return;
	}

	const std::string clean_name = Strings::Escape(Strings::ToLower(npc->GetCleanName()));
	ProcessMatchedObjectives(
		client,
		fmt::format(
			"("
			"(o.`objective_type` = 'npc_kill' AND o.`target_id` = {}) OR "
			"(o.`objective_type` = 'npc_name_kill' AND o.`zone_id` = {} AND LOWER(o.`target_name`) = '{}') OR "
			"(o.`objective_type` = 'zone_kill' AND o.`zone_id` = {}) OR "
			"(o.`objective_type` = 'race_kill' AND o.`target_id` = {}) OR "
			"(o.`objective_type` = 'bodytype_kill' AND o.`target_id` = {})"
			")",
			npc->GetNPCTypeID(),
			zone->GetZoneID(),
			clean_name,
			zone->GetZoneID(),
			npc->GetRace(),
			npc->GetBodyType()
		),
		1,
		false
	);
}

void AchievementManager::SendHelp(Client *client)
{
	client->Message(Chat::White, "Achievement commands:");
	client->Message(Chat::White, "#ach - Open the native achievement window.");
	client->Message(Chat::White, "#ach window - Reopen the native achievement window.");
	client->Message(Chat::White, "#ach status - Show your achievement totals.");
	client->Message(Chat::White, "#ach categories - List achievement categories.");
	client->Message(Chat::White, "#ach category [id] - List achievements in a category.");
	client->Message(Chat::White, "#ach detail [id] - Show achievement objectives.");
	client->Message(Chat::White, "#ach check - Recheck automatic achievements for your character.");
}

void AchievementManager::RecheckAutomatic(Client *client)
{
	if (!client) {
		return;
	}

	ProcessLevel(client);
	ProcessZoneVisit(client);
	for (int skill_id = 0; skill_id <= EQ::skills::HIGHEST_SKILL; ++skill_id) {
		ProcessSkill(client, skill_id, client->GetRawSkill(static_cast<EQ::skills::SkillType>(skill_id)));
	}
}

void AchievementManager::SendStatus(Client *client)
{
	const uint32 character_id = client->CharacterID();

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT "
			"(SELECT COUNT(*) FROM `custom_achievements` WHERE `enabled` = 1 AND `hidden` = 0), "
			"(SELECT COUNT(*) FROM `custom_character_achievements` ca "
			"JOIN `custom_achievements` a ON a.`id` = ca.`achievement_id` "
			"WHERE ca.`character_id` = {} AND a.`enabled` = 1), "
			"(SELECT COALESCE(SUM(a.`points`), 0) FROM `custom_character_achievements` ca "
			"JOIN `custom_achievements` a ON a.`id` = ca.`achievement_id` "
			"WHERE ca.`character_id` = {} AND a.`enabled` = 1), "
			"(SELECT COUNT(*) FROM `custom_achievement_categories` WHERE `enabled` = 1)",
			character_id,
			character_id
		)
	);

	if (!results.Success() || !results.RowCount()) {
		client->Message(Chat::White, "Achievements are not available. Check the custom achievement tables.");
		return;
	}

	auto row = results.begin();
	client->Message(
		Chat::White,
		fmt::format(
			"Achievements: {} / {} complete, {} points, {} categories.",
			ToUInt(row[1]),
			ToUInt(row[0]),
			ToUInt(row[2]),
			ToUInt(row[3])
		).c_str()
	);
}

void AchievementManager::SendCategories(Client *client)
{
	const auto categories = LoadCategorySummaries(client->CharacterID());
	if (categories.empty()) {
		client->Message(Chat::White, "No achievement categories are available.");
		return;
	}

	client->Message(Chat::White, "Achievement Categories:");
	for (const auto &category : categories) {
		client->Message(
			Chat::White,
			fmt::format(
				"[{}] {} - {} / {} complete, {} points",
				category.id,
				category.name,
				category.completed,
				category.total,
				category.points
			).c_str()
		);
	}
}

void AchievementManager::SendCategory(Client *client, uint32 category_id)
{
	const auto achievements = LoadAchievements(client->CharacterID(), category_id);
	if (achievements.empty()) {
		client->Message(Chat::White, fmt::format("No achievements found in category {}.", category_id).c_str());
		return;
	}

	client->Message(Chat::White, fmt::format("Achievements in category {}:", category_id).c_str());
	for (const auto &achievement : achievements) {
		client->Message(
			Chat::White,
			fmt::format(
				"[{}] [{}] {} ({} pts) - {}",
				achievement.id,
				achievement.completed ? "Complete" : "Open",
				achievement.name,
				achievement.points,
				achievement.description
			).c_str()
		);
	}
}

void AchievementManager::SendDetail(Client *client, uint32 achievement_id)
{
	const uint32 character_id = client->CharacterID();

	auto achievement_results = database.QueryDatabase(
		fmt::format(
			"SELECT a.`name`, a.`description`, a.`points`, COALESCE(ca.`completed_at`, 0) "
			"FROM `custom_achievements` a "
			"LEFT JOIN `custom_character_achievements` ca "
			"ON ca.`achievement_id` = a.`id` AND ca.`character_id` = {} "
			"WHERE a.`id` = {} AND a.`enabled` = 1 LIMIT 1",
			character_id,
			achievement_id
		)
	);

	if (!achievement_results.Success() || !achievement_results.RowCount()) {
		client->Message(Chat::White, fmt::format("Achievement {} was not found.", achievement_id).c_str());
		return;
	}

	auto achievement = achievement_results.begin();
	client->Message(
		Chat::White,
		fmt::format(
			"[{}] {} ({} pts) - {}",
			achievement_id,
			achievement[0] ? achievement[0] : "",
			ToUInt(achievement[2]),
			achievement[1] ? achievement[1] : ""
		).c_str()
	);

	if (ToUInt(achievement[3])) {
		client->Message(Chat::White, fmt::format("Completed at Unix time {}.", ToUInt(achievement[3])).c_str());
	}

	const auto objectives = LoadObjectives(character_id, achievement_id);
	for (const auto &objective : objectives) {
		client->Message(
			Chat::White,
			fmt::format(
				" - [{}] {} ({}/{})",
				objective.completed ? "Done" : "Open",
				ObjectiveDisplayName(objective.type, objective.target_name, objective.required_count),
				objective.current_count,
				objective.required_count
			).c_str()
		);
	}
}

void AchievementManager::SendNativeWindow(Client *client, uint32 category_id, uint32 achievement_id)
{
	if (!client) {
		return;
	}

	const uint32 character_id = client->CharacterID();
	uint32 total = 0;
	uint32 completed = 0;
	uint32 points = 0;
	uint32 category_count = 0;

	auto status_results = database.QueryDatabase(
		fmt::format(
			"SELECT "
			"(SELECT COUNT(*) FROM `custom_achievements` WHERE `enabled` = 1 AND `hidden` = 0), "
			"(SELECT COUNT(*) FROM `custom_character_achievements` ca "
			"JOIN `custom_achievements` a ON a.`id` = ca.`achievement_id` "
			"WHERE ca.`character_id` = {} AND a.`enabled` = 1), "
			"(SELECT COALESCE(SUM(a.`points`), 0) FROM `custom_character_achievements` ca "
			"JOIN `custom_achievements` a ON a.`id` = ca.`achievement_id` "
			"WHERE ca.`character_id` = {} AND a.`enabled` = 1), "
			"(SELECT COUNT(*) FROM `custom_achievement_categories` WHERE `enabled` = 1)",
			character_id,
			character_id
		)
	);

	if (status_results.Success() && status_results.RowCount()) {
		auto row = status_results.begin();
		total = ToUInt(row[0]);
		completed = ToUInt(row[1]);
		points = ToUInt(row[2]);
		category_count = ToUInt(row[3]);
	}

	if (achievement_id && !category_id) {
		category_id = LoadAchievementCategory(achievement_id);
	}

	auto categories = LoadCategorySummaries(character_id);
	uint32 selected_category_id = 0;
	for (const auto &category : categories) {
		if (category.id == category_id) {
			selected_category_id = category.id;
			break;
		}
	}

	if (!selected_category_id) {
		for (const auto &category : categories) {
			if (category.total > 0) {
				selected_category_id = category.id;
				break;
			}
		}
	}

	if (!selected_category_id && !categories.empty()) {
		selected_category_id = categories.front().id;
	}

	auto achievements = selected_category_id ? LoadAchievements(character_id, selected_category_id) : std::vector<AchievementSummary>();
	uint32 selected_achievement_id = 0;
	if (achievement_id) {
		for (const auto &achievement : achievements) {
			if (achievement.id == achievement_id) {
				selected_achievement_id = achievement.id;
				break;
			}
		}
	}

	if (!selected_achievement_id && !achievements.empty()) {
		selected_achievement_id = achievements.front().id;
	}

	client->Message(Chat::White, "ACH|window|clear");
	client->Message(
		Chat::White,
		fmt::format(
			"ACH|status|completed={}|total={}|points={}|categories={}",
			completed,
			total,
			points,
			category_count
		).c_str()
	);

	for (const auto &category : categories) {
		client->Message(
			Chat::White,
			fmt::format(
				"ACH|category|id={}|parent={}|name={}|completed={}|total={}|points={}",
				category.id,
				category.parent_id,
				ProtocolValue(category.name, 80),
				category.completed,
				category.total,
				category.points
			).c_str()
		);
	}

	client->Message(
		Chat::White,
		fmt::format(
			"ACH|selection|category={}|achievement={}",
			selected_category_id,
			selected_achievement_id
		).c_str()
	);

	for (const auto &achievement : achievements) {
		client->Message(
			Chat::White,
			fmt::format(
				"ACH|achievement|id={}|category={}|completed={}|points={}|name={}|description={}",
				achievement.id,
				achievement.category_id,
				achievement.completed ? 1 : 0,
				achievement.points,
				ProtocolValue(achievement.name, 96),
				ProtocolValue(achievement.description, 160)
			).c_str()
		);
	}

	if (selected_achievement_id) {
		SendNativeDetail(client, selected_achievement_id);
	}
	else {
		client->Message(Chat::White, "ACH|detail|id=0|completed=0|points=0|name=No achievements|description=No achievements are available for this category.");
	}

	client->Message(Chat::White, "ACH|window|show");
}

void AchievementManager::SendNativeCategory(Client *client, uint32 category_id, uint32 achievement_id)
{
	if (!client) {
		return;
	}

	const uint32 character_id = client->CharacterID();
	const auto achievements = category_id ? LoadAchievements(character_id, category_id) : std::vector<AchievementSummary>();
	uint32 selected_achievement_id = 0;
	if (achievement_id) {
		for (const auto &achievement : achievements) {
			if (achievement.id == achievement_id) {
				selected_achievement_id = achievement.id;
				break;
			}
		}
	}

	if (!selected_achievement_id && !achievements.empty()) {
		selected_achievement_id = achievements.front().id;
	}

	client->Message(Chat::White, "ACH|achievements|clear");
	client->Message(
		Chat::White,
		fmt::format(
			"ACH|selection|category={}|achievement={}",
			category_id,
			selected_achievement_id
		).c_str()
	);

	for (const auto &achievement : achievements) {
		client->Message(
			Chat::White,
			fmt::format(
				"ACH|achievement|id={}|category={}|completed={}|points={}|name={}|description={}",
				achievement.id,
				achievement.category_id,
				achievement.completed ? 1 : 0,
				achievement.points,
				ProtocolValue(achievement.name, 96),
				ProtocolValue(achievement.description, 160)
			).c_str()
		);
	}

	if (selected_achievement_id) {
		SendNativeDetail(client, selected_achievement_id);
	}
	else {
		client->Message(Chat::White, "ACH|detail|id=0|completed=0|points=0|name=No achievements|description=No achievements are available for this category.");
	}

	client->Message(Chat::White, "ACH|window|show");
}

void AchievementManager::SendNativeDetail(Client *client, uint32 achievement_id)
{
	if (!client) {
		return;
	}

	const uint32 character_id = client->CharacterID();
	auto achievement_results = database.QueryDatabase(
		fmt::format(
			"SELECT a.`name`, a.`description`, a.`points`, COALESCE(ca.`completed_at`, 0) "
			"FROM `custom_achievements` a "
			"LEFT JOIN `custom_character_achievements` ca "
			"ON ca.`achievement_id` = a.`id` AND ca.`character_id` = {} "
			"WHERE a.`id` = {} AND a.`enabled` = 1 LIMIT 1",
			character_id,
			achievement_id
		)
	);

	if (!achievement_results.Success() || !achievement_results.RowCount()) {
		client->Message(
			Chat::White,
			fmt::format(
				"ACH|detail|id={}|completed=0|points=0|name=Achievement unavailable|description=Achievement {} was not found.",
				achievement_id,
				achievement_id
			).c_str()
		);
		return;
	}

	auto achievement = achievement_results.begin();
	const uint32 completed_at = ToUInt(achievement[3]);
	client->Message(
		Chat::White,
		fmt::format(
			"ACH|detail|id={}|completed={}|completed_at={}|points={}|name={}|description={}",
			achievement_id,
			completed_at ? 1 : 0,
			completed_at,
			ToUInt(achievement[2]),
			ProtocolValue(achievement[0] ? achievement[0] : "", 96),
			ProtocolValue(achievement[1] ? achievement[1] : "", 260)
		).c_str()
	);

	const auto objectives = LoadObjectives(character_id, achievement_id);
	for (const auto &objective : objectives) {
		client->Message(
			Chat::White,
			fmt::format(
				"ACH|objective|id={}|completed={}|current={}|required={}|name={}",
				objective.id,
				objective.completed ? 1 : 0,
				objective.current_count,
				objective.required_count,
				ProtocolValue(ObjectiveDisplayName(objective.type, objective.target_name, objective.required_count), 120)
			).c_str()
		);
	}
}

std::vector<AchievementManager::CategorySummary> AchievementManager::LoadCategorySummaries(uint32 character_id)
{
	std::vector<CategorySummary> categories;

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT c.`id`, c.`parent_id`, c.`name`, "
			"COUNT(a.`id`) AS total_count, "
			"COALESCE(SUM(CASE WHEN ca.`achievement_id` IS NULL THEN 0 ELSE 1 END), 0) AS completed_count, "
			"COALESCE(SUM(CASE WHEN ca.`achievement_id` IS NULL THEN 0 ELSE a.`points` END), 0) AS points "
			"FROM `custom_achievement_categories` c "
			"LEFT JOIN `custom_achievements` a ON a.`category_id` = c.`id` AND a.`enabled` = 1 AND a.`hidden` = 0 "
			"LEFT JOIN `custom_character_achievements` ca ON ca.`achievement_id` = a.`id` AND ca.`character_id` = {} "
			"WHERE c.`enabled` = 1 "
			"GROUP BY c.`id`, c.`parent_id`, c.`name`, c.`sort_order` "
			"ORDER BY c.`sort_order`, c.`id`",
			character_id
		)
	);

	if (!results.Success()) {
		return categories;
	}

	for (auto row = results.begin(); row != results.end(); ++row) {
		CategorySummary summary;
		summary.id = ToUInt(row[0]);
		summary.parent_id = ToUInt(row[1]);
		summary.name = row[2] ? row[2] : "";
		summary.total = ToUInt(row[3]);
		summary.completed = ToUInt(row[4]);
		summary.points = ToUInt(row[5]);
		categories.push_back(summary);
	}

	return categories;
}

std::vector<AchievementManager::AchievementSummary> AchievementManager::LoadAchievements(uint32 character_id, uint32 category_id)
{
	std::vector<AchievementSummary> achievements;

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT a.`id`, a.`category_id`, a.`name`, a.`description`, a.`points`, COALESCE(ca.`completed_at`, 0) "
			"FROM `custom_achievements` a "
			"LEFT JOIN `custom_character_achievements` ca "
			"ON ca.`achievement_id` = a.`id` AND ca.`character_id` = {} "
			"WHERE a.`enabled` = 1 AND a.`hidden` = 0 AND a.`category_id` = {} "
			"ORDER BY a.`sort_order`, a.`id`",
			character_id,
			category_id
		)
	);

	if (!results.Success()) {
		return achievements;
	}

	for (auto row = results.begin(); row != results.end(); ++row) {
		AchievementSummary summary;
		summary.id = ToUInt(row[0]);
		summary.category_id = ToUInt(row[1]);
		summary.name = row[2] ? row[2] : "";
		summary.description = row[3] ? row[3] : "";
		summary.points = ToUInt(row[4]);
		summary.completed_at = ToUInt(row[5]);
		summary.completed = summary.completed_at != 0;
		achievements.push_back(summary);
	}

	return achievements;
}

std::vector<AchievementManager::ObjectiveSummary> AchievementManager::LoadObjectives(uint32 character_id, uint32 achievement_id)
{
	std::vector<ObjectiveSummary> objectives;

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT o.`id`, o.`objective_type`, o.`target_name`, o.`required_count`, "
			"COALESCE(p.`count`, 0), COALESCE(p.`completed_at`, 0) "
			"FROM `custom_achievement_objectives` o "
			"LEFT JOIN `custom_character_achievement_progress` p "
			"ON p.`objective_id` = o.`id` AND p.`character_id` = {} "
			"WHERE o.`achievement_id` = {} "
			"ORDER BY o.`objective_index`, o.`id`",
			character_id,
			achievement_id
		)
	);

	if (!results.Success()) {
		return objectives;
	}

	for (auto row = results.begin(); row != results.end(); ++row) {
		ObjectiveSummary summary;
		summary.id = ToUInt(row[0]);
		summary.type = row[1] ? row[1] : "";
		summary.target_name = row[2] ? row[2] : "";
		summary.required_count = ToUInt(row[3]);
		summary.current_count = ToUInt(row[4]);
		summary.completed = ToUInt(row[5]) != 0;
		objectives.push_back(summary);
	}

	return objectives;
}

uint32 AchievementManager::LoadAchievementCategory(uint32 achievement_id)
{
	if (!achievement_id) {
		return 0;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `category_id` FROM `custom_achievements` "
			"WHERE `id` = {} AND `enabled` = 1 LIMIT 1",
			achievement_id
		)
	);

	if (!results.Success() || !results.RowCount()) {
		return 0;
	}

	auto row = results.begin();
	return ToUInt(row[0]);
}

void AchievementManager::ProcessMatchedObjectives(Client *client, const std::string &match_sql, uint32 progress, bool absolute_progress)
{
	if (!client || match_sql.empty()) {
		return;
	}

	const uint32 character_id = client->CharacterID();
	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT o.`id`, o.`achievement_id`, o.`required_count`, "
			"COALESCE(p.`count`, 0), COALESCE(p.`completed_at`, 0) "
			"FROM `custom_achievement_objectives` o "
			"JOIN `custom_achievements` a ON a.`id` = o.`achievement_id` AND a.`enabled` = 1 "
			"LEFT JOIN `custom_character_achievement_progress` p "
			"ON p.`objective_id` = o.`id` AND p.`character_id` = {} "
			"WHERE {}",
			character_id,
			match_sql
		)
	);

	if (!results.Success()) {
		return;
	}

	for (auto row = results.begin(); row != results.end(); ++row) {
		const uint32 objective_id = ToUInt(row[0]);
		const uint32 achievement_id = ToUInt(row[1]);
		const uint32 required_count = ToUInt(row[2]);
		const uint32 current_count = ToUInt(row[3]);
		const bool already_completed = ToUInt(row[4]) != 0;
		const uint32 new_count = absolute_progress ? progress : current_count + progress;

		if (!already_completed || new_count > current_count) {
			UpdateObjectiveProgress(character_id, objective_id, new_count, new_count >= required_count);
		}

		TryCompleteAchievement(client, achievement_id);
	}
}

void AchievementManager::UpdateObjectiveProgress(uint32 character_id, uint32 objective_id, uint32 count, bool completed)
{
	database.QueryDatabase(
		fmt::format(
			"INSERT INTO `custom_character_achievement_progress` "
			"(`character_id`, `objective_id`, `count`, `completed_at`, `updated_at`) "
			"VALUES ({}, {}, {}, {}, UNIX_TIMESTAMP()) "
			"ON DUPLICATE KEY UPDATE "
			"`count` = GREATEST(`count`, VALUES(`count`)), "
			"`completed_at` = IF(`completed_at` = 0 AND VALUES(`completed_at`) > 0, VALUES(`completed_at`), `completed_at`), "
			"`updated_at` = VALUES(`updated_at`)",
			character_id,
			objective_id,
			count,
			completed ? "UNIX_TIMESTAMP()" : "0"
		)
	);
}

void AchievementManager::TryCompleteAchievement(Client *client, uint32 achievement_id)
{
	const uint32 character_id = client->CharacterID();
	if (HasCompletedAchievement(character_id, achievement_id)) {
		return;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT "
			"COUNT(*) AS required_count, "
			"COALESCE(SUM(CASE WHEN p.`completed_at` > 0 THEN 1 ELSE 0 END), 0) AS completed_count "
			"FROM `custom_achievement_objectives` o "
			"LEFT JOIN `custom_character_achievement_progress` p "
			"ON p.`objective_id` = o.`id` AND p.`character_id` = {} "
			"WHERE o.`achievement_id` = {} AND o.`optional` = 0",
			character_id,
			achievement_id
		)
	);

	if (!results.Success() || !results.RowCount()) {
		return;
	}

	auto row = results.begin();
	const uint32 required_count = ToUInt(row[0]);
	const uint32 completed_count = ToUInt(row[1]);
	if (!required_count || completed_count < required_count) {
		return;
	}

	database.QueryDatabase(
		fmt::format(
			"INSERT INTO `custom_character_achievements` "
			"(`character_id`, `achievement_id`, `completed_at`, `awarded_at`, `completion_count`, `announced`) "
			"VALUES ({}, {}, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), 1, 0)",
			character_id,
			achievement_id
		)
	);

	auto achievement = database.QueryDatabase(
		fmt::format(
			"SELECT `name`, `description`, `points` FROM `custom_achievements` WHERE `id` = {} LIMIT 1",
			achievement_id
		)
	);

	if (achievement.Success() && achievement.RowCount()) {
		auto achievement_row = achievement.begin();
		client->Message(
			Chat::Yellow,
			fmt::format(
				"Achievement complete: {} ({} pts) - {}",
				achievement_row[0] ? achievement_row[0] : "",
				ToUInt(achievement_row[2]),
				achievement_row[1] ? achievement_row[1] : ""
			).c_str()
		);
	}
}

bool AchievementManager::HasCompletedAchievement(uint32 character_id, uint32 achievement_id)
{
	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT 1 FROM `custom_character_achievements` "
			"WHERE `character_id` = {} AND `achievement_id` = {} LIMIT 1",
			character_id,
			achievement_id
		)
	);

	return results.Success() && results.RowCount() > 0;
}

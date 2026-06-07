#include "../client.h"

#include "common/rulesys.h"
#include "common/seperator.h"
#include "common/skills.h"
#include "common/strings.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace {

enum class AutoSkillCommandMode
{
	Status,
	Enable,
	Disable
};

std::string NormalizeSkillName(const std::string &name)
{
	auto normalized = Strings::ToLower(name);
	Strings::FindReplace(normalized, " ", "");
	return normalized;
}

std::map<std::string, EQ::skills::SkillType> BuildSkillLookup()
{
	std::map<std::string, EQ::skills::SkillType> lookup;

	for (const auto skill_id : Client::GetAvailableAutoSkills()) {
		const auto skill_name = EQ::skills::GetSkillName(skill_id);
		lookup[Strings::ToLower(skill_name)] = skill_id;
		lookup[NormalizeSkillName(skill_name)] = skill_id;
		lookup[std::to_string(static_cast<int>(skill_id))] = skill_id;
	}

	return lookup;
}

std::pair<AutoSkillCommandMode, bool> ParseMode(const std::string &argument)
{
	const auto value = Strings::ToLower(argument);

	if (value == "enable" || value == "on" || value == "true" || value == "1" || value == "yes") {
		return {AutoSkillCommandMode::Enable, true};
	}

	if (value == "disable" || value == "off" || value == "false" || value == "0" || value == "no" || value == "disabled") {
		return {AutoSkillCommandMode::Disable, true};
	}

	if (value == "status") {
		return {AutoSkillCommandMode::Status, true};
	}

	return {AutoSkillCommandMode::Status, false};
}

std::string ExtractSkillArgument(const Seperator *sep, bool has_mode)
{
	std::string skill_argument;
	const int last_skill_arg = has_mode ? sep->argnum - 1 : sep->argnum;

	for (int i = 1; i <= last_skill_arg; ++i) {
		if (!skill_argument.empty()) {
			skill_argument += " ";
		}
		skill_argument += sep->arg[i];
	}

	return skill_argument;
}

void ShowUsage(Client *client)
{
	client->Message(Chat::Skills, "Usage: #autoskill list");
	client->Message(Chat::Skills, "Usage: #autoskill <skill id or name> [enable|disable|status]");
}

void ShowSkillList(Client *client)
{
	client->Message(Chat::Skills, "Available auto-skills:");

	auto skills = client->GetAutoSkillsList();
	std::sort(skills.begin(), skills.end());

	if (skills.empty()) {
		client->Message(Chat::Skills, "You do not currently have any skills available for auto-skill.");
		return;
	}

	for (const auto skill_id : skills) {
		client->Message(
			Chat::Skills,
			"%s (ID: %d) - %s",
			EQ::skills::GetSkillName(skill_id).c_str(),
			static_cast<int>(skill_id),
			client->GetAutoSkillStatus(skill_id) ? "enabled" : "disabled"
		);
	}
}

} // namespace

void command_autoskill(Client *c, const Seperator *sep)
{
	if (!c || !sep) {
		return;
	}

	if (!RuleB(CustomFeatures, AutoskillsEnabled)) {
		c->Message(Chat::Skills, "Auto-skills are disabled on this server.");
		return;
	}

	if (sep->argnum < 1 || !sep->arg[1][0]) {
		ShowUsage(c);
		return;
	}

	if (Strings::EqualFold(sep->arg[1], "list")) {
		ShowSkillList(c);
		return;
	}

	static const auto skill_lookup = BuildSkillLookup();

	const auto [mode, has_mode] = ParseMode(sep->arg[sep->argnum]);
	const auto skill_argument = ExtractSkillArgument(sep, has_mode);
	const auto lookup_key = Strings::IsNumber(skill_argument) ? skill_argument : NormalizeSkillName(skill_argument);
	const auto skill_iter = skill_lookup.find(lookup_key);

	if (skill_iter == skill_lookup.end()) {
		c->Message(Chat::Skills, "Autoskill configuration failed. Invalid skill name or ID.");
		ShowUsage(c);
		return;
	}

	const auto skill_id = skill_iter->second;
	if (!c->HasSkill(skill_id)) {
		c->Message(Chat::Skills, "Autoskill configuration failed. You do not have that skill.");
		return;
	}

	const auto skill_name = EQ::skills::GetSkillName(skill_id);
	switch (mode) {
		case AutoSkillCommandMode::Enable:
			c->SetAutoSkillStatus(skill_id, true);
			c->Message(Chat::Skills, "Auto-skill %s (%d) enabled.", skill_name.c_str(), static_cast<int>(skill_id));
			break;
		case AutoSkillCommandMode::Disable:
			c->SetAutoSkillStatus(skill_id, false);
			c->Message(Chat::Skills, "Auto-skill %s (%d) disabled.", skill_name.c_str(), static_cast<int>(skill_id));
			break;
		case AutoSkillCommandMode::Status:
		default:
			c->Message(
				Chat::Skills,
				"Auto-skill: %s (ID: %d) is currently %s.",
				skill_name.c_str(),
				static_cast<int>(skill_id),
				c->GetAutoSkillStatus(skill_id) ? "enabled" : "disabled"
			);
			break;
	}
}

/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "common/rulesys.h"
#include "zone/client.h"

namespace {
	void SendFeatureStatus(Client *c, const char *name, bool enabled)
	{
		c->Message(Chat::White, "%s: %s", name, enabled ? "enabled" : "disabled");
	}
}

void command_customfeatures(Client *c, const Seperator *)
{
	c->Message(Chat::White, "All Features rule gates:");
	SendFeatureStatus(c, "Multiclass", RuleB(CustomFeatures, MulticlassEnabled));
	SendFeatureStatus(c, "Achievements", RuleB(CustomFeatures, AchievementsEnabled));
	SendFeatureStatus(c, "Advanced Loot", RuleB(CustomFeatures, AutoLootEnabled));
	SendFeatureStatus(c, "GearScore", RuleB(CustomFeatures, GearScoreEnabled));
	SendFeatureStatus(c, "Live Items", RuleB(CustomFeatures, LiveItemsEnabled));
	SendFeatureStatus(c, "Live Spells", RuleB(CustomFeatures, LiveSpellsEnabled));
	SendFeatureStatus(c, "Item Rarity", RuleB(CustomFeatures, ItemRarityEnabled));
	SendFeatureStatus(c, "HPFIX", RuleB(CustomFeatures, HpFixEnabled));
	SendFeatureStatus(c, "AI Dialogue", RuleB(CustomFeatures, AiDialogueEnabled));
	SendFeatureStatus(c, "Tradeskills", RuleB(CustomFeatures, TradeskillsEnabled));
	SendFeatureStatus(c, "Augs-in-Augs", RuleB(CustomFeatures, AugsInAugsEnabled));
	SendFeatureStatus(c, "Dynamic Quests", RuleB(CustomFeatures, DynamicQuestsEnabled));
	SendFeatureStatus(c, "MQ Interface", RuleB(CustomFeatures, MqInterfaceEnabled));
	SendFeatureStatus(c, "Pet Bags", RuleB(CustomFeatures, PetBagsEnabled));
	SendFeatureStatus(c, "Faction Window", RuleB(CustomFeatures, FactionWindowEnabled));
	SendFeatureStatus(c, "Faction Show All", RuleB(CustomFeatures, FactionWindowShowAllByDefault));
	SendFeatureStatus(c, "DPS Parser", RuleB(CustomFeatures, DpsParserEnabled));
	SendFeatureStatus(c, "Improved Autofollow", RuleB(CustomFeatures, ImprovedAutoFollowEnabled));
	SendFeatureStatus(c, "UseItem Command", RuleB(CustomFeatures, UseItemCommandEnabled));
	SendFeatureStatus(c, "Autoskills", RuleB(CustomFeatures, AutoskillsEnabled));
	SendFeatureStatus(c, "Server Auth Stats", RuleB(CustomFeatures, ServerAuthStatsEnabled));
	SendFeatureStatus(c, "Fellowships", RuleB(CustomFeatures, FellowshipsEnabled));
	SendFeatureStatus(c, "Fellowship Opcode Discovery", RuleB(CustomFeatures, FellowshipOpcodeDiscoveryEnabled));
}

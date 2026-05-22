/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "common/say_link.h"
#include "common/strings.h"
#include "zone/client.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace {
	enum class ItemEditFieldType {
		Number,
		Text
	};

	struct ItemEditField {
		const char *key;
		const char *label;
		ItemEditFieldType type;
	};

	const std::vector<ItemEditField> &GetItemEditFields()
	{
		static const std::vector<ItemEditField> fields = {
			{"name", "Name", ItemEditFieldType::Text},
			{"lore", "Lore", ItemEditFieldType::Text},
			{"idfile", "IDFile", ItemEditFieldType::Text},
			{"icon", "Icon", ItemEditFieldType::Number},
			{"color", "Color", ItemEditFieldType::Number},
			{"itemtype", "Item Type", ItemEditFieldType::Number},
			{"slots", "Slots", ItemEditFieldType::Number},
			{"classes", "Classes", ItemEditFieldType::Number},
			{"races", "Races", ItemEditFieldType::Number},
			{"reqlevel", "Req Level", ItemEditFieldType::Number},
			{"reclevel", "Rec Level", ItemEditFieldType::Number},
			{"damage", "Damage", ItemEditFieldType::Number},
			{"delay", "Delay", ItemEditFieldType::Number},
			{"ac", "AC", ItemEditFieldType::Number},
			{"hp", "HP", ItemEditFieldType::Number},
			{"mana", "Mana", ItemEditFieldType::Number},
			{"endur", "Endur", ItemEditFieldType::Number},
			{"haste", "Haste", ItemEditFieldType::Number},
			{"attack", "Attack", ItemEditFieldType::Number},
			{"accuracy", "Accuracy", ItemEditFieldType::Number},
			{"avoidance", "Avoidance", ItemEditFieldType::Number},
			{"shielding", "Shielding", ItemEditFieldType::Number},
			{"spellshield", "Spell Shield", ItemEditFieldType::Number},
			{"dotshielding", "DoT Shielding", ItemEditFieldType::Number},
			{"damageshield", "Damage Shield", ItemEditFieldType::Number},
			{"healamt", "Heal Amt", ItemEditFieldType::Number},
			{"spelldmg", "Spell Dmg", ItemEditFieldType::Number},
			{"str", "STR", ItemEditFieldType::Number},
			{"sta", "STA", ItemEditFieldType::Number},
			{"agi", "AGI", ItemEditFieldType::Number},
			{"dex", "DEX", ItemEditFieldType::Number},
			{"int", "INT", ItemEditFieldType::Number},
			{"wis", "WIS", ItemEditFieldType::Number},
			{"cha", "CHA", ItemEditFieldType::Number},
			{"mr", "MR", ItemEditFieldType::Number},
			{"fr", "FR", ItemEditFieldType::Number},
			{"cr", "CR", ItemEditFieldType::Number},
			{"dr", "DR", ItemEditFieldType::Number},
			{"pr", "PR", ItemEditFieldType::Number},
			{"svcorruption", "Corruption", ItemEditFieldType::Number},
			{"heroicstr", "Heroic STR", ItemEditFieldType::Number},
			{"heroicsta", "Heroic STA", ItemEditFieldType::Number},
			{"heroicagi", "Heroic AGI", ItemEditFieldType::Number},
			{"heroicdex", "Heroic DEX", ItemEditFieldType::Number},
			{"heroicint", "Heroic INT", ItemEditFieldType::Number},
			{"heroicwis", "Heroic WIS", ItemEditFieldType::Number},
			{"heroiccha", "Heroic CHA", ItemEditFieldType::Number},
			{"heroicmr", "Heroic MR", ItemEditFieldType::Number},
			{"heroicfr", "Heroic FR", ItemEditFieldType::Number},
			{"heroiccr", "Heroic CR", ItemEditFieldType::Number},
			{"heroicdr", "Heroic DR", ItemEditFieldType::Number},
			{"heroicpr", "Heroic PR", ItemEditFieldType::Number},
			{"heroicsvcorruption", "Heroic Corr", ItemEditFieldType::Number},
			{"proceffect", "Proc Effect", ItemEditFieldType::Number},
			{"proctype", "Proc Type", ItemEditFieldType::Number},
			{"proclevel", "Proc Level", ItemEditFieldType::Number},
			{"proclevel2", "Proc Level2", ItemEditFieldType::Number},
			{"procrate", "Proc Rate", ItemEditFieldType::Number},
			{"procname", "Proc Name", ItemEditFieldType::Text},
			{"worneffect", "Worn Effect", ItemEditFieldType::Number},
			{"worntype", "Worn Type", ItemEditFieldType::Number},
			{"wornlevel", "Worn Level", ItemEditFieldType::Number},
			{"wornlevel2", "Worn Level2", ItemEditFieldType::Number},
			{"wornname", "Worn Name", ItemEditFieldType::Text},
			{"focuseffect", "Focus Effect", ItemEditFieldType::Number},
			{"focustype", "Focus Type", ItemEditFieldType::Number},
			{"focuslevel", "Focus Level", ItemEditFieldType::Number},
			{"focuslevel2", "Focus Level2", ItemEditFieldType::Number},
			{"focusname", "Focus Name", ItemEditFieldType::Text},
			{"clickeffect", "Click Effect", ItemEditFieldType::Number},
			{"clicktype", "Click Type", ItemEditFieldType::Number},
			{"clicklevel", "Click Level", ItemEditFieldType::Number},
			{"clicklevel2", "Click Level2", ItemEditFieldType::Number},
			{"clickname", "Click Name", ItemEditFieldType::Text},
			{"scrolleffect", "Scroll Effect", ItemEditFieldType::Number},
			{"scrolltype", "Scroll Type", ItemEditFieldType::Number},
			{"scrolllevel", "Scroll Level", ItemEditFieldType::Number},
			{"scrolllevel2", "Scroll Level2", ItemEditFieldType::Number},
			{"scrollname", "Scroll Name", ItemEditFieldType::Text}
		};

		return fields;
	}

	std::string NormalizeFieldName(std::string field)
	{
		field = Strings::ToLower(field);
		field.erase(
			std::remove_if(
				field.begin(),
				field.end(),
				[](const char c) {
					return c == '_' || c == '-' || c == ' ' || c == '.';
				}
			),
			field.end()
		);
		return field;
	}

	const ItemEditField *FindItemEditField(const std::string &field)
	{
		const auto normalized = NormalizeFieldName(field);
		for (const auto &entry : GetItemEditFields()) {
			if (normalized == entry.key) {
				return &entry;
			}
		}

		return nullptr;
	}

	int64 GetItemEditNumber(const EQ::ItemData *item, const std::string &field)
	{
		if (!item) {
			return 0;
		}

		if (field == "icon") { return item->Icon; }
		if (field == "color") { return item->Color; }
		if (field == "itemtype") { return item->ItemType; }
		if (field == "slots") { return item->Slots; }
		if (field == "classes") { return item->Classes; }
		if (field == "races") { return item->Races; }
		if (field == "reqlevel") { return item->ReqLevel; }
		if (field == "reclevel") { return item->RecLevel; }
		if (field == "damage") { return item->Damage; }
		if (field == "delay") { return item->Delay; }
		if (field == "ac") { return item->AC; }
		if (field == "hp") { return item->HP; }
		if (field == "mana") { return item->Mana; }
		if (field == "endur") { return item->Endur; }
		if (field == "haste") { return item->Haste; }
		if (field == "attack") { return item->Attack; }
		if (field == "accuracy") { return item->Accuracy; }
		if (field == "avoidance") { return item->Avoidance; }
		if (field == "shielding") { return item->Shielding; }
		if (field == "spellshield") { return item->SpellShield; }
		if (field == "dotshielding") { return item->DotShielding; }
		if (field == "damageshield") { return item->DamageShield; }
		if (field == "healamt") { return item->HealAmt; }
		if (field == "spelldmg") { return item->SpellDmg; }
		if (field == "str") { return item->AStr; }
		if (field == "sta") { return item->ASta; }
		if (field == "agi") { return item->AAgi; }
		if (field == "dex") { return item->ADex; }
		if (field == "int") { return item->AInt; }
		if (field == "wis") { return item->AWis; }
		if (field == "cha") { return item->ACha; }
		if (field == "mr") { return item->MR; }
		if (field == "fr") { return item->FR; }
		if (field == "cr") { return item->CR; }
		if (field == "dr") { return item->DR; }
		if (field == "pr") { return item->PR; }
		if (field == "svcorruption") { return item->SVCorruption; }
		if (field == "heroicstr") { return item->HeroicStr; }
		if (field == "heroicsta") { return item->HeroicSta; }
		if (field == "heroicagi") { return item->HeroicAgi; }
		if (field == "heroicdex") { return item->HeroicDex; }
		if (field == "heroicint") { return item->HeroicInt; }
		if (field == "heroicwis") { return item->HeroicWis; }
		if (field == "heroiccha") { return item->HeroicCha; }
		if (field == "heroicmr") { return item->HeroicMR; }
		if (field == "heroicfr") { return item->HeroicFR; }
		if (field == "heroiccr") { return item->HeroicCR; }
		if (field == "heroicdr") { return item->HeroicDR; }
		if (field == "heroicpr") { return item->HeroicPR; }
		if (field == "heroicsvcorruption") { return item->HeroicSVCorrup; }
		if (field == "proceffect") { return item->Proc.Effect; }
		if (field == "proctype") { return item->Proc.Type; }
		if (field == "proclevel") { return item->Proc.Level; }
		if (field == "proclevel2") { return item->Proc.Level2; }
		if (field == "procrate") { return item->ProcRate; }
		if (field == "worneffect") { return item->Worn.Effect; }
		if (field == "worntype") { return item->Worn.Type; }
		if (field == "wornlevel") { return item->Worn.Level; }
		if (field == "wornlevel2") { return item->Worn.Level2; }
		if (field == "focuseffect") { return item->Focus.Effect; }
		if (field == "focustype") { return item->Focus.Type; }
		if (field == "focuslevel") { return item->Focus.Level; }
		if (field == "focuslevel2") { return item->Focus.Level2; }
		if (field == "clickeffect") { return item->Click.Effect; }
		if (field == "clicktype") { return item->Click.Type; }
		if (field == "clicklevel") { return item->Click.Level; }
		if (field == "clicklevel2") { return item->Click.Level2; }
		if (field == "scrolleffect") { return item->Scroll.Effect; }
		if (field == "scrolltype") { return item->Scroll.Type; }
		if (field == "scrolllevel") { return item->Scroll.Level; }
		if (field == "scrolllevel2") { return item->Scroll.Level2; }

		return 0;
	}

	std::string GetItemEditText(const EQ::ItemData *item, const std::string &field)
	{
		if (!item) {
			return "";
		}

		if (field == "name") { return item->Name; }
		if (field == "lore") { return item->Lore; }
		if (field == "idfile") { return item->IDFile; }
		if (field == "procname") { return item->ProcName; }
		if (field == "wornname") { return item->WornName; }
		if (field == "focusname") { return item->FocusName; }
		if (field == "clickname") { return item->ClickName; }
		if (field == "scrollname") { return item->ScrollName; }

		return "";
	}

	std::string DynamicItemSetKey(const std::string &field)
	{
		return fmt::format("dynamic_item.set.{}", field);
	}

	std::string DynamicItemModKey(const std::string &field)
	{
		return fmt::format("dynamic_item.mod.{}", field);
	}

	EQ::ItemInstance *GetCursorItem(Client *c)
	{
		return c ? c->GetInv().GetItem(EQ::invslot::slotCursor) : nullptr;
	}

	void SaveCursorEdit(Client *c, EQ::ItemInstance *inst)
	{
		c->SendItemPacket(EQ::invslot::slotCursor, inst, ItemPacketCharInventory);

		if (!database.SaveInventory(c->CharacterID(), inst, EQ::invslot::slotCursor)) {
			c->Message(Chat::Red, "Item edit applied, but saving the cursor slot failed.");
		}
	}

	void SendItemEditUsage(Client *c)
	{
		c->Message(Chat::White, "Usage: hold an item on your cursor and use #itemedit");
		c->Message(Chat::White, "Usage: #itemedit set [field] [value]");
		c->Message(Chat::White, "Usage: #itemedit add [field] [amount]");
		c->Message(Chat::White, "Usage: #itemedit clear [field|all]");
		c->Message(Chat::White, "Usage: #itemedit proc [spell_id] [rate] [level]");
	}

	std::string NumericEditLinks(const std::string &field)
	{
		return fmt::format(
			"{} {} {} {} {}",
			Saylink::Silent(fmt::format("#itemedit add {} -10", field), "-10"),
			Saylink::Silent(fmt::format("#itemedit add {} -1", field), "-1"),
			Saylink::Silent(fmt::format("#itemedit add {} 1", field), "+1"),
			Saylink::Silent(fmt::format("#itemedit add {} 10", field), "+10"),
			Saylink::Silent(fmt::format("#itemedit clear {}", field), "clear")
		);
	}

	void SendFieldLine(Client *c, EQ::ItemInstance *inst, const ItemEditField &field)
	{
		const auto *resolved = inst->GetItem();
		const auto *base = inst->GetUnscaledItem();

		if (field.type == ItemEditFieldType::Text) {
			const std::string value = GetItemEditText(resolved, field.key);
			const std::string base_value = GetItemEditText(base, field.key);
			const bool overridden = !inst->GetCustomData(DynamicItemSetKey(field.key)).empty();

			c->Message(
				Chat::White,
				fmt::format(
					"{}: [{}] base [{}] {} | #itemedit set {} <text> | {}",
					field.label,
					value,
					base_value,
					overridden ? "override" : "",
					field.key,
					Saylink::Silent(fmt::format("#itemedit clear {}", field.key), "clear")
				).c_str()
			);
			return;
		}

		const auto value = GetItemEditNumber(resolved, field.key);
		const auto base_value = GetItemEditNumber(base, field.key);
		const auto modifier = inst->GetCustomData(DynamicItemModKey(field.key));
		const auto override_value = inst->GetCustomData(DynamicItemSetKey(field.key));
		std::string note;

		if (!override_value.empty()) {
			note = fmt::format(" set [{}]", override_value);
		}

		if (!modifier.empty()) {
			note += fmt::format(" mod [{}]", modifier);
		}

		c->Message(
			Chat::White,
			fmt::format(
				"{}: [{}] base [{}]{} | {}",
				field.label,
				value,
				base_value,
				note,
				NumericEditLinks(field.key)
			).c_str()
		);
	}

	void SendItemEditWindow(Client *c, EQ::ItemInstance *inst)
	{
		const auto *item = inst ? inst->GetItem() : nullptr;
		if (!item) {
			c->Message(Chat::White, "Hold an item on your cursor before using #itemedit.");
			return;
		}

		EQ::SayLinkEngine linker;
		linker.SetLinkType(EQ::saylink::SayLinkItemInst);
		linker.SetItemInst(inst);

		c->Message(
			Chat::White,
			fmt::format(
				"Item Edit: {} | {} | {} | {}",
				linker.GenerateLink(),
				Saylink::Silent("#itemedit clear all", "clear all"),
				Saylink::Silent("#itemedit proc 0", "clear proc"),
				Saylink::Silent("#itemedit", "refresh")
			).c_str()
		);
		c->Message(Chat::White, "Click +/- for additive per-instance modifiers. Use #itemedit set field value for exact overrides.");

		for (const auto &field : GetItemEditFields()) {
			SendFieldLine(c, inst, field);
		}
	}
}

void command_itemedit(Client *c, const Seperator *sep)
{
	auto *inst = GetCursorItem(c);
	if (!inst || !inst->GetItem()) {
		SendItemEditUsage(c);
		c->Message(Chat::White, "No item is currently on your cursor.");
		return;
	}

	const int arguments = sep->argnum;
	if (!arguments || !strcasecmp(sep->arg[1], "help") || !strcasecmp(sep->arg[1], "list")) {
		SendItemEditWindow(c, inst);
		return;
	}

	const bool is_set = !strcasecmp(sep->arg[1], "set");
	const bool is_add = !strcasecmp(sep->arg[1], "add") || !strcasecmp(sep->arg[1], "mod");
	const bool is_clear = !strcasecmp(sep->arg[1], "clear") || !strcasecmp(sep->arg[1], "reset");
	const bool is_proc = !strcasecmp(sep->arg[1], "proc");

	if (is_clear) {
		if (arguments < 2 || !strcasecmp(sep->arg[2], "all")) {
			inst->ClearDynamicItemData();
			SaveCursorEdit(c, inst);
			c->Message(Chat::White, "Cleared all dynamic item edits from the cursor item.");
			SendItemEditWindow(c, inst);
			return;
		}

		const auto *field = FindItemEditField(sep->arg[2]);
		if (!field) {
			c->Message(Chat::White, fmt::format("Unknown item edit field [{}].", sep->arg[2]).c_str());
			return;
		}

		inst->DeleteDynamicItemData(field->key);
		inst->DeleteDynamicItemModifier(field->key);
		SaveCursorEdit(c, inst);
		c->Message(Chat::White, fmt::format("Cleared dynamic item edits for [{}].", field->label).c_str());
		SendItemEditWindow(c, inst);
		return;
	}

	if (is_proc) {
		if (arguments < 2 || !sep->IsNumber(2)) {
			c->Message(Chat::White, "Usage: #itemedit proc [spell_id] [rate] [level]");
			return;
		}

		const int spell_id = Strings::ToInt(sep->arg[2]);
		if (spell_id <= 0) {
			inst->DeleteDynamicItemData("proceffect");
			inst->DeleteDynamicItemData("proctype");
			inst->DeleteDynamicItemData("proclevel");
			inst->DeleteDynamicItemData("proclevel2");
			inst->DeleteDynamicItemData("procrate");
			inst->DeleteDynamicItemData("procname");
			c->Message(Chat::White, "Cleared proc fields from the cursor item.");
		} else {
			const int proc_rate = (arguments >= 3 && sep->IsNumber(3)) ? Strings::ToInt(sep->arg[3]) : 100;
			const int proc_level = (arguments >= 4 && sep->IsNumber(4)) ? Strings::ToInt(sep->arg[4]) : 1;
			inst->SetDynamicItemData("proceffect", spell_id);
			inst->SetDynamicItemData("proctype", EQ::item::ItemEffectCombatProc);
			inst->SetDynamicItemData("proclevel", proc_level);
			inst->SetDynamicItemData("proclevel2", 255);
			inst->SetDynamicItemData("procrate", proc_rate);
			c->Message(Chat::White, fmt::format("Set cursor item proc to spell [{}], rate [{}], level [{}].", spell_id, proc_rate, proc_level).c_str());
		}

		SaveCursorEdit(c, inst);
		SendItemEditWindow(c, inst);
		return;
	}

	if (is_add) {
		if (arguments < 3 || !sep->IsNumber(3)) {
			c->Message(Chat::White, "Usage: #itemedit add [field] [amount]");
			return;
		}

		const auto *field = FindItemEditField(sep->arg[2]);
		if (!field || field->type != ItemEditFieldType::Number) {
			c->Message(Chat::White, fmt::format("[{}] is not a numeric item edit field.", sep->arg[2]).c_str());
			return;
		}

		const int current_modifier = Strings::ToInt(inst->GetCustomData(DynamicItemModKey(field->key)));
		const int delta = Strings::ToInt(sep->arg[3]);
		inst->SetDynamicItemModifier(field->key, current_modifier + delta);
		SaveCursorEdit(c, inst);
		c->Message(
			Chat::White,
			fmt::format(
				"Adjusted [{}] modifier by [{}]. New modifier [{}].",
				field->label,
				delta,
				current_modifier + delta
			).c_str()
		);
		SendItemEditWindow(c, inst);
		return;
	}

	if (is_set) {
		if (arguments < 3) {
			c->Message(Chat::White, "Usage: #itemedit set [field] [value]");
			return;
		}

		const auto *field = FindItemEditField(sep->arg[2]);
		if (!field) {
			c->Message(Chat::White, fmt::format("Unknown item edit field [{}].", sep->arg[2]).c_str());
			return;
		}

		if (field->type == ItemEditFieldType::Number) {
			if (!sep->IsNumber(3)) {
				c->Message(Chat::White, fmt::format("[{}] requires a numeric value.", field->label).c_str());
				return;
			}

			inst->SetDynamicItemData(field->key, Strings::ToInt(sep->arg[3]));
		} else {
			inst->SetDynamicItemData(field->key, std::string(sep->argplus[3]));
		}

		SaveCursorEdit(c, inst);
		c->Message(Chat::White, fmt::format("Set [{}] override on the cursor item.", field->label).c_str());
		SendItemEditWindow(c, inst);
		return;
	}

	SendItemEditUsage(c);
}

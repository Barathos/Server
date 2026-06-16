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
#include "inventory_profile.h"

#include "common/data_verification.h"
#include "common/evolving_items.h"
#include "common/rulesys.h"
#include "common/shareddb.h"
#include "common/strings.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <type_traits>
#include <vector>

int32 next_item_serial_number = 1;
std::unordered_set<uint64> guids{};

namespace {
	constexpr const char *DynamicItemModPrefix = "dynamic_item.mod.";
	constexpr const char *DynamicItemSetPrefix = "dynamic_item.set.";
	constexpr const char *AugmentInstanceDataPrefix = "_eqemu_augment_instance.";
	constexpr const char *AugmentInstanceCustomDataField = "custom_data";
	constexpr const char *AugmentInstanceGuidField = "guid";

	bool HasPrefix(const std::string &value, const char *prefix)
	{
		return value.rfind(prefix, 0) == 0;
	}

	std::string NormalizeDynamicItemField(std::string field)
	{
		field = Strings::ToLower(field);
		field.erase(
			std::remove_if(
				field.begin(),
				field.end(),
				[](const char c) { return c == '_' || c == '-' || c == ' ' || c == '.'; }
			),
			field.end()
		);
		return field;
	}

	bool IsDynamicItemDataIdentifier(const std::string &identifier)
	{
		const auto key = Strings::ToLower(identifier);
		return HasPrefix(key, DynamicItemModPrefix) || HasPrefix(key, DynamicItemSetPrefix);
	}

	std::string GetDynamicItemKey(const char *prefix, const std::string &identifier)
	{
		return std::string(prefix) + NormalizeDynamicItemField(identifier);
	}

	int HexValue(const char c)
	{
		if (c >= '0' && c <= '9') {
			return c - '0';
		}

		if (c >= 'a' && c <= 'f') {
			return c - 'a' + 10;
		}

		if (c >= 'A' && c <= 'F') {
			return c - 'A' + 10;
		}

		return -1;
	}

	std::string HexEncode(const std::string &value)
	{
		static constexpr char HexDigits[] = "0123456789ABCDEF";
		std::string encoded;
		encoded.reserve(value.size() * 2);

		for (const auto c : value) {
			const auto byte = static_cast<unsigned char>(c);
			encoded.push_back(HexDigits[byte >> 4]);
			encoded.push_back(HexDigits[byte & 0x0F]);
		}

		return encoded;
	}

	std::string HexDecode(const std::string &value)
	{
		if (value.size() % 2 != 0) {
			return "";
		}

		std::string decoded;
		decoded.reserve(value.size() / 2);

		for (size_t i = 0; i < value.size(); i += 2) {
			const auto high = HexValue(value[i]);
			const auto low = HexValue(value[i + 1]);
			if (high < 0 || low < 0) {
				return "";
			}

			decoded.push_back(static_cast<char>((high << 4) | low));
		}

		return decoded;
	}

	std::string GetAugmentInstanceDataKey(const uint8 slot, const char *field)
	{
		return std::string(AugmentInstanceDataPrefix) + std::to_string(slot) + "." + field;
	}

	bool ParseAugmentInstanceDataKey(const std::string &identifier, uint8 &slot, std::string &field)
	{
		if (!HasPrefix(identifier, AugmentInstanceDataPrefix)) {
			return false;
		}

		const auto remaining = identifier.substr(std::strlen(AugmentInstanceDataPrefix));
		const auto separator = remaining.find('.');
		if (separator == std::string::npos) {
			return false;
		}

		const auto slot_text = remaining.substr(0, separator);
		if (!Strings::IsNumber(slot_text)) {
			return false;
		}

		const auto slot_id = Strings::ToUnsignedInt(slot_text);
		if (slot_id > EQ::invaug::SOCKET_END) {
			return false;
		}

		slot = static_cast<uint8>(slot_id);
		field = remaining.substr(separator + 1);
		return !field.empty();
	}

	template <typename T>
	void ApplyDynamicItemNumber(T &target, int64 value, bool additive)
	{
		if constexpr (std::is_same_v<T, bool>) {
			target = additive ? ((target ? 1 : 0) + value) != 0 : value != 0;
		} else if constexpr (std::is_floating_point_v<T>) {
			target = static_cast<T>((additive ? target : 0) + value);
		} else {
			const int64 current = additive ? static_cast<int64>(target) : 0;
			int64 result = current + value;

			if constexpr (std::is_signed_v<T>) {
				result = std::max<int64>(std::numeric_limits<T>::min(), std::min<int64>(result, std::numeric_limits<T>::max()));
			} else {
				result = std::max<int64>(0, std::min<int64>(result, std::numeric_limits<T>::max()));
			}

			target = static_cast<T>(result);
		}
	}

	bool ApplyDynamicItemStringField(EQ::ItemData &item, const std::string &field, const std::string &value)
	{
		if (field == "name") {
			strn0cpy(item.Name, value.c_str(), sizeof(item.Name));
			return true;
		}

		if (field == "lore") {
			strn0cpy(item.Lore, value.c_str(), sizeof(item.Lore));
			return true;
		}

		if (field == "idfile") {
			strn0cpy(item.IDFile, value.c_str(), sizeof(item.IDFile));
			return true;
		}

		if (field == "comment") {
			strn0cpy(item.Comment, value.c_str(), sizeof(item.Comment));
			return true;
		}

		if (field == "filename") {
			strn0cpy(item.Filename, value.c_str(), sizeof(item.Filename));
			return true;
		}

		if (field == "charmfile") {
			strn0cpy(item.CharmFile, value.c_str(), sizeof(item.CharmFile));
			return true;
		}

		if (field == "clickname") {
			strn0cpy(item.ClickName, value.c_str(), sizeof(item.ClickName));
			return true;
		}

		if (field == "procname") {
			strn0cpy(item.ProcName, value.c_str(), sizeof(item.ProcName));
			return true;
		}

		if (field == "wornname") {
			strn0cpy(item.WornName, value.c_str(), sizeof(item.WornName));
			return true;
		}

		if (field == "focusname") {
			strn0cpy(item.FocusName, value.c_str(), sizeof(item.FocusName));
			return true;
		}

		if (field == "scrollname") {
			strn0cpy(item.ScrollName, value.c_str(), sizeof(item.ScrollName));
			return true;
		}

		return false;
	}

	bool ApplyDynamicItemNumericField(EQ::ItemData &item, const std::string &field, const int64 value, const bool additive)
	{
		if (field == "ac") { ApplyDynamicItemNumber(item.AC, value, additive); return true; }
		if (field == "agi" || field == "aagi") { ApplyDynamicItemNumber(item.AAgi, value, additive); return true; }
		if (field == "cha" || field == "acha") { ApplyDynamicItemNumber(item.ACha, value, additive); return true; }
		if (field == "dex" || field == "adex") { ApplyDynamicItemNumber(item.ADex, value, additive); return true; }
		if (field == "int" || field == "aint") { ApplyDynamicItemNumber(item.AInt, value, additive); return true; }
		if (field == "sta" || field == "asta") { ApplyDynamicItemNumber(item.ASta, value, additive); return true; }
		if (field == "str" || field == "astr") { ApplyDynamicItemNumber(item.AStr, value, additive); return true; }
		if (field == "wis" || field == "awis") { ApplyDynamicItemNumber(item.AWis, value, additive); return true; }
		if (field == "hp") { ApplyDynamicItemNumber(item.HP, value, additive); return true; }
		if (field == "mana") { ApplyDynamicItemNumber(item.Mana, value, additive); return true; }
		if (field == "endur" || field == "endurance") { ApplyDynamicItemNumber(item.Endur, value, additive); return true; }
		if (field == "regen") { ApplyDynamicItemNumber(item.Regen, value, additive); return true; }
		if (field == "manaregen") { ApplyDynamicItemNumber(item.ManaRegen, value, additive); return true; }
		if (field == "enduranceregen" || field == "endurregen") { ApplyDynamicItemNumber(item.EnduranceRegen, value, additive); return true; }
		if (field == "cr") { ApplyDynamicItemNumber(item.CR, value, additive); return true; }
		if (field == "dr") { ApplyDynamicItemNumber(item.DR, value, additive); return true; }
		if (field == "fr") { ApplyDynamicItemNumber(item.FR, value, additive); return true; }
		if (field == "mr") { ApplyDynamicItemNumber(item.MR, value, additive); return true; }
		if (field == "pr") { ApplyDynamicItemNumber(item.PR, value, additive); return true; }
		if (field == "svcorruption" || field == "corrup" || field == "corruption") { ApplyDynamicItemNumber(item.SVCorruption, value, additive); return true; }
		if (field == "attack") { ApplyDynamicItemNumber(item.Attack, value, additive); return true; }
		if (field == "accuracy") { ApplyDynamicItemNumber(item.Accuracy, value, additive); return true; }
		if (field == "avoidance") { ApplyDynamicItemNumber(item.Avoidance, value, additive); return true; }
		if (field == "combateffects") { ApplyDynamicItemNumber(item.CombatEffects, value, additive); return true; }
		if (field == "shielding") { ApplyDynamicItemNumber(item.Shielding, value, additive); return true; }
		if (field == "spellshield") { ApplyDynamicItemNumber(item.SpellShield, value, additive); return true; }
		if (field == "stunresist") { ApplyDynamicItemNumber(item.StunResist, value, additive); return true; }
		if (field == "strikethrough") { ApplyDynamicItemNumber(item.StrikeThrough, value, additive); return true; }
		if (field == "dotshielding") { ApplyDynamicItemNumber(item.DotShielding, value, additive); return true; }
		if (field == "damageshield") { ApplyDynamicItemNumber(item.DamageShield, value, additive); return true; }
		if (field == "dsmitigation") { ApplyDynamicItemNumber(item.DSMitigation, value, additive); return true; }
		if (field == "haste") { ApplyDynamicItemNumber(item.Haste, value, additive); return true; }
		if (field == "healamt" || field == "healing") { ApplyDynamicItemNumber(item.HealAmt, value, additive); return true; }
		if (field == "spelldmg" || field == "spelldamage") { ApplyDynamicItemNumber(item.SpellDmg, value, additive); return true; }
		if (field == "clairvoyance") { ApplyDynamicItemNumber(item.Clairvoyance, value, additive); return true; }
		if (field == "purity") { ApplyDynamicItemNumber(item.Purity, value, additive); return true; }
		if (field == "damage") { ApplyDynamicItemNumber(item.Damage, value, additive); return true; }
		if (field == "delay") { ApplyDynamicItemNumber(item.Delay, value, additive); return true; }
		if (field == "range") { ApplyDynamicItemNumber(item.Range, value, additive); return true; }
		if (field == "backstabdmg" || field == "backstabdamage") { ApplyDynamicItemNumber(item.BackstabDmg, value, additive); return true; }
		if (field == "elemdmgtype" || field == "elementaldamagetype") { ApplyDynamicItemNumber(item.ElemDmgType, value, additive); return true; }
		if (field == "elemdmgamt" || field == "elementaldamage") { ApplyDynamicItemNumber(item.ElemDmgAmt, value, additive); return true; }
		if (field == "banedmgamt" || field == "banedamage") { ApplyDynamicItemNumber(item.BaneDmgAmt, value, additive); return true; }
		if (field == "banedmgbody") { ApplyDynamicItemNumber(item.BaneDmgBody, value, additive); return true; }
		if (field == "banedmgrace") { ApplyDynamicItemNumber(item.BaneDmgRace, value, additive); return true; }
		if (field == "banedmgraceamt") { ApplyDynamicItemNumber(item.BaneDmgRaceAmt, value, additive); return true; }
		if (field == "extradmgskill") { ApplyDynamicItemNumber(item.ExtraDmgSkill, value, additive); return true; }
		if (field == "extradmgamt" || field == "extradamage") { ApplyDynamicItemNumber(item.ExtraDmgAmt, value, additive); return true; }
		if (field == "skillmodtype") { ApplyDynamicItemNumber(item.SkillModType, value, additive); return true; }
		if (field == "skillmodvalue") { ApplyDynamicItemNumber(item.SkillModValue, value, additive); return true; }
		if (field == "skillmodmax") { ApplyDynamicItemNumber(item.SkillModMax, value, additive); return true; }
		if (field == "bardtype") { ApplyDynamicItemNumber(item.BardType, value, additive); return true; }
		if (field == "bardvalue") { ApplyDynamicItemNumber(item.BardValue, value, additive); return true; }
		if (field == "weight") { ApplyDynamicItemNumber(item.Weight, value, additive); return true; }
		if (field == "price") { ApplyDynamicItemNumber(item.Price, value, additive); return true; }
		if (field == "favor") { ApplyDynamicItemNumber(item.Favor, value, additive); return true; }
		if (field == "guildfavor") { ApplyDynamicItemNumber(item.GuildFavor, value, additive); return true; }
		if (field == "pointtype") { ApplyDynamicItemNumber(item.PointType, value, additive); return true; }
		if (field == "icon") { ApplyDynamicItemNumber(item.Icon, value, additive); return true; }
		if (field == "color") { ApplyDynamicItemNumber(item.Color, value, additive); return true; }
		if (field == "material") { ApplyDynamicItemNumber(item.Material, value, additive); return true; }
		if (field == "elitematerial") { ApplyDynamicItemNumber(item.EliteMaterial, value, additive); return true; }
		if (field == "herosforgemodel") { ApplyDynamicItemNumber(item.HerosForgeModel, value, additive); return true; }
		if (field == "light") { ApplyDynamicItemNumber(item.Light, value, additive); return true; }
		if (field == "size") { ApplyDynamicItemNumber(item.Size, value, additive); return true; }
		if (field == "slots") { ApplyDynamicItemNumber(item.Slots, value, additive); return true; }
		if (field == "classes") { ApplyDynamicItemNumber(item.Classes, value, additive); return true; }
		if (field == "races") { ApplyDynamicItemNumber(item.Races, value, additive); return true; }
		if (field == "deity") { ApplyDynamicItemNumber(item.Deity, value, additive); return true; }
		if (field == "itemclass") { ApplyDynamicItemNumber(item.ItemClass, value, additive); return true; }
		if (field == "itemtype") { ApplyDynamicItemNumber(item.ItemType, value, additive); return true; }
		if (field == "subtype") { ApplyDynamicItemNumber(item.SubType, value, additive); return true; }
		if (field == "reqlevel") { ApplyDynamicItemNumber(item.ReqLevel, value, additive); return true; }
		if (field == "reclevel") { ApplyDynamicItemNumber(item.RecLevel, value, additive); return true; }
		if (field == "recskill") { ApplyDynamicItemNumber(item.RecSkill, value, additive); return true; }
		if (field == "maxcharges") { ApplyDynamicItemNumber(item.MaxCharges, value, additive); return true; }
		if (field == "stacksize") { ApplyDynamicItemNumber(item.StackSize, value, additive); return true; }
		if (field == "stackable") { ApplyDynamicItemNumber(item.Stackable, value, additive); return true; }
		if (field == "magic") { ApplyDynamicItemNumber(item.Magic, value, additive); return true; }
		if (field == "loregroup") { ApplyDynamicItemNumber(item.LoreGroup, value, additive); item.LoreFlag = item.LoreGroup != 0; return true; }
		if (field == "loreflag") {
			ApplyDynamicItemNumber(item.LoreFlag, value, additive);
			if (!item.LoreFlag) {
				item.LoreGroup = 0;
			}
			else if (item.LoreGroup == 0) {
				item.LoreGroup = -1;
			}
			return true;
		}
		if (field == "pendingloreflag" || field == "pendinglore") { ApplyDynamicItemNumber(item.PendingLoreFlag, value, additive); return true; }
		if (field == "nodrop") { ApplyDynamicItemNumber(item.NoDrop, value, additive); return true; }
		if (field == "norent") { ApplyDynamicItemNumber(item.NoRent, value, additive); return true; }
		if (field == "attuneable") { ApplyDynamicItemNumber(item.Attuneable, value, additive); return true; }
		if (field == "notransfer") { ApplyDynamicItemNumber(item.NoTransfer, value, additive); return true; }
		if (field == "nopet") { ApplyDynamicItemNumber(item.NoPet, value, additive); return true; }
		if (field == "questitemflag") { ApplyDynamicItemNumber(item.QuestItemFlag, value, additive); return true; }
		if (field == "augtype") { ApplyDynamicItemNumber(item.AugType, value, additive); return true; }
		if (field == "augrestrict") { ApplyDynamicItemNumber(item.AugRestrict, value, additive); return true; }
		if (field == "augdistiller") { ApplyDynamicItemNumber(item.AugDistiller, value, additive); return true; }
		if (field == "bagtype") { ApplyDynamicItemNumber(item.BagType, value, additive); return true; }
		if (field == "bagslots") { ApplyDynamicItemNumber(item.BagSlots, value, additive); return true; }
		if (field == "bagsize") { ApplyDynamicItemNumber(item.BagSize, value, additive); return true; }
		if (field == "bagwr") { ApplyDynamicItemNumber(item.BagWR, value, additive); return true; }
		if (field == "procrate") { ApplyDynamicItemNumber(item.ProcRate, value, additive); return true; }
		if (field == "casttime") { ApplyDynamicItemNumber(item.CastTime, value, additive); return true; }
		if (field == "casttime2") { ApplyDynamicItemNumber(item.CastTime_, value, additive); return true; }
		if (field == "recastdelay") { ApplyDynamicItemNumber(item.RecastDelay, value, additive); return true; }
		if (field == "recasttype") { ApplyDynamicItemNumber(item.RecastType, value, additive); return true; }
		if (field == "charmfileid") { ApplyDynamicItemNumber(item.CharmFileID, value, additive); return true; }
		if (field == "scriptfileid") { ApplyDynamicItemNumber(item.ScriptFileID, value, additive); return true; }
		if (field == "evolvingitem") { ApplyDynamicItemNumber(item.EvolvingItem, value, additive); return true; }
		if (field == "evolvingid") { ApplyDynamicItemNumber(item.EvolvingID, value, additive); return true; }
		if (field == "evolvinglevel") { ApplyDynamicItemNumber(item.EvolvingLevel, value, additive); return true; }
		if (field == "evolvingmax") { ApplyDynamicItemNumber(item.EvolvingMax, value, additive); return true; }
		if (field == "hstr" || field == "heroicstr") { ApplyDynamicItemNumber(item.HeroicStr, value, additive); return true; }
		if (field == "hint" || field == "heroicint") { ApplyDynamicItemNumber(item.HeroicInt, value, additive); return true; }
		if (field == "hwis" || field == "heroicwis") { ApplyDynamicItemNumber(item.HeroicWis, value, additive); return true; }
		if (field == "hagi" || field == "heroicagi") { ApplyDynamicItemNumber(item.HeroicAgi, value, additive); return true; }
		if (field == "hdex" || field == "heroicdex") { ApplyDynamicItemNumber(item.HeroicDex, value, additive); return true; }
		if (field == "hsta" || field == "heroicsta") { ApplyDynamicItemNumber(item.HeroicSta, value, additive); return true; }
		if (field == "hcha" || field == "heroiccha") { ApplyDynamicItemNumber(item.HeroicCha, value, additive); return true; }
		if (field == "hmr" || field == "heroicmr") { ApplyDynamicItemNumber(item.HeroicMR, value, additive); return true; }
		if (field == "hfr" || field == "heroicfr") { ApplyDynamicItemNumber(item.HeroicFR, value, additive); return true; }
		if (field == "hcr" || field == "heroiccr") { ApplyDynamicItemNumber(item.HeroicCR, value, additive); return true; }
		if (field == "hdr" || field == "heroicdr") { ApplyDynamicItemNumber(item.HeroicDR, value, additive); return true; }
		if (field == "hpr" || field == "heroicpr") { ApplyDynamicItemNumber(item.HeroicPR, value, additive); return true; }
		if (field == "hsvcorruption" || field == "heroicsvcorruption" || field == "heroiccorrup") { ApplyDynamicItemNumber(item.HeroicSVCorrup, value, additive); return true; }
		if (field == "clickeffect") { ApplyDynamicItemNumber(item.Click.Effect, value, additive); return true; }
		if (field == "clicktype") { ApplyDynamicItemNumber(item.Click.Type, value, additive); return true; }
		if (field == "clicklevel") { ApplyDynamicItemNumber(item.Click.Level, value, additive); return true; }
		if (field == "clicklevel2") { ApplyDynamicItemNumber(item.Click.Level2, value, additive); return true; }
		if (field == "proceffect") { ApplyDynamicItemNumber(item.Proc.Effect, value, additive); return true; }
		if (field == "proctype") { ApplyDynamicItemNumber(item.Proc.Type, value, additive); return true; }
		if (field == "proclevel") { ApplyDynamicItemNumber(item.Proc.Level, value, additive); return true; }
		if (field == "proclevel2") { ApplyDynamicItemNumber(item.Proc.Level2, value, additive); return true; }
		if (field == "worneffect") { ApplyDynamicItemNumber(item.Worn.Effect, value, additive); return true; }
		if (field == "worntype") { ApplyDynamicItemNumber(item.Worn.Type, value, additive); return true; }
		if (field == "wornlevel") { ApplyDynamicItemNumber(item.Worn.Level, value, additive); return true; }
		if (field == "wornlevel2") { ApplyDynamicItemNumber(item.Worn.Level2, value, additive); return true; }
		if (field == "focuseffect") { ApplyDynamicItemNumber(item.Focus.Effect, value, additive); return true; }
		if (field == "focustype") { ApplyDynamicItemNumber(item.Focus.Type, value, additive); return true; }
		if (field == "focuslevel") { ApplyDynamicItemNumber(item.Focus.Level, value, additive); return true; }
		if (field == "focuslevel2") { ApplyDynamicItemNumber(item.Focus.Level2, value, additive); return true; }
		if (field == "scrolleffect") { ApplyDynamicItemNumber(item.Scroll.Effect, value, additive); return true; }
		if (field == "scrolltype") { ApplyDynamicItemNumber(item.Scroll.Type, value, additive); return true; }
		if (field == "scrolllevel") { ApplyDynamicItemNumber(item.Scroll.Level, value, additive); return true; }
		if (field == "scrolllevel2") { ApplyDynamicItemNumber(item.Scroll.Level2, value, additive); return true; }
		if (field == "bardeffect") { ApplyDynamicItemNumber(item.Bard.Effect, value, additive); return true; }
		if (field == "bardeffecttype") { ApplyDynamicItemNumber(item.Bard.Type, value, additive); return true; }
		if (field == "bardlevel") { ApplyDynamicItemNumber(item.Bard.Level, value, additive); return true; }
		if (field == "bardlevel2") { ApplyDynamicItemNumber(item.Bard.Level2, value, additive); return true; }

		return false;
	}

	bool ApplyDynamicItemDataField(EQ::ItemData &item, const std::string &identifier, const std::string &value, const bool additive)
	{
		const auto field = NormalizeDynamicItemField(identifier);

		if (!additive && ApplyDynamicItemStringField(item, field, value)) {
			return true;
		}

		return ApplyDynamicItemNumericField(item, field, Strings::ToBigInt(value), additive);
	}
}

static inline int32 GetNextItemInstSerialNumber()
{
	// The Bazaar relies on each item a client has up for Trade having a unique
	// identifier. This 'SerialNumber' is sent in Serialized item packets and
	// is used in Bazaar packets to identify the item a player is buying or inspecting.
	//
	// E.g. A trader may have 3 Five dose cloudy potions, each with a different number of remaining charges
	// up for sale with different prices.
	//
	// NextItemInstSerialNumber is the next one to hand out.
	//
	// It is very unlikely to reach 2,147,483,647. Maybe we should call abort(), rather than wrapping back to 1.
	if (next_item_serial_number >= INT32_MAX) {
		next_item_serial_number = 1;
	}
	else {
		next_item_serial_number++;
	}

	while (guids.contains(next_item_serial_number)) {
		next_item_serial_number++;
	}

	return next_item_serial_number;
}

//
// class EQ::ItemInstance
//
EQ::ItemInstance::ItemInstance(const ItemData* item, int16 charges) {

	if (item) {
		m_item = new ItemData(*item);
	}

	m_charges = charges;

	if (m_item && m_item->IsClassCommon()) {
		m_color = m_item->Color;
	}

	if (IsEvolving()) {
		SetTimer("evolve", RuleI(EvolvingItems, DelayUponEquipping));
	}

	m_SerialNumber  = GetNextItemInstSerialNumber();
}

EQ::ItemInstance::ItemInstance(SharedDatabase *db, uint32 item_id, int16 charges) {

	m_item     = db->GetItem(item_id);

	if (m_item) {
		m_item = new ItemData(*m_item);
	}

	m_charges = charges;

	if (m_item && m_item->IsClassCommon()) {
		m_color = m_item->Color;
	} else {
		m_color = 0;
	}

	if (IsEvolving()) {
		SetTimer("evolve", RuleI(EvolvingItems, DelayUponEquipping));
	}

	m_SerialNumber  = GetNextItemInstSerialNumber();
}

EQ::ItemInstance::ItemInstance(ItemInstTypes use_type) {
	m_use_type     = use_type;
}

void EQ::ItemInstance::AssignNewSerialNumber()
{
	m_SerialNumber = GetNextItemInstSerialNumber();
}

// Make a copy of an EQ::ItemInstance object
EQ::ItemInstance::ItemInstance(const ItemInstance& copy)
{
	m_use_type = copy.m_use_type;

	if (copy.m_item) {
		m_item = new ItemData(*copy.m_item);
	} else {
		m_item = nullptr;
	}

	m_charges       = copy.m_charges;
	m_price         = copy.m_price;
	m_color         = copy.m_color;
	m_merchantslot  = copy.m_merchantslot;
	m_currentslot   = copy.m_currentslot;
	m_attuned       = copy.m_attuned;
	m_merchantcount = copy.m_merchantcount;

	// Copy container contents
	for (auto it = copy.m_contents.begin(); it != copy.m_contents.end(); ++it) {
		ItemInstance* inst_old = it->second;
		ItemInstance* inst_new = nullptr;

		if (inst_old) {
			inst_new = inst_old->Clone();
		}

		if (inst_new) {
			m_contents[it->first] = inst_new;
		}
	}

	std::map<std::string, std::string>::const_iterator iter;
	for (iter = copy.m_custom_data.begin(); iter != copy.m_custom_data.end(); ++iter) {
		m_custom_data[iter->first] = iter->second;
	}

	m_SerialNumber = copy.m_SerialNumber;
	m_custom_data  = copy.m_custom_data;
	m_timers       = copy.m_timers;

	m_exp       = copy.m_exp;
	m_evolveLvl = copy.m_evolveLvl;

	if (copy.m_scaledItem) {
		m_scaledItem = new ItemData(*copy.m_scaledItem);
	} else {
		m_scaledItem = nullptr;
	}

	if (copy.m_dynamicItem) {
		m_dynamicItem = new ItemData(*copy.m_dynamicItem);
	} else {
		m_dynamicItem = nullptr;
	}

	m_evolving_details    = copy.m_evolving_details;
	m_scaling             = copy.m_scaling;
	m_ornamenticon        = copy.m_ornamenticon;
	m_ornamentidfile      = copy.m_ornamentidfile;
	m_ornament_hero_model = copy.m_ornament_hero_model;
	m_recast_timestamp    = copy.m_recast_timestamp;
	m_new_id_file         = copy.m_new_id_file;
}

// Clean up container contents
EQ::ItemInstance::~ItemInstance()
{
	Clear();
	safe_delete(m_item);
	safe_delete(m_scaledItem);
	safe_delete(m_dynamicItem);
}

// Query item type
bool EQ::ItemInstance::IsType(item::ItemClass item_class) const
{
	// IsType(<ItemClassTypes>) does not protect against 'm_item = nullptr'

	// Check usage type
	if (m_use_type == ItemInstWorldContainer && item_class == item::ItemClassBag) {
		return true;
	}

	if (!m_item) {
		return false;
	}

	return (m_item->ItemClass == item_class);
}

bool EQ::ItemInstance::IsClassCommon() const
{
	return (m_item && m_item->IsClassCommon());
}

bool EQ::ItemInstance::IsClassBag() const
{
	return (m_item && m_item->IsClassBag());
}

bool EQ::ItemInstance::IsClassBook() const
{
	return (m_item && m_item->IsClassBook());
}

// Is item stackable?
bool EQ::ItemInstance::IsStackable() const
{
	return (m_item && m_item->Stackable);
}

bool EQ::ItemInstance::IsCharged() const
{
	if (!m_item) {
		return false;
	}

	if (m_item->MaxCharges > 1) {
		return true;
	} else {
		return false;
	}
}

// Can item be equipped?
bool EQ::ItemInstance::IsEquipable(uint16 race, uint16 class_) const
{
	if (!m_item || !m_item->Slots) {
		return false;
	}

	return m_item->IsEquipable(race, class_);
}

bool EQ::ItemInstance::IsEquipableByClassMask(uint16 race, uint32 class_mask) const
{
	if (!m_item || !m_item->Slots) {
		return false;
	}

	return m_item->IsEquipableByClassMask(race, class_mask);
}

// Can item be equipped by Class?
bool EQ::ItemInstance::IsClassEquipable(uint16 class_) const
{
	if (!m_item || !m_item->Slots) {
		return false;
	}

	return m_item->IsClassEquipable(class_);
}

bool EQ::ItemInstance::IsClassMaskEquipable(uint32 class_mask) const
{
	if (!m_item || !m_item->Slots) {
		return false;
	}

	return m_item->IsClassMaskEquipable(class_mask);
}

// Can item be equipped by Race?
bool EQ::ItemInstance::IsRaceEquipable(uint16 race) const
{
	if (!m_item || !m_item->Slots) {
		return false;
	}

	return m_item->IsRaceEquipable(race);
}

// Can equip at this slot?
bool EQ::ItemInstance::IsEquipable(int16 slot_id) const
{
	if (!m_item || !m_item->Slots) {
		return false;
	}

	if (slot_id < EQ::invslot::EQUIPMENT_BEGIN || slot_id > EQ::invslot::EQUIPMENT_END) {
		return false;
	}

	return ((m_item->Slots & (1 << slot_id)) != 0);
}

bool EQ::ItemInstance::IsAugmentable() const
{
	if (!m_item) {
		return false;
	}

	for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
		if (m_item->AugSlotType[index] != 0) {
			return true;
		}
	}

	return false;
}

bool EQ::ItemInstance::AvailableWearSlot(uint32 aug_wear_slots) const {
	if (!m_item || !m_item->IsClassCommon()) {
		return false;
	}

	int index = invslot::EQUIPMENT_BEGIN;
	for (; index <= invslot::EQUIPMENT_END; ++index) {
		if (m_item->Slots & (1 << index)) {
			if (aug_wear_slots & (1 << index)) {
				break;
			}
		}
	}

	return (index <= EQ::invslot::EQUIPMENT_END);
}

int8 EQ::ItemInstance::AvailableAugmentSlot(int32 augment_type) const
{
	if (!m_item || !m_item->IsClassCommon()) {
		return INVALID_INDEX;
	}

	for (int16 slot_id = invaug::SOCKET_BEGIN; slot_id <= invaug::SOCKET_END; ++slot_id) {
		if (IsAugmentSlotAvailable(augment_type, slot_id)) {
			return slot_id;
		}
	}

	return INVALID_INDEX;
}

bool EQ::ItemInstance::IsAugmentSlotAvailable(int32 augment_type, uint8 slot) const
{
	if (!m_item || !m_item->IsClassCommon() || GetItem(slot)) {
		return false;
	}

	return (
		slot < invaug::SOCKET_COUNT &&
		(
			augment_type == -1 ||
			(
				m_item->AugSlotType[slot] &&
				((1 << (m_item->AugSlotType[slot] - 1)) & augment_type)
			)
		) &&
		(
			RuleB(Items, AugmentItemAllowInvisibleAugments) ||
			m_item->AugSlotVisible[slot]
		)
	);
}

// Retrieve item inside container
EQ::ItemInstance* EQ::ItemInstance::GetItem(uint8 index) const
{
	auto it = m_contents.find(index);
	if (it != m_contents.end()) {
		return it->second;
	}

	return nullptr;
}

uint32 EQ::ItemInstance::GetItemID(uint8 slot) const
{
	const auto item = GetItem(slot);
	if (item) {
		return item->GetID();
	}

	return 0;
}

void EQ::ItemInstance::PutItem(uint8 index, const ItemInstance& inst)
{
	// Clean up item already in slot (if exists)
	DeleteItem(index);

	// Delegate to internal method
	_PutItem(index, inst.Clone());
}

// Remove item inside container
void EQ::ItemInstance::DeleteItem(uint8 index)
{
	ItemInstance* inst = PopItem(index);
	safe_delete(inst);
}

// Remove item from container without memory delete
// Hands over memory ownership to client of this function call
EQ::ItemInstance* EQ::ItemInstance::PopItem(uint8 index)
{
	auto iter = m_contents.find(index);
	if (iter != m_contents.end()) {
		ItemInstance* inst = iter->second;
		m_contents.erase(index);
		return inst; // Return pointer that needs to be deleted (or otherwise managed)
	}

	return nullptr;
}

// Remove all items from container
void EQ::ItemInstance::Clear()
{
	// Destroy container contents
	for (auto iter = m_contents.begin(); iter != m_contents.end(); ++iter) {
		safe_delete(iter->second);
	}
	m_contents.clear();
}

// Remove all items from container
void EQ::ItemInstance::ClearByFlags(byFlagSetting is_nodrop, byFlagSetting is_norent)
{
	// TODO: This needs work...

	// Destroy container contents
	std::map<uint8, ItemInstance*>::const_iterator cur, end, del;
	cur = m_contents.begin();
	end = m_contents.end();
	for (; cur != end;) {
		ItemInstance* inst = cur->second;
		if (inst == nullptr) {
			cur = m_contents.erase(cur);
			continue;
		}

		const ItemData* item = inst->GetItem();
		if (item == nullptr) {
			cur = m_contents.erase(cur);
			continue;
		}

		del = cur;
		++cur;

		switch (is_nodrop) {
		case byFlagSet:
			if (item->NoDrop == 0) {
				safe_delete(inst);
				m_contents.erase(del->first);
				continue;
			}
			// no 'break;' deletes 'byFlagNotSet' type - can't add at the moment because it really *breaks* the process somewhere
		case byFlagNotSet:
			if (item->NoDrop != 0) {
				safe_delete(inst);
				m_contents.erase(del->first);
				continue;
			}
		default:
			break;
		}

		switch (is_norent) {
		case byFlagSet:
			if (item->NoRent == 0) {
				safe_delete(inst);
				m_contents.erase(del->first);
				continue;
			}
			// no 'break;' deletes 'byFlagNotSet' type - can't add at the moment because it really *breaks* the process somewhere
		case byFlagNotSet:
			if (item->NoRent != 0) {
				safe_delete(inst);
				m_contents.erase(del->first);
				continue;
			}
		default:
			break;
		}
	}
}

uint8 EQ::ItemInstance::FirstOpenSlot() const
{
	if (!m_item)
		return INVALID_INDEX;

	uint8 slots = m_item->BagSlots, i;
	for (i = invbag::SLOT_BEGIN; i < slots; i++) {
		if (!GetItem(i))
			break;
	}

	return (i < slots) ? i : INVALID_INDEX;
}

uint8 EQ::ItemInstance::GetTotalItemCount() const
{
	if (!m_item) {
		return 0;
	}

	uint8 item_count = 1;

	if (!m_item->IsClassBag()) {
		return item_count;
	}

	for (int index = invbag::SLOT_BEGIN; index < m_item->BagSlots; ++index) {
		if (GetItem(index)) {
			++item_count;
		}
	}

	return item_count;
}

bool EQ::ItemInstance::IsNoneEmptyContainer()
{
	if (!m_item || !m_item->IsClassBag())
		return false;

	for (int index = invbag::SLOT_BEGIN; index < m_item->BagSlots; ++index) {
		if (GetItem(index))
			return true;
	}

	return false;
}

// Retrieve augment inside item
EQ::ItemInstance* EQ::ItemInstance::GetAugment(uint8 augment_index) const
{
	if (m_item && m_item->IsClassCommon()) {
		return GetItem(augment_index);
	}

	return nullptr;
}

bool EQ::ItemInstance::IsOrnamentationAugment(EQ::ItemInstance* augment) const
{
	if (!m_item || !m_item->IsClassCommon() || !augment) {
		return false;
	}

	const auto augment_item = augment->GetItem();
	if (!augment_item) {
		return false;
	}

	const std::string& idfile = augment_item->IDFile;

	if (
		EQ::ValueWithin(
			augment->GetAugmentType(),
			OrnamentationAugmentTypes::StandardOrnamentation,
			OrnamentationAugmentTypes::SpecialOrnamentation
		) ||
		(
			idfile != "IT63" &&
			idfile != "IT64"
		) ||
		augment_item->HerosForgeModel
	) {
		return true;
	}

	return false;
}

EQ::ItemInstance* EQ::ItemInstance::GetOrnamentationAugment() const
{
	if (!m_item || !m_item->IsClassCommon()) {
		return nullptr;
	}

	for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; i++) {
		const auto augment = GetAugment(i);
		if (augment && IsOrnamentationAugment(augment)) {
			return augment;
		}
	}

	return nullptr;
}

uint32 EQ::ItemInstance::GetOrnamentHeroModel(int32 material_slot) const
{
	// Not a Hero Forge item.
	if (m_ornament_hero_model == 0) {
		return 0;
	}

	// Item is using an explicit Hero Forge ID
	if (m_ornament_hero_model >= 1000) {
		return m_ornament_hero_model;
	}

	// Item is using a shorthand ID
	return (m_ornament_hero_model * 100) + material_slot;
}

bool EQ::ItemInstance::UpdateOrnamentationInfo()
{
	if (!m_item || !m_item->IsClassCommon()) {
		return false;
	}

	const auto augment = GetOrnamentationAugment();

	if (augment) {
		const auto augment_item = GetOrnamentationAugment()->GetItem();

		if (augment_item) {
			SetOrnamentIcon(augment_item->Icon);
			SetOrnamentHeroModel(augment_item->HerosForgeModel);

			if (strlen(augment_item->IDFile) > 2) {
				SetOrnamentationIDFile(Strings::ToUnsignedInt(&augment_item->IDFile[2]));
			} else {
				SetOrnamentationIDFile(0);
			}

			return true;
		}
	}

	SetOrnamentIcon(0);
	SetOrnamentHeroModel(0);
	SetOrnamentationIDFile(0);

	return false;
}

bool EQ::ItemInstance::CanTransform(const ItemData *ItemToTry, const ItemData *Container, bool AllowAll) {
	if (!ItemToTry || !Container) return false;

	if (ItemToTry->ItemType == item::ItemTypeArrow || strnlen(Container->CharmFile, 30) == 0)
		return false;

	if (AllowAll && strncasecmp(Container->CharmFile, "ITEMTRANSFIGSHIELD", 18) && strncasecmp(Container->CharmFile, "ITEMTransfigBow", 15)) {
		switch (ItemToTry->ItemType) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 35:
			case 45:
				return true;
		}
	}

	static std::map<std::string, int> types;
	types["itemtransfig1hp"] = 2;
	types["itemtransfig1hs"] = 0;
	types["itemtransfig2hb"] = 4;
	types["itemtransfig2hp"] = 35;
	types["itemtransfig2hs"] = 1;
	types["itemtransfigblunt"] = 3;
	types["itemtransfig1hb"] = 3;
	types["itemtransfigbow"] = 5;
	types["itemtransfighth"] = 45;
	types["itemtransfigshield"] = 8;
	types["itemtransfigslashing"] = 0;

	auto i = types.find(MakeLowerString(Container->CharmFile));
	if (i != types.end() && i->second == ItemToTry->ItemType)
		return true;

	static std::map<std::string, int> typestwo;
	typestwo["itemtransfigblunt"] = 4;
	typestwo["itemtransfigslashing"] = 1;

	i = typestwo.find(MakeLowerString(Container->CharmFile));
	if (i != typestwo.end() && i->second == ItemToTry->ItemType)
		return true;

	return false;
}

uint32 EQ::ItemInstance::GetAugmentItemID(uint8 augment_index) const
{
	if (!m_item || !m_item->IsClassCommon()) {
		return 0;
	}

	return GetItemID(augment_index);
}

// Add an augment to the item
void EQ::ItemInstance::PutAugment(uint8 slot, const ItemInstance& augment)
{
	if (!m_item || !m_item->IsClassCommon())
		return;

	PutItem(slot, augment);
}

void EQ::ItemInstance::PutAugment(SharedDatabase *db, uint8 slot, uint32 item_id)
{
	if (item_id == 0) { return; }
	if (db == nullptr) { return; /* TODO: add log message for nullptr */ }

	const ItemInstance* aug = db->CreateItem(item_id);
	if (aug) {
		PutAugment(slot, *aug);
		safe_delete(aug);
	}
}

// Remove augment from item and destroy it
void EQ::ItemInstance::DeleteAugment(uint8 index)
{
	if (!m_item || !m_item->IsClassCommon())
		return;

	DeleteItem(index);
}

// Remove augment from item and return it
EQ::ItemInstance* EQ::ItemInstance::RemoveAugment(uint8 index)
{
	if (!m_item || !m_item->IsClassCommon())
		return nullptr;

	return PopItem(index);
}

bool EQ::ItemInstance::IsAugmented()
{
	if (!m_item || !m_item->IsClassCommon()) {
		return false;
	}

	for (uint8 slot_id = invaug::SOCKET_BEGIN; slot_id <= invaug::SOCKET_END; ++slot_id) {
		if (GetAugmentItemID(slot_id)) {
			return true;
		}
	}

	return false;
}

bool EQ::ItemInstance::ContainsAugmentByID(uint32 item_id)
{
	if (!m_item || !m_item->IsClassCommon()) {
		return false;
	}

	if (!item_id) {
		return false;
	}

	for (uint8 augment_slot = invaug::SOCKET_BEGIN; augment_slot <= invaug::SOCKET_END; ++augment_slot) {
		if (GetAugmentItemID(augment_slot) == item_id) {
			return true;
		}
	}

	return false;
}

bool EQ::ItemInstance::ContainsEquivalentAugment(const ItemInstance &augment) const
{
	if (!m_item || !m_item->IsClassCommon() || !augment.GetID()) {
		return false;
	}

	const auto candidate_key = augment.GetLiveItemAugmentDuplicateKey();
	for (uint8 augment_slot = invaug::SOCKET_BEGIN; augment_slot <= invaug::SOCKET_END; ++augment_slot) {
		auto *existing = GetAugment(augment_slot);
		if (existing && existing->GetLiveItemAugmentDuplicateKey() == candidate_key) {
			return true;
		}
	}

	return false;
}

int EQ::ItemInstance::CountAugmentByID(uint32 item_id)
{
	int quantity = 0;
	if (!m_item || !m_item->IsClassCommon()) {
		return quantity;
	}

	if (!item_id) {
		return quantity;
	}

	for (uint8 augment_slot = invaug::SOCKET_BEGIN; augment_slot <= invaug::SOCKET_END; ++augment_slot) {
		if (GetAugmentItemID(augment_slot) == item_id) {
			quantity++;
		}
	}

	return quantity;
}

// Has attack/delay?
bool EQ::ItemInstance::IsWeapon() const
{
	if (!m_item || !m_item->IsClassCommon())
		return false;

	if (m_item->ItemType == item::ItemTypeArrow && m_item->Damage != 0)
		return true;
	else
		return ((m_item->Damage != 0) && (m_item->Delay != 0));
}

bool EQ::ItemInstance::IsAmmo() const
{
	if (!m_item)
		return false;

	if ((m_item->ItemType == item::ItemTypeArrow) ||
		(m_item->ItemType == item::ItemTypeLargeThrowing) ||
		(m_item->ItemType == item::ItemTypeSmallThrowing)
		) {
		return true;
	}

	return false;

}

bool EQ::ItemInstance::RefreshItemData(const ItemData *item)
{
	if (!m_item || !item || item->ID != m_item->ID) {
		return false;
	}

	if (std::memcmp(m_item, item, sizeof(ItemData)) == 0) {
		return false;
	}

	auto *mutable_item = const_cast<ItemData *>(m_item);
	std::memcpy(mutable_item, item, sizeof(ItemData));

	m_scaling = m_item->CharmFileID != 0;
	if (m_scaling) {
		ScaleItem();
	} else {
		safe_delete(m_scaledItem);
		RebuildDynamicItemData();
	}

	AssignNewSerialNumber();

	return true;
}

const EQ::ItemData* EQ::ItemInstance::GetItem() const
{
	if (!m_item)
		return nullptr;

	if (m_dynamicItem)
		return m_dynamicItem;

	if (m_scaledItem)
		return m_scaledItem;

	return m_item;
}

const EQ::ItemData* EQ::ItemInstance::GetClientItem() const
{
	if (m_dynamicItem)
		return m_dynamicItem;

	return m_item;
}

const EQ::ItemData* EQ::ItemInstance::GetUnscaledItem() const
{
	// No operator calls and defaults to nullptr
	return m_item;
}

std::string EQ::ItemInstance::GetCustomDataString() const {
	std::string ret_val;
	auto append_pair = [&ret_val](const std::string &key, const std::string &value) {
		if (ret_val.length() > 0) {
			ret_val += "^";
		}

		ret_val += key;
		ret_val += "^";
		ret_val += value;
	};

	auto iter = m_custom_data.begin();
	while (iter != m_custom_data.end()) {
		uint8 ignored_slot = 0;
		std::string ignored_field;
		if (ParseAugmentInstanceDataKey(iter->first, ignored_slot, ignored_field)) {
			++iter;
			continue;
		}

		append_pair(iter->first, iter->second);
		++iter;
	}

	if (m_item && m_item->IsClassCommon()) {
		for (uint8 slot = invaug::SOCKET_BEGIN; slot <= invaug::SOCKET_END; ++slot) {
			const auto *augment = GetAugment(slot);
			if (!augment) {
				continue;
			}

			const auto augment_custom_data = augment->GetCustomDataString();
			if (augment_custom_data.empty()) {
				continue;
			}

			append_pair(GetAugmentInstanceDataKey(slot, AugmentInstanceCustomDataField), HexEncode(augment_custom_data));
			append_pair(GetAugmentInstanceDataKey(slot, AugmentInstanceGuidField), std::to_string(augment->GetSerialNumber()));
		}
	}

	return ret_val;
}


std::string EQ::ItemInstance::GetLiveItemInstanceID() const
{
	return GetCustomData("live_items.instance_id");
}

std::string EQ::ItemInstance::GetLiveItemAugmentDuplicateKey() const
{
	const auto policy = Strings::ToLower(GetCustomData("live_items.augment_duplicate_policy"));
	if (policy == "instance") {
		const auto instance_id = GetLiveItemInstanceID();
		if (!instance_id.empty()) {
			return "instance:" + instance_id;
		}
	}

	if (policy == "exclusive_key") {
		const auto exclusive_key = GetCustomData("live_items.augment_exclusive_key");
		if (!exclusive_key.empty()) {
			return "exclusive:" + exclusive_key;
		}
	}

	return "template:" + std::to_string(GetID());
}

void EQ::ItemInstance::StampLiveItemMetadata(const std::string &instance_id, const std::string &source, uint32 template_id)
{
	if (!template_id) {
		template_id = GetID();
	}

	SetCustomData("live_items.instance_id", instance_id);
	SetCustomData("live_items.template_id", static_cast<int>(template_id));
	SetCustomData("live_items.source", source);
	SetCustomData("live_items.version", 1);
}

void EQ::ItemInstance::SetCustomDataString(const std::string& str)
{
	std::map<uint8, std::string> augment_custom_data;
	std::map<uint8, int32> augment_guids;
	std::vector<std::string> components;
	for (const auto &component : Strings::Split(str, "^")) {
		if (!component.empty()) {
			components.push_back(component);
		}
	}

	auto value_count = components.size() / 2;

	for (auto i = 0; i < value_count; i++) {
		auto identifier = components[i * 2];
		auto value = components[(i * 2) + 1];

		uint8 augment_slot = 0;
		std::string augment_field;
		if (ParseAugmentInstanceDataKey(identifier, augment_slot, augment_field)) {
			if (Strings::EqualFold(augment_field, AugmentInstanceCustomDataField)) {
				augment_custom_data[augment_slot] = HexDecode(value);
			}
			else if (Strings::EqualFold(augment_field, AugmentInstanceGuidField) && Strings::IsNumber(value)) {
				augment_guids[augment_slot] = static_cast<int32>(Strings::ToUnsignedBigInt(value));
			}
			continue;
		}

		SetCustomData(identifier, value);
	}

	for (const auto &[augment_slot, custom_data] : augment_custom_data) {
		auto *augment = GetAugment(augment_slot);
		if (!augment) {
			continue;
		}

		if (!custom_data.empty()) {
			augment->SetCustomDataString(custom_data);
		}

		const auto guid_iter = augment_guids.find(augment_slot);
		if (guid_iter != augment_guids.end() && guid_iter->second != 0) {
			EQ::ItemInstance::AddGUIDToMap(guid_iter->second);
			augment->SetSerialNumber(guid_iter->second);
		}
	}

	for (const auto &[augment_slot, guid] : augment_guids) {
		if (guid == 0 || augment_custom_data.find(augment_slot) != augment_custom_data.end()) {
			continue;
		}

		auto *augment = GetAugment(augment_slot);
		if (augment) {
			EQ::ItemInstance::AddGUIDToMap(guid);
			augment->SetSerialNumber(guid);
		}
	}

	RebuildDynamicItemData();
}

std::string EQ::ItemInstance::GetCustomData(const std::string& identifier) const {
	std::map<std::string, std::string>::const_iterator iter = m_custom_data.find(identifier);
	if (iter != m_custom_data.end()) {
		return iter->second;
	}

	return "";
}

void EQ::ItemInstance::SetCustomData(const std::string& identifier, const std::string& value) {
	const bool is_dynamic = IsDynamicItemDataIdentifier(identifier);
	m_custom_data.erase(identifier);
	m_custom_data[identifier] = value;

	if (is_dynamic) {
		RebuildDynamicItemData();
		AssignNewSerialNumber();
	}
}

void EQ::ItemInstance::SetCustomData(const std::string &identifier, const char *value)
{
	SetCustomData(identifier, std::string(value ? value : ""));
}

void EQ::ItemInstance::SetCustomData(const std::string& identifier, int value) {
	const bool is_dynamic = IsDynamicItemDataIdentifier(identifier);
	m_custom_data.erase(identifier);
	std::stringstream ss;
	ss << value;
	m_custom_data[identifier] = ss.str();

	if (is_dynamic) {
		RebuildDynamicItemData();
		AssignNewSerialNumber();
	}
}

void EQ::ItemInstance::SetCustomData(const std::string& identifier, float value) {
	const bool is_dynamic = IsDynamicItemDataIdentifier(identifier);
	m_custom_data.erase(identifier);
	std::stringstream ss;
	ss << value;
	m_custom_data[identifier] = ss.str();

	if (is_dynamic) {
		RebuildDynamicItemData();
		AssignNewSerialNumber();
	}
}

void EQ::ItemInstance::SetCustomData(const std::string& identifier, bool value) {
	const bool is_dynamic = IsDynamicItemDataIdentifier(identifier);
	m_custom_data.erase(identifier);
	std::stringstream ss;
	ss << value;
	m_custom_data[identifier] = ss.str();

	if (is_dynamic) {
		RebuildDynamicItemData();
		AssignNewSerialNumber();
	}
}

void EQ::ItemInstance::DeleteCustomData(const std::string& identifier) {
	auto iter = m_custom_data.find(identifier);
	if (iter != m_custom_data.end()) {
		const bool is_dynamic = IsDynamicItemDataIdentifier(identifier);
		m_custom_data.erase(iter);

		if (is_dynamic) {
			RebuildDynamicItemData();
			AssignNewSerialNumber();
		}
	}
}

void EQ::ItemInstance::SetDynamicItemModifier(const std::string &identifier, int value)
{
	SetCustomData(GetDynamicItemKey(DynamicItemModPrefix, identifier), value);
}

void EQ::ItemInstance::SetDynamicItemData(const std::string &identifier, const std::string& value)
{
	SetCustomData(GetDynamicItemKey(DynamicItemSetPrefix, identifier), value);
}

void EQ::ItemInstance::SetDynamicItemData(const std::string &identifier, int value)
{
	SetCustomData(GetDynamicItemKey(DynamicItemSetPrefix, identifier), value);
}

void EQ::ItemInstance::DeleteDynamicItemModifier(const std::string& identifier)
{
	DeleteCustomData(GetDynamicItemKey(DynamicItemModPrefix, identifier));
}

void EQ::ItemInstance::DeleteDynamicItemData(const std::string& identifier)
{
	DeleteCustomData(GetDynamicItemKey(DynamicItemSetPrefix, identifier));
}

void EQ::ItemInstance::ClearDynamicItemData()
{
	std::vector<std::string> identifiers;

	for (const auto &entry : m_custom_data) {
		if (IsDynamicItemDataIdentifier(entry.first)) {
			identifiers.push_back(entry.first);
		}
	}

	for (const auto &identifier : identifiers) {
		m_custom_data.erase(identifier);
	}

	if (!identifiers.empty()) {
		RebuildDynamicItemData();
		AssignNewSerialNumber();
	}
}

bool EQ::ItemInstance::HasDynamicItemData() const
{
	return m_dynamicItem != nullptr;
}

uint32 EQ::ItemInstance::GetClientItemID() const
{
	if (!m_item) {
		return 0;
	}

	if (!m_dynamicItem) {
		return m_item->ID;
	}

	constexpr uint32 DynamicClientItemIDMin   = 950000;
	constexpr uint32 DynamicClientItemIDRange = 49999;
	const auto serial = static_cast<uint32>(std::abs(m_SerialNumber));

	return DynamicClientItemIDMin + (serial % DynamicClientItemIDRange);
}

// Clone a type of EQ::ItemInstance object
// c++ doesn't allow a polymorphic copy constructor,
// so we have to resort to a polymorphic Clone()
EQ::ItemInstance* EQ::ItemInstance::Clone() const
{
	// Pseudo-polymorphic copy constructor
	return new ItemInstance(*this);
}

bool EQ::ItemInstance::IsSlotAllowed(int16 slot_id) const {
	if (!m_item) { return false; }
	else if (InventoryProfile::SupportsContainers(slot_id)) { return true; }
	else if (m_item->Slots & (1 << slot_id)) { return true; }
	else if (slot_id > invslot::EQUIPMENT_END) { return true; } // why do we call 'InventoryProfile::SupportsContainers' with this here?
	else { return false; }
}

bool EQ::ItemInstance::IsDroppable(bool recurse) const
{
	if (!m_item) {
		return false;
	}
	/*if (m_ornamentidfile) // not implemented
		return false;*/
	if (m_attuned) {
		return false;
	}

	if (RuleI(World, FVNoDropFlag) == FVNoDropFlagRule::Enabled && m_item->FVNoDrop == 0) {
		return true;
	}

	if (m_item->NoDrop == 0) {
		return false;
	}

	if (recurse) {
		for (auto iter: m_contents) {
			if (!iter.second) {
				continue;
			}

			if (!iter.second->IsDroppable(recurse)) {
				return false;
			}
		}
	}

	return true;
}

void EQ::ItemInstance::Initialize(SharedDatabase *db) {
	// if there's no actual item, don't do anything
	if (!m_item) {
		return;
	}

	// initialize scaling items
	if (m_item->CharmFileID != 0) {
		m_scaling = true;
		ScaleItem();
	}

	// initialize evolving items
	else if (db && m_item->LoreGroup >= 1000) {
		// not complete yet
	}
}

void EQ::ItemInstance::RebuildDynamicItemData()
{
	safe_delete(m_dynamicItem);

	if (!m_item) {
		return;
	}

	const ItemData *source_item = m_scaledItem ? m_scaledItem : m_item;

	auto apply_dynamic_data = [this, source_item](const char *prefix, bool additive) {
		for (const auto &entry : m_custom_data) {
			const auto key = Strings::ToLower(entry.first);
			if (!HasPrefix(key, prefix)) {
				continue;
			}

			if (!m_dynamicItem) {
				m_dynamicItem = new ItemData(*source_item);
			}

			ApplyDynamicItemDataField(*m_dynamicItem, entry.first.substr(std::strlen(prefix)), entry.second, additive);
		}
	};

	apply_dynamic_data(DynamicItemSetPrefix, false);
	apply_dynamic_data(DynamicItemModPrefix, true);

	if (m_dynamicItem) {
		m_dynamicItem->CharmFileID = 0;
		m_dynamicItem->CharmFile[0] = '\0';
	}
}

void EQ::ItemInstance::ScaleItem() {
	if (!m_item)
		return;

	if (m_scaledItem) {
		memcpy(m_scaledItem, m_item, sizeof(ItemData));
	}
	else {
		m_scaledItem = new ItemData(*m_item);
	}

	float Mult = (float)(GetExp()) / 10000;	// scaling is determined by exp, with 10,000 being full stats

	m_scaledItem->AStr = (int8)((float)m_item->AStr*Mult);
	m_scaledItem->ASta = (int8)((float)m_item->ASta*Mult);
	m_scaledItem->AAgi = (int8)((float)m_item->AAgi*Mult);
	m_scaledItem->ADex = (int8)((float)m_item->ADex*Mult);
	m_scaledItem->AInt = (int8)((float)m_item->AInt*Mult);
	m_scaledItem->AWis = (int8)((float)m_item->AWis*Mult);
	m_scaledItem->ACha = (int8)((float)m_item->ACha*Mult);

	m_scaledItem->MR = (int8)((float)m_item->MR*Mult);
	m_scaledItem->PR = (int8)((float)m_item->PR*Mult);
	m_scaledItem->DR = (int8)((float)m_item->DR*Mult);
	m_scaledItem->CR = (int8)((float)m_item->CR*Mult);
	m_scaledItem->FR = (int8)((float)m_item->FR*Mult);

	m_scaledItem->HP = (int32)((float)m_item->HP*Mult);
	m_scaledItem->Mana = (int32)((float)m_item->Mana*Mult);
	m_scaledItem->AC = (int32)((float)m_item->AC*Mult);

	// check these..some may not need to be modified (really need to check all stats/bonuses)
	//m_scaledItem->SkillModValue = (int32)((float)m_item->SkillModValue*Mult);
	//m_scaledItem->BaneDmgAmt = (int8)((float)m_item->BaneDmgAmt*Mult);	// watch (10 entries with charmfileid)
	m_scaledItem->BardValue = (int32)((float)m_item->BardValue*Mult);		// watch (no entries with charmfileid)
	m_scaledItem->ElemDmgAmt = (uint8)((float)m_item->ElemDmgAmt*Mult);		// watch (no entries with charmfileid)
	m_scaledItem->Damage = (uint32)((float)m_item->Damage*Mult);			// watch

	m_scaledItem->CombatEffects = (int8)((float)m_item->CombatEffects*Mult);
	m_scaledItem->Shielding = (int8)((float)m_item->Shielding*Mult);
	m_scaledItem->StunResist = (int8)((float)m_item->StunResist*Mult);
	m_scaledItem->StrikeThrough = (int8)((float)m_item->StrikeThrough*Mult);
	m_scaledItem->ExtraDmgAmt = (uint32)((float)m_item->ExtraDmgAmt*Mult);
	m_scaledItem->SpellShield = (int8)((float)m_item->SpellShield*Mult);
	m_scaledItem->Avoidance = (int8)((float)m_item->Avoidance*Mult);
	m_scaledItem->Accuracy = (int8)((float)m_item->Accuracy*Mult);

	m_scaledItem->FactionAmt1 = (int32)((float)m_item->FactionAmt1*Mult);
	m_scaledItem->FactionAmt2 = (int32)((float)m_item->FactionAmt2*Mult);
	m_scaledItem->FactionAmt3 = (int32)((float)m_item->FactionAmt3*Mult);
	m_scaledItem->FactionAmt4 = (int32)((float)m_item->FactionAmt4*Mult);

	m_scaledItem->Endur = (uint32)((float)m_item->Endur*Mult);
	m_scaledItem->DotShielding = (uint32)((float)m_item->DotShielding*Mult);
	m_scaledItem->Attack = (uint32)((float)m_item->Attack*Mult);
	m_scaledItem->Regen = (uint32)((float)m_item->Regen*Mult);
	m_scaledItem->ManaRegen = (uint32)((float)m_item->ManaRegen*Mult);
	m_scaledItem->EnduranceRegen = (uint32)((float)m_item->EnduranceRegen*Mult);
	m_scaledItem->Haste = (uint32)((float)m_item->Haste*Mult);
	m_scaledItem->DamageShield = (uint32)((float)m_item->DamageShield*Mult);

	m_scaledItem->Purity = (uint32)((float)m_item->Purity*Mult);
	m_scaledItem->BackstabDmg = (uint32)((float)m_item->BackstabDmg*Mult);
	m_scaledItem->DSMitigation = (uint32)((float)m_item->DSMitigation*Mult);
	m_scaledItem->HeroicStr = (int32)((float)m_item->HeroicStr*Mult);
	m_scaledItem->HeroicInt = (int32)((float)m_item->HeroicInt*Mult);
	m_scaledItem->HeroicWis = (int32)((float)m_item->HeroicWis*Mult);
	m_scaledItem->HeroicAgi = (int32)((float)m_item->HeroicAgi*Mult);
	m_scaledItem->HeroicDex = (int32)((float)m_item->HeroicDex*Mult);
	m_scaledItem->HeroicSta = (int32)((float)m_item->HeroicSta*Mult);
	m_scaledItem->HeroicCha = (int32)((float)m_item->HeroicCha*Mult);
	m_scaledItem->HeroicMR = (int32)((float)m_item->HeroicMR*Mult);
	m_scaledItem->HeroicFR = (int32)((float)m_item->HeroicFR*Mult);
	m_scaledItem->HeroicCR = (int32)((float)m_item->HeroicCR*Mult);
	m_scaledItem->HeroicDR = (int32)((float)m_item->HeroicDR*Mult);
	m_scaledItem->HeroicPR = (int32)((float)m_item->HeroicPR*Mult);
	m_scaledItem->HeroicSVCorrup = (int32)((float)m_item->HeroicSVCorrup*Mult);
	m_scaledItem->HealAmt = (int32)((float)m_item->HealAmt*Mult);
	m_scaledItem->SpellDmg = (int32)((float)m_item->SpellDmg*Mult);
	m_scaledItem->Clairvoyance = (uint32)((float)m_item->Clairvoyance*Mult);

	m_scaledItem->CharmFileID = 0;	// this stops the client from trying to scale the item itself.
	RebuildDynamicItemData();
}

void EQ::ItemInstance::SetTimer(std::string name, uint32 time) {
	Timer t(time);
	t.Start(time, false);
	m_timers[name] = t;
}

void EQ::ItemInstance::StopTimer(std::string name) {
	auto iter = m_timers.find(name);
	if(iter != m_timers.end()) {
		m_timers.erase(iter);
	}
}

void EQ::ItemInstance::ClearTimers() {
	m_timers.clear();
}

int EQ::ItemInstance::GetItemArmorClass(bool augments) const
{
	int ac = 0;
	const auto item = GetItem();
	if (item) {
		ac = item->AC;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					ac += GetAugment(i)->GetItemArmorClass();
	}
	return ac;
}

int EQ::ItemInstance::GetItemElementalDamage(int &magic, int &fire, int &cold, int &poison, int &disease, int &chromatic, int &prismatic, int &physical, int &corruption, bool augments) const
{
	const auto item = GetItem();
	if (item) {
		switch (item->ElemDmgType) {
		case RESIST_MAGIC:
			magic += item->ElemDmgAmt;
			break;
		case RESIST_FIRE:
			fire += item->ElemDmgAmt;
			break;
		case RESIST_COLD:
			cold += item->ElemDmgAmt;
			break;
		case RESIST_POISON:
			poison += item->ElemDmgAmt;
			break;
		case RESIST_DISEASE:
			disease += item->ElemDmgAmt;
			break;
		case RESIST_CHROMATIC:
			chromatic += item->ElemDmgAmt;
			break;
		case RESIST_PRISMATIC:
			prismatic += item->ElemDmgAmt;
			break;
		case RESIST_PHYSICAL:
			physical += item->ElemDmgAmt;
			break;
		case RESIST_CORRUPTION:
			corruption += item->ElemDmgAmt;
			break;
		}

		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					GetAugment(i)->GetItemElementalDamage(magic, fire, cold, poison, disease, chromatic, prismatic, physical, corruption);
	}
	return magic + fire + cold + poison + disease + chromatic + prismatic + physical + corruption;
}

int EQ::ItemInstance::GetItemElementalFlag(bool augments) const
{
	int flag = 0;
	const auto item = GetItem();
	if (item) {
		flag = item->ElemDmgType;
		if (flag)
			return flag;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i))
					flag = GetAugment(i)->GetItemElementalFlag();
				if (flag)
					return flag;
			}
		}
	}
	return flag;
}

int EQ::ItemInstance::GetItemElementalDamage(bool augments) const
{
	int64 damage = 0;
	const auto item = GetItem();
	if (item) {
		damage = item->ElemDmgAmt;
		if (damage)
			return damage;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i))
					damage = GetAugment(i)->GetItemElementalDamage();
				if (damage)
					return damage;
			}
		}
	}
	return damage;
}

int EQ::ItemInstance::GetItemRecommendedLevel(bool augments) const
{
	int level = 0;
	const auto item = GetItem();
	if (item) {
		level = item->RecLevel;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				int temp = 0;
				if (GetAugment(i)) {
					temp = GetAugment(i)->GetItemRecommendedLevel();
					if (temp > level)
						level = temp;
				}
			}
		}
	}

	return level;
}

int EQ::ItemInstance::GetItemRequiredLevel(bool augments) const
{
	int level = 0;
	const auto item = GetItem();
	if (item) {
		level = item->ReqLevel;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				int temp = 0;
				if (GetAugment(i)) {
					temp = GetAugment(i)->GetItemRequiredLevel();
					if (temp > level)
						level = temp;
				}
			}
		}
	}

	return level;
}

int EQ::ItemInstance::GetItemWeaponDamage(bool augments) const
{
	int64 damage = 0;
	const auto item = GetItem();
	if (item) {
		damage = item->Damage;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					damage += GetAugment(i)->GetItemWeaponDamage();
		}
	}
	return damage;
}

int EQ::ItemInstance::GetItemBackstabDamage(bool augments) const
{
	int64 damage = 0;
	const auto item = GetItem();
	if (item) {
		damage = item->BackstabDmg;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					damage += GetAugment(i)->GetItemBackstabDamage();
		}
	}
	return damage;
}

int EQ::ItemInstance::GetItemBaneDamageBody(bool augments) const
{
	int body = 0;
	const auto item = GetItem();
	if (item) {
		body = item->BaneDmgBody;
		if (body)
			return body;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i)) {
					body = GetAugment(i)->GetItemBaneDamageBody();
					if (body)
						return body;
				}
		}
	}
	return body;
}

int EQ::ItemInstance::GetItemBaneDamageRace(bool augments) const
{
	int race = Race::Doug;
	const auto item = GetItem();
	if (item) {
		race = item->BaneDmgRace;
		if (race)
			return race;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i)) {
					race = GetAugment(i)->GetItemBaneDamageRace();
					if (race)
						return race;
				}
		}
	}
	return race;
}

int EQ::ItemInstance::GetItemBaneDamageBody(uint8 against, bool augments) const
{
	int64 damage = 0;
	const auto item = GetItem();
	if (item) {
		if (item->BaneDmgBody == against)
			damage += item->BaneDmgAmt;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					damage += GetAugment(i)->GetItemBaneDamageBody(against);
		}
	}
	return damage;
}

int EQ::ItemInstance::GetItemBaneDamageRace(uint16 against, bool augments) const
{
	int64 damage = 0;
	const auto item = GetItem();
	if (item) {
		if (item->BaneDmgRace == against)
			damage += item->BaneDmgRaceAmt;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					damage += GetAugment(i)->GetItemBaneDamageRace(against);
		}
	}
	return damage;
}

int EQ::ItemInstance::GetItemMagical(bool augments) const
{
	const auto item = GetItem();
	if (item) {
		if (item->Magic)
			return 1;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i) && GetAugment(i)->GetItemMagical())
					return 1;
		}
	}
	return 0;
}

int EQ::ItemInstance::GetItemHP(bool augments) const
{
	int hp = 0;
	const auto item = GetItem();
	if (item) {
		hp = item->HP;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					hp += GetAugment(i)->GetItemHP();
	}
	return hp;
}

int EQ::ItemInstance::GetItemMana(bool augments) const
{
	int mana = 0;
	const auto item = GetItem();
	if (item) {
		mana = item->Mana;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					mana += GetAugment(i)->GetItemMana();
	}
	return mana;
}

int EQ::ItemInstance::GetItemEndur(bool augments) const
{
	int endur = 0;
	const auto item = GetItem();
	if (item) {
		endur = item->Endur;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					endur += GetAugment(i)->GetItemEndur();
	}
	return endur;
}

int EQ::ItemInstance::GetItemAttack(bool augments) const
{
	int atk = 0;
	const auto item = GetItem();
	if (item) {
		atk = item->Attack;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					atk += GetAugment(i)->GetItemAttack();
	}
	return atk;
}

int EQ::ItemInstance::GetItemStr(bool augments) const
{
	int str = 0;
	const auto item = GetItem();
	if (item) {
		str = item->AStr;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					str += GetAugment(i)->GetItemStr();
	}
	return str;
}

int EQ::ItemInstance::GetItemSta(bool augments) const
{
	int sta = 0;
	const auto item = GetItem();
	if (item) {
		sta = item->ASta;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					sta += GetAugment(i)->GetItemSta();
	}
	return sta;
}

int EQ::ItemInstance::GetItemDex(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->ADex;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemDex();
	}
	return total;
}

int EQ::ItemInstance::GetItemAgi(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->AAgi;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemAgi();
	}
	return total;
}

int EQ::ItemInstance::GetItemInt(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->AInt;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemInt();
	}
	return total;
}

int EQ::ItemInstance::GetItemWis(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->AWis;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemWis();
	}
	return total;
}

int EQ::ItemInstance::GetItemCha(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->ACha;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemCha();
	}
	return total;
}

int EQ::ItemInstance::GetItemMR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->MR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemMR();
	}
	return total;
}

int EQ::ItemInstance::GetItemFR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->FR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemFR();
	}
	return total;
}

int EQ::ItemInstance::GetItemCR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->CR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemCR();
	}
	return total;
}

int EQ::ItemInstance::GetItemPR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->PR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemPR();
	}
	return total;
}

int EQ::ItemInstance::GetItemDR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->DR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemDR();
	}
	return total;
}

int EQ::ItemInstance::GetItemCorrup(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->SVCorruption;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemCorrup();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicStr(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicStr;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicStr();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicSta(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicSta;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicSta();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicDex(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicDex;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicDex();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicAgi(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicAgi;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicAgi();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicInt(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicInt;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicInt();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicWis(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicWis;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicWis();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicCha(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicCha;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicCha();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicMR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicMR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicMR();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicFR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicFR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicFR();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicCR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicCR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicCR();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicPR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicPR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicPR();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicDR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicDR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicDR();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicCorrup(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicSVCorrup;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicCorrup();
	}
	return total;
}

int EQ::ItemInstance::GetItemHaste(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->Haste;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i)) {
					int temp = GetAugment(i)->GetItemHaste();
					if (temp > total)
						total = temp;
				}
	}
	return total;
}

int EQ::ItemInstance::RemoveTaskDeliveredItems()
{
	int count = IsStackable() ? GetCharges() : 1;
	count -= GetTaskDeliveredCount();
	if (IsStackable())
	{
		SetCharges(count);
	}
	SetTaskDeliveredCount(0);
	return count;
}

uint32 EQ::ItemInstance::GetItemGuildFavor() const
{
	uint32 total = 0;
	const auto item = GetItem();
	if (item) {
		return total = item->GuildFavor;
	}
	return 0;
}

std::vector<uint32> EQ::ItemInstance::GetAugmentIDs() const
{
	std::vector<uint32> augments;

	for (uint8 slot_id = invaug::SOCKET_BEGIN; slot_id <= invaug::SOCKET_END; slot_id++) {
		augments.push_back(GetAugment(slot_id) ? GetAugmentItemID(slot_id) : 0);
	}

	return augments;
}

std::vector<std::string> EQ::ItemInstance::GetAugmentNames() const
{
	std::vector<std::string> augment_names;

	for (uint8 slot_id = invaug::SOCKET_BEGIN; slot_id <= invaug::SOCKET_END; slot_id++) {
		const auto augment = GetAugment(slot_id);
		augment_names.push_back(augment ? augment->GetItem()->Name : "");
	}

	return augment_names;
}

int EQ::ItemInstance::GetItemRegen(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->Regen;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemRegen();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemManaRegen(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->ManaRegen;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemManaRegen();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemDamageShield(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->DamageShield;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemDamageShield();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemDSMitigation(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->DSMitigation;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemDSMitigation();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemHealAmt(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->HealAmt;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemHealAmt();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemSpellDamage(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->SpellDmg;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemSpellDamage();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemClairvoyance(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->Clairvoyance;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemClairvoyance();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemSkillsStat(EQ::skills::SkillType skill, bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->ExtraDmgSkill == skill ? item->ExtraDmgAmt : 0;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemSkillsStat(skill);
				}
			}
		}
	}
	return stat;
}

void EQ::ItemInstance::AddGUIDToMap(uint64 existing_serial_number)
{
	guids.emplace(existing_serial_number);
}

void EQ::ItemInstance::ClearGUIDMap()
{
	guids.clear();
}

bool EQ::ItemInstance::TransferOwnership(Database &db, const uint32 to_char_id) const
{
	if (!to_char_id || !IsEvolving()) {
		return false;
	}

	SetEvolveCharID(to_char_id);
	CharacterEvolvingItemsRepository::UpdateCharID(db, GetEvolveUniqueID(), to_char_id);
	return true;
}

uint32 EQ::ItemInstance::GetAugmentEvolveUniqueID(uint8 augment_index) const
{
	if (!m_item || !m_item->IsClassCommon()) {
		return 0;
	}

	const auto item = GetItem(augment_index);
	if (item) {
		return item->GetEvolveUniqueID();
	}

	return 0;
}

void EQ::ItemInstance::SetTimer(std::string name, uint32 time) const{
	Timer t(time);
	t.Start(time, false);
	m_timers[name] = t;
}

void EQ::ItemInstance::SetEvolveEquipped(const bool in) const
{
	if (!IsEvolving()) {
		return;
	}

	m_evolving_details.equipped = in;
	if (in && !GetTimers().contains("evolve")) {
		SetTimer("evolve", RuleI(EvolvingItems, DelayUponEquipping));
		return;
	}

	if (in) {
		GetTimers().at("evolve").SetTimer(RuleI(EvolvingItems, DelayUponEquipping));
		return;
	}

	GetTimers().at("evolve").Disable();
}

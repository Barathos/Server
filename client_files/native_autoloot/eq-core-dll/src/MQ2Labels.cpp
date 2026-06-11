// MQ2Labels.cpp : Defines the entry point for the DLL application.
//

// MQ2 Custom Labels


#include "MQ2Main.h"
#include <map>
#include <string>

typedef string(*pEqTypesFunc)();

extern bool NativeHpFixGetEqTypeLabel(DWORD eq_type, const char* control_name, char* out, size_t out_size);
extern bool NativeHpFixGetClientHpValues(int* current, int* maximum, int* percent_out);
extern bool isThjClientEnabled;

map<DWORD, pEqTypesFunc> eqTypesMap;
map<DWORD, DWORD> gThjStatLabelsMap;
map<DWORD, pEqTypesFunc> gThjFuncLabelsMap;

namespace {

constexpr WORD NativeOpServerAuthStats = 0x1338;
constexpr DWORD NativeStatClassesBitmask = 1;
constexpr DWORD NativeMaxServerAuthStat = 256;

#pragma pack(push, 1)
struct NativeServerAuthStatEntry {
	DWORD StatKey;
	ULONGLONG StatValue;
};
#pragma pack(pop)

ULONGLONG gNativeServerAuthStats[NativeMaxServerAuthStat] = {0};
bool gNativeServerAuthStatsSeen[NativeMaxServerAuthStat] = {false};

bool NativeHasServerAuthStat(DWORD stat_key)
{
	return stat_key < NativeMaxServerAuthStat && gNativeServerAuthStatsSeen[stat_key];
}

bool NativeHpFixIsLocalCharacterObject(EQ_Character1* character)
{
	PCHARINFO char_info = GetCharInfo();
	return char_info && character == (EQ_Character1*)&char_info->vtable2;
}

ULONGLONG NativeGetServerAuthStat(DWORD stat_key)
{
	return NativeHasServerAuthStat(stat_key) ? gNativeServerAuthStats[stat_key] : 0;
}

std::string NativeClassListLabel(bool abbreviations)
{
	if (!NativeHasServerAuthStat(NativeStatClassesBitmask)) {
		return "";
	}

	static const char* class_names[] = {
		"Warrior", "Cleric", "Paladin", "Ranger", "Shadow Knight", "Druid",
		"Monk", "Bard", "Rogue", "Shaman", "Necromancer", "Wizard",
		"Magician", "Enchanter", "Beastlord", "Berserker"
	};

	static const char* class_abbreviations[] = {
		"WAR", "CLR", "PAL", "RNG", "SHD", "DRU", "MNK", "BRD",
		"ROG", "SHM", "NEC", "WIZ", "MAG", "ENC", "BST", "BER"
	};

	const auto class_bits = NativeGetServerAuthStat(NativeStatClassesBitmask);
	const auto* labels = abbreviations ? class_abbreviations : class_names;
	std::string text;

	for (DWORD i = 0; i < (sizeof(class_names) / sizeof(class_names[0])); ++i) {
		if (!(class_bits & (1ULL << i))) {
			continue;
		}

		if (!text.empty()) {
			text += "\n";
		}
		text += labels[i];
	}

	return text;
}

std::string NativeClassNamesLabel()
{
	return NativeClassListLabel(false);
}

std::string NativeClassAbbreviationsLabel()
{
	return NativeClassListLabel(true);
}

} // namespace

// ---- THJ custom inventory stat labels (active when isThjClientEnabled) ----
// Ported from the THJ dinput8 reconstruction. THJ sends these values as
// OP_ServerAuthStats keys (already stored in gNativeServerAuthStats); this maps the
// THJ inventory EQTypes to those keys plus a few computed labels so the THJ Stats
// window renders real values instead of "Unknown".

static std::string ThjFormatStat(unsigned int statKey)
{
	if (!NativeHasServerAuthStat(statKey)) {
		return "";
	}
	char buffer[64] = {0};
	_snprintf(buffer, sizeof(buffer), "%I64u", NativeGetServerAuthStat(statKey));
	buffer[sizeof(buffer) - 1] = 0;
	return buffer;
}

static std::string ThjFormatStatDifference(unsigned int positiveStatKey, unsigned int negativeStatKey)
{
	if (!NativeHasServerAuthStat(positiveStatKey) || !NativeHasServerAuthStat(negativeStatKey)) {
		return "";
	}
	long long value = (long long)NativeGetServerAuthStat(positiveStatKey) -
		(long long)NativeGetServerAuthStat(negativeStatKey);
	char buffer[64] = {0};
	_snprintf(buffer, sizeof(buffer), "%I64d", value);
	buffer[sizeof(buffer) - 1] = 0;
	return buffer;
}

template <unsigned int PositiveStatKey, unsigned int NegativeStatKey>
static std::string ThjStatDeltaLabel()
{
	return ThjFormatStatDifference(PositiveStatKey, NegativeStatKey);
}

static std::string ThjItemExpLabel()
{
	if (!NativeHasServerAuthStat(69)) {
		return "";
	}
	unsigned long long value = NativeGetServerAuthStat(69);
	unsigned long long whole = value / 1000;
	unsigned int fraction = (unsigned int)((value % 1000) / 10);
	char buffer[64] = {0};
	_snprintf(buffer, sizeof(buffer), "%I64u.%02u%%", whole, fraction);
	buffer[sizeof(buffer) - 1] = 0;
	return buffer;
}

static bool ThjGetCustomLabel(DWORD eqType, std::string& out)
{
	auto funcIt = gThjFuncLabelsMap.find(eqType);
	if (funcIt != gThjFuncLabelsMap.end()) {
		if (funcIt->second) {
			out = (*funcIt->second)();
		}
		return true;
	}
	auto statIt = gThjStatLabelsMap.find(eqType);
	if (statIt != gThjStatLabelsMap.end()) {
		out = ThjFormatStat(statIt->second);
		return true;
	}
	return false;
}

static void ThjRegisterStatLabel(DWORD eqType, DWORD statKey)
{
	gThjStatLabelsMap[eqType] = statKey;
}

static void ThjRegisterStatLabels()
{
	ThjRegisterStatLabel(5, 10);
	ThjRegisterStatLabel(6, 11);
	ThjRegisterStatLabel(7, 12);
	ThjRegisterStatLabel(8, 13);
	ThjRegisterStatLabel(9, 15);
	ThjRegisterStatLabel(10, 14);
	ThjRegisterStatLabel(11, 16);
	ThjRegisterStatLabel(12, 21);
	ThjRegisterStatLabel(13, 20);
	ThjRegisterStatLabel(14, 18);
	ThjRegisterStatLabel(15, 19);
	ThjRegisterStatLabel(16, 17);
	ThjRegisterStatLabel(22, 39);
	ThjRegisterStatLabel(23, 25);
	ThjRegisterStatLabel(211, 26);
	ThjRegisterStatLabel(212, 22);
	ThjRegisterStatLabel(213, 23);
	ThjRegisterStatLabel(214, 24);
	ThjRegisterStatLabel(215, 43);
	ThjRegisterStatLabel(216, 42);
	ThjRegisterStatLabel(217, 44);
	ThjRegisterStatLabel(218, 45);
	ThjRegisterStatLabel(219, 46);
	ThjRegisterStatLabel(220, 47);
	ThjRegisterStatLabel(221, 48);
	ThjRegisterStatLabel(222, 49);
	ThjRegisterStatLabel(223, 50);
	ThjRegisterStatLabel(225, 41);
	ThjRegisterStatLabel(226, 40);
	ThjRegisterStatLabel(227, 52);
	ThjRegisterStatLabel(238, 10);
	ThjRegisterStatLabel(239, 11);
	ThjRegisterStatLabel(240, 12);
	ThjRegisterStatLabel(241, 13);
	ThjRegisterStatLabel(242, 15);
	ThjRegisterStatLabel(243, 14);
	ThjRegisterStatLabel(244, 16);
	ThjRegisterStatLabel(245, 21);
	ThjRegisterStatLabel(246, 20);
	ThjRegisterStatLabel(247, 18);
	ThjRegisterStatLabel(248, 19);
	ThjRegisterStatLabel(249, 17);
	ThjRegisterStatLabel(251, 27);
	ThjRegisterStatLabel(252, 28);
	ThjRegisterStatLabel(253, 29);
	ThjRegisterStatLabel(254, 30);
	ThjRegisterStatLabel(255, 32);
	ThjRegisterStatLabel(256, 31);
	ThjRegisterStatLabel(257, 33);
	ThjRegisterStatLabel(258, 38);
	ThjRegisterStatLabel(259, 37);
	ThjRegisterStatLabel(260, 35);
	ThjRegisterStatLabel(261, 36);
	ThjRegisterStatLabel(262, 34);
	ThjRegisterStatLabel(264, 119);
	ThjRegisterStatLabel(265, 120);
	ThjRegisterStatLabel(266, 121);
	ThjRegisterStatLabel(267, 122);
	ThjRegisterStatLabel(268, 124);
	ThjRegisterStatLabel(269, 123);
	ThjRegisterStatLabel(270, 125);
	ThjRegisterStatLabel(271, 130);
	ThjRegisterStatLabel(272, 129);
	ThjRegisterStatLabel(273, 127);
	ThjRegisterStatLabel(274, 128);
	ThjRegisterStatLabel(275, 126);
	ThjRegisterStatLabel(277, 102);
	ThjRegisterStatLabel(279, 103);
	ThjRegisterStatLabel(280, 104);
	ThjRegisterStatLabel(281, 105);
	ThjRegisterStatLabel(282, 106);
	ThjRegisterStatLabel(283, 107);
	ThjRegisterStatLabel(284, 108);
	ThjRegisterStatLabel(285, 109);
	ThjRegisterStatLabel(286, 109);
	ThjRegisterStatLabel(6667, 9);
	ThjRegisterStatLabel(6668, 8);
	ThjRegisterStatLabel(6669, 68);
	ThjRegisterStatLabel(6670, 69);
	ThjRegisterStatLabel(6671, 70);
	ThjRegisterStatLabel(6672, 71);
	ThjRegisterStatLabel(6673, 72);
	ThjRegisterStatLabel(6674, 73);
	ThjRegisterStatLabel(6675, 74);
	ThjRegisterStatLabel(6676, 75);
	ThjRegisterStatLabel(6677, 76);
	ThjRegisterStatLabel(6678, 77);
	ThjRegisterStatLabel(6679, 78);
	ThjRegisterStatLabel(6680, 79);
	ThjRegisterStatLabel(6681, 80);
	ThjRegisterStatLabel(6682, 81);
	ThjRegisterStatLabel(6683, 82);
	ThjRegisterStatLabel(6684, 83);
	ThjRegisterStatLabel(6685, 84);
	ThjRegisterStatLabel(6686, 85);
	ThjRegisterStatLabel(6687, 86);
	ThjRegisterStatLabel(6688, 87);
	ThjRegisterStatLabel(6689, 88);
	ThjRegisterStatLabel(6690, 89);
	ThjRegisterStatLabel(6691, 90);
	ThjRegisterStatLabel(6692, 91);
	ThjRegisterStatLabel(6693, 92);
	ThjRegisterStatLabel(6694, 93);
	ThjRegisterStatLabel(6695, 94);
	ThjRegisterStatLabel(6696, 95);
	ThjRegisterStatLabel(6697, 96);
	ThjRegisterStatLabel(6698, 97);
	ThjRegisterStatLabel(6699, 98);
	ThjRegisterStatLabel(6700, 99);
	ThjRegisterStatLabel(6701, 100);
	ThjRegisterStatLabel(6702, 101);
	ThjRegisterStatLabel(6703, 115);
	ThjRegisterStatLabel(6704, 112);
	ThjRegisterStatLabel(6705, 113);
	ThjRegisterStatLabel(6706, 114);
	ThjRegisterStatLabel(6707, 116);
	ThjRegisterStatLabel(6708, 117);
	ThjRegisterStatLabel(6709, 118);

	// Computed labels take precedence over the raw stat map for these EQTypes.
	gThjFuncLabelsMap[6670] = ThjItemExpLabel;
	gThjFuncLabelsMap[264] = ThjStatDeltaLabel<119, 27>;
	gThjFuncLabelsMap[265] = ThjStatDeltaLabel<120, 28>;
	gThjFuncLabelsMap[266] = ThjStatDeltaLabel<121, 29>;
	gThjFuncLabelsMap[267] = ThjStatDeltaLabel<122, 30>;
	gThjFuncLabelsMap[268] = ThjStatDeltaLabel<124, 32>;
	gThjFuncLabelsMap[269] = ThjStatDeltaLabel<123, 31>;
	gThjFuncLabelsMap[270] = ThjStatDeltaLabel<125, 33>;
	gThjFuncLabelsMap[271] = ThjStatDeltaLabel<130, 38>;
	gThjFuncLabelsMap[272] = ThjStatDeltaLabel<129, 37>;
	gThjFuncLabelsMap[273] = ThjStatDeltaLabel<127, 35>;
	gThjFuncLabelsMap[274] = ThjStatDeltaLabel<128, 36>;
	gThjFuncLabelsMap[275] = ThjStatDeltaLabel<126, 34>;
}

void NativeLabelsHandleWorldMessage(unsigned __int16 opcode, const char* buffer, size_t size)
{
	if (opcode != NativeOpServerAuthStats || !buffer || size < sizeof(DWORD)) {
		return;
	}

	const DWORD count = *(const DWORD*)buffer;
	const size_t max_entries = (size - sizeof(DWORD)) / sizeof(NativeServerAuthStatEntry);
	if (count > max_entries) {
		return;
	}

	const auto* entries = (const NativeServerAuthStatEntry*)(buffer + sizeof(DWORD));
	for (DWORD i = 0; i < count; ++i) {
		const DWORD stat_key = entries[i].StatKey;
		if (stat_key > 0 && stat_key < NativeMaxServerAuthStat) {
			gNativeServerAuthStats[stat_key] = entries[i].StatValue;
			gNativeServerAuthStatsSeen[stat_key] = true;
		}
	}
}

// CSidlManager::CreateLabel 0x5F2470

// the tool tip is already copied out of the 
// in class CControlTemplate.  use this struct
// to mock up the class, so we don't have to
// worry about class instatiation and crap

struct _CControl {
    /*0x000*/    DWORD Fluff[0x24]; // if this changes update ISXEQLabels.cpp too
    /*0x090*/    CXSTR * EQType;
};

// optimize off because the tramp looks blank to the compiler
// and it doesn't respect the fact the it will be a real routine
#pragma optimize ("g", off)

class CSidlManagerHook {
public:
    class CXWnd * CreateLabel_Trampoline(class CXWnd *, struct _CControl *);
    class CXWnd * CreateLabel_Detour(class CXWnd *CWin, struct _CControl *CControl)
    {
        CLABELWND *p;
        class CXWnd *tmp = CreateLabel_Trampoline(CWin, CControl);
        p = (CLABELWND *)tmp;
        if (CControl->EQType) {
            *((DWORD *)&p->SidlPiece) = atoi(CControl->EQType->Text);
        } else {
            *((DWORD *)&p->SidlPiece) = 0;
        }

        return tmp;
    }
};

DETOUR_TRAMPOLINE_EMPTY(class CXWnd * CSidlManagerHook::CreateLabel_Trampoline(class CXWnd *, struct _CControl *));

#pragma optimize ("g", on)

// CLabelHook::Draw_Detour

class CLabelHook {
public:
    VOID Draw_Trampoline(VOID);
    VOID Draw_Detour(VOID)
    {
        PCLABELWND pThisLabel;
        __asm {mov [pThisLabel], ecx};
        //          (PCLABELWND)this;
        Draw_Trampoline();
        CHAR Buffer[MAX_STRING] = {0};
        BOOL Found=FALSE;
        DWORD index;

		std::string eqtypesString = "";


		auto eqtype_iter = eqTypesMap.find((DWORD)pThisLabel->SidlPiece);
		CHAR control_name[MAX_STRING] = {0};
		__try {
			if (CXMLData* xml_data = ((CXWnd*)pThisLabel)->GetXMLData()) {
				GetCXStr(xml_data->Name.Ptr, control_name, MAX_STRING);
			}
		}
		__except(EXCEPTION_EXECUTE_HANDLER) {
			control_name[0] = 0;
		}

		CHAR hpfix_text[MAX_STRING] = {0};
		if (NativeHpFixGetEqTypeLabel((DWORD)pThisLabel->SidlPiece, control_name, hpfix_text, sizeof(hpfix_text))) {
			eqtypesString = hpfix_text;
			Found = TRUE;
		}
		else if (eqtype_iter != eqTypesMap.end()) {
			auto func = eqtype_iter->second;
			if (func) {
				eqtypesString = (*func)();
				Found = TRUE;
			}
        } else if (isThjClientEnabled && ThjGetCustomLabel((DWORD)pThisLabel->SidlPiece, eqtypesString)) {
            Found = !eqtypesString.empty();
        } else if ((DWORD)pThisLabel->SidlPiece==9999) {
            if (!pThisLabel->Wnd.XMLToolTip) {
                strcpy(Buffer,"BadCustom");
                Found=TRUE;
            } else {
                //strcpy(Buffer,&pThisLabel->XMLToolTip->Text[0]);
                STMLToPlainText(&pThisLabel->Wnd.XMLToolTip->Text[0],Buffer);
                ParseMacroParameter(((PCHARINFO)pCharData)->pSpawn,Buffer);
                if (!strcmp(Buffer,"NULL"))
                    Buffer[0]=0;
                Found=TRUE;
            }
        }
        if (Found) SetCXStr(&(pThisLabel->Wnd.WindowText),(PCHAR)eqtypesString.c_str());
    }
}; 

DETOUR_TRAMPOLINE_EMPTY(VOID CLabelHook::Draw_Trampoline(VOID));

class NativeHpFixCharacterHook {
public:
	int CurHP_Trampoline(int unknown, unsigned char include_bonuses);
	int CurHP_Detour(int unknown, unsigned char include_bonuses)
	{
		int current = 0;
		if (NativeHpFixIsLocalCharacterObject((EQ_Character1*)this) &&
			NativeHpFixGetClientHpValues(&current, nullptr, nullptr)) {
			return current;
		}

		return CurHP_Trampoline(unknown, include_bonuses);
	}

	int MaxHP_Trampoline(int unknown, int include_bonuses);
	int MaxHP_Detour(int unknown, int include_bonuses)
	{
		int maximum = 0;
		if (NativeHpFixIsLocalCharacterObject((EQ_Character1*)this) &&
			NativeHpFixGetClientHpValues(nullptr, &maximum, nullptr)) {
			return maximum;
		}

		return MaxHP_Trampoline(unknown, include_bonuses);
	}
};

DETOUR_TRAMPOLINE_EMPTY(int NativeHpFixCharacterHook::CurHP_Trampoline(int, unsigned char));
DETOUR_TRAMPOLINE_EMPTY(int NativeHpFixCharacterHook::MaxHP_Trampoline(int, int));

BOOL StealNextGauge=FALSE;
DWORD NextGauge=0;

std::string testDisplayFunction()
{
	return "Test";
}

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializeMQ2Labels(VOID)
{
	static bool initialized = false;
	if (initialized) {
		return;
	}

	initialized = true;
 //   DebugSpewAlways("Initializing MQ2Labels");
	eqTypesMap[1000] = testDisplayFunction; //and so forth 
	eqTypesMap[3] = NativeClassNamesLabel;
	eqTypesMap[6666] = NativeClassAbbreviationsLabel;

	if (isThjClientEnabled) {
		ThjRegisterStatLabels();
	}

    // Add commands, macro parameters, hooks, etc.
    //EasyClassDetour(CLabel__Draw,CLabelHook,Draw_Detour,VOID,(VOID),Draw_Trampoline);
    EzDetour(CLabel__Draw,&CLabelHook::Draw_Detour,&CLabelHook::Draw_Trampoline);
    EzDetour(CSidlManager__CreateLabel,&CSidlManagerHook::CreateLabel_Detour,&CSidlManagerHook::CreateLabel_Trampoline);
    EzDetour(EQ_Character__Cur_HP,&NativeHpFixCharacterHook::CurHP_Detour,&NativeHpFixCharacterHook::CurHP_Trampoline);
    EzDetour(EQ_Character__Max_HP,&NativeHpFixCharacterHook::MaxHP_Detour,&NativeHpFixCharacterHook::MaxHP_Trampoline);


    // currently in testing:
    //    EasyClassDetour(CGauge__Draw,CGaugeHook,Draw_Detour,VOID,(VOID),Draw_Trampoline);
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownLabelsPlugin(VOID)
{
   // DebugSpewAlways("Shutting down MQ2Labels");

    // Remove commands, macro parameters, hooks, etc.
    RemoveDetour(CSidlManager__CreateLabel);
    RemoveDetour(CLabel__Draw);
    RemoveDetour(EQ_Character__Cur_HP);
    RemoveDetour(EQ_Character__Max_HP);
    //RemoveDetour(CGaugeWnd__Draw);
}


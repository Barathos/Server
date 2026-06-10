// MQ2Labels.cpp : Defines the entry point for the DLL application.
//

// MQ2 Custom Labels


#include "MQ2Main.h"
#include <map>
#include <string>

typedef string(*pEqTypesFunc)();

extern bool NativeHpFixGetEqTypeLabel(DWORD eq_type, const char* control_name, char* out, size_t out_size);
extern bool NativeHpFixGetClientHpValues(int* current, int* maximum, int* percent_out);

map<DWORD, pEqTypesFunc> eqTypesMap;

namespace {

constexpr WORD NativeOpServerAuthStats = 0x1338;
constexpr DWORD NativeStatClassesBitmask = 1;
constexpr DWORD NativeMaxServerAuthStat = 128;

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


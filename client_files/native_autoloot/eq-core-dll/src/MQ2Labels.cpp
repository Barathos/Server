// MQ2Labels.cpp : Defines the entry point for the DLL application.
//

// MQ2 Custom Labels


#include "MQ2Main.h"
#include <map>
#include <string>

typedef string(*pEqTypesFunc)();

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
		if (eqtype_iter != eqTypesMap.end()) {
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

BOOL StealNextGauge=FALSE;
DWORD NextGauge=0;

std::string testDisplayFunction()
{
	return "Test";
}

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializeMQ2Labels(VOID)
{
 //   DebugSpewAlways("Initializing MQ2Labels");
	eqTypesMap[1000] = testDisplayFunction; //and so forth 
	eqTypesMap[3] = NativeClassNamesLabel;
	eqTypesMap[6666] = NativeClassAbbreviationsLabel;

    // Add commands, macro parameters, hooks, etc.
    //EasyClassDetour(CLabel__Draw,CLabelHook,Draw_Detour,VOID,(VOID),Draw_Trampoline);
    EzDetour(CLabel__Draw,&CLabelHook::Draw_Detour,&CLabelHook::Draw_Trampoline);
    EzDetour(CSidlManager__CreateLabel,&CSidlManagerHook::CreateLabel_Detour,&CSidlManagerHook::CreateLabel_Trampoline);


    // currently in testing:
    //    EasyClassDetour(CGauge__Draw,CGaugeHook,Draw_Detour,VOID,(VOID),Draw_Trampoline);
    //    EasyDetour(__GetGaugeValueFromEQ,GetGaugeValueFromEQ_Hook,int,(int,class CXStr *,bool *),GetGaugeValueFromEQ_Trampoline);
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownLabelsPlugin(VOID)
{
   // DebugSpewAlways("Shutting down MQ2Labels");

    // Remove commands, macro parameters, hooks, etc.
    RemoveDetour(CSidlManager__CreateLabel);
    RemoveDetour(CLabel__Draw);
    //RemoveDetour(CGaugeWnd__Draw);
    //RemoveDetour(__GetGaugeValueFromEQ);
}


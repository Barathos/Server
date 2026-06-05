#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <dinput.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if !defined(_M_IX86)
#error This DLL is intended for the 32-bit EQ client and must be built as Win32.
#endif

namespace nativeinterface {

constexpr uintptr_t kEqImageBase = 0x400000;
constexpr uintptr_t kPInstSpawnManager = 0xE641D0;
constexpr uintptr_t kPInstCMapViewWnd = 0xD1FC54;
constexpr uintptr_t kPInstCharData = 0xDD261C;
constexpr uintptr_t kPInstCharSpawn = 0xDD2644;
constexpr uintptr_t kPInstTarget = 0xDD2648;
constexpr uintptr_t kPInstSpellManager = 0xE646B0;
constexpr uintptr_t kPInstCEverQuest = 0xE67CCC;

constexpr uintptr_t kPInstCTargetWnd = 0xD1FC60;
constexpr uintptr_t kPInstCPlayerWnd = 0xD1FC68;
constexpr uintptr_t kPInstCCastSpellWnd = 0xD1FC84;
constexpr uintptr_t kPInstCInventoryWnd = 0xD1FC8C;

constexpr uintptr_t kPInstEQItemListCandidates[] = {
    0xDCD9A8,
    0xDCF8E0,
    0xDCFD18,
};

constexpr uintptr_t kCEverQuestDspChat = 0x51F1A0;
constexpr uintptr_t kCEverQuestInterpretCmd = 0x51FCE0;
constexpr uintptr_t kEQCharacterGetConLevel = 0x577CB0;
constexpr uintptr_t kCItemDisplayWndUpdateStrings = 0x69AE30;
constexpr uintptr_t kCStmlWndAppendSTML = 0x886720;
constexpr uintptr_t kCStmlWndForceParseNow = 0x887070;
constexpr uintptr_t kCXStrCtorCString = 0x805C20;
constexpr uintptr_t kCXStrDtor = 0x465AE0;

constexpr size_t kSpawnNext = 0x008;
constexpr size_t kSpawnY = 0x064;
constexpr size_t kSpawnX = 0x068;
constexpr size_t kSpawnZ = 0x06c;
constexpr size_t kSpawnSpeedY = 0x070;
constexpr size_t kSpawnSpeedX = 0x074;
constexpr size_t kSpawnSpeedZ = 0x078;
constexpr size_t kSpawnHeading = 0x080;
constexpr size_t kSpawnName = 0x0a4;
constexpr size_t kSpawnDisplayedName = 0x0e4;
constexpr size_t kSpawnType = 0x125;
constexpr size_t kSpawnID = 0x148;
constexpr size_t kSpawnLevel = 0x250;
constexpr size_t kSpawnGM = 0x25c;

constexpr size_t kSpawnManagerFirstSpawn = 0x008;

constexpr size_t kMapViewVTable = 0x358;
constexpr size_t kMapLines = 0x5a8;
constexpr size_t kMapLabels = 0x5ac;
constexpr size_t kMapVTableBytes = 0x17c;
constexpr size_t kMapPostDraw2Index = 4;
constexpr uint32_t kMapTargetColor = 0xFFFF4040;
constexpr uint32_t kMapXTargetColor = 0xFFFF80FF;
constexpr uint32_t kMapGroundColor = 0xFFC0C0C0;
constexpr uint32_t kMapLocationColor = 0xFF00FF80;
constexpr uint32_t kMapRadiusColor = 0xFF808080;
constexpr float kPi = 3.14159265358979323846f;

constexpr size_t kGroundItemNext = 0x04;
constexpr size_t kGroundItemDropID = 0x0c;
constexpr size_t kGroundItemName = 0x1c;
constexpr size_t kGroundItemY = 0x70;
constexpr size_t kGroundItemX = 0x74;
constexpr size_t kGroundItemZ = 0x78;

constexpr size_t kCharDataXTargetMgr = 0x2460;
constexpr size_t kXTargetMgrSlots = 0x04;
constexpr size_t kXTargetMgrArray = 0x08;
constexpr size_t kXTargetSlotSize = 0x4c;
constexpr size_t kXTargetType = 0x00;
constexpr size_t kXTargetSpawnID = 0x08;
constexpr size_t kXTargetName = 0x0c;
constexpr size_t kMaxXTargets = 20;

constexpr size_t kSpellWindowDisplayWnd = 0x210;
constexpr size_t kSpellWindowSpellID = 0x238;

constexpr size_t kItemWindowPItemOffsets[] = {
    0x2b0, // This client: Known live/test pItem offset shifted down with DisplayWnd.
    0x2c0, // Known beta reference.
    0x2c8, // Known live/test reference.
    0x2ac, // Previous local guess; retained only as a fallback.
};

constexpr size_t kItemWindowDisplayOffsets[] = {
    0x220,
    0x21c,
    0x230,
    0x238,
};

constexpr size_t kContentsItemOffsets[] = {
    0x24,  // Older CONTENTS::Item1 reference.
    0x88,  // Test/beta CONTENTS::Item1 reference.
    0x9c,  // Previous local guess; retained only if it validates as ITEMINFO.
    0x13c, // CONTENTS::Item2 reference.
    0x144, // Previous local fallback; retained only if it validates as ITEMINFO.
};

constexpr size_t kItemName = 0x000;
constexpr size_t kItemLoreName = 0x040;
constexpr size_t kItemIDFile = 0x0b0;
constexpr size_t kItemNumber = 0x0ec;
constexpr size_t kItemEquipSlots = 0x0f0;
constexpr size_t kItemCost = 0x0f4;
constexpr size_t kItemIconNumber = 0x0f8;
constexpr size_t kItemWeight = 0x108;
constexpr size_t kItemNoRent = 0x10c;
constexpr size_t kItemNoDrop = 0x10d;
constexpr size_t kItemAttuneable = 0x10e;
constexpr size_t kItemSize = 0x119;
constexpr size_t kItemType = 0x11a;
constexpr size_t kItemTradeSkills = 0x11b;
constexpr size_t kItemLore = 0x11c;
constexpr size_t kItemArtifact = 0x120;
constexpr size_t kItemSummoned = 0x121;
constexpr size_t kItemStr = 0x122;
constexpr size_t kItemSta = 0x124;
constexpr size_t kItemCha = 0x126;
constexpr size_t kItemDex = 0x128;
constexpr size_t kItemInt = 0x12a;
constexpr size_t kItemAgi = 0x12c;
constexpr size_t kItemWis = 0x12e;
constexpr size_t kItemHP = 0x130;
constexpr size_t kItemMana = 0x134;
constexpr size_t kItemAC = 0x138;
constexpr size_t kItemRequiredLevel = 0x13c;
constexpr size_t kItemRecommendedLevel = 0x140;
constexpr size_t kItemClasses = 0x170;
constexpr size_t kItemRaces = 0x174;
constexpr size_t kItemDeity = 0x178;
constexpr size_t kItemMagic = 0x180;
constexpr size_t kItemDelay = 0x182;
constexpr size_t kItemRange = 0x185;
constexpr size_t kItemDamage = 0x188;
constexpr size_t kItemAugSlot1 = 0x1ec;
constexpr size_t kItemAugSlot2 = 0x1f4;
constexpr size_t kItemAugSlot3 = 0x1fc;
constexpr size_t kItemAugSlot4 = 0x204;
constexpr size_t kItemAugSlot5 = 0x20c;
constexpr size_t kItemAugSlot6 = 0x214;
constexpr size_t kItemAugType = 0x21c;
constexpr size_t kItemAugRestrictions = 0x220;
constexpr size_t kItemClicky = 0x284;
constexpr size_t kItemProc = 0x2e8;
constexpr size_t kItemWorn = 0x34c;
constexpr size_t kItemFocus = 0x3b0;
constexpr size_t kItemScroll = 0x414;
constexpr size_t kItemCombatEffects = 0x4f0;
constexpr size_t kItemShielding = 0x4f4;
constexpr size_t kItemStunResist = 0x4f8;
constexpr size_t kItemDoTShielding = 0x4fc;
constexpr size_t kItemStrikeThrough = 0x500;
constexpr size_t kItemDmgBonusSkill = 0x504;
constexpr size_t kItemDmgBonusValue = 0x508;
constexpr size_t kItemSpellShield = 0x50c;
constexpr size_t kItemAvoidance = 0x510;
constexpr size_t kItemAccuracy = 0x514;
constexpr size_t kItemCastTime = 0x51c;
constexpr size_t kItemCombine = 0x520;
constexpr size_t kItemSlots = 0x521;
constexpr size_t kItemSizeCapacity = 0x522;
constexpr size_t kItemWeightReduction = 0x523;
constexpr size_t kItemEndurance = 0x550;
constexpr size_t kItemAttack = 0x554;
constexpr size_t kItemHPRegen = 0x558;
constexpr size_t kItemManaRegen = 0x55c;
constexpr size_t kItemEnduranceRegen = 0x560;
constexpr size_t kItemHaste = 0x564;
constexpr size_t kItemDamShield = 0x568;
constexpr size_t kItemStackSize = 0x580;
constexpr size_t kItemMaxPower = 0x588;
constexpr size_t kItemPurity = 0x58c;
constexpr size_t kItemQuestItem = 0x598;
constexpr size_t kItemClairvoyance = 0x5a0;

constexpr size_t kItemSpellID = 0x00;
constexpr size_t kItemSpellRequiredLevel = 0x04;
constexpr size_t kItemSpellEffectType = 0x05;
constexpr size_t kItemSpellMaxCharges = 0x0c;
constexpr size_t kItemSpellCastTime = 0x10;
constexpr size_t kItemSpellTimerID = 0x14;
constexpr size_t kItemSpellRecastType = 0x18;
constexpr size_t kItemSpellProcRate = 0x1c;

constexpr size_t kSpellManagerSpells = 0x2c180;
constexpr uint32_t kMaxSpellID = 0xAFC9;
constexpr size_t kSpellRange = 0x000;
constexpr size_t kSpellAERange = 0x004;
constexpr size_t kSpellCastTime = 0x010;
constexpr size_t kSpellRecastTime = 0x018;
constexpr size_t kSpellDurationType = 0x01c;
constexpr size_t kSpellDurationValue1 = 0x020;
constexpr size_t kSpellMana = 0x028;
constexpr size_t kSpellBase = 0x02c;
constexpr size_t kSpellBase2 = 0x05c;
constexpr size_t kSpellMax = 0x08c;
constexpr size_t kSpellCalc = 0x0ec;
constexpr size_t kSpellAttrib = 0x11c;
constexpr size_t kSpellDescriptionNumber = 0x154;
constexpr size_t kSpellResistAdj = 0x158;
constexpr size_t kSpellIcon = 0x168;
constexpr size_t kSpellID = 0x174;
constexpr size_t kSpellEnduranceCost = 0x194;
constexpr size_t kSpellCanMGB = 0x241;
constexpr size_t kSpellLevel = 0x247;
constexpr size_t kSpellSpellType = 0x26b;
constexpr size_t kSpellResist = 0x26d;
constexpr size_t kSpellTargetType = 0x26e;
constexpr size_t kSpellSkill = 0x270;
constexpr size_t kSpellName = 0x27a;
constexpr size_t kSpellTarget = 0x2ba;
constexpr size_t kSpellCastOnYou = 0x3ba;
constexpr size_t kSpellCastOnAnother = 0x41a;
constexpr size_t kSpellWearOff = 0x47a;
constexpr size_t kSpellEffectSlots = 12;
constexpr int kSpellEffectNone = 254;
constexpr int kSpellEffectStackingBlock = 148;
constexpr int kSpellEffectStackingOverwrite = 149;

enum SpawnType : uint8_t {
    kSpawnPlayer = 0,
    kSpawnNPC = 1,
    kSpawnCorpse = 2
};

template <typename T>
T NativeClamp(T value, T min_value, T max_value) {
    return value < min_value ? min_value : (value > max_value ? max_value : value);
}

struct Config {
    bool map_enabled = true;
    bool map_show_npcs = true;
    bool map_show_players = false;
    bool map_show_corpses = false;
    bool map_chain_eq_labels = true;
    bool map_use_con_color = true;
    bool map_show_target = true;
    bool map_target_line = true;
    bool map_show_ground = false;
    bool map_show_vectors = false;
    bool map_named_only = false;
    bool map_show_xtargets = true;
    bool map_xtarget_labels = true;
    float map_target_radius = 0.0f;
    float map_cast_radius = 0.0f;
    float map_spell_radius = 0.0f;
    std::string map_highlight_filter;
    uint32_t map_highlight_color = 0xFFFF00FF;
    int map_highlight_size = 24;
    int map_max_labels = 0;
    int map_refresh_ms = 1000;
    std::string map_name_filter;
    std::string map_hide_filter;
    bool xtar_enabled = true;
    bool inspect_items = true;
    bool inspect_spells = true;
};

#pragma pack(push, 1)
struct Point3 {
    float x;
    float y;
    float z;
};

struct MapLabelNative {
    uint32_t unknown0;
    MapLabelNative* next;
    MapLabelNative* prev;
    Point3 location;
    uint32_t color;
    uint32_t size;
    char* label;
    uint32_t layer;
    uint32_t width;
    uint32_t height;
    uint32_t unknown30;
    uint32_t unknown34;
};

struct MapLineNative {
    MapLineNative* next;
    MapLineNative* prev;
    Point3 start;
    Point3 end;
    uint32_t color;
    uint32_t layer;
};

struct CXStr {
    void* text;
};

struct CXSize {
    DWORD a;
    DWORD b;
    DWORD c;
    DWORD d;
    DWORD e;
    DWORD f;
};
#pragma pack(pop)

static_assert(sizeof(MapLabelNative) == 0x38, "MapLabelNative must match EQ MAPLABEL size");
static_assert(sizeof(MapLineNative) == 0x28, "MapLineNative must match EQ MAPLINE size");

struct ManagedMapLabel {
    MapLabelNative native{};
    std::string text;
};

struct MapLocation {
    std::string label;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct XTargetInfo {
    int slot = 0;
    uint32_t type = 0;
    uint32_t spawn_id = 0;
    std::string name;
    void* spawn = nullptr;
    float distance = 0.0f;
};

struct ItemPowerInfo {
    uint32_t item_id = 0;
    uint32_t item_level = 0;
    uint32_t score = 0;
    uint32_t version = 0;
    std::string role;
    std::string source;
    std::string name;
    DWORD last_seen = 0;
};

struct InlineHook {
    void* target = nullptr;
    void* detour = nullptr;
    BYTE original_bytes[16]{};
    size_t length = 0;
    BYTE* gateway = nullptr;
    bool installed = false;

    template <typename T>
    T original() const {
        return reinterpret_cast<T>(gateway);
    }
};

using MapPostDraw2Proc = int(__thiscall*)(void*);
using ItemUpdateStringsProc = void(__thiscall*)(void*);
using InterpretCmdProc = void(__thiscall*)(void*, void*, char*);
using DspChatProc = void(__thiscall*)(void*, const char*, DWORD, bool, bool);
using GetConLevelProc = int(__thiscall*)(void*, void*);
using CXStrCtorCStringProc = CXStr*(__thiscall*)(CXStr*, const char*);
using CXStrDtorProc = void(__thiscall*)(CXStr*);
using CStmlWndAppendSTMLProc = CXSize*(__thiscall*)(void*, CXSize*, CXStr);
using CStmlWndForceParseNowProc = void(__thiscall*)(void*);

HMODULE g_module = nullptr;
HMODULE g_real_dinput = nullptr;
HANDLE g_worker = nullptr;
volatile bool g_shutdown = false;
Config g_config;
InlineHook g_item_update_hook;
InlineHook g_command_hook;
InlineHook g_chat_hook;

void* g_hooked_map_window = nullptr;
void** g_old_map_vtable = nullptr;
void** g_new_map_vtable = nullptr;
MapPostDraw2Proc g_old_map_post_draw2 = nullptr;
std::vector<ManagedMapLabel> g_map_labels;
std::vector<MapLineNative> g_map_lines;
std::vector<MapLocation> g_map_locations;
DWORD g_last_map_refresh = 0;
__declspec(thread) bool g_in_update_strings = false;
__declspec(thread) bool g_in_map_post_draw = false;
int g_map_draw_log_count = 0;
int g_item_update_log_count = 0;
int g_item_lookup_log_count = 0;
int g_item_append_log_count = 0;
int g_spell_append_log_count = 0;
int g_item_power_transport_log_count = 0;
int g_stml_append_log_count = 0;
CRITICAL_SECTION g_item_power_lock{};
bool g_item_power_lock_ready = false;
std::unordered_map<uint32_t, ItemPowerInfo> g_item_power_by_id;

uintptr_t Rebase(uintptr_t address) {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    return base + (address - kEqImageBase);
}

std::string GetClientDirectory() {
    char path[MAX_PATH]{};
    if (!GetModuleFileNameA(nullptr, path, MAX_PATH)) {
        return ".";
    }

    char* slash = strrchr(path, '\\');
    if (slash) {
        *slash = '\0';
    }

    return path;
}

std::string JoinClientPath(const char* file_name) {
    std::string path = GetClientDirectory();
    if (!path.empty() && path.back() != '\\') {
        path.push_back('\\');
    }
    path += file_name;
    return path;
}

void Log(const char* fmt, ...) {
    char line[2048]{};
    va_list args;
    va_start(args, fmt);
    _vsnprintf_s(line, sizeof(line), _TRUNCATE, fmt, args);
    va_end(args);

    std::string path = JoinClientPath("native_interface.log");
    FILE* fp = nullptr;
    if (fopen_s(&fp, path.c_str(), "a") == 0 && fp) {
        SYSTEMTIME st{};
        GetLocalTime(&st);
        fprintf(fp, "[%04u-%02u-%02u %02u:%02u:%02u] %s\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, line);
        fclose(fp);
    }
}

bool IsReadableMemory(const void* ptr, size_t bytes) {
    if (!ptr || bytes == 0) {
        return false;
    }

    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(ptr, &mbi, sizeof(mbi))) {
        return false;
    }

    if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
        return false;
    }

    uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t end = start + bytes;
    uintptr_t region_end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    return end <= region_end;
}

template <typename T>
bool SafeRead(const void* address, T& value) {
    if (!IsReadableMemory(address, sizeof(T))) {
        return false;
    }

    __try {
        value = *reinterpret_cast<const T*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

template <typename T>
T ReadValue(const void* address, T fallback = T{}) {
    T value{};
    if (SafeRead(address, value)) {
        return value;
    }
    return fallback;
}

void* ReadPtr(const void* address) {
    void* value = nullptr;
    SafeRead(address, value);
    return value;
}

void* ReadPtrOffset(const void* base, size_t offset) {
    if (!base) {
        return nullptr;
    }
    return ReadPtr(static_cast<const BYTE*>(base) + offset);
}

void* ReadGlobalPtr(uintptr_t rebased_global_ptr) {
    return ReadPtr(reinterpret_cast<const void*>(Rebase(rebased_global_ptr)));
}

bool WriteGlobalPtr(uintptr_t rebased_global_ptr, void* value) {
    void** target = reinterpret_cast<void**>(Rebase(rebased_global_ptr));
    if (!IsReadableMemory(target, sizeof(void*))) {
        return false;
    }

    __try {
        *target = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CopyFixedStringRaw(const char* address, size_t max_len, char* buffer, size_t buffer_len) {
    if (!address || !buffer || buffer_len == 0 || max_len == 0) {
        return false;
    }

    buffer[0] = '\0';
    size_t limit = max_len;
    if (limit > buffer_len - 1) {
        limit = buffer_len - 1;
    }

    __try {
        for (size_t i = 0; i < limit; ++i) {
            char c = *(address + i);
            if (c == '\0') {
                break;
            }
            buffer[i] = c;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        buffer[0] = '\0';
        return false;
    }

    return true;
}

std::string ReadFixedString(const void* address, size_t max_len) {
    if (!address || max_len == 0 || !IsReadableMemory(address, 1)) {
        return {};
    }

    char buffer[256]{};
    if (!CopyFixedStringRaw(static_cast<const char*>(address), max_len, buffer, sizeof(buffer))) {
        return {};
    }

    return std::string(buffer);
}

void LoadConfig() {
    std::string path = JoinClientPath("native_interface.ini");
    g_config.map_enabled = GetPrivateProfileIntA("Map", "Enabled", 1, path.c_str()) != 0;
    g_config.map_show_npcs = GetPrivateProfileIntA("Map", "ShowNPCs", 1, path.c_str()) != 0;
    g_config.map_show_players = GetPrivateProfileIntA("Map", "ShowPlayers", 0, path.c_str()) != 0;
    g_config.map_show_corpses = GetPrivateProfileIntA("Map", "ShowCorpses", 0, path.c_str()) != 0;
    g_config.map_chain_eq_labels = GetPrivateProfileIntA("Map", "ChainEQLabels", 1, path.c_str()) != 0;
    g_config.map_use_con_color = GetPrivateProfileIntA("Map", "UseConColor", 1, path.c_str()) != 0;
    g_config.map_show_target = GetPrivateProfileIntA("Map", "ShowTarget", 1, path.c_str()) != 0;
    g_config.map_target_line = GetPrivateProfileIntA("Map", "TargetLine", 1, path.c_str()) != 0;
    g_config.map_show_ground = GetPrivateProfileIntA("Map", "Ground", 0, path.c_str()) != 0;
    g_config.map_show_vectors = GetPrivateProfileIntA("Map", "Vector", 0, path.c_str()) != 0;
    g_config.map_named_only = GetPrivateProfileIntA("Map", "Named", 0, path.c_str()) != 0;
    g_config.map_show_xtargets = GetPrivateProfileIntA("Map", "XTargets", 1, path.c_str()) != 0;
    g_config.map_xtarget_labels = GetPrivateProfileIntA("Map", "XTargetLabels", 1, path.c_str()) != 0;
    g_config.map_highlight_color = static_cast<uint32_t>(GetPrivateProfileIntA("Map", "HighlightColor", 0x00FF00FF, path.c_str())) | 0xFF000000;
    g_config.map_highlight_size = NativeClamp(static_cast<int>(GetPrivateProfileIntA("Map", "HighlightSize", 24, path.c_str())), 4, 200);
    int max_labels = static_cast<int>(GetPrivateProfileIntA("Map", "MaxLabels", 0, path.c_str()));
    g_config.map_max_labels = max_labels <= 0 ? 0 : NativeClamp(max_labels, 1, 2048);
    int refresh_ms = static_cast<int>(GetPrivateProfileIntA("Map", "RefreshMs", 1000, path.c_str()));
    g_config.map_refresh_ms = NativeClamp(refresh_ms, 250, 5000);
    char float_value[64]{};
    GetPrivateProfileStringA("Map", "TargetRadius", "0", float_value, sizeof(float_value), path.c_str());
    g_config.map_target_radius = std::max(0.0f, static_cast<float>(atof(float_value)));
    GetPrivateProfileStringA("Map", "CastRadius", "0", float_value, sizeof(float_value), path.c_str());
    g_config.map_cast_radius = std::max(0.0f, static_cast<float>(atof(float_value)));
    GetPrivateProfileStringA("Map", "SpellRadius", "0", float_value, sizeof(float_value), path.c_str());
    g_config.map_spell_radius = std::max(0.0f, static_cast<float>(atof(float_value)));
    char name_filter[256]{};
    GetPrivateProfileStringA("Map", "NameFilter", "", name_filter, sizeof(name_filter), path.c_str());
    g_config.map_name_filter = name_filter;
    char hide_filter[256]{};
    GetPrivateProfileStringA("Map", "HideFilter", "", hide_filter, sizeof(hide_filter), path.c_str());
    g_config.map_hide_filter = hide_filter;
    char highlight_filter[256]{};
    GetPrivateProfileStringA("Map", "HighlightFilter", "", highlight_filter, sizeof(highlight_filter), path.c_str());
    g_config.map_highlight_filter = highlight_filter;
    g_config.xtar_enabled = GetPrivateProfileIntA("XTarget", "Enabled", 1, path.c_str()) != 0;
    g_config.inspect_items = GetPrivateProfileIntA("Inspect", "Items", 1, path.c_str()) != 0;
    g_config.inspect_spells = GetPrivateProfileIntA("Inspect", "Spells", 1, path.c_str()) != 0;
}

void SaveConfigBool(const char* section, const char* key, bool value) {
    std::string path = JoinClientPath("native_interface.ini");
    WritePrivateProfileStringA(section, key, value ? "1" : "0", path.c_str());
}

void SaveConfigString(const char* section, const char* key, const std::string& value) {
    std::string path = JoinClientPath("native_interface.ini");
    WritePrivateProfileStringA(section, key, value.c_str(), path.c_str());
}

void SaveConfigInt(const char* section, const char* key, int value) {
    char buffer[32]{};
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%d", value);
    std::string path = JoinClientPath("native_interface.ini");
    WritePrivateProfileStringA(section, key, buffer, path.c_str());
}

void SaveConfigFloat(const char* section, const char* key, float value) {
    char buffer[32]{};
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%.2f", value);
    std::string path = JoinClientPath("native_interface.ini");
    WritePrivateProfileStringA(section, key, buffer, path.c_str());
}

void Chat(const char* fmt, ...) {
    char line[1024]{};
    va_list args;
    va_start(args, fmt);
    _vsnprintf_s(line, sizeof(line), _TRUNCATE, fmt, args);
    va_end(args);

    Log("%s", line);

    void* everquest = ReadGlobalPtr(kPInstCEverQuest);
    if (!everquest) {
        return;
    }

    auto dsp_chat = reinterpret_cast<DspChatProc>(Rebase(kCEverQuestDspChat));
    dsp_chat(everquest, line, 15, false, true);
}

bool CommandMatch(const char* line, const char* command, const char** arguments) {
    if (!line || !command) {
        return false;
    }

    while (*line == ' ' || *line == '\t') {
        ++line;
    }

    size_t command_len = strlen(command);
    if (_strnicmp(line, command, command_len) != 0) {
        return false;
    }

    char next = line[command_len];
    if (next != '\0' && next != ' ' && next != '\t') {
        return false;
    }

    if (arguments) {
        const char* current = line + command_len;
        while (*current == ' ' || *current == '\t') {
            ++current;
        }
        *arguments = current;
    }

    return true;
}

std::string TrimCopy(const char* text) {
    if (!text) {
        return {};
    }

    while (*text == ' ' || *text == '\t') {
        ++text;
    }

    const char* end = text + strlen(text);
    while (end > text && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
        --end;
    }

    return std::string(text, end);
}

std::string LowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool StartsWith(const char* value, const char* prefix) {
    return value && prefix && strncmp(value, prefix, strlen(prefix)) == 0;
}

bool ContainsInsensitive(const std::string& value, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    return LowerCopy(value).find(LowerCopy(needle)) != std::string::npos;
}

bool ParseOnOff(const char* args, bool current_value, bool default_if_missing) {
    if (!args || !args[0]) {
        return default_if_missing;
    }

    if (CommandMatch(args, "on", nullptr) || CommandMatch(args, "1", nullptr) ||
        CommandMatch(args, "true", nullptr)) {
        return true;
    }

    if (CommandMatch(args, "off", nullptr) || CommandMatch(args, "0", nullptr) ||
        CommandMatch(args, "false", nullptr)) {
        return false;
    }

    return current_value;
}

std::vector<std::string> SplitWords(const char* text) {
    std::vector<std::string> words;
    if (!text) {
        return words;
    }

    while (*text) {
        while (*text == ' ' || *text == '\t') {
            ++text;
        }
        if (!*text) {
            break;
        }

        const char* start = text;
        while (*text && *text != ' ' && *text != '\t') {
            ++text;
        }
        words.emplace_back(start, text);
    }

    return words;
}

bool TryParseInt(const std::string& value, int& out) {
    if (value.empty()) {
        return false;
    }

    char* end = nullptr;
    long parsed = strtol(value.c_str(), &end, 10);
    if (!end || *end != '\0') {
        return false;
    }

    out = static_cast<int>(parsed);
    return true;
}

bool TryParseFloat(const std::string& value, float& out) {
    if (value.empty()) {
        return false;
    }

    char* end = nullptr;
    float parsed = strtof(value.c_str(), &end);
    if (!end || *end != '\0') {
        return false;
    }

    out = parsed;
    return true;
}

bool TryParseColorToken(const std::string& value, uint32_t& out) {
    if (value.empty()) {
        return false;
    }

    std::string token = value;
    if (token[0] == '#') {
        token.erase(token.begin());
    } else if (token.size() > 2 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
        token.erase(0, 2);
    }

    if (token.empty() || token.size() > 8) {
        return false;
    }

    char* end = nullptr;
    unsigned long parsed = strtoul(token.c_str(), &end, 16);
    if (!end || *end != '\0') {
        return false;
    }

    uint32_t color = static_cast<uint32_t>(parsed);
    if (token.size() <= 6) {
        color |= 0xFF000000;
    }
    out = color;
    return true;
}

bool ReadTransportUInt(const char* message, const char* key, uint32_t& value) {
    const char* found = strstr(message, key);
    if (!found) {
        return false;
    }

    found += strlen(key);
    char* end = nullptr;
    unsigned long parsed = strtoul(found, &end, 10);
    if (end == found) {
        return false;
    }

    value = static_cast<uint32_t>(parsed);
    return true;
}

std::string ReadTransportString(const char* message, const char* key) {
    const char* found = strstr(message, key);
    if (!found) {
        return {};
    }

    found += strlen(key);
    const char* end = strchr(found, '|');
    return end ? std::string(found, end) : std::string(found);
}

bool ParseItemPowerTransport(const char* message) {
    if (!StartsWith(message, "ITEMPOWER|")) {
        return false;
    }

    uint32_t item_id = 0;
    if (!ReadTransportUInt(message, "item_id=", item_id) || item_id == 0) {
        return true;
    }

    if (StartsWith(message, "ITEMPOWER|clear|")) {
        if (g_item_power_lock_ready) {
            EnterCriticalSection(&g_item_power_lock);
            g_item_power_by_id.erase(item_id);
            LeaveCriticalSection(&g_item_power_lock);
        }
        return true;
    }

    ItemPowerInfo info{};
    info.item_id = item_id;
    info.last_seen = GetTickCount();
    ReadTransportUInt(message, "ilvl=", info.item_level);
    ReadTransportUInt(message, "score=", info.score);
    ReadTransportUInt(message, "version=", info.version);
    info.role = ReadTransportString(message, "role=");
    info.source = ReadTransportString(message, "source=");
    info.name = ReadTransportString(message, "name=");

    if (info.item_level == 0 && info.score == 0) {
        return true;
    }

    if (g_item_power_lock_ready) {
        EnterCriticalSection(&g_item_power_lock);
        g_item_power_by_id[item_id] = info;
        LeaveCriticalSection(&g_item_power_lock);
    }

    if (g_item_power_transport_log_count < 16) {
        ++g_item_power_transport_log_count;
        Log("cached item power item=%u ilvl=%u score=%u role=%s version=%u source=%s name=%s",
            info.item_id,
            info.item_level,
            info.score,
            info.role.c_str(),
            info.version,
            info.source.c_str(),
            info.name.c_str());
    }

    return true;
}

bool LookupItemPower(uint32_t item_id, ItemPowerInfo& info) {
    if (!item_id || !g_item_power_lock_ready) {
        return false;
    }

    EnterCriticalSection(&g_item_power_lock);
    const auto iter = g_item_power_by_id.find(item_id);
    if (iter == g_item_power_by_id.end()) {
        LeaveCriticalSection(&g_item_power_lock);
        return false;
    }

    info = iter->second;
    LeaveCriticalSection(&g_item_power_lock);
    return true;
}

size_t ItemPowerCacheSize() {
    if (!g_item_power_lock_ready) {
        return 0;
    }

    EnterCriticalSection(&g_item_power_lock);
    const size_t size = g_item_power_by_id.size();
    LeaveCriticalSection(&g_item_power_lock);
    return size;
}

bool WriteJump(BYTE* target, void* detour) {
    DWORD old_protect = 0;
    if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old_protect)) {
        return false;
    }

    target[0] = 0xE9;
    *reinterpret_cast<int32_t*>(target + 1) =
        static_cast<int32_t>(reinterpret_cast<BYTE*>(detour) - target - 5);

    DWORD unused = 0;
    VirtualProtect(target, 5, old_protect, &unused);
    FlushInstructionCache(GetCurrentProcess(), target, 5);
    return true;
}

bool InstallInlineHook(InlineHook& hook, void* target, void* detour, size_t length) {
    if (hook.installed) {
        return true;
    }

    if (!target || !detour || length < 5 || length > sizeof(hook.original_bytes)) {
        return false;
    }

    hook.gateway = static_cast<BYTE*>(VirtualAlloc(nullptr, length + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!hook.gateway) {
        return false;
    }

    hook.target = target;
    hook.detour = detour;
    hook.length = length;
    memcpy(hook.original_bytes, target, length);
    memcpy(hook.gateway, target, length);
    WriteJump(hook.gateway + length, static_cast<BYTE*>(target) + length);

    DWORD old_protect = 0;
    if (!VirtualProtect(target, length, PAGE_EXECUTE_READWRITE, &old_protect)) {
        VirtualFree(hook.gateway, 0, MEM_RELEASE);
        hook.gateway = nullptr;
        return false;
    }

    memset(target, 0x90, length);
    WriteJump(static_cast<BYTE*>(target), detour);

    DWORD unused = 0;
    VirtualProtect(target, length, old_protect, &unused);
    FlushInstructionCache(GetCurrentProcess(), target, length);

    hook.installed = true;
    return true;
}

void RemoveInlineHook(InlineHook& hook) {
    if (!hook.installed || !hook.target) {
        return;
    }

    DWORD old_protect = 0;
    if (VirtualProtect(hook.target, hook.length, PAGE_EXECUTE_READWRITE, &old_protect)) {
        memcpy(hook.target, hook.original_bytes, hook.length);
        DWORD unused = 0;
        VirtualProtect(hook.target, hook.length, old_protect, &unused);
        FlushInstructionCache(GetCurrentProcess(), hook.target, hook.length);
    }

    if (hook.gateway) {
        VirtualFree(hook.gateway, 0, MEM_RELEASE);
    }

    hook = InlineHook{};
}

void* GetSpell(uint32_t spell_id) {
    if (spell_id == 0 || spell_id >= kMaxSpellID) {
        return nullptr;
    }

    void* spell_mgr = ReadGlobalPtr(kPInstSpellManager);
    if (!spell_mgr) {
        return nullptr;
    }

    return ReadPtr(static_cast<BYTE*>(spell_mgr) + kSpellManagerSpells + spell_id * sizeof(void*));
}

std::string GetSpellName(uint32_t spell_id) {
    void* spell = GetSpell(spell_id);
    if (!spell) {
        char fallback[32]{};
        _snprintf_s(fallback, sizeof(fallback), _TRUNCATE, "Spell %u", spell_id);
        return fallback;
    }
    return ReadFixedString(static_cast<BYTE*>(spell) + kSpellName, 0x40);
}

std::string EscapeStml(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        default:
            out.push_back(c);
            break;
        }
    }
    return out;
}

void AppendLine(std::string& out, const char* label, int value) {
    if (value == 0) {
        return;
    }

    char line[128]{};
    _snprintf_s(line, sizeof(line), _TRUNCATE, "%s: %d<br>", label, value);
    out += line;
}

void AppendHexLine(std::string& out, const char* label, uint32_t value) {
    if (value == 0) {
        return;
    }

    char line[128]{};
    _snprintf_s(line, sizeof(line), _TRUNCATE, "%s: 0x%X<br>", label, value);
    out += line;
}

void AppendFloatLine(std::string& out, const char* label, float value) {
    if (value == 0.0f) {
        return;
    }

    char line[128]{};
    _snprintf_s(line, sizeof(line), _TRUNCATE, "%s: %.1f<br>", label, value);
    out += line;
}

void AppendFlags(std::string& out, void* item) {
    std::vector<std::string> flags;
    if (ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemNoDrop) != 0) {
        flags.emplace_back("No Drop");
    }
    if (ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemNoRent) != 0) {
        flags.emplace_back("No Rent");
    }
    if (ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemAttuneable) != 0) {
        flags.emplace_back("Attuneable");
    }
    if (ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemMagic) != 0) {
        flags.emplace_back("Magic");
    }
    if (ReadValue<uint32_t>(static_cast<BYTE*>(item) + kItemLore) != 0) {
        flags.emplace_back("Lore");
    }
    if (ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemSummoned) != 0) {
        flags.emplace_back("Summoned");
    }
    if (ReadValue<uint32_t>(static_cast<BYTE*>(item) + kItemArtifact) != 0) {
        flags.emplace_back("Artifact");
    }
    if (ReadValue<uint32_t>(static_cast<BYTE*>(item) + kItemQuestItem) != 0) {
        flags.emplace_back("Quest");
    }
    if (ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemTradeSkills) != 0) {
        flags.emplace_back("Tradeskill");
    }

    if (!flags.empty()) {
        out += "Flags: ";
        for (size_t i = 0; i < flags.size(); ++i) {
            if (i) {
                out += ", ";
            }
            out += flags[i];
        }
        out += "<br>";
    }
}

void AppendMoneyLine(std::string& out, uint32_t copper) {
    if (copper == 0) {
        return;
    }

    uint32_t cp = copper;
    uint32_t sp = cp / 10;
    cp %= 10;
    uint32_t gp = sp / 10;
    sp %= 10;
    uint32_t pp = gp / 10;
    gp %= 10;

    char line[128]{};
    _snprintf_s(line, sizeof(line), _TRUNCATE, "Value: %upp %ugp %usp %ucp<br>", pp, gp, sp, cp);
    out += line;
}

double PositiveScore(double value) {
    return value > 0.0 ? value : 0.0;
}

double SoftCapScore(double value, double cap) {
    value = PositiveScore(value);
    if (cap <= 0.0) {
        return value;
    }

    const double soft_cap = cap * 0.70;
    if (value <= soft_cap) {
        return value;
    }

    return soft_cap + ((value - soft_cap) * 0.35);
}

uint32_t RoundScoreValue(double value) {
    if (value <= 0.0) {
        return 0;
    }

    return static_cast<uint32_t>(std::lround(value));
}

double LocalSlotBudget(void* item) {
    const int damage = ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemDamage);
    const int delay = ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemDelay);
    if (damage > 0 && delay > 0) {
        return 1.25;
    }

    const uint32_t slots = ReadValue<uint32_t>(static_cast<BYTE*>(item) + kItemEquipSlots);
    struct SlotBudget {
        uint8_t slot;
        double budget;
    };
    static const SlotBudget budgets[] = {
        {0, 1.00}, {1, 0.60}, {2, 1.00}, {3, 0.75}, {4, 0.60},
        {5, 0.75}, {6, 0.95}, {7, 0.95}, {8, 0.90}, {9, 0.65},
        {10, 0.65}, {11, 0.75}, {12, 0.85}, {13, 1.25}, {14, 1.15},
        {15, 0.60}, {16, 0.60}, {17, 1.35}, {18, 1.20}, {19, 0.85},
        {20, 0.85}, {21, 1.00}, {22, 0.25}
    };

    double best = 1.0;
    bool found = false;
    for (const auto& budget : budgets) {
        if ((slots & (1u << budget.slot)) && (!found || budget.budget > best)) {
            best = budget.budget;
            found = true;
        }
    }
    return best;
}

uint16_t LocalItemLevel(uint32_t score, double slot_budget, uint8_t req_level, uint8_t rec_level) {
    const double budget = std::max(slot_budget, 0.25);
    const double normalized_power = std::max(1.0, static_cast<double>(score) / budget);
    const auto computed_level = static_cast<uint16_t>(
        NativeClamp<int>(
            static_cast<int>(std::lround(std::pow(normalized_power, 0.58))),
            1,
            127
        )
    );
    const auto rec_floor = static_cast<uint16_t>(std::ceil(static_cast<double>(rec_level) * 0.85));
    return std::max<uint16_t>(
        std::max<uint16_t>(computed_level, static_cast<uint16_t>(req_level)),
        std::max<uint16_t>(rec_floor, 1)
    );
}

struct LocalRoleWeights {
    double hp = 0.0;
    double mana = 0.0;
    double endurance = 0.0;
    double ac = 0.0;
    double str = 0.0;
    double sta = 0.0;
    double dex = 0.0;
    double agi = 0.0;
    double intel = 0.0;
    double wis = 0.0;
    double haste = 0.0;
    double atk = 0.0;
    double accuracy = 0.0;
    double combat_effects = 0.0;
    double strikethrough = 0.0;
    double weapon = 0.0;
    double regen = 0.0;
    double mana_regen = 0.0;
    double endurance_regen = 0.0;
};

LocalRoleWeights GetLocalRoleWeights(const std::string& role) {
    LocalRoleWeights weights{};
    if (role == "tank") {
        weights.hp = 1.30;
        weights.ac = 1.50;
        weights.sta = 1.25;
        weights.agi = 1.15;
        weights.weapon = 0.70;
        weights.mana = 0.35;
    } else if (role == "melee") {
        weights.weapon = 1.60;
        weights.str = 1.25;
        weights.dex = 1.25;
        weights.haste = 1.40;
        weights.atk = 1.35;
        weights.accuracy = 1.50;
        weights.combat_effects = 1.30;
        weights.strikethrough = 1.25;
        weights.hp = 0.60;
        weights.ac = 0.60;
        weights.mana = 0.10;
    } else if (role == "caster") {
        weights.hp = 0.50;
        weights.ac = 0.50;
        weights.mana = 1.25;
        weights.intel = 1.20;
        weights.wis = 1.20;
        weights.mana_regen = 1.25;
        weights.weapon = 0.10;
        weights.haste = 0.10;
    } else if (role == "healer") {
        weights.hp = 0.70;
        weights.ac = 0.70;
        weights.mana = 1.35;
        weights.intel = 1.20;
        weights.wis = 1.20;
        weights.mana_regen = 1.35;
        weights.weapon = 0.10;
        weights.haste = 0.10;
    } else {
        weights.hp = 1.00;
        weights.mana = 0.75;
        weights.endurance = 0.90;
        weights.ac = 1.00;
        weights.str = 1.10;
        weights.sta = 1.10;
        weights.dex = 1.10;
        weights.agi = 1.00;
        weights.intel = 0.85;
        weights.wis = 0.85;
        weights.weapon = 1.00;
    }
    return weights;
}

uint32_t LocalRoleScore(void* item, const std::string& role) {
    const LocalRoleWeights weights = GetLocalRoleWeights(role);
    double score = 0.0;

    score += PositiveScore(ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemHP)) * 0.10 * weights.hp;
    score += PositiveScore(ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemMana)) * 0.08 * weights.mana;
    score += PositiveScore(ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemEndurance)) * 0.08 * weights.endurance;
    score += PositiveScore(ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemAC)) * 1.80 * weights.ac;

    score += PositiveScore(ReadValue<int16_t>(static_cast<BYTE*>(item) + kItemStr)) * 0.35 * weights.str;
    score += PositiveScore(ReadValue<int16_t>(static_cast<BYTE*>(item) + kItemSta)) * 0.35 * weights.sta;
    score += PositiveScore(ReadValue<int16_t>(static_cast<BYTE*>(item) + kItemDex)) * 0.35 * weights.dex;
    score += PositiveScore(ReadValue<int16_t>(static_cast<BYTE*>(item) + kItemAgi)) * 0.35 * weights.agi;
    score += PositiveScore(ReadValue<int16_t>(static_cast<BYTE*>(item) + kItemInt)) * 0.35 * weights.intel;
    score += PositiveScore(ReadValue<int16_t>(static_cast<BYTE*>(item) + kItemWis)) * 0.35 * weights.wis;

    score += SoftCapScore(ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemHPRegen), 30.0) * 2.00 * weights.regen;
    score += SoftCapScore(ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemManaRegen), 15.0) * 2.50 * weights.mana_regen;
    score += SoftCapScore(ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemEnduranceRegen), 15.0) * 2.50 * weights.endurance_regen;

    score += SoftCapScore(ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemAttack), 250.0) * 0.25 * weights.atk;
    score += SoftCapScore(ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemAccuracy), 150.0) * 0.90 * weights.accuracy;
    score += SoftCapScore(ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemCombatEffects), 100.0) * 0.80 * weights.combat_effects;
    score += SoftCapScore(ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemStrikeThrough), 35.0) * 2.50 * weights.strikethrough;
    score += SoftCapScore(ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemHaste), 100.0) * 2.20 * weights.haste;

    const int damage = ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemDamage);
    const int delay = ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemDelay);
    if (damage > 0 && delay > 0) {
        const double weapon_score = (static_cast<double>(damage) * 10.0 / static_cast<double>(delay)) * 12.0;
        score += weapon_score * weights.weapon;
    }

    return RoundScoreValue(score);
}

bool CalculateLocalItemPower(void* item, uint32_t item_id, ItemPowerInfo& info) {
    if (!item || !item_id) {
        return false;
    }

    struct RoleScore {
        const char* role;
        uint32_t score;
    };

    RoleScore scores[] = {
        {"tank", LocalRoleScore(item, "tank")},
        {"melee", LocalRoleScore(item, "melee")},
        {"caster", LocalRoleScore(item, "caster")},
        {"healer", LocalRoleScore(item, "healer")},
        {"hybrid", LocalRoleScore(item, "hybrid")}
    };

    RoleScore best = scores[0];
    for (const auto& score : scores) {
        if (score.score > best.score) {
            best = score;
        }
    }

    if (best.score == 0) {
        return false;
    }

    info = ItemPowerInfo{};
    info.item_id = item_id;
    info.score = best.score;
    info.role = best.role;
    info.source = "local";
    info.item_level = LocalItemLevel(
        info.score,
        LocalSlotBudget(item),
        ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemRequiredLevel),
        ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemRecommendedLevel)
    );
    info.last_seen = GetTickCount();
    return true;
}

std::string ItemPowerRoleDisplayName(const std::string& role) {
    if (role == "tank") {
        return "Tank";
    }
    if (role == "melee") {
        return "Melee DPS";
    }
    if (role == "caster") {
        return "Caster DPS";
    }
    if (role == "healer") {
        return "Healer";
    }
    if (role == "hybrid") {
        return "Hybrid";
    }
    if (role == "none") {
        return "";
    }
    return role;
}

bool AppendItemPowerLines(std::string& out, void* item, uint32_t item_id) {
    ItemPowerInfo info{};
    if (!LookupItemPower(item_id, info) && !CalculateLocalItemPower(item, item_id, info)) {
        return false;
    }

    if (info.score == 0) {
        return false;
    }

    char line[256]{};
    _snprintf_s(line, sizeof(line), _TRUNCATE, "Item Level: %u<br>", info.item_level);
    out += line;

    _snprintf_s(line, sizeof(line), _TRUNCATE, "Gearscore: %u<br>", info.score);
    out += line;

    const std::string role = ItemPowerRoleDisplayName(info.role);
    if (!role.empty()) {
        out += "Best Role: ";
        out += EscapeStml(role);
        out += "<br>";
    }

    return true;
}

int64_t Abs64(int64_t value) {
    return value < 0 ? -value : value;
}

int64_t CalcSpellValue(int calc, int64_t base, int64_t max, int tick, int min_level, int level) {
    if (calc == 0) {
        return base;
    }

    if (calc == 100) {
        if (max > 0 && (base > max || level > min_level)) {
            return max;
        }
        return base;
    }

    int64_t change = 0;
    switch (calc) {
    case 101: change = level / 2; break;
    case 102: change = level; break;
    case 103: change = level * 2; break;
    case 104: change = level * 3; break;
    case 105: change = level * 4; break;
    case 106: change = level * 5; break;
    case 107: change = -1 * tick; break;
    case 108: change = -2 * tick; break;
    case 109: change = level / 4; break;
    case 110: change = level / 6; break;
    case 111: if (level > 16) change = (level - 16) * 6; break;
    case 112: if (level > 24) change = (level - 24) * 8; break;
    case 113: if (level > 34) change = (level - 34) * 10; break;
    case 114: if (level > 44) change = (level - 44) * 15; break;
    case 115: if (level > 15) change = (level - 15) * 7; break;
    case 116: if (level > 24) change = (level - 24) * 10; break;
    case 117: if (level > 34) change = (level - 34) * 13; break;
    case 118: if (level > 44) change = (level - 44) * 20; break;
    case 119: change = level / 8; break;
    case 120: change = -5 * tick; break;
    case 121: change = level / 3; break;
    case 122: change = -12 * tick; break;
    case 123: if (tick > 1) change = Abs64(max) - Abs64(base); break;
    case 124: if (level > 50) change = level - 50; break;
    case 125: if (level > 50) change = (level - 50) * 2; break;
    case 126: if (level > 50) change = (level - 50) * 3; break;
    case 127: if (level > 50) change = (level - 50) * 4; break;
    case 128: if (level > 50) change = (level - 50) * 5; break;
    case 129: if (level > 50) change = (level - 50) * 10; break;
    case 130: if (level > 50) change = (level - 50) * 15; break;
    case 131: if (level > 50) change = (level - 50) * 20; break;
    case 132: if (level > 50) change = (level - 50) * 25; break;
    case 139: if (level > 30) change = (level - 30) / 2; break;
    case 140: if (level > 30) change = level - 30; break;
    case 141: if (level > 30) change = 3 * (level - 30) / 2; break;
    case 142: if (level > 30) change = 2 * (level - 30); break;
    case 143: change = 3 * level / 4; break;
    case 3000: return base;
    default:
        if (calc > 0 && calc < 1000) {
            change = level * calc;
        } else if (calc >= 1000 && calc < 2000) {
            change = tick * (calc - 1000) * -1;
        } else if (calc >= 2000) {
            change = level * (calc - 2000);
        }
        break;
    }

    int64_t value = Abs64(base) + change;
    if (max != 0 && value > Abs64(max)) {
        value = Abs64(max);
    }
    return base < 0 ? -value : value;
}

const char* SpellEffectName(int attrib) {
    static const char* kSpellEffectNames[] = {
#include "native_spell_effect_names.inl"
    };

    if (attrib >= 0 && attrib < static_cast<int>(sizeof(kSpellEffectNames) / sizeof(kSpellEffectNames[0]))) {
        return kSpellEffectNames[attrib];
    }

    switch (attrib) {
    case 0: return "Current Hit Points";
    case 1: return "Armor Class";
    case 2: return "Attack";
    case 3: return "Movement Speed";
    case 4: return "STR";
    case 5: return "DEX";
    case 6: return "AGI";
    case 7: return "STA";
    case 8: return "INT";
    case 9: return "WIS";
    case 10: return "CHA";
    case 11: return "Attack Speed";
    case 12: return "Invisibility";
    case 13: return "See Invisible";
    case 14: return "Water Breathing";
    case 15: return "Current Mana";
    case 20: return "Blindness";
    case 21: return "Stun";
    case 22: return "Charm";
    case 23: return "Fear";
    case 27: return "Dispel Magic";
    case 28: return "Invisibility Vs Undead";
    case 29: return "Invisibility Vs Animals";
    case 30: return "NPC Aggro Radius";
    case 31: return "Mesmerize";
    case 32: return "Summon";
    case 33: return "Summon Pet";
    case 34: return "Confuse";
    case 35: return "Disease Counter";
    case 36: return "Poison Counter";
    case 37: return "Detect Hostile";
    case 38: return "Detect Magic";
    case 39: return "Stacking: No Twincast";
    case 40: return "Invulnerability";
    case 41: return "Banish";
    case 42: return "Shadow Step";
    case 43: return "Berserk";
    case 44: return "Lycanthropy";
    case 45: return "Vampirism";
    case 46: return "Fire Resist";
    case 47: return "Cold Resist";
    case 48: return "Poison Resist";
    case 49: return "Disease Resist";
    case 50: return "Magic Resist";
    case 51: return "Detect Traps";
    case 52: return "Detect Undead";
    case 53: return "Detect Summoned";
    case 54: return "Detect Animals";
    case 55: return "Absorb Damage";
    case 56: return "True North";
    case 57: return "Levitation";
    case 58: return "Illusion";
    case 59: return "Damage Shield";
    case 60: return "Transfer Item";
    case 61: return "Identify";
    case 62: return "Item ID";
    case 63: return "Memblur";
    case 64: return "Spin Stun";
    case 65: return "Infravision";
    case 66: return "Ultravision";
    case 67: return "Eye Of Zomm";
    case 68: return "Reclaim Energy";
    case 69: return "Max Hit Points";
    case 70: return "Corpse Bomb";
    case 71: return "Create Undead Pet";
    case 72: return "Preserve Corpse";
    case 73: return "Bind Sight";
    case 74: return "Feign Death";
    case 75: return "Ventriloquism";
    case 76: return "Sentinel";
    case 77: return "Locate Corpse";
    case 78: return "Spell Shield";
    case 79: return "Current Hit Points Once";
    case 80: return "Enchant Light";
    case 81: return "Resurrect";
    case 82: return "Summon Player";
    case 83: return "Teleport";
    case 84: return "Toss";
    case 85: return "Add Proc";
    case 86: return "Reaction Radius";
    case 87: return "Magnification";
    case 88: return "Evacuate";
    case 89: return "Player Size";
    case 90: return "Ignore Pet";
    case 91: return "Summon Corpse";
    case 92: return "Hate";
    case 93: return "Control Weather";
    case 94: return "Make Fragile";
    case 95: return "Sacrifice";
    case 96: return "Silence";
    case 97: return "Max Mana";
    case 98: return "Bard Haste";
    case 99: return "Root";
    case 100: return "Heal Over Time";
    case 101: return "Complete Heal";
    case 102: return "Pet Fearless";
    case 103: return "Summon Pet";
    case 104: return "Translocate";
    case 105: return "Anti Gate";
    case 106: return "Summon Warder";
    case 107: return "Alter NPC Level";
    case 108: return "Summon Familiar";
    case 109: return "Summon In Bag";
    case 110: return "Archery";
    case 111: return "All Resists";
    case 112: return "Casting Level";
    case 113: return "Summon Mount";
    case 114: return "Hate Multiplier";
    case 115: return "Food/Water";
    case 116: return "Curse Counter";
    case 117: return "Make Weapons Magical";
    case 118: return "Singing Skill";
    case 119: return "Melee Overhaste";
    case 120: return "Healing Effectiveness";
    case 121: return "Reverse Damage Shield";
    case 122: return "Reduce Skill";
    case 123: return "Immunity";
    case 124: return "Spell Damage";
    case 125: return "Healing";
    case 126: return "Spell Resist Rate";
    case 127: return "Spell Haste";
    case 128: return "Spell Duration";
    case 129: return "Spell Range";
    case 130: return "Spell/Bash Hate";
    case 131: return "Chance of Using Reagent";
    case 132: return "Spell Mana Cost";
    case 134: return "Limit: Max Level";
    case 135: return "Limit: Resist";
    case 136: return "Limit: Target";
    case 137: return "Limit: Effect";
    case 138: return "Limit: Spell Type";
    case 139: return "Limit: Spell";
    case 140: return "Limit: Min Duration";
    case 141: return "Limit: Instant";
    case 142: return "Limit: Min Level";
    case 143: return "Limit: Min Cast Time";
    case 144: return "Limit: Max Cast Time";
    case 145: return "NPC Warder Banish";
    case 146: return "Portal Locations";
    case 147: return "Hit Points Percent";
    case 148: return "Stacking: Block";
    case 149: return "Stacking: Overwrite";
    case 150: return "Death Save";
    case 151: return "Pocket Pet";
    case 152: return "Summon Pets";
    case 153: return "Balance Party Damage";
    case 154: return "Remove Detrimental";
    case 155: return "PoP Resurrect";
    case 156: return "Mirror";
    case 157: return "Spell Damage Shield";
    case 158: return "Reflect Spell";
    case 159: return "All Stats";
    case 160: return "Drunk";
    case 167: return "Increase Pet Power";
    case 168: return "Defensive";
    case 169: return "Chance to Critical Melee";
    case 170: return "Chance to Critical Cast";
    case 254: return "No Effect";
    default: return nullptr;
    }
}

bool SpellEffectUsesPercent(int attrib) {
    switch (attrib) {
    case 3:
    case 11:
    case 98:
    case 124:
    case 125:
    case 126:
    case 127:
    case 128:
    case 129:
    case 130:
    case 131:
    case 132:
    case 167:
    case 168:
    case 177:
    case 195:
    case 200:
    case 232:
    case 233:
    case 258:
    case 273:
    case 274:
    case 275:
    case 279:
    case 280:
    case 294:
    case 302:
    case 320:
    case 483:
        return true;
    default:
        return false;
    }
}

int LowestSpellLevel(void* spell) {
    int lowest_level = 255;
    for (size_t i = 0; i < 16; ++i) {
        int level = ReadValue<uint8_t>(static_cast<BYTE*>(spell) + kSpellLevel + i);
        if (level > 0 && level < 255) {
            lowest_level = std::min(lowest_level, level);
        }
    }
    return lowest_level == 255 ? 1 : lowest_level;
}

void AppendSpellClassLevels(std::string& out, void* spell) {
    static const char* kClassNames[16] = {
        "WAR", "CLR", "PAL", "RNG", "SHD", "DRU", "MNK", "BRD",
        "ROG", "SHM", "NEC", "WIZ", "MAG", "ENC", "BST", "BER"
    };

    std::string classes;
    for (size_t i = 0; i < 16; ++i) {
        int level = ReadValue<uint8_t>(static_cast<BYTE*>(spell) + kSpellLevel + i);
        if (level <= 0 || level >= 255) {
            continue;
        }

        char part[32]{};
        _snprintf_s(part, sizeof(part), _TRUNCATE, "%s%s(%d)", classes.empty() ? "" : ", ", kClassNames[i], level);
        classes += part;
    }

    if (!classes.empty()) {
        out += classes + "<br>";
    }
}

std::string FormatSpellEffectLine(void* spell, size_t slot) {
    int attrib = ReadValue<int32_t>(static_cast<BYTE*>(spell) + kSpellAttrib + slot * sizeof(int32_t), kSpellEffectNone);
    int base = ReadValue<int32_t>(static_cast<BYTE*>(spell) + kSpellBase + slot * sizeof(int32_t));
    int base2 = ReadValue<int32_t>(static_cast<BYTE*>(spell) + kSpellBase2 + slot * sizeof(int32_t));
    int max = ReadValue<int32_t>(static_cast<BYTE*>(spell) + kSpellMax + slot * sizeof(int32_t));
    int calc = ReadValue<int32_t>(static_cast<BYTE*>(spell) + kSpellCalc + slot * sizeof(int32_t));

    if (attrib == kSpellEffectNone || attrib < 0 || attrib > 700) {
        return {};
    }

    if (attrib == 10 && (base <= 1 || base > 255)) {
        return {};
    }

    int adjusted_base = base;
    int adjusted_max = max;
    if (attrib == 11) {
        adjusted_base -= 100;
        adjusted_max -= 100;
    } else if (attrib == 127 || attrib == 128 || attrib == 132) {
        adjusted_max = base2;
    }

    int min_level = LowestSpellLevel(spell);
    int calc_base = attrib == kSpellEffectStackingBlock ? adjusted_max : adjusted_base;
    int64_t value = CalcSpellValue(calc, calc_base, adjusted_max, 1, min_level, min_level);
    bool percent = SpellEffectUsesPercent(attrib);
    const char* name = SpellEffectName(attrib);
    char display_name_fallback[32]{};
    const char* display_name = name;
    if (!display_name) {
        _snprintf_s(display_name_fallback, sizeof(display_name_fallback), _TRUNCATE, "SPA %d", attrib);
        display_name = display_name_fallback;
    }

    char line[256]{};
    const char* direction = value < 0 ? "Decrease" : "Increase";
    int64_t magnitude = Abs64(value);

    switch (attrib) {
    case 0:
    case 15:
    case 35:
    case 36:
    case 46:
    case 47:
    case 48:
    case 49:
    case 50:
    case 69:
    case 78:
    case 79:
    case 92:
    case 97:
    case 99:
    case 100:
    case 124:
    case 147:
        _snprintf_s(line, sizeof(line), _TRUNCATE, "Slot %u: %s %s by %lld%s<br>",
            static_cast<unsigned>(slot + 1), direction, display_name, magnitude, percent ? "%" : "");
        break;
    case 1:
        _snprintf_s(line, sizeof(line), _TRUNCATE, "Slot %u: %s %s by %lld<br>",
            static_cast<unsigned>(slot + 1), direction, display_name, magnitude);
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 127:
    case 128:
    case 129:
    case 130:
    case 131:
    case 132:
    case 167:
    case 168:
        _snprintf_s(line, sizeof(line), _TRUNCATE, "Slot %u: %s %s by %lld%s<br>",
            static_cast<unsigned>(slot + 1), direction, display_name, magnitude, percent ? "%" : "");
        break;
    case 12:
    case 13:
    case 14:
    case 20:
    case 22:
    case 23:
    case 37:
    case 38:
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
    case 56:
    case 57:
    case 58:
    case 60:
    case 61:
    case 62:
    case 65:
    case 66:
    case 67:
    case 68:
    case 70:
    case 71:
    case 72:
    case 73:
    case 75:
    case 76:
    case 77:
    case 80:
    case 82:
    case 91:
    case 96:
        _snprintf_s(line, sizeof(line), _TRUNCATE, "Slot %u: %s<br>",
            static_cast<unsigned>(slot + 1), display_name);
        break;
    case 21:
    case 64:
        _snprintf_s(line, sizeof(line), _TRUNCATE, "Slot %u: Stun for %.1fs<br>",
            static_cast<unsigned>(slot + 1), static_cast<float>(magnitude) / 1000.0f);
        break;
    case 27:
    case 85:
    case 134:
    case 135:
    case 136:
    case 137:
    case 138:
    case 139:
    case 140:
    case 141:
    case 142:
    case 143:
    case 144:
        _snprintf_s(line, sizeof(line), _TRUNCATE, "Slot %u: %s (%d, %d, %d)<br>",
            static_cast<unsigned>(slot + 1), display_name, base, base2, max);
        break;
    case kSpellEffectStackingBlock:
    case kSpellEffectStackingOverwrite: {
        int effect_slot = attrib == kSpellEffectStackingBlock ? (base2 > 0 ? base2 : calc - 200) : calc - 200;
        int comparison_max = attrib == kSpellEffectStackingOverwrite && max > 1000 ? max - 1000 : max;
        const char* referenced = SpellEffectName(base);
        char fallback[48]{};
        if (!referenced) {
            _snprintf_s(fallback, sizeof(fallback), _TRUNCATE, "Effect %d", base);
            referenced = fallback;
        }

        if (effect_slot > 0 && comparison_max > 0) {
            _snprintf_s(line, sizeof(line), _TRUNCATE, "Slot %u: %s %s spell if slot %d is effect '%s' and < %lld<br>",
                static_cast<unsigned>(slot + 1), display_name,
                attrib == kSpellEffectStackingBlock ? "new" : "existing",
                effect_slot, referenced, magnitude);
        } else if (effect_slot > 0) {
            _snprintf_s(line, sizeof(line), _TRUNCATE, "Slot %u: %s %s spell if slot %d is effect '%s'<br>",
                static_cast<unsigned>(slot + 1), display_name,
                attrib == kSpellEffectStackingBlock ? "new" : "existing",
                effect_slot, referenced);
        } else {
            _snprintf_s(line, sizeof(line), _TRUNCATE, "Slot %u: %s %s spell if effect '%s'<br>",
                static_cast<unsigned>(slot + 1), display_name,
                attrib == kSpellEffectStackingBlock ? "new" : "existing",
                referenced);
        }
        break;
    }
    default:
        _snprintf_s(line, sizeof(line), _TRUNCATE, "Slot %u: %s by %lld%s<br>",
            static_cast<unsigned>(slot + 1), display_name, magnitude, percent ? "%" : "");
        break;
    }

    return std::string(line);
}

void AppendSpellCore(std::string& out, uint32_t spell_id, const char* title, bool verbose) {
    if (spell_id == 0 || spell_id == 0xFFFFFFFF || spell_id >= kMaxSpellID) {
        return;
    }

    void* spell = GetSpell(spell_id);
    std::string name = EscapeStml(GetSpellName(spell_id));

    char line[256]{};
    if (strcmp(title, "Spell") == 0) {
        _snprintf_s(line, sizeof(line), _TRUNCATE, "Spell: %s (#%u)<br>", name.c_str(), spell_id);
    } else {
        _snprintf_s(line, sizeof(line), _TRUNCATE, "Spell Info for %s effect: %s<br>", title, name.c_str());
    }
    out += line;

    if (!spell || !verbose) {
        return;
    }

    _snprintf_s(line, sizeof(line), _TRUNCATE, "ID: %04u<br>", spell_id);
    out += line;

    int cast_time = ReadValue<int32_t>(static_cast<BYTE*>(spell) + kSpellCastTime);
    int recast_time = ReadValue<int32_t>(static_cast<BYTE*>(spell) + kSpellRecastTime);
    if (cast_time > 0 || recast_time > 0) {
        _snprintf_s(line, sizeof(line), _TRUNCATE, "CastTime: %.2fs&nbsp;&nbsp;&nbsp;RecastTime: %.2fs<br>",
            static_cast<float>(cast_time) / 1000.0f,
            static_cast<float>(recast_time) / 1000.0f);
        out += line;
    }

    float range = ReadValue<float>(static_cast<BYTE*>(spell) + kSpellRange);
    float ae_range = ReadValue<float>(static_cast<BYTE*>(spell) + kSpellAERange);
    if (range > 0.0f || ae_range > 0.0f) {
        if (range > 0.0f) {
            _snprintf_s(line, sizeof(line), _TRUNCATE, "Range: %.0f", range);
            out += line;
        }
        if (ae_range > 0.0f) {
            _snprintf_s(line, sizeof(line), _TRUNCATE, "%sAERange: %.0f", range > 0.0f ? "&nbsp;&nbsp;&nbsp;" : "", ae_range);
            out += line;
        }
        out += "<br>";
    }

    for (size_t i = 0; i < kSpellEffectSlots; ++i) {
        out += FormatSpellEffectLine(spell, i);
    }

    AppendSpellClassLevels(out, spell);

    std::string cast_on_you = EscapeStml(ReadFixedString(static_cast<BYTE*>(spell) + kSpellCastOnYou, 0x60));
    std::string cast_on_another = EscapeStml(ReadFixedString(static_cast<BYTE*>(spell) + kSpellCastOnAnother, 0x60));
    std::string wear_off = EscapeStml(ReadFixedString(static_cast<BYTE*>(spell) + kSpellWearOff, 0x60));
    if (!cast_on_you.empty()) {
        out += "Cast on you: " + cast_on_you + "<br>";
    }
    if (!cast_on_another.empty()) {
        out += "Cast on another: " + cast_on_another + "<br>";
    }
    if (!wear_off.empty()) {
        out += "Wears off: " + wear_off + "<br>";
    }
}

void AppendItemSpellLine(std::string& out, void* item, size_t offset, const char* label) {
    uint32_t spell_id = ReadValue<uint32_t>(static_cast<BYTE*>(item) + offset + kItemSpellID);
    if (spell_id == 0 || spell_id == 0xFFFFFFFF || spell_id >= kMaxSpellID) {
        return;
    }

    AppendSpellCore(out, spell_id, label, true);
}

bool AppendSTML(void* stml_wnd, const std::string& text) {
    if (!stml_wnd || text.empty()) {
        return false;
    }

    auto ctor = reinterpret_cast<CXStrCtorCStringProc>(Rebase(kCXStrCtorCString));
    auto append = reinterpret_cast<CStmlWndAppendSTMLProc>(Rebase(kCStmlWndAppendSTML));
    auto parse_now = reinterpret_cast<CStmlWndForceParseNowProc>(Rebase(kCStmlWndForceParseNow));

    CXStr value{};
    CXSize result{};
    __try {
        ctor(&value, text.c_str());

        // AppendSTML takes CXStr by value. EQ destroys that stack parameter
        // before returning, so do not call the CXStr destructor here.
        append(stml_wnd, &result, value);
        parse_now(stml_wnd);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("AppendSTML exception stml=%p bytes=%u", stml_wnd, static_cast<unsigned>(text.size()));
        return false;
    }

    if (g_stml_append_log_count < 12) {
        ++g_stml_append_log_count;
        Log("AppendSTML stml=%p bytes=%u", stml_wnd, static_cast<unsigned>(text.size()));
    }
    return true;
}

bool LooksLikeItem(void* item) {
    if (!item || !IsReadableMemory(item, 0x200)) {
        return false;
    }

    uint32_t item_id = ReadValue<uint32_t>(static_cast<BYTE*>(item) + kItemNumber);
    if (item_id == 0 || item_id > 10000000) {
        return false;
    }

    return !ReadFixedString(static_cast<BYTE*>(item) + kItemName, 0x40).empty();
}

struct ItemInspectContext {
    void* contents = nullptr;
    void* item = nullptr;
    void* display_wnd = nullptr;
    size_t p_item_offset = 0;
    size_t contents_item_offset = 0;
    size_t display_offset = 0;
};

bool FindItemInContents(void* contents, void** out_item, size_t* out_item_offset) {
    if (out_item) {
        *out_item = nullptr;
    }
    if (out_item_offset) {
        *out_item_offset = 0;
    }
    if (!contents) {
        return false;
    }

    for (size_t offset : kContentsItemOffsets) {
        void* item = ReadPtrOffset(contents, offset);
        if (LooksLikeItem(item)) {
            if (out_item) {
                *out_item = item;
            }
            if (out_item_offset) {
                *out_item_offset = offset;
            }
            return true;
        }
    }

    return false;
}

void* GetItemFromContents(void* contents) {
    void* item = nullptr;
    size_t item_offset = 0;
    if (FindItemInContents(contents, &item, &item_offset)) {
        return item;
    }
    return nullptr;
}

void* FindItemDisplayWnd(void* window, size_t* out_display_offset) {
    if (out_display_offset) {
        *out_display_offset = 0;
    }
    if (!window) {
        return nullptr;
    }

    for (size_t offset : kItemWindowDisplayOffsets) {
        void* display_wnd = ReadPtrOffset(window, offset);
        if (display_wnd && IsReadableMemory(display_wnd, sizeof(void*))) {
            if (out_display_offset) {
                *out_display_offset = offset;
            }
            return display_wnd;
        }
    }

    return nullptr;
}

bool FindDisplayedItem(void* window, ItemInspectContext& context) {
    context = ItemInspectContext{};
    if (!window) {
        return false;
    }

    for (size_t p_item_offset : kItemWindowPItemOffsets) {
        void* contents = ReadPtrOffset(window, p_item_offset);
        void* item = nullptr;
        size_t contents_item_offset = 0;
        if (!FindItemInContents(contents, &item, &contents_item_offset)) {
            continue;
        }

        size_t display_offset = 0;
        void* display_wnd = FindItemDisplayWnd(window, &display_offset);
        if (!display_wnd) {
            continue;
        }

        context.contents = contents;
        context.item = item;
        context.display_wnd = display_wnd;
        context.p_item_offset = p_item_offset;
        context.contents_item_offset = contents_item_offset;
        context.display_offset = display_offset;
        return true;
    }

    return false;
}

void LogItemLookupFailure(void* window) {
    if (g_item_lookup_log_count >= 12) {
        return;
    }

    ++g_item_lookup_log_count;
    Log("ItemInspect lookup failed window=%p pItem[2b0]=%p pItem[2c0]=%p pItem[2c8]=%p pItem[2ac]=%p display[220]=%p display[21c]=%p display[230]=%p display[238]=%p",
        window,
        ReadPtrOffset(window, 0x2b0),
        ReadPtrOffset(window, 0x2c0),
        ReadPtrOffset(window, 0x2c8),
        ReadPtrOffset(window, 0x2ac),
        ReadPtrOffset(window, 0x220),
        ReadPtrOffset(window, 0x21c),
        ReadPtrOffset(window, 0x230),
        ReadPtrOffset(window, 0x238));
}

bool AppendItemInspect(void* window) {
    if (!g_config.inspect_items || !window) {
        return false;
    }

    ItemInspectContext context{};
    if (!FindDisplayedItem(window, context)) {
        LogItemLookupFailure(window);
        return false;
    }

    void* item = context.item;
    void* display_wnd = context.display_wnd;
    std::string out = "<br><c \"#00FFFF\">Native Interface<br>";

    uint32_t item_id = ReadValue<uint32_t>(static_cast<BYTE*>(item) + kItemNumber);
    std::string name = EscapeStml(ReadFixedString(static_cast<BYTE*>(item) + kItemName, 0x40));
    if (g_item_append_log_count < 12) {
        ++g_item_append_log_count;
        Log("ItemInspect append window=%p display=%p pItemOff=0x%X contents=%p itemOff=0x%X item=%p item_id=%u name=%s displayOff=0x%X",
            window,
            display_wnd,
            static_cast<unsigned>(context.p_item_offset),
            context.contents,
            static_cast<unsigned>(context.contents_item_offset),
            item,
            item_id,
            name.c_str(),
            static_cast<unsigned>(context.display_offset));
    }

    char line[256]{};
    _snprintf_s(line, sizeof(line), _TRUNCATE, "Item ID: %u<br>", item_id);
    out += line;
    AppendItemPowerLines(out, item, item_id);

    std::string lore = EscapeStml(ReadFixedString(static_cast<BYTE*>(item) + kItemLoreName, 0x70));
    if (!lore.empty() && lore != name && !(lore.size() > 1 && lore[0] == '*' && lore.substr(1) == name)) {
        out += "Item Lore: " + lore + "<br>";
    }

    AppendMoneyLine(out, ReadValue<uint32_t>(static_cast<BYTE*>(item) + kItemCost));

    int stack_size = ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemStackSize);
    if (stack_size > 1) {
        AppendLine(out, "Stackable Count", stack_size);
    }

    int damage = ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemDamage);
    int delay = ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemDelay);
    if (damage > 0 && delay > 0) {
        float ratio = static_cast<float>(delay) / static_cast<float>(damage);
        _snprintf_s(line, sizeof(line), _TRUNCATE, "Ratio: %.3f<br>", ratio);
        out += line;
        float efficiency = (static_cast<float>(damage) * 2.0f / static_cast<float>(delay)) * 50.0f;
        _snprintf_s(line, sizeof(line), _TRUNCATE, "Efficiency: %.0f<br>", efficiency);
        out += line;
    }

    uint32_t proc_spell = ReadValue<uint32_t>(static_cast<BYTE*>(item) + kItemProc + kItemSpellID);
    if (proc_spell > 0 && proc_spell != 0xFFFFFFFF && proc_spell < kMaxSpellID) {
        int required_level = ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemProc + kItemSpellRequiredLevel);
        int proc_rate = ReadValue<int32_t>(static_cast<BYTE*>(item) + kItemProc + kItemSpellProcRate);
        if (required_level <= 0) {
            required_level = 1;
        }
        if (proc_rate != 0) {
            _snprintf_s(line, sizeof(line), _TRUNCATE, "Procs at level %d (Proc rate modifier: %d)<br>",
                required_level, proc_rate);
        } else {
            _snprintf_s(line, sizeof(line), _TRUNCATE, "Procs at level %d<br>", required_level);
        }
        out += line;
    }

    uint32_t click_spell = ReadValue<uint32_t>(static_cast<BYTE*>(item) + kItemClicky + kItemSpellID);
    if (click_spell > 0 && click_spell != 0xFFFFFFFF && click_spell < kMaxSpellID) {
        int required_level = ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemClicky + kItemSpellRequiredLevel);
        if (required_level <= 0) {
            required_level = 1;
        }
        _snprintf_s(line, sizeof(line), _TRUNCATE, "Clickable at level %d<br>", required_level);
        out += line;
    }

    uint8_t slots = ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemSlots);
    if (slots) {
        AppendLine(out, "Container Slots", slots);
        AppendLine(out, "Weight Reduction", ReadValue<uint8_t>(static_cast<BYTE*>(item) + kItemWeightReduction));
    }

    AppendItemSpellLine(out, item, kItemClicky, "Clicky");
    AppendItemSpellLine(out, item, kItemProc, "Proc");
    AppendItemSpellLine(out, item, kItemWorn, "Worn");
    AppendItemSpellLine(out, item, kItemFocus, "Focus");
    AppendItemSpellLine(out, item, kItemScroll, "Scroll");

    out += "</c>";
    AppendSTML(display_wnd, out);
    return true;
}

void AppendSpellInspect(void* window) {
    if (!g_config.inspect_spells || !window) {
        return;
    }

    uint32_t spell_id = ReadValue<uint32_t>(static_cast<BYTE*>(window) + kSpellWindowSpellID);
    if (spell_id == 0 || spell_id >= kMaxSpellID || !GetSpell(spell_id)) {
        return;
    }

    void* display_wnd = ReadPtrOffset(window, kSpellWindowDisplayWnd);
    if (!display_wnd) {
        return;
    }

    if (g_spell_append_log_count < 12) {
        ++g_spell_append_log_count;
        Log("SpellInspect append window=%p display=%p spell_id=%u", window, display_wnd, spell_id);
    }

    std::string out = "<br><c \"#00FFFF\">Native Interface<br>";
    AppendSpellCore(out, spell_id, "Spell", true);
    out += "</c>";
    AppendSTML(display_wnd, out);
}

void __fastcall ItemUpdateStringsDetour(void* self, void*) {
    ItemUpdateStringsProc original = g_item_update_hook.original<ItemUpdateStringsProc>();
    if (!original) {
        return;
    }

    if (g_in_update_strings) {
        original(self);
        return;
    }

    g_in_update_strings = true;
    original(self);
    if (g_item_update_log_count < 12) {
        ++g_item_update_log_count;
        Log("ItemDisplay UpdateStrings detour self=%p", self);
    }

    if (!AppendItemInspect(self)) {
        AppendSpellInspect(self);
    }
    g_in_update_strings = false;
}

void __fastcall DspChatDetour(void* self, void*, const char* message, DWORD color, bool eq_log, bool do_percent_subst) {
    if (ParseItemPowerTransport(message)) {
        return;
    }

    DspChatProc original = g_chat_hook.original<DspChatProc>();
    if (original) {
        original(self, message, color, eq_log, do_percent_subst);
    }
}

std::string CleanSpawnName(std::string name) {
    if (name.empty()) {
        return name;
    }

    std::replace(name.begin(), name.end(), '_', ' ');
    while (!name.empty() && (name.back() == ' ' || name.back() == '\t')) {
        name.pop_back();
    }
    return name;
}

bool ShouldShowSpawn(uint8_t type, uint8_t gm, bool is_target) {
    if (is_target && g_config.map_show_target) {
        return true;
    }

    if (gm) {
        return g_config.map_show_players || g_config.map_show_npcs;
    }

    switch (type) {
    case kSpawnNPC:
        return g_config.map_show_npcs;
    case kSpawnPlayer:
        return g_config.map_show_players;
    case kSpawnCorpse:
        return g_config.map_show_corpses;
    default:
        return false;
    }
}

bool NamePassesFilters(const std::string& name, bool is_target) {
    if (is_target && g_config.map_show_target) {
        return true;
    }

    if (!g_config.map_name_filter.empty() && !ContainsInsensitive(name, g_config.map_name_filter)) {
        return false;
    }

    if (!g_config.map_hide_filter.empty() && ContainsInsensitive(name, g_config.map_hide_filter)) {
        return false;
    }

    return true;
}

bool TryParseUInt32(const std::string& value, uint32_t& out) {
    if (value.empty()) {
        return false;
    }

    char* end = nullptr;
    unsigned long parsed = strtoul(value.c_str(), &end, 10);
    if (!end || *end != '\0') {
        return false;
    }

    out = static_cast<uint32_t>(parsed);
    return true;
}

double SpawnDistanceSquared(void* local_spawn, void* spawn) {
    if (!local_spawn || !spawn) {
        return 0.0;
    }

    double dx = static_cast<double>(ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnX, 0.0f)) -
        static_cast<double>(ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnX, 0.0f));
    double dy = static_cast<double>(ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnY, 0.0f)) -
        static_cast<double>(ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnY, 0.0f));
    double dz = static_cast<double>(ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnZ, 0.0f)) -
        static_cast<double>(ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnZ, 0.0f));
    return dx * dx + dy * dy + dz * dz;
}

float SpawnDistance(void* local_spawn, void* spawn) {
    return static_cast<float>(sqrt(SpawnDistanceSquared(local_spawn, spawn)));
}

bool IsFiniteWorldCoord(float value) {
    return std::isfinite(value) && value > -100000.0f && value < 100000.0f;
}

std::string GetSpawnCleanName(void* spawn) {
    if (!spawn) {
        return {};
    }

    std::string name = CleanSpawnName(ReadFixedString(static_cast<BYTE*>(spawn) + kSpawnDisplayedName, 0x40));
    if (name.empty()) {
        name = CleanSpawnName(ReadFixedString(static_cast<BYTE*>(spawn) + kSpawnName, 0x40));
    }
    return name;
}

void* FindSpawnByID(uint32_t spawn_id) {
    if (spawn_id == 0) {
        return nullptr;
    }

    void* spawn_mgr = ReadGlobalPtr(kPInstSpawnManager);
    void* spawn = ReadPtrOffset(spawn_mgr, kSpawnManagerFirstSpawn);
    std::unordered_set<void*> visited;
    while (spawn && visited.insert(spawn).second) {
        void* next = ReadPtrOffset(spawn, kSpawnNext);
        uint32_t current_id = ReadValue<uint32_t>(static_cast<BYTE*>(spawn) + kSpawnID, 0);
        if (current_id == spawn_id) {
            return spawn;
        }
        spawn = next;
    }

    return nullptr;
}

bool LooksLikeXTargetMgr(void* manager) {
    if (!manager || !IsReadableMemory(manager, kXTargetMgrArray + sizeof(void*))) {
        return false;
    }

    uint32_t slots = ReadValue<uint32_t>(static_cast<BYTE*>(manager) + kXTargetMgrSlots, 0xFFFFFFFF);
    if (slots > kMaxXTargets) {
        return false;
    }

    void* array = ReadPtrOffset(manager, kXTargetMgrArray);
    if (slots == 0) {
        return true;
    }

    return array && IsReadableMemory(array, static_cast<size_t>(slots) * kXTargetSlotSize);
}

std::vector<XTargetInfo> ReadXTargets() {
    std::vector<XTargetInfo> targets;
    void* char_data = ReadGlobalPtr(kPInstCharData);
    if (!char_data) {
        return targets;
    }

    void* manager = ReadPtrOffset(char_data, kCharDataXTargetMgr);
    if (!LooksLikeXTargetMgr(manager)) {
        void* inline_manager = static_cast<BYTE*>(char_data) + kCharDataXTargetMgr;
        manager = LooksLikeXTargetMgr(inline_manager) ? inline_manager : nullptr;
    }
    if (!manager) {
        return targets;
    }

    uint32_t slot_count = ReadValue<uint32_t>(static_cast<BYTE*>(manager) + kXTargetMgrSlots, 0);
    slot_count = std::min<uint32_t>(slot_count, kMaxXTargets);
    void* array = ReadPtrOffset(manager, kXTargetMgrArray);
    if (!array || !IsReadableMemory(array, static_cast<size_t>(slot_count) * kXTargetSlotSize)) {
        return targets;
    }

    void* local_spawn = ReadGlobalPtr(kPInstCharSpawn);
    targets.reserve(slot_count);
    for (uint32_t i = 0; i < slot_count; ++i) {
        BYTE* slot = static_cast<BYTE*>(array) + static_cast<size_t>(i) * kXTargetSlotSize;
        uint32_t spawn_id = ReadValue<uint32_t>(slot + kXTargetSpawnID, 0);
        std::string name = CleanSpawnName(ReadFixedString(slot + kXTargetName, 0x40));
        if (spawn_id == 0 && name.empty()) {
            continue;
        }

        XTargetInfo info{};
        info.slot = static_cast<int>(i + 1);
        info.type = ReadValue<uint32_t>(slot + kXTargetType, 0);
        info.spawn_id = spawn_id;
        info.name = name;
        info.spawn = FindSpawnByID(spawn_id);
        if (info.spawn && local_spawn) {
            info.distance = SpawnDistance(local_spawn, info.spawn);
            if (info.name.empty()) {
                info.name = GetSpawnCleanName(info.spawn);
            }
        }
        targets.push_back(std::move(info));
    }

    return targets;
}

const XTargetInfo* FindXTargetForSpawn(const std::vector<XTargetInfo>& targets, void* spawn, uint32_t spawn_id) {
    if (!spawn && spawn_id == 0) {
        return nullptr;
    }

    for (const auto& target : targets) {
        if ((spawn && target.spawn == spawn) || (spawn_id != 0 && target.spawn_id == spawn_id)) {
            return &target;
        }
    }
    return nullptr;
}

bool LooksLikeGroundItem(void* item) {
    if (!item || !IsReadableMemory(item, 0x80)) {
        return false;
    }

    std::string name = CleanSpawnName(ReadFixedString(static_cast<BYTE*>(item) + kGroundItemName, 0x20));
    if (name.empty()) {
        return false;
    }

    float x = ReadValue<float>(static_cast<BYTE*>(item) + kGroundItemX, 0.0f);
    float y = ReadValue<float>(static_cast<BYTE*>(item) + kGroundItemY, 0.0f);
    float z = ReadValue<float>(static_cast<BYTE*>(item) + kGroundItemZ, 0.0f);
    return IsFiniteWorldCoord(x) && IsFiniteWorldCoord(y) && IsFiniteWorldCoord(z);
}

void* FindGroundItemList() {
    for (uintptr_t candidate_address : kPInstEQItemListCandidates) {
        void* head = ReadGlobalPtr(candidate_address);
        if (LooksLikeGroundItem(head)) {
            return head;
        }

        void* nested_head = ReadPtr(head);
        if (LooksLikeGroundItem(nested_head)) {
            return nested_head;
        }
    }

    return nullptr;
}

bool CanAddMapLabel() {
    return g_config.map_max_labels <= 0 || static_cast<int>(g_map_labels.size()) < g_config.map_max_labels;
}

void AddMapLabelText(const std::string& text, float x, float y, float z, uint32_t color, uint32_t size = 3, uint32_t layer = 2) {
    if (text.empty() || !CanAddMapLabel() || !IsFiniteWorldCoord(x) || !IsFiniteWorldCoord(y) || !IsFiniteWorldCoord(z)) {
        return;
    }

    ManagedMapLabel managed{};
    managed.text = text;
    managed.native.location.x = -x;
    managed.native.location.y = -y;
    managed.native.location.z = z;
    managed.native.color = color;
    managed.native.size = size;
    managed.native.layer = layer;
    managed.native.width = 20;
    managed.native.height = 14;
    g_map_labels.push_back(std::move(managed));
}

void AddWorldMapLine(float start_x, float start_y, float start_z, float end_x, float end_y, float end_z,
    uint32_t color, uint32_t layer = 2) {
    if (!IsFiniteWorldCoord(start_x) || !IsFiniteWorldCoord(start_y) || !IsFiniteWorldCoord(start_z) ||
        !IsFiniteWorldCoord(end_x) || !IsFiniteWorldCoord(end_y) || !IsFiniteWorldCoord(end_z)) {
        return;
    }

    MapLineNative line{};
    line.start.x = -start_x;
    line.start.y = -start_y;
    line.start.z = start_z;
    line.end.x = -end_x;
    line.end.y = -end_y;
    line.end.z = end_z;
    line.color = color;
    line.layer = layer;
    g_map_lines.push_back(line);
}

void AddMapCircle(float center_x, float center_y, float center_z, float radius, uint32_t color, uint32_t segments = 48) {
    if (radius <= 0.0f || !IsFiniteWorldCoord(center_x) || !IsFiniteWorldCoord(center_y) || !IsFiniteWorldCoord(center_z)) {
        return;
    }

    segments = NativeClamp<uint32_t>(segments, 12, 96);
    for (uint32_t i = 0; i < segments; ++i) {
        float a1 = (static_cast<float>(i) / static_cast<float>(segments)) * (2.0f * kPi);
        float a2 = (static_cast<float>(i + 1) / static_cast<float>(segments)) * (2.0f * kPi);
        AddWorldMapLine(center_x + cosf(a1) * radius, center_y + sinf(a1) * radius, center_z,
            center_x + cosf(a2) * radius, center_y + sinf(a2) * radius, center_z, color, 2);
    }
}

void AddMapBoxMarker(float center_x, float center_y, float center_z, float radius, uint32_t color) {
    if (radius <= 0.0f) {
        return;
    }

    AddWorldMapLine(center_x - radius, center_y - radius, center_z, center_x + radius, center_y - radius, center_z, color, 2);
    AddWorldMapLine(center_x + radius, center_y - radius, center_z, center_x + radius, center_y + radius, center_z, color, 2);
    AddWorldMapLine(center_x + radius, center_y + radius, center_z, center_x - radius, center_y + radius, center_z, color, 2);
    AddWorldMapLine(center_x - radius, center_y + radius, center_z, center_x - radius, center_y - radius, center_z, color, 2);
}

bool LooksNamedSpawn(uint8_t type, const std::string& name) {
    if (type != kSpawnNPC || name.empty()) {
        return false;
    }

    std::string lowered = LowerCopy(name);
    if (lowered.find("corpse") != std::string::npos) {
        return false;
    }

    return lowered.rfind("a ", 0) != 0 && lowered.rfind("an ", 0) != 0 && lowered.rfind("the ", 0) != 0;
}

bool NameIsHighlighted(const std::string& name) {
    return !g_config.map_highlight_filter.empty() && ContainsInsensitive(name, g_config.map_highlight_filter);
}

void* FindSpawnByNameOrID(const std::string& query, std::string& found_name) {
    found_name.clear();
    if (query.empty()) {
        return nullptr;
    }

    uint32_t query_id = 0;
    bool match_id = TryParseUInt32(query, query_id);
    void* spawn_mgr = ReadGlobalPtr(kPInstSpawnManager);
    void* local_spawn = ReadGlobalPtr(kPInstCharSpawn);
    void* spawn = ReadPtrOffset(spawn_mgr, kSpawnManagerFirstSpawn);
    void* best_spawn = nullptr;
    double best_distance = 1.0e30;

    std::unordered_set<void*> visited;
    while (spawn && visited.insert(spawn).second) {
        void* next = ReadPtrOffset(spawn, kSpawnNext);
        if (spawn == local_spawn) {
            spawn = next;
            continue;
        }

        std::string name = CleanSpawnName(ReadFixedString(static_cast<BYTE*>(spawn) + kSpawnDisplayedName, 0x40));
        if (name.empty()) {
            name = CleanSpawnName(ReadFixedString(static_cast<BYTE*>(spawn) + kSpawnName, 0x40));
        }

        uint32_t spawn_id = ReadValue<uint32_t>(static_cast<BYTE*>(spawn) + kSpawnID, 0);
        bool matches = (match_id && spawn_id == query_id) || (!name.empty() && ContainsInsensitive(name, query));
        if (matches) {
            double distance = SpawnDistanceSquared(local_spawn, spawn);
            if (!best_spawn || distance < best_distance) {
                best_spawn = spawn;
                best_distance = distance;
                found_name = name;
            }
        }

        spawn = next;
    }

    return best_spawn;
}

uint32_t ConColorToARGB(int con_color) {
    switch (con_color) {
    case 0:
    case 1:
        return 0xFF505050;
    case 2:
        return 0xFF00FF00;
    case 3:
        return 0xFF00FFFF;
    case 4:
        return 0xFF0000FF;
    case 5:
        return 0xFFFFFFFF;
    case 6:
        return 0xFFFFFF00;
    case 7:
    default:
        return 0xFFFF0000;
    }
}

int ApproximateConLevel(int player_level, int spawn_level) {
    if (player_level <= 0 || spawn_level <= 0) {
        return 5;
    }

    int diff = spawn_level - player_level;
    if (diff >= 3) {
        return 7;
    }
    if (diff >= 1) {
        return 6;
    }
    if (diff == 0) {
        return 5;
    }
    if (diff >= -2) {
        return 4;
    }
    if (diff >= -4) {
        return 3;
    }
    if (diff >= -7) {
        return 2;
    }
    return 1;
}

uint32_t GetSpawnConColor(void* spawn, int spawn_level) {
    void* char_data = ReadGlobalPtr(kPInstCharData);
    if (char_data) {
        auto get_con_level = reinterpret_cast<GetConLevelProc>(Rebase(kEQCharacterGetConLevel));
        __try {
            int con_level = get_con_level(char_data, spawn);
            if (con_level >= 0 && con_level <= 7) {
                return ConColorToARGB(con_level);
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    void* local_spawn = ReadGlobalPtr(kPInstCharSpawn);
    int player_level = ReadValue<int32_t>(static_cast<BYTE*>(local_spawn) + kSpawnLevel, 0);
    return ConColorToARGB(ApproximateConLevel(player_level, spawn_level));
}

uint32_t SpawnColor(void* spawn, uint8_t type, uint8_t gm, int spawn_level) {
    if (g_config.map_use_con_color && spawn && (type == kSpawnNPC || type == kSpawnPlayer)) {
        return GetSpawnConColor(spawn, spawn_level);
    }

    if (gm) {
        return 0xFFFF66FF;
    }

    switch (type) {
    case kSpawnNPC:
        return 0xFF00E5FF;
    case kSpawnPlayer:
        return 0xFF66CCFF;
    case kSpawnCorpse:
        return 0xFFBBBBBB;
    default:
        return 0xFFFFFFFF;
    }
}

void UpdateMapLabels() {
    g_map_labels.clear();
    g_map_lines.clear();

    if (!g_config.map_enabled) {
        return;
    }

    void* spawn_mgr = ReadGlobalPtr(kPInstSpawnManager);
    void* local_spawn = ReadGlobalPtr(kPInstCharSpawn);
    void* target_spawn = ReadGlobalPtr(kPInstTarget);
    void* spawn = ReadPtrOffset(spawn_mgr, kSpawnManagerFirstSpawn);
    if (!spawn) {
        return;
    }

    std::vector<XTargetInfo> xtargets = (g_config.xtar_enabled || g_config.map_show_xtargets) ? ReadXTargets() : std::vector<XTargetInfo>{};
    g_map_labels.reserve(384);
    g_map_lines.reserve(128);
    std::unordered_set<void*> visited;
    bool target_in_spawn_list = false;
    while (spawn && visited.insert(spawn).second) {

        void* next = ReadPtrOffset(spawn, kSpawnNext);
        if (spawn == local_spawn) {
            spawn = next;
            continue;
        }

        uint8_t type = ReadValue<uint8_t>(static_cast<BYTE*>(spawn) + kSpawnType, 0xFF);
        uint8_t gm = ReadValue<uint8_t>(static_cast<BYTE*>(spawn) + kSpawnGM, 0);
        bool is_target = spawn == target_spawn;
        if (is_target) {
            target_in_spawn_list = true;
        }

        uint32_t spawn_id = ReadValue<uint32_t>(static_cast<BYTE*>(spawn) + kSpawnID, 0);
        const XTargetInfo* xtarget = FindXTargetForSpawn(xtargets, spawn, spawn_id);
        bool is_xtarget = xtarget != nullptr;
        if (!ShouldShowSpawn(type, gm, is_target) && !(g_config.map_show_xtargets && is_xtarget)) {
            spawn = next;
            continue;
        }

        std::string name = GetSpawnCleanName(spawn);
        if (name.empty()) {
            spawn = next;
            continue;
        }
        if (!NamePassesFilters(name, is_target)) {
            spawn = next;
            continue;
        }
        if (g_config.map_named_only && !is_target && !is_xtarget && !LooksNamedSpawn(type, name)) {
            spawn = next;
            continue;
        }

        int level = ReadValue<int32_t>(static_cast<BYTE*>(spawn) + kSpawnLevel, 0);
        float x = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnX, 0.0f);
        float y = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnY, 0.0f);
        float z = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnZ, 0.0f);

        uint32_t color = (is_target && g_config.map_show_target) ? kMapTargetColor : SpawnColor(spawn, type, gm, level);
        uint32_t label_size = 3;
        if (is_xtarget && !is_target) {
            color = kMapXTargetColor;
        }
        if (NameIsHighlighted(name)) {
            color = g_config.map_highlight_color;
            label_size = static_cast<uint32_t>(NativeClamp(g_config.map_highlight_size, 4, 200));
            AddMapBoxMarker(x, y, z, 12.0f, g_config.map_highlight_color);
        }

        std::string label = name;
        if (is_xtarget && g_config.map_xtarget_labels && xtarget) {
            char suffix[64]{};
            if (xtarget->distance > 0.0f) {
                _snprintf_s(suffix, sizeof(suffix), _TRUNCATE, " [XT%d %.0f]", xtarget->slot, xtarget->distance);
            } else {
                _snprintf_s(suffix, sizeof(suffix), _TRUNCATE, " [XT%d]", xtarget->slot);
            }
            label += suffix;
        }
        AddMapLabelText(label, x, y, z, color, label_size);

        if (g_config.map_show_vectors) {
            float speed_x = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnSpeedX, 0.0f);
            float speed_y = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnSpeedY, 0.0f);
            float heading = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnHeading, 0.0f);
            float vector_x = speed_x;
            float vector_y = speed_y;
            if (fabsf(vector_x) < 0.01f && fabsf(vector_y) < 0.01f) {
                float angle = heading * (2.0f * kPi / 512.0f);
                vector_x = sinf(angle) * 12.0f;
                vector_y = cosf(angle) * 12.0f;
            } else {
                vector_x *= 8.0f;
                vector_y *= 8.0f;
            }
            AddWorldMapLine(x, y, z, x + vector_x, y + vector_y, z, color, 2);
        }

        spawn = next;
    }

    if (g_config.map_show_ground) {
        void* ground = FindGroundItemList();
        std::unordered_set<void*> ground_visited;
        unsigned ground_count = 0;
        while (ground && ground_visited.insert(ground).second && ground_count < 512) {
            void* next_ground = ReadPtrOffset(ground, kGroundItemNext);
            if (!LooksLikeGroundItem(ground)) {
                break;
            }

            std::string name = CleanSpawnName(ReadFixedString(static_cast<BYTE*>(ground) + kGroundItemName, 0x20));
            float x = ReadValue<float>(static_cast<BYTE*>(ground) + kGroundItemX, 0.0f);
            float y = ReadValue<float>(static_cast<BYTE*>(ground) + kGroundItemY, 0.0f);
            float z = ReadValue<float>(static_cast<BYTE*>(ground) + kGroundItemZ, 0.0f);
            AddMapLabelText(std::string("G: ") + name, x, y, z, kMapGroundColor, 3);
            AddMapBoxMarker(x, y, z, 4.0f, kMapGroundColor);
            ++ground_count;
            ground = next_ground;
        }
    }

    for (const auto& location : g_map_locations) {
        AddMapLabelText(location.label, location.x, location.y, location.z, kMapLocationColor, 4);
        AddMapBoxMarker(location.x, location.y, location.z, 8.0f, kMapLocationColor);
    }

    for (size_t i = 0; i < g_map_labels.size(); ++i) {
        g_map_labels[i].native.next = (i + 1 < g_map_labels.size()) ? &g_map_labels[i + 1].native : nullptr;
        g_map_labels[i].native.prev = (i > 0) ? &g_map_labels[i - 1].native : nullptr;
        g_map_labels[i].native.label = const_cast<char*>(g_map_labels[i].text.c_str());
    }

    if (target_in_spawn_list && local_spawn && target_spawn && local_spawn != target_spawn) {
        float local_x = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnX, 0.0f);
        float local_y = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnY, 0.0f);
        float local_z = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnZ, 0.0f);
        float target_x = ReadValue<float>(static_cast<BYTE*>(target_spawn) + kSpawnX, 0.0f);
        float target_y = ReadValue<float>(static_cast<BYTE*>(target_spawn) + kSpawnY, 0.0f);
        float target_z = ReadValue<float>(static_cast<BYTE*>(target_spawn) + kSpawnZ, 0.0f);

        if (g_config.map_target_line) {
            AddWorldMapLine(local_x, local_y, local_z, target_x, target_y, target_z, kMapTargetColor, 2);
        }
        if (g_config.map_target_radius > 0.0f) {
            AddMapCircle(target_x, target_y, target_z, g_config.map_target_radius, kMapTargetColor);
        }
    }

    if (local_spawn) {
        float local_x = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnX, 0.0f);
        float local_y = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnY, 0.0f);
        float local_z = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnZ, 0.0f);
        AddMapCircle(local_x, local_y, local_z, g_config.map_cast_radius, kMapRadiusColor);
        AddMapCircle(local_x, local_y, local_z, g_config.map_spell_radius, 0xFF80C0FF);
    }

    for (size_t i = 0; i < g_map_lines.size(); ++i) {
        g_map_lines[i].next = (i + 1 < g_map_lines.size()) ? &g_map_lines[i + 1] : nullptr;
        g_map_lines[i].prev = (i > 0) ? &g_map_lines[i - 1] : nullptr;
    }
}

void AttachMapLabels(void* map_window, MapLabelNative*& original_labels) {
    original_labels = nullptr;
    if (!map_window || g_map_labels.empty()) {
        return;
    }

    auto labels_ptr = reinterpret_cast<MapLabelNative**>(static_cast<BYTE*>(map_window) + kMapLabels);
    original_labels = ReadValue<MapLabelNative*>(labels_ptr, nullptr);

    MapLabelNative* head = &g_map_labels.front().native;
    MapLabelNative* tail = &g_map_labels.back().native;
    if (g_config.map_chain_eq_labels && original_labels) {
        tail->next = original_labels;
    }

    *labels_ptr = head;
}

void DetachMapLabels(void* map_window, MapLabelNative* original_labels) {
    if (!map_window || g_map_labels.empty()) {
        return;
    }

    auto labels_ptr = reinterpret_cast<MapLabelNative**>(static_cast<BYTE*>(map_window) + kMapLabels);
    MapLabelNative* tail = &g_map_labels.back().native;
    if (g_config.map_chain_eq_labels && original_labels) {
        tail->next = nullptr;
    }

    *labels_ptr = original_labels;
}

void AttachMapLines(void* map_window, MapLineNative*& original_lines) {
    original_lines = nullptr;
    if (!map_window || g_map_lines.empty()) {
        return;
    }

    auto lines_ptr = reinterpret_cast<MapLineNative**>(static_cast<BYTE*>(map_window) + kMapLines);
    original_lines = ReadValue<MapLineNative*>(lines_ptr, nullptr);

    MapLineNative* head = &g_map_lines.front();
    MapLineNative* tail = &g_map_lines.back();
    tail->next = original_lines;
    *lines_ptr = head;
}

void DetachMapLines(void* map_window, MapLineNative* original_lines) {
    if (!map_window || g_map_lines.empty()) {
        return;
    }

    auto lines_ptr = reinterpret_cast<MapLineNative**>(static_cast<BYTE*>(map_window) + kMapLines);
    MapLineNative* tail = &g_map_lines.back();
    tail->next = nullptr;
    *lines_ptr = original_lines;
}

int __fastcall MapPostDraw2Detour(void* self, void*) {
    if (g_in_map_post_draw) {
        return g_old_map_post_draw2 ? g_old_map_post_draw2(self) : 0;
    }

    g_in_map_post_draw = true;

    DWORD now = GetTickCount();
    DWORD update_start = now;
    if (now - g_last_map_refresh > static_cast<DWORD>(g_config.map_refresh_ms)) {
        UpdateMapLabels();
        g_last_map_refresh = now;
    }
    DWORD update_elapsed = GetTickCount() - update_start;

    if (g_map_draw_log_count < 8) {
        ++g_map_draw_log_count;
        Log("Map draw pass labels=%u lines=%u update_ms=%lu parent=%p draw_self=%p",
            static_cast<unsigned>(g_map_labels.size()), static_cast<unsigned>(g_map_lines.size()),
            update_elapsed, g_hooked_map_window, self);
    }

    MapLabelNative* original_labels = nullptr;
    MapLineNative* original_lines = nullptr;
    void* map_window = g_hooked_map_window ? g_hooked_map_window : ReadGlobalPtr(kPInstCMapViewWnd);
    AttachMapLabels(map_window, original_labels);
    AttachMapLines(map_window, original_lines);

    int result = 0;
    if (g_old_map_post_draw2) {
        result = g_old_map_post_draw2(self);
    }

    DetachMapLines(map_window, original_lines);
    DetachMapLabels(map_window, original_labels);
    g_in_map_post_draw = false;
    return result;
}

void RestoreMapHook() {
    if (!g_hooked_map_window || !g_new_map_vtable || !g_old_map_vtable) {
        return;
    }

    auto vtable_field = reinterpret_cast<void***>(static_cast<BYTE*>(g_hooked_map_window) + kMapViewVTable);
    void** current = ReadValue<void**>(vtable_field, nullptr);
    if (current == g_new_map_vtable) {
        DWORD old_protect = 0;
        if (VirtualProtect(vtable_field, sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
            *vtable_field = g_old_map_vtable;
            DWORD unused = 0;
            VirtualProtect(vtable_field, sizeof(void*), old_protect, &unused);
        }
    }

    VirtualFree(g_new_map_vtable, 0, MEM_RELEASE);
    g_hooked_map_window = nullptr;
    g_old_map_vtable = nullptr;
    g_new_map_vtable = nullptr;
    g_old_map_post_draw2 = nullptr;
}

bool InstallMapHook() {
    if (!g_config.map_enabled) {
        return false;
    }

    void* map_window = ReadGlobalPtr(kPInstCMapViewWnd);
    if (!map_window) {
        return false;
    }

    if (map_window == g_hooked_map_window && g_new_map_vtable) {
        return true;
    }

    RestoreMapHook();

    auto vtable_field = reinterpret_cast<void***>(static_cast<BYTE*>(map_window) + kMapViewVTable);
    void** old_vtable = ReadValue<void**>(vtable_field, nullptr);
    if (!old_vtable || !IsReadableMemory(old_vtable, kMapVTableBytes)) {
        return false;
    }

    void** new_vtable = static_cast<void**>(VirtualAlloc(nullptr, kMapVTableBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!new_vtable) {
        return false;
    }

    memcpy(new_vtable, old_vtable, kMapVTableBytes);
    g_old_map_post_draw2 = reinterpret_cast<MapPostDraw2Proc>(new_vtable[kMapPostDraw2Index]);
    if (new_vtable[kMapPostDraw2Index] == reinterpret_cast<void*>(&MapPostDraw2Detour)) {
        VirtualFree(new_vtable, 0, MEM_RELEASE);
        g_old_map_post_draw2 = nullptr;
        return false;
    }
    new_vtable[kMapPostDraw2Index] = reinterpret_cast<void*>(&MapPostDraw2Detour);

    DWORD old_protect = 0;
    if (!VirtualProtect(vtable_field, sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
        VirtualFree(new_vtable, 0, MEM_RELEASE);
        return false;
    }

    *vtable_field = new_vtable;
    DWORD unused = 0;
    VirtualProtect(vtable_field, sizeof(void*), old_protect, &unused);

    g_hooked_map_window = map_window;
    g_old_map_vtable = old_vtable;
    g_new_map_vtable = new_vtable;
    Log("Installed map PostDraw2 hook at map window %p", map_window);
    return true;
}

void InstallItemHook() {
    void* target = reinterpret_cast<void*>(Rebase(kCItemDisplayWndUpdateStrings));
    if (InstallInlineHook(g_item_update_hook, target, reinterpret_cast<void*>(&ItemUpdateStringsDetour), 6)) {
        Log("Installed item display UpdateStrings hook at %p", target);
    } else {
        Log("Failed to install item display UpdateStrings hook at %p", target);
    }
}

void InstallChatHook() {
    void* target = reinterpret_cast<void*>(Rebase(kCEverQuestDspChat));
    if (InstallInlineHook(g_chat_hook, target, reinterpret_cast<void*>(&DspChatDetour), 6)) {
        Log("Installed DspChat hook at %p", target);
    } else {
        Log("Failed to install DspChat hook at %p", target);
    }
}

void PrintMapStatus() {
    void* map_window = ReadGlobalPtr(kPInstCMapViewWnd);
    void* spawn_mgr = ReadGlobalPtr(kPInstSpawnManager);
    void* first_spawn = ReadPtrOffset(spawn_mgr, kSpawnManagerFirstSpawn);
    void* target_spawn = ReadGlobalPtr(kPInstTarget);
    char max_labels[32]{};
    if (g_config.map_max_labels <= 0) {
        strcpy_s(max_labels, "unlimited");
    } else {
        _snprintf_s(max_labels, sizeof(max_labels), _TRUNCATE, "%d", g_config.map_max_labels);
    }
    Chat("NativeMap: %s, hook=%s, labels=%u, map=%p, hooked=%p, first_spawn=%p",
        g_config.map_enabled ? "on" : "off",
        g_new_map_vtable ? "installed" : "not installed",
        static_cast<unsigned>(g_map_labels.size()),
        map_window,
        g_hooked_map_window,
        first_spawn);
    Chat("NativeMap filters: npcs=%d players=%d corpses=%d con_color=%d target=%d target_line=%d normal_labels=%d max=%s refresh_ms=%d",
        g_config.map_show_npcs ? 1 : 0,
        g_config.map_show_players ? 1 : 0,
        g_config.map_show_corpses ? 1 : 0,
        g_config.map_use_con_color ? 1 : 0,
        g_config.map_show_target ? 1 : 0,
        g_config.map_target_line ? 1 : 0,
        g_config.map_chain_eq_labels ? 1 : 0,
        max_labels,
        g_config.map_refresh_ms);
    Chat("NativeMap extra: ground=%d vector=%d named=%d xtargets=%d xtlabels=%d radii target=%.1f cast=%.1f spell=%.1f",
        g_config.map_show_ground ? 1 : 0,
        g_config.map_show_vectors ? 1 : 0,
        g_config.map_named_only ? 1 : 0,
        g_config.map_show_xtargets ? 1 : 0,
        g_config.map_xtarget_labels ? 1 : 0,
        g_config.map_target_radius,
        g_config.map_cast_radius,
        g_config.map_spell_radius);
    Chat("NativeMap search: filter='%s' hide='%s' highlight='%s' current_target=%p lines=%u",
        g_config.map_name_filter.empty() ? "" : g_config.map_name_filter.c_str(),
        g_config.map_hide_filter.empty() ? "" : g_config.map_hide_filter.c_str(),
        g_config.map_highlight_filter.empty() ? "" : g_config.map_highlight_filter.c_str(),
        target_spawn,
        static_cast<unsigned>(g_map_lines.size()));
}

void RefreshMapOverlay() {
    g_last_map_refresh = 0;
    UpdateMapLabels();
}

void SetMapBool(bool& setting, const char* ini_key, bool enabled, bool refresh) {
    setting = enabled;
    SaveConfigBool("Map", ini_key, enabled);
    if (refresh) {
        RefreshMapOverlay();
    }
}

void SetMapFloat(float& setting, const char* ini_key, float value) {
    setting = std::max(0.0f, value);
    SaveConfigFloat("Map", ini_key, setting);
    RefreshMapOverlay();
}

bool HandleTextFilterCommand(const char* args, std::string& setting, const char* ini_key, const char* label) {
    std::string value = TrimCopy(args);
    if (value.empty()) {
        Chat("NativeMap %s filter is '%s'.", label, setting.empty() ? "" : setting.c_str());
        return true;
    }

    std::string lowered = LowerCopy(value);
    if (lowered == "clear" || lowered == "off" || lowered == "none") {
        setting.clear();
        SaveConfigString("Map", ini_key, setting);
        RefreshMapOverlay();
        Chat("NativeMap %s filter cleared.", label);
        return true;
    }

    setting = value;
    SaveConfigString("Map", ini_key, setting);
    RefreshMapOverlay();
    Chat("NativeMap %s filter set to '%s'.", label, setting.c_str());
    return true;
}

bool HandleTargetCommand(const char* args) {
    std::string query = TrimCopy(args);
    if (query.empty()) {
        Chat("NativeMap target usage: /nimap target <name-or-id>|clear");
        return true;
    }

    std::string lowered = LowerCopy(query);
    if (lowered == "clear" || lowered == "off") {
        if (WriteGlobalPtr(kPInstTarget, nullptr)) {
            RefreshMapOverlay();
            Chat("NativeMap target cleared.");
        } else {
            Chat("NativeMap target clear failed.");
        }
        return true;
    }

    std::string found_name;
    void* spawn = FindSpawnByNameOrID(query, found_name);
    if (!spawn) {
        Chat("NativeMap found no spawn matching '%s'.", query.c_str());
        return true;
    }

    if (WriteGlobalPtr(kPInstTarget, spawn)) {
        RefreshMapOverlay();
        Chat("NativeMap target selected: %s.", found_name.empty() ? query.c_str() : found_name.c_str());
    } else {
        Chat("NativeMap target select failed.");
    }
    return true;
}

bool HandleMapFilterCommand(const char* args) {
    if (!args || !args[0] || CommandMatch(args, "status", nullptr)) {
        RefreshMapOverlay();
        PrintMapStatus();
        return true;
    }

    if (CommandMatch(args, "help", nullptr)) {
        Chat("MapFilter: NPC|PC|Corpse|Target|TargetLine|NormalLabels|NPCConColor|Ground|Vector|Named|XTargets|XTargetLabels on/off");
        Chat("MapFilter: Custom <text>, Hide <text>, TargetRadius|CastRadius|SpellRadius <distance>");
        return true;
    }

    const char* sub_args = nullptr;
    if (CommandMatch(args, "NPC", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_npcs, !g_config.map_show_npcs);
        SetMapBool(g_config.map_show_npcs, "ShowNPCs", enabled, true);
        Chat("MapFilter NPC is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "PC", &sub_args) || CommandMatch(args, "Player", &sub_args) ||
        CommandMatch(args, "Players", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_players, !g_config.map_show_players);
        SetMapBool(g_config.map_show_players, "ShowPlayers", enabled, true);
        Chat("MapFilter PC is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "Corpse", &sub_args) || CommandMatch(args, "Corpses", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_corpses, !g_config.map_show_corpses);
        SetMapBool(g_config.map_show_corpses, "ShowCorpses", enabled, true);
        Chat("MapFilter Corpse is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "Target", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_target, !g_config.map_show_target);
        SetMapBool(g_config.map_show_target, "ShowTarget", enabled, true);
        Chat("MapFilter Target is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "TargetLine", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_target_line, !g_config.map_target_line);
        SetMapBool(g_config.map_target_line, "TargetLine", enabled, true);
        Chat("MapFilter TargetLine is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "NormalLabels", &sub_args) || CommandMatch(args, "Labels", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_chain_eq_labels, !g_config.map_chain_eq_labels);
        SetMapBool(g_config.map_chain_eq_labels, "ChainEQLabels", enabled, true);
        Chat("MapFilter NormalLabels is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "NPCConColor", &sub_args) || CommandMatch(args, "ConColor", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_use_con_color, !g_config.map_use_con_color);
        SetMapBool(g_config.map_use_con_color, "UseConColor", enabled, true);
        Chat("MapFilter NPCConColor is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "Ground", &sub_args) || CommandMatch(args, "GroundItems", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_ground, !g_config.map_show_ground);
        SetMapBool(g_config.map_show_ground, "Ground", enabled, true);
        Chat("MapFilter Ground is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "Vector", &sub_args) || CommandMatch(args, "Vectors", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_vectors, !g_config.map_show_vectors);
        SetMapBool(g_config.map_show_vectors, "Vector", enabled, true);
        Chat("MapFilter Vector is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "Named", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_named_only, !g_config.map_named_only);
        SetMapBool(g_config.map_named_only, "Named", enabled, true);
        Chat("MapFilter Named is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "XTargets", &sub_args) || CommandMatch(args, "XTarget", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_xtargets, !g_config.map_show_xtargets);
        SetMapBool(g_config.map_show_xtargets, "XTargets", enabled, true);
        Chat("MapFilter XTargets is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "XTargetLabels", &sub_args) || CommandMatch(args, "XTLabels", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_xtarget_labels, !g_config.map_xtarget_labels);
        SetMapBool(g_config.map_xtarget_labels, "XTargetLabels", enabled, true);
        Chat("MapFilter XTargetLabels is now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "TargetRadius", &sub_args)) {
        float value = 0.0f;
        if (!TryParseFloat(TrimCopy(sub_args), value)) {
            Chat("MapFilter TargetRadius is %.1f. Usage: /mapfilter TargetRadius <distance>", g_config.map_target_radius);
            return true;
        }
        SetMapFloat(g_config.map_target_radius, "TargetRadius", value);
        Chat("MapFilter TargetRadius set to %.1f.", g_config.map_target_radius);
        return true;
    }

    if (CommandMatch(args, "CastRadius", &sub_args)) {
        float value = 0.0f;
        if (!TryParseFloat(TrimCopy(sub_args), value)) {
            Chat("MapFilter CastRadius is %.1f. Usage: /mapfilter CastRadius <distance>", g_config.map_cast_radius);
            return true;
        }
        SetMapFloat(g_config.map_cast_radius, "CastRadius", value);
        Chat("MapFilter CastRadius set to %.1f.", g_config.map_cast_radius);
        return true;
    }

    if (CommandMatch(args, "SpellRadius", &sub_args)) {
        float value = 0.0f;
        if (!TryParseFloat(TrimCopy(sub_args), value)) {
            Chat("MapFilter SpellRadius is %.1f. Usage: /mapfilter SpellRadius <distance>", g_config.map_spell_radius);
            return true;
        }
        SetMapFloat(g_config.map_spell_radius, "SpellRadius", value);
        Chat("MapFilter SpellRadius set to %.1f.", g_config.map_spell_radius);
        return true;
    }

    if (CommandMatch(args, "Custom", &sub_args) || CommandMatch(args, "Filter", &sub_args)) {
        return HandleTextFilterCommand(sub_args, g_config.map_name_filter, "NameFilter", "name");
    }

    if (CommandMatch(args, "Hide", &sub_args)) {
        return HandleTextFilterCommand(sub_args, g_config.map_hide_filter, "HideFilter", "hide");
    }

    Chat("MapFilter supported: NPC, PC, Corpse, Target, TargetLine, NormalLabels, NPCConColor, Ground, Vector, Named, XTargets, Custom, Hide.");
    return true;
}

bool HandleNativeMapCommand(const char* line) {
    const char* args = nullptr;
    if (CommandMatch(line, "/mapfilter", &args)) {
        return HandleMapFilterCommand(args);
    }

    if (!CommandMatch(line, "/nimap", &args) &&
        !CommandMatch(line, "/nativeinterfacemap", &args)) {
        return false;
    }

    if (!args || !args[0] || CommandMatch(args, "status", nullptr)) {
        RefreshMapOverlay();
        PrintMapStatus();
        return true;
    }

    if (CommandMatch(args, "help", nullptr)) {
        Chat("NativeMap: on|off|status|reload|npcs|players|corpses|con|labels|showtarget|targetline|ground|vector|named|filter|search|hide|target");
        Chat("NativeMap commands: /nimap or /nativeinterfacemap.");
        Chat("Map tools: /mapfilter help, /mapshow <text>, /maphide <text>, /highlight <text>, /maploc add <label>, /xtarinfo");
        return true;
    }

    if (CommandMatch(args, "on", nullptr)) {
        g_config.map_enabled = true;
        SaveConfigBool("Map", "Enabled", true);
        g_last_map_refresh = 0;
        InstallMapHook();
        UpdateMapLabels();
        Chat("NativeMap is now on.");
        PrintMapStatus();
        return true;
    }

    if (CommandMatch(args, "off", nullptr)) {
        g_config.map_enabled = false;
        SaveConfigBool("Map", "Enabled", false);
        RestoreMapHook();
        g_map_labels.clear();
        g_map_lines.clear();
        Chat("NativeMap is now off.");
        return true;
    }

    if (CommandMatch(args, "reload", nullptr)) {
        LoadConfig();
        g_last_map_refresh = 0;
        UpdateMapLabels();
        InstallMapHook();
        Chat("NativeMap config reloaded.");
        PrintMapStatus();
        return true;
    }

    const char* sub_args = nullptr;
    if (CommandMatch(args, "npcs", &sub_args) || CommandMatch(args, "npc", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_npcs, true);
        SetMapBool(g_config.map_show_npcs, "ShowNPCs", enabled, true);
        Chat("NativeMap NPC labels %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "players", &sub_args) || CommandMatch(args, "player", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_players, true);
        SetMapBool(g_config.map_show_players, "ShowPlayers", enabled, true);
        Chat("NativeMap player labels %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "corpses", &sub_args) || CommandMatch(args, "corpse", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_corpses, true);
        SetMapBool(g_config.map_show_corpses, "ShowCorpses", enabled, true);
        Chat("NativeMap corpse labels %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "labels", &sub_args) || CommandMatch(args, "normallabels", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_chain_eq_labels, true);
        SetMapBool(g_config.map_chain_eq_labels, "ChainEQLabels", enabled, true);
        Chat("NativeMap normal map labels %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "con", &sub_args) || CommandMatch(args, "concolor", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_use_con_color, true);
        SetMapBool(g_config.map_use_con_color, "UseConColor", enabled, true);
        Chat("NativeMap con colors %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "showtarget", &sub_args) || CommandMatch(args, "targetlabel", &sub_args) ||
        CommandMatch(args, "highlight", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_target, true);
        SetMapBool(g_config.map_show_target, "ShowTarget", enabled, true);
        Chat("NativeMap target highlight %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "targetline", &sub_args) || CommandMatch(args, "line", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_target_line, true);
        SetMapBool(g_config.map_target_line, "TargetLine", enabled, true);
        Chat("NativeMap target line %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "ground", &sub_args) || CommandMatch(args, "grounditems", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_ground, true);
        SetMapBool(g_config.map_show_ground, "Ground", enabled, true);
        Chat("NativeMap ground labels %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "vector", &sub_args) || CommandMatch(args, "vectors", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_vectors, true);
        SetMapBool(g_config.map_show_vectors, "Vector", enabled, true);
        Chat("NativeMap movement vectors %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "named", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_named_only, true);
        SetMapBool(g_config.map_named_only, "Named", enabled, true);
        Chat("NativeMap named-only filter %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "filter", &sub_args) || CommandMatch(args, "name", &sub_args) || CommandMatch(args, "search", &sub_args)) {
        return HandleTextFilterCommand(sub_args, g_config.map_name_filter, "NameFilter", "name");
    }

    if (CommandMatch(args, "hide", &sub_args)) {
        return HandleTextFilterCommand(sub_args, g_config.map_hide_filter, "HideFilter", "hide");
    }

    if (CommandMatch(args, "target", &sub_args)) {
        return HandleTargetCommand(sub_args);
    }

    Chat("NativeMap commands: /nimap help, or /mapfilter help.");
    return true;
}

void PrintXTargets(bool list_entries) {
    std::vector<XTargetInfo> targets = ReadXTargets();
    Chat("XTarInfo: %s, slots=%u, map=%d, labels=%d",
        g_config.xtar_enabled ? "on" : "off",
        static_cast<unsigned>(targets.size()),
        g_config.map_show_xtargets ? 1 : 0,
        g_config.map_xtarget_labels ? 1 : 0);

    if (!list_entries) {
        return;
    }

    if (targets.empty()) {
        Chat("XTarInfo: no extended targets found.");
        return;
    }

    for (const auto& target : targets) {
        Chat("XT%d: id=%u type=%u dist=%.1f name='%s'",
            target.slot,
            target.spawn_id,
            target.type,
            target.distance,
            target.name.empty() ? "" : target.name.c_str());
    }
}

bool HandleXTarInfoCommand(const char* line) {
    const char* args = nullptr;
    if (!CommandMatch(line, "/xtarinfo", &args) && !CommandMatch(line, "/xtar", &args)) {
        return false;
    }

    if (!args || !args[0] || CommandMatch(args, "status", nullptr)) {
        PrintXTargets(false);
        return true;
    }

    if (CommandMatch(args, "help", nullptr)) {
        Chat("XTarInfo: on|off|status|list|map on/off|labels on/off");
        return true;
    }

    if (CommandMatch(args, "list", nullptr)) {
        PrintXTargets(true);
        return true;
    }

    if (CommandMatch(args, "on", nullptr) || CommandMatch(args, "off", nullptr)) {
        bool enabled = ParseOnOff(args, g_config.xtar_enabled, !g_config.xtar_enabled);
        g_config.xtar_enabled = enabled;
        SaveConfigBool("XTarget", "Enabled", enabled);
        RefreshMapOverlay();
        Chat("XTarInfo is now %s.", enabled ? "on" : "off");
        return true;
    }

    const char* sub_args = nullptr;
    if (CommandMatch(args, "map", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_show_xtargets, !g_config.map_show_xtargets);
        SetMapBool(g_config.map_show_xtargets, "XTargets", enabled, true);
        Chat("XTarInfo map labels are now %s.", enabled ? "on" : "off");
        return true;
    }

    if (CommandMatch(args, "labels", &sub_args) || CommandMatch(args, "label", &sub_args)) {
        bool enabled = ParseOnOff(sub_args, g_config.map_xtarget_labels, !g_config.map_xtarget_labels);
        SetMapBool(g_config.map_xtarget_labels, "XTargetLabels", enabled, true);
        Chat("XTarInfo label suffixes are now %s.", enabled ? "on" : "off");
        return true;
    }

    Chat("XTarInfo commands: /xtarinfo help.");
    return true;
}

bool HandleHighlightCommand(const char* line) {
    const char* args = nullptr;
    if (!CommandMatch(line, "/highlight", &args)) {
        return false;
    }

    std::string value = TrimCopy(args);
    if (value.empty() || CommandMatch(value.c_str(), "status", nullptr)) {
        Chat("Highlight: filter='%s' color=#%06X size=%d",
            g_config.map_highlight_filter.empty() ? "" : g_config.map_highlight_filter.c_str(),
            g_config.map_highlight_color & 0x00FFFFFF,
            g_config.map_highlight_size);
        return true;
    }

    std::vector<std::string> words = SplitWords(value.c_str());
    std::string first = words.empty() ? "" : LowerCopy(words[0]);
    if (first == "clear" || first == "off" || first == "none") {
        g_config.map_highlight_filter.clear();
        SaveConfigString("Map", "HighlightFilter", g_config.map_highlight_filter);
        RefreshMapOverlay();
        Chat("Highlight filter cleared.");
        return true;
    }

    if (first == "color") {
        uint32_t color = 0;
        if (words.size() == 2 && TryParseColorToken(words[1], color)) {
            g_config.map_highlight_color = color | 0xFF000000;
            SaveConfigInt("Map", "HighlightColor", static_cast<int>(g_config.map_highlight_color & 0x00FFFFFF));
            RefreshMapOverlay();
            Chat("Highlight color set to #%06X.", g_config.map_highlight_color & 0x00FFFFFF);
            return true;
        }

        int r = 0;
        int g = 0;
        int b = 0;
        if (words.size() >= 4 && TryParseInt(words[1], r) && TryParseInt(words[2], g) && TryParseInt(words[3], b)) {
            r = NativeClamp(r, 0, 255);
            g = NativeClamp(g, 0, 255);
            b = NativeClamp(b, 0, 255);
            g_config.map_highlight_color = 0xFF000000 | (static_cast<uint32_t>(r) << 16) |
                (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
            SaveConfigInt("Map", "HighlightColor", static_cast<int>(g_config.map_highlight_color & 0x00FFFFFF));
            RefreshMapOverlay();
            Chat("Highlight color set to #%06X.", g_config.map_highlight_color & 0x00FFFFFF);
            return true;
        }

        Chat("Highlight color usage: /highlight color #RRGGBB or /highlight color <r> <g> <b>");
        return true;
    }

    if (first == "size") {
        int size = 0;
        if (words.size() < 2 || !TryParseInt(words[1], size)) {
            Chat("Highlight size is %d. Usage: /highlight size <4-200>", g_config.map_highlight_size);
            return true;
        }

        g_config.map_highlight_size = NativeClamp(size, 4, 200);
        SaveConfigInt("Map", "HighlightSize", g_config.map_highlight_size);
        RefreshMapOverlay();
        Chat("Highlight size set to %d.", g_config.map_highlight_size);
        return true;
    }

    g_config.map_highlight_filter = value;
    SaveConfigString("Map", "HighlightFilter", g_config.map_highlight_filter);
    RefreshMapOverlay();
    Chat("Highlight filter set to '%s'.", g_config.map_highlight_filter.c_str());
    return true;
}

bool HandleMapLocCommand(const char* line) {
    const char* args = nullptr;
    if (!CommandMatch(line, "/maploc", &args)) {
        return false;
    }

    std::string value = TrimCopy(args);
    if (value.empty() || CommandMatch(value.c_str(), "help", nullptr)) {
        Chat("MapLoc: add <label>|clear|list. Markers are runtime-only.");
        return true;
    }

    if (CommandMatch(value.c_str(), "clear", nullptr)) {
        g_map_locations.clear();
        RefreshMapOverlay();
        Chat("MapLoc markers cleared.");
        return true;
    }

    if (CommandMatch(value.c_str(), "list", nullptr)) {
        Chat("MapLoc markers: %u", static_cast<unsigned>(g_map_locations.size()));
        for (size_t i = 0; i < g_map_locations.size() && i < 20; ++i) {
            const auto& loc = g_map_locations[i];
            Chat("%u: %s (%.1f, %.1f, %.1f)", static_cast<unsigned>(i + 1), loc.label.c_str(), loc.x, loc.y, loc.z);
        }
        return true;
    }

    const char* label_args = nullptr;
    std::string label;
    if (CommandMatch(value.c_str(), "add", &label_args)) {
        label = TrimCopy(label_args);
    } else {
        label = value;
    }
    if (label.empty()) {
        char fallback[32]{};
        _snprintf_s(fallback, sizeof(fallback), _TRUNCATE, "Loc %u", static_cast<unsigned>(g_map_locations.size() + 1));
        label = fallback;
    }

    void* local_spawn = ReadGlobalPtr(kPInstCharSpawn);
    if (!local_spawn) {
        Chat("MapLoc failed: local spawn is unavailable.");
        return true;
    }

    MapLocation location{};
    location.label = label;
    location.x = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnX, 0.0f);
    location.y = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnY, 0.0f);
    location.z = ReadValue<float>(static_cast<BYTE*>(local_spawn) + kSpawnZ, 0.0f);
    g_map_locations.push_back(std::move(location));
    RefreshMapOverlay();
    Chat("MapLoc added '%s'.", label.c_str());
    return true;
}

bool HandleMapShowHideCommand(const char* line) {
    const char* args = nullptr;
    if (CommandMatch(line, "/mapshow", &args)) {
        return HandleTextFilterCommand(args, g_config.map_name_filter, "NameFilter", "name");
    }

    if (CommandMatch(line, "/maphide", &args)) {
        return HandleTextFilterCommand(args, g_config.map_hide_filter, "HideFilter", "hide");
    }

    return false;
}

void PrintGroundProbe() {
    for (uintptr_t candidate_address : kPInstEQItemListCandidates) {
        void* head = ReadGlobalPtr(candidate_address);
        void* nested = ReadPtr(head);
        std::string head_name = LooksLikeGroundItem(head)
            ? CleanSpawnName(ReadFixedString(static_cast<BYTE*>(head) + kGroundItemName, 0x20))
            : "";
        std::string nested_name = LooksLikeGroundItem(nested)
            ? CleanSpawnName(ReadFixedString(static_cast<BYTE*>(nested) + kGroundItemName, 0x20))
            : "";
        Chat("NativeInterface ground pinst=%p head=%p '%s' nested=%p '%s'",
            reinterpret_cast<void*>(Rebase(candidate_address)),
            head,
            head_name.c_str(),
            nested,
            nested_name.c_str());
    }
}

bool HandleNativeInterfaceCommand(const char* line) {
    const char* args = nullptr;
    if (!CommandMatch(line, "/ni", &args) &&
        !CommandMatch(line, "/nativeinterface", &args)) {
        return false;
    }

    if (!args || !args[0] || CommandMatch(args, "help", nullptr)) {
        Chat("NativeInterface: status|map|xtar|ground|windows|spawn <name-or-id>");
        Chat("NativeInterface commands: /ni or /nativeinterface.");
        return true;
    }

    if (CommandMatch(args, "status", nullptr)) {
        Chat("NativeInterface status: map_hook=%d item_hook=%d chat_hook=%d cmd_hook=%d labels=%u lines=%u locs=%u power_cache=%u",
            g_new_map_vtable ? 1 : 0,
            g_item_update_hook.installed ? 1 : 0,
            g_chat_hook.installed ? 1 : 0,
            g_command_hook.installed ? 1 : 0,
            static_cast<unsigned>(g_map_labels.size()),
            static_cast<unsigned>(g_map_lines.size()),
            static_cast<unsigned>(g_map_locations.size()),
            static_cast<unsigned>(ItemPowerCacheSize()));
        Chat("NativeInterface globals: char=%p self=%p target=%p map=%p spawn_mgr=%p",
            ReadGlobalPtr(kPInstCharData),
            ReadGlobalPtr(kPInstCharSpawn),
            ReadGlobalPtr(kPInstTarget),
            ReadGlobalPtr(kPInstCMapViewWnd),
            ReadGlobalPtr(kPInstSpawnManager));
        return true;
    }

    if (CommandMatch(args, "map", nullptr)) {
        RefreshMapOverlay();
        PrintMapStatus();
        return true;
    }

    if (CommandMatch(args, "xtar", nullptr) || CommandMatch(args, "xtarget", nullptr)) {
        PrintXTargets(true);
        return true;
    }

    if (CommandMatch(args, "ground", nullptr)) {
        PrintGroundProbe();
        return true;
    }

    if (CommandMatch(args, "windows", nullptr)) {
        Chat("NativeInterface windows: target=%p player=%p map=%p cast=%p inv=%p itemdisplay_hook=%d",
            ReadGlobalPtr(kPInstCTargetWnd),
            ReadGlobalPtr(kPInstCPlayerWnd),
            ReadGlobalPtr(kPInstCMapViewWnd),
            ReadGlobalPtr(kPInstCCastSpellWnd),
            ReadGlobalPtr(kPInstCInventoryWnd),
            g_item_update_hook.installed ? 1 : 0);
        return true;
    }

    const char* sub_args = nullptr;
    if (CommandMatch(args, "spawn", &sub_args)) {
        std::string query = TrimCopy(sub_args);
        if (query.empty()) {
            Chat("NativeInterface spawn usage: /ni spawn <name-or-id>");
            return true;
        }

        std::string found_name;
        void* spawn = FindSpawnByNameOrID(query, found_name);
        if (!spawn) {
            Chat("NativeInterface spawn: no match for '%s'.", query.c_str());
            return true;
        }

        void* local_spawn = ReadGlobalPtr(kPInstCharSpawn);
        uint32_t spawn_id = ReadValue<uint32_t>(static_cast<BYTE*>(spawn) + kSpawnID, 0);
        uint8_t type = ReadValue<uint8_t>(static_cast<BYTE*>(spawn) + kSpawnType, 0xFF);
        int level = ReadValue<int32_t>(static_cast<BYTE*>(spawn) + kSpawnLevel, 0);
        float x = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnX, 0.0f);
        float y = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnY, 0.0f);
        float z = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnZ, 0.0f);
        float speed_x = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnSpeedX, 0.0f);
        float speed_y = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnSpeedY, 0.0f);
        float heading = ReadValue<float>(static_cast<BYTE*>(spawn) + kSpawnHeading, 0.0f);
        Chat("NativeInterface spawn '%s': ptr=%p id=%u type=%u level=%d dist=%.1f loc=(%.1f,%.1f,%.1f) speed=(%.2f,%.2f) heading=%.2f",
            found_name.empty() ? query.c_str() : found_name.c_str(),
            spawn,
            spawn_id,
            type,
            level,
            SpawnDistance(local_spawn, spawn),
            x,
            y,
            z,
            speed_x,
            speed_y,
            heading);
        return true;
    }

    Chat("NativeInterface commands: /ni help.");
    return true;
}

void __fastcall InterpretCmdDetour(void* self, void*, void* player, char* line) {
    if (HandleNativeMapCommand(line) ||
        HandleXTarInfoCommand(line) ||
        HandleHighlightCommand(line) ||
        HandleMapLocCommand(line) ||
        HandleMapShowHideCommand(line) ||
        HandleNativeInterfaceCommand(line)) {
        return;
    }

    InterpretCmdProc original = g_command_hook.original<InterpretCmdProc>();
    if (original) {
        original(self, player, line);
    }
}

bool HandleCommandLine(const char* line) {
    return HandleNativeMapCommand(line) ||
        HandleXTarInfoCommand(line) ||
        HandleHighlightCommand(line) ||
        HandleMapLocCommand(line) ||
        HandleMapShowHideCommand(line) ||
        HandleNativeInterfaceCommand(line);
}

void InstallCommandHook() {
    void* target = reinterpret_cast<void*>(Rebase(kCEverQuestInterpretCmd));
    if (InstallInlineHook(g_command_hook, target, reinterpret_cast<void*>(&InterpretCmdDetour), 7)) {
        Log("Installed InterpretCmd hook at %p", target);
    } else {
        Log("Failed to install InterpretCmd hook at %p", target);
    }
}

HMODULE LoadRealDInput() {
    if (g_real_dinput) {
        return g_real_dinput;
    }

    char system_dir[MAX_PATH]{};
    if (!GetSystemDirectoryA(system_dir, MAX_PATH)) {
        return nullptr;
    }

    std::string path = system_dir;
    if (!path.empty() && path.back() != '\\') {
        path.push_back('\\');
    }
    path += "dinput8.dll";

    g_real_dinput = LoadLibraryA(path.c_str());
    return g_real_dinput;
}

DWORD WINAPI WorkerThread(LPVOID) {
    Sleep(1200);
    LoadConfig();
    if (!g_item_power_lock_ready) {
        InitializeCriticalSection(&g_item_power_lock);
        g_item_power_lock_ready = true;
    }
    Log("Native Interface DLL starting. Map=%d InspectItems=%d InspectSpells=%d",
        g_config.map_enabled ? 1 : 0, g_config.inspect_items ? 1 : 0, g_config.inspect_spells ? 1 : 0);

    InstallItemHook();

    DWORD last_config_reload = GetTickCount();
    while (!g_shutdown) {
        DWORD now = GetTickCount();
        if (now - last_config_reload > 5000) {
            LoadConfig();
            last_config_reload = now;
        }

        if (g_config.map_enabled) {
            InstallMapHook();
        } else {
            RestoreMapHook();
        }

        Sleep(500);
    }

    return 0;
}

bool HandleChatMessage(const char* message) {
    return ParseItemPowerTransport(message);
}

void Start(HMODULE module) {
    g_module = module;
    g_shutdown = false;
    if (!g_worker) {
        g_worker = CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
    }
}

void Shutdown() {
    g_shutdown = true;
    RemoveInlineHook(g_chat_hook);
    RemoveInlineHook(g_item_update_hook);
    RemoveInlineHook(g_command_hook);
    RestoreMapHook();
    if (g_item_power_lock_ready) {
        DeleteCriticalSection(&g_item_power_lock);
        g_item_power_lock_ready = false;
    }
    if (g_worker) {
        CloseHandle(g_worker);
        g_worker = nullptr;
    }
}

} // namespace nativeinterface

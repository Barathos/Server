#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <dinput.h>

#include <cstdint>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

namespace {
	constexpr std::uintptr_t kEqImageBase = 0x400000;
	constexpr std::uintptr_t kCEverQuestDspChat = 0x51F1A0;
	constexpr std::uintptr_t kCItemDisplayWndUpdateStrings = 0x69AE30;
	constexpr std::uintptr_t kCStmlWndAppendSTML = 0x886720;

	using DirectInput8CreateProc = HRESULT(WINAPI *)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);
	using DllCanUnloadNowProc = HRESULT(WINAPI *)();
	using DllGetClassObjectProc = HRESULT(WINAPI *)(REFCLSID, REFIID, LPVOID *);
	using DllRegisterServerProc = HRESULT(WINAPI *)();
	using DllUnregisterServerProc = HRESULT(WINAPI *)();
	using GetdfDIJoystickProc = LPCDIDATAFORMAT(WINAPI *)();

	HMODULE g_real_dinput8 = nullptr;
	DirectInput8CreateProc g_direct_input8_create = nullptr;
	DllCanUnloadNowProc g_dll_can_unload_now = nullptr;
	DllGetClassObjectProc g_dll_get_class_object = nullptr;
	DllRegisterServerProc g_dll_register_server = nullptr;
	DllUnregisterServerProc g_dll_unregister_server = nullptr;
	GetdfDIJoystickProc g_getdf_di_joystick = nullptr;

	CRITICAL_SECTION g_rarity_lock;
	bool g_rarity_lock_ready = false;

	struct RarityInfo {
		int tier = 0;
		const char *name = "Common";
		const char *hex = "#F0F0F0";
	};

	std::unordered_map<std::uint32_t, RarityInfo> g_rarity_by_item;

	struct InlineHook {
		void *target = nullptr;
		void *gateway = nullptr;
		BYTE original[5] {};
		bool installed = false;
	};

	InlineHook g_chat_hook;
	InlineHook g_item_display_hook;

	void Trace(const char *format, ...)
	{
		char module_path[MAX_PATH] {};
		GetModuleFileNameA(nullptr, module_path, MAX_PATH);
		char *slash = strrchr(module_path, '\\');
		if (slash) {
			*(slash + 1) = '\0';
		}
		else {
			module_path[0] = '\0';
		}

		char log_path[MAX_PATH] {};
		sprintf_s(log_path, "%sitem_rarity_native.log", module_path);

		FILE *file = nullptr;
		if (fopen_s(&file, log_path, "a") || !file) {
			return;
		}

		SYSTEMTIME now {};
		GetLocalTime(&now);
		fprintf(
			file,
			"[%04u-%02u-%02u %02u:%02u:%02u] ",
			now.wYear,
			now.wMonth,
			now.wDay,
			now.wHour,
			now.wMinute,
			now.wSecond
		);

		va_list args;
		va_start(args, format);
		vfprintf(file, format, args);
		va_end(args);

		fprintf(file, "\n");
		fclose(file);
	}

	std::uintptr_t Rebase(std::uintptr_t preferred_address)
	{
		const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr));
		return base + (preferred_address - kEqImageBase);
	}

	bool InstallHook(InlineHook &hook, void *target, void *detour)
	{
		if (hook.installed) {
			return true;
		}

		hook.target = target;
		memcpy(hook.original, target, sizeof(hook.original));

		auto *gateway = static_cast<BYTE *>(VirtualAlloc(nullptr, 10, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		if (!gateway) {
			return false;
		}

		memcpy(gateway, hook.original, sizeof(hook.original));
		gateway[5] = 0xE9;
		*reinterpret_cast<std::int32_t *>(gateway + 6) = static_cast<std::int32_t>(
			(reinterpret_cast<std::uintptr_t>(target) + 5) - (reinterpret_cast<std::uintptr_t>(gateway) + 10)
		);

		DWORD old_protect = 0;
		if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old_protect)) {
			VirtualFree(gateway, 0, MEM_RELEASE);
			return false;
		}

		auto *patch = static_cast<BYTE *>(target);
		patch[0] = 0xE9;
		*reinterpret_cast<std::int32_t *>(patch + 1) = static_cast<std::int32_t>(
			reinterpret_cast<std::uintptr_t>(detour) - reinterpret_cast<std::uintptr_t>(target) - 5
		);

		VirtualProtect(target, 5, old_protect, &old_protect);
		FlushInstructionCache(GetCurrentProcess(), target, 5);

		hook.gateway = gateway;
		hook.installed = true;
		return true;
	}

	void RemoveHook(InlineHook &hook)
	{
		if (!hook.installed || !hook.target) {
			return;
		}

		DWORD old_protect = 0;
		if (VirtualProtect(hook.target, 5, PAGE_EXECUTE_READWRITE, &old_protect)) {
			memcpy(hook.target, hook.original, sizeof(hook.original));
			VirtualProtect(hook.target, 5, old_protect, &old_protect);
			FlushInstructionCache(GetCurrentProcess(), hook.target, 5);
		}

		if (hook.gateway) {
			VirtualFree(hook.gateway, 0, MEM_RELEASE);
		}

		hook = InlineHook {};
	}

	const RarityInfo &RarityForTier(int tier)
	{
		static const RarityInfo kRarities[] = {
			{0, "Common", "#F0F0F0"},
			{1, "Uncommon", "#66FF66"},
			{2, "Rare", "#66A3FF"},
			{3, "Legendary", "#FFD15C"},
			{4, "Unique", "#C080FF"}
		};

		if (tier < 0 || tier > 4) {
			return kRarities[0];
		}

		return kRarities[tier];
	}

	bool StartsWith(const char *value, const char *prefix)
	{
		return value && prefix && strncmp(value, prefix, strlen(prefix)) == 0;
	}

	bool ReadPairUInt(const char *message, const char *key, std::uint32_t &value)
	{
		const char *found = strstr(message, key);
		if (!found) {
			return false;
		}

		found += strlen(key);
		char *end = nullptr;
		const auto parsed = strtoul(found, &end, 10);
		if (end == found) {
			return false;
		}

		value = static_cast<std::uint32_t>(parsed);
		return true;
	}

	bool ParseRarityTransport(const char *message)
	{
		if (!StartsWith(message, "ITEMRARITY|")) {
			return false;
		}

		std::uint32_t item_id = 0;
		if (!ReadPairUInt(message, "item_id=", item_id) || !item_id) {
			return true;
		}

		if (StartsWith(message, "ITEMRARITY|clear|")) {
			if (g_rarity_lock_ready) {
				EnterCriticalSection(&g_rarity_lock);
				g_rarity_by_item.erase(item_id);
				LeaveCriticalSection(&g_rarity_lock);
			}
			Trace("cleared rarity cache for item %u", item_id);
			return true;
		}

		std::uint32_t tier = 0;
		if (!ReadPairUInt(message, "rarity=", tier) || tier > 4) {
			return true;
		}

		if (g_rarity_lock_ready) {
			EnterCriticalSection(&g_rarity_lock);
			g_rarity_by_item[item_id] = RarityForTier(static_cast<int>(tier));
			LeaveCriticalSection(&g_rarity_lock);
		}

		Trace("cached rarity item=%u tier=%u", item_id, tier);
		return true;
	}

#pragma pack(push, 1)
	struct ItemInfo {
		char name[0x40];
		BYTE pad_to_item_number[0xAC];
		DWORD item_number;
	};

	struct Contents {
		BYTE pad_to_item1[0x9C];
		ItemInfo *item1;
		BYTE pad_to_item2[0xA4];
		ItemInfo *item2;
	};

	struct ItemDisplayWindow {
		BYTE pad_to_display_wnd[0x21C];
		void *display_wnd;
		BYTE pad_to_item[0x8C];
		Contents *item;
	};
#pragma pack(pop)

	static_assert(offsetof(ItemInfo, item_number) == 0xEC, "ItemInfo item_number offset mismatch");
	static_assert(offsetof(Contents, item1) == 0x9C, "Contents item1 offset mismatch");
	static_assert(offsetof(Contents, item2) == 0x144, "Contents item2 offset mismatch");
	static_assert(offsetof(ItemDisplayWindow, display_wnd) == 0x21C, "ItemDisplayWindow display_wnd offset mismatch");
	static_assert(offsetof(ItemDisplayWindow, item) == 0x2AC, "ItemDisplayWindow item offset mismatch");

	ItemInfo *GetItemInfo(Contents *contents)
	{
		if (!contents) {
			return nullptr;
		}

		return contents->item1 ? contents->item1 : contents->item2;
	}

	bool LookupRarity(std::uint32_t item_id, RarityInfo &rarity)
	{
		if (!item_id || !g_rarity_lock_ready) {
			return false;
		}

		bool found = false;
		EnterCriticalSection(&g_rarity_lock);
		const auto iter = g_rarity_by_item.find(item_id);
		if (iter != g_rarity_by_item.end()) {
			rarity = iter->second;
			found = true;
		}
		LeaveCriticalSection(&g_rarity_lock);

		return found;
	}

	using DspChatProc = void(__thiscall *)(void *, const char *, DWORD, bool, bool);
	using ItemDisplayUpdateProc = void(__thiscall *)(void *);
	using AppendSTMLProc = void(__thiscall *)(void *, char *);

	void __fastcall DspChatDetour(void *self, void *, const char *message, DWORD color, bool eq_log, bool do_percent_subst)
	{
		if (ParseRarityTransport(message)) {
			return;
		}

		reinterpret_cast<DspChatProc>(g_chat_hook.gateway)(self, message, color, eq_log, do_percent_subst);
	}

	void __fastcall ItemDisplayUpdateDetour(void *self, void *)
	{
		reinterpret_cast<ItemDisplayUpdateProc>(g_item_display_hook.gateway)(self);

		auto *window = static_cast<ItemDisplayWindow *>(self);
		if (!window || !window->display_wnd) {
			return;
		}

		ItemInfo *item = GetItemInfo(window->item);
		if (!item || !item->item_number) {
			return;
		}

		RarityInfo rarity;
		if (!LookupRarity(item->item_number, rarity)) {
			return;
		}

		char stml[256] {};
		sprintf_s(
			stml,
			"<BR><c \"%s\">%s item: %s</c><BR>",
			rarity.hex,
			rarity.name,
			item->name[0] ? item->name : "Unknown Item"
		);

		const auto append_stml = reinterpret_cast<AppendSTMLProc>(Rebase(kCStmlWndAppendSTML));
		append_stml(window->display_wnd, stml);
	}

	DWORD WINAPI InitThread(LPVOID)
	{
		Sleep(1000);

		const auto chat = reinterpret_cast<void *>(Rebase(kCEverQuestDspChat));
		const auto item_display = reinterpret_cast<void *>(Rebase(kCItemDisplayWndUpdateStrings));

		const bool chat_ok = InstallHook(g_chat_hook, chat, reinterpret_cast<void *>(&DspChatDetour));
		const bool item_ok = InstallHook(g_item_display_hook, item_display, reinterpret_cast<void *>(&ItemDisplayUpdateDetour));

		Trace("item rarity native hooks installed chat=%d item_display=%d", chat_ok ? 1 : 0, item_ok ? 1 : 0);
		return 0;
	}

	void LoadRealDInput()
	{
		if (g_real_dinput8) {
			return;
		}

		char path[MAX_PATH] {};
		GetSystemDirectoryA(path, MAX_PATH);
		strcat_s(path, "\\dinput8.dll");

		g_real_dinput8 = LoadLibraryA(path);
		if (!g_real_dinput8) {
			return;
		}

		g_direct_input8_create = reinterpret_cast<DirectInput8CreateProc>(GetProcAddress(g_real_dinput8, "DirectInput8Create"));
		g_dll_can_unload_now = reinterpret_cast<DllCanUnloadNowProc>(GetProcAddress(g_real_dinput8, "DllCanUnloadNow"));
		g_dll_get_class_object = reinterpret_cast<DllGetClassObjectProc>(GetProcAddress(g_real_dinput8, "DllGetClassObject"));
		g_dll_register_server = reinterpret_cast<DllRegisterServerProc>(GetProcAddress(g_real_dinput8, "DllRegisterServer"));
		g_dll_unregister_server = reinterpret_cast<DllUnregisterServerProc>(GetProcAddress(g_real_dinput8, "DllUnregisterServer"));
		g_getdf_di_joystick = reinterpret_cast<GetdfDIJoystickProc>(GetProcAddress(g_real_dinput8, "GetdfDIJoystick"));
	}
}

BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID)
{
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(module);
			InitializeCriticalSection(&g_rarity_lock);
			g_rarity_lock_ready = true;
			LoadRealDInput();
			CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
			break;
		case DLL_PROCESS_DETACH:
			RemoveHook(g_chat_hook);
			RemoveHook(g_item_display_hook);
			if (g_rarity_lock_ready) {
				DeleteCriticalSection(&g_rarity_lock);
				g_rarity_lock_ready = false;
			}
			if (g_real_dinput8) {
				FreeLibrary(g_real_dinput8);
				g_real_dinput8 = nullptr;
			}
			break;
	}

	return TRUE;
}

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE instance, DWORD version, REFIID riid, LPVOID *out, LPUNKNOWN outer)
{
	LoadRealDInput();
	if (!g_direct_input8_create) {
		return E_FAIL;
	}

	return g_direct_input8_create(instance, version, riid, out, outer);
}

extern "C" HRESULT WINAPI DllCanUnloadNow()
{
	LoadRealDInput();
	return g_dll_can_unload_now ? g_dll_can_unload_now() : E_FAIL;
}

extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID *out)
{
	LoadRealDInput();
	return g_dll_get_class_object ? g_dll_get_class_object(clsid, riid, out) : E_FAIL;
}

extern "C" HRESULT WINAPI DllRegisterServer()
{
	LoadRealDInput();
	return g_dll_register_server ? g_dll_register_server() : E_FAIL;
}

extern "C" HRESULT WINAPI DllUnregisterServer()
{
	LoadRealDInput();
	return g_dll_unregister_server ? g_dll_unregister_server() : E_FAIL;
}

extern "C" LPCDIDATAFORMAT WINAPI GetdfDIJoystick()
{
	LoadRealDInput();
	return g_getdf_di_joystick ? g_getdf_di_joystick() : nullptr;
}

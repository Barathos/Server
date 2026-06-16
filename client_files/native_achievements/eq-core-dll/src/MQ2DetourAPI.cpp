#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x510
#define DIRECTINPUT_VERSION 0x800

#include "MQ2Main.h"

#ifndef ISXEQ
struct AchievementDetour
{
	DWORD address = 0;
	DWORD count = 0;
	unsigned char original[50] = { 0 };
	PBYTE detour = nullptr;
	PBYTE trampoline = nullptr;
	AchievementDetour* next = nullptr;
	AchievementDetour* previous = nullptr;
};

static AchievementDetour* g_detours = nullptr;
static bool g_detour_lock_initialized = false;
CRITICAL_SECTION gDetourCS;

static AchievementDetour* FindDetour(DWORD address)
{
	for (AchievementDetour* detour = g_detours; detour; detour = detour->next) {
		if (detour->address == address) {
			return detour;
		}
	}

	return nullptr;
}

BOOL AddDetour(DWORD address, PBYTE detour_callback, PBYTE trampoline, DWORD count)
{
	if (!g_detour_lock_initialized || !address || !detour_callback || !trampoline || count > sizeof(AchievementDetour::original)) {
		return FALSE;
	}

	CAutoLock lock(&gDetourCS);
	if (FindDetour(address)) {
		return FALSE;
	}

	AchievementDetour* detour = new AchievementDetour;
	detour->address = address;
	detour->count = count;
	memcpy(detour->original, reinterpret_cast<void*>(address), count);

	if (!DetourFunctionWithEmptyTrampoline(trampoline, reinterpret_cast<PBYTE>(address), detour_callback)) {
		delete detour;
		return FALSE;
	}

	detour->detour = detour_callback;
	detour->trampoline = trampoline;
	detour->next = g_detours;
	if (g_detours) {
		g_detours->previous = detour;
	}
	g_detours = detour;
	return TRUE;
}

VOID AddDetourf(DWORD address, ...)
{
	va_list args;
	va_start(args, address);
	PBYTE detour = reinterpret_cast<PBYTE>(va_arg(args, DWORD));
	PBYTE trampoline = reinterpret_cast<PBYTE>(va_arg(args, DWORD));
	va_end(args);

	AddDetour(address, detour, trampoline, 20);
}

VOID RemoveDetour(DWORD address)
{
	if (!g_detour_lock_initialized) {
		return;
	}

	CAutoLock lock(&gDetourCS);
	AchievementDetour* detour = FindDetour(address);
	if (!detour) {
		return;
	}

	DetourRemove(detour->trampoline, detour->detour);

	if (detour->previous) {
		detour->previous->next = detour->next;
	}
	else {
		g_detours = detour->next;
	}

	if (detour->next) {
		detour->next->previous = detour->previous;
	}

	delete detour;
}

static void RemoveAllDetours()
{
	while (g_detours) {
		RemoveDetour(g_detours->address);
	}
}

VOID InitializeMQ2Detours()
{
	if (g_detour_lock_initialized) {
		return;
	}

	InitializeCriticalSection(&gDetourCS);
	g_detour_lock_initialized = true;
}

VOID ShutdownMQ2Detours()
{
	if (!g_detour_lock_initialized) {
		return;
	}

	RemoveAllDetours();
	DeleteCriticalSection(&gDetourCS);
	g_detour_lock_initialized = false;
}
#endif

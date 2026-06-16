#pragma once

#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x510
#endif
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x800
#endif

#pragma warning(disable:4530)
#pragma warning(disable:4786)

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <winsock.h>
#include <math.h>
#include <map>
#include <string>
#include <algorithm>
#include <chrono>
#include <dinput.h>
#include <detours.h>

using namespace std;

#define PLUGIN_API

#ifdef ISXEQ_LEGACY
#define LEGACY_API extern
#define LEGACY_VAR extern
#else
#define LEGACY_API EQLIB_API
#define LEGACY_VAR EQLIB_VAR
#endif

#ifdef EQLIB_EXPORTS
#define EQLIB_API extern "C" __declspec(dllexport)
#define EQLIB_VAR extern "C" __declspec(dllexport)
#define EQLIB_OBJECT __declspec(dllexport)
#else
#define EQLIB_API extern "C" __declspec(dllimport)
#define EQLIB_VAR extern "C" __declspec(dllimport)
#define EQLIB_OBJECT __declspec(dllimport)
#endif

#ifndef ISXEQ
#define FUNCTION_AT_ADDRESS(function, offset) __declspec(naked) function\
{\
	__asm{mov eax, offset};\
	__asm{jmp eax};\
}

#define FUNCTION_AT_VARIABLE_ADDRESS(function, variable) __declspec(naked) function\
{\
	__asm{mov eax, [variable]};\
	__asm{jmp eax};\
}

#define FUNCTION_AT_VIRTUAL_ADDRESS(function, virtualoffset) __declspec(naked) function\
{\
	__asm{mov eax, [ecx]};\
	__asm{lea eax, [eax + virtualoffset]};\
	__asm{mov eax, [eax]};\
	__asm{jmp eax};\
}
#endif

#define SetWndNotification(thisclass) \
{\
	int (thisclass::*pfWndNotification)(CXWnd *pWnd, unsigned int Message, void *unknown)=&thisclass::WndNotification;\
	SetvfTable(34,*(DWORD*)&pfWndNotification);\
}

#define MAX_STRING 2048

#ifndef DOUBLE
typedef double DOUBLE;
#endif

#define XWM_LCLICK 1
#define XWM_CLOSE 10

#include "EQData.h"
#include "EQUIStructs.h"
#include "EQClasses.h"
#include "eqgame.h"
#include "MQ2Prototypes.h"
#include "MQ2Internal.h"
#include "MQ2Globals.h"

EQLIB_API VOID InitializeMQ2Detours();
EQLIB_API VOID ShutdownMQ2Detours();

#ifndef ISXEQ
#ifdef ISXEQ_LEGACY
#define RemoveDetour(address)
#define AddDetour __noop
#define AddDetourf __noop
#define EzDetour(offset, detour, trampoline)
#define EzDetourwName(offset, detour, trampoline, name)
#else
EQLIB_API BOOL AddDetour(DWORD address, PBYTE detour = nullptr, PBYTE trampoline = nullptr, DWORD count = 20);
EQLIB_API VOID AddDetourf(DWORD address, ...);
EQLIB_API VOID RemoveDetour(DWORD address);
#define EzDetour(offset, detour, trampoline) AddDetourf((DWORD)offset, detour, trampoline)
#define EzDetourwName(offset, detour, trampoline, name) AddDetourf((DWORD)offset, detour, trampoline)
#endif
#else
#define RemoveDetour EzUnDetour
#endif

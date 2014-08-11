// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "GkosWinHooks.h"
#include <cassert>

#pragma data_seg(".shared")
HHOOK    gs_gkosKeyboardHook     = NULL;
int      gs_processCount         = 0;
unsigned gs_gkosKeyboardKeyState = 0;
#pragma data_seg()

HINSTANCE g_hInstance = NULL;

struct GkosKeyboardKey {
    unsigned vkey;
    unsigned gkosKeyNum;
};

static const GkosKeyboardKey g_gkosKeyboardKeys[] = {
    { VK_F7, 3 }, // c 0x43
    { VK_F8, 6 }, // r 0x52
};

//============================================================================
extern "C" {

__declspec(dllexport) bool IsGkosKeyboardKeyPressed (unsigned gkosKeyNum) {
#pragma warning(suppress: 4800)
    return gs_gkosKeyboardKeyState & (1 << gkosKeyNum);
}

}

//============================================================================
__declspec(dllexport) LRESULT CALLBACK GkosKeyboardHookProc (
    int    code,
    WPARAM wParam,
    LPARAM lParam
) {

    bool processedKey = false;

    // Assert if F7 is being released
    const unsigned gkosKeyboardKeyCount = sizeof(g_gkosKeyboardKeys) / sizeof(g_gkosKeyboardKeys[0]);
    for (unsigned i = 0; i < gkosKeyboardKeyCount; ++i) {
        const GkosKeyboardKey & kbdKey = g_gkosKeyboardKeys[i];
        if (wParam == kbdKey.vkey) {
            if (lParam & (1 << 31)) // Key is being released
                gs_gkosKeyboardKeyState &= ~(1 << kbdKey.gkosKeyNum);
            else
                gs_gkosKeyboardKeyState |=  (1 << kbdKey.gkosKeyNum);
            processedKey = true;
        }
    }

    // If incoming code is negative, Windows requires we call the next hook.
    // if processed, return non-zero to prevent passing key along.
    if (processedKey && code >= 0)
        return 1;

    return CallNextHookEx(gs_gkosKeyboardHook, code, wParam, lParam);

}

//============================================================================
__declspec(dllexport) bool GkosRegisterHooks () {

    gs_gkosKeyboardHook = SetWindowsHookEx(
        WH_KEYBOARD,
        &GkosKeyboardHookProc,
        g_hInstance,
        0
    );
    // GetLastError()
    assert(gs_gkosKeyboardHook);

#pragma warning(suppress: 4800)
    return bool(gs_gkosKeyboardHook);

}

//============================================================================
__declspec(dllexport) BOOL GkosReleaseHooks () {

    if (gs_gkosKeyboardHook)
        return UnhookWindowsHookEx(gs_gkosKeyboardHook);

    return TRUE;

}

//============================================================================
BOOL APIENTRY DllMain (
    HMODULE hModule,
    DWORD   ul_reason_for_call,
    LPVOID  lpReserved
) {

	switch (ul_reason_for_call) {
    	case DLL_PROCESS_ATTACH: {
            if (gs_processCount == 0) { // First loading time.  Set hook(s).
                g_hInstance = hModule;
                GkosRegisterHooks();
            }
            ++gs_processCount;
        } break;

    	case DLL_THREAD_ATTACH:
    	case DLL_THREAD_DETACH:
            break;

    	case DLL_PROCESS_DETACH: {
            if (gs_processCount == 1) { // Last program detaching.  Unhook.
                GkosReleaseHooks();
            }
            --gs_processCount;
        } break;
	}

	return TRUE;

}

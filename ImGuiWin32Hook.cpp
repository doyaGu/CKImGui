#include <cstring>
#include <cassert>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "imgui.h"

#include <MinHook.h>

typedef BOOL (WINAPI *LPFNPEEKMESSAGEA)(LPMSG, HWND, UINT, UINT, UINT);
typedef BOOL (WINAPI *LPFNGETMESSAGEA)(LPMSG, HWND, UINT, UINT);

namespace {
    HMODULE g_hModule;
    LPFNPEEKMESSAGEA lpfnPeekMessageA;
    LPFNGETMESSAGEA lpfnGetMessageA;
    char g_CompositeChar[2];
    int g_CompositeCharCount;
    int g_CompositeCharIndex;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern "C" BOOL WINAPI HookPeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
    if (!lpfnPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg))
        return FALSE;

    assert(lpMsg != nullptr);

    if (lpMsg->hwnd != nullptr && (wRemoveMsg & PM_REMOVE) != 0) {
        switch (lpMsg->message) {
            case WM_CHAR: {
                ImGuiIO &io = ImGui::GetIO();
                ImGui::GetIO();
                if (g_CompositeCharCount == 0) {
                    if (lpMsg->wParam < 127) {
                        io.AddInputCharacter(lpMsg->wParam);
                        return 0;
                    } else {
                        g_CompositeCharCount = 2;
                        g_CompositeCharIndex = 0;
                    }
                }

                if (g_CompositeCharCount > 0) {
                    g_CompositeChar[g_CompositeCharIndex++] = (char) lpMsg->wParam;
                    --g_CompositeCharCount;
                }

                if (g_CompositeCharCount == 0) {
                    wchar_t cc = *(wchar_t *) g_CompositeChar;
                    wchar_t wch = 0;
                    ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *) &cc, 2, &wch, 1);
                    io.AddInputCharacter(wch);
                    memset(g_CompositeChar, 0, sizeof(g_CompositeChar));
                }
            }
                break;
            default:
                if (ImGui_ImplWin32_WndProcHandler(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam)) {
                    // We still want 'WM_CHAR' messages, so translate message
                    TranslateMessage(lpMsg);
                    // Change message so it is ignored by the recipient window
                    lpMsg->message = WM_NULL;
                }
                break;
        }
    }

    return TRUE;
}

extern "C" BOOL WINAPI HookGetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
    // Implement 'GetMessage' with a timeout
    while (!PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE) && g_hModule != nullptr)
        MsgWaitForMultipleObjects(0, nullptr, FALSE, 500, QS_ALLINPUT);

    if (g_hModule == nullptr && lpMsg->message != WM_QUIT)
        memset(lpMsg, 0, sizeof(MSG)); // Clear message structure, so application does not process it

    return lpMsg->message != WM_QUIT;
}

bool ImGuiWin32InstallHooks() {
    if (MH_Initialize() != MH_OK)
        return false;
    if (MH_CreateHookApi(L"user32", "PeekMessageA", (LPVOID) &HookPeekMessageA, (LPVOID *) &lpfnPeekMessageA) != MH_OK ||
        MH_EnableHook((LPVOID) &PeekMessageA) != MH_OK) {
        return false;
    }
    if (MH_CreateHookApi(L"user32", "GetMessageA", (LPVOID) &HookGetMessageA, (LPVOID *) &lpfnGetMessageA) != MH_OK ||
        MH_EnableHook((LPVOID) &GetMessageA) != MH_OK) {
        return false;
    }
    return true;
}

bool ImGuiWin32UninstallHooks() {
    if (MH_DisableHook((LPVOID) &PeekMessageA) != MH_OK)
        return false;
    if (MH_DisableHook((LPVOID) &GetMessageA) != MH_OK)
        return false;
    if (MH_Uninitialize() != MH_OK)
        return false;
    return true;
}
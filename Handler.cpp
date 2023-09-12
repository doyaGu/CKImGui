#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include "CKContext.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "imgui_impl_ck2.h"

#include "Hooks.h"

typedef LRESULT (CALLBACK *LPFNWNDPROC)(HWND, UINT, WPARAM, LPARAM);

static HWND g_WndHandle = nullptr;
static LPFNWNDPROC g_WndProc;
CKContext *g_CKContext = nullptr;
static bool g_IsReady = false;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 1;
    return g_WndProc(hWnd, msg, wParam, lParam);
}

static void HookWndProc() {
    g_WndProc = reinterpret_cast<LPFNWNDPROC>(GetWindowLongPtr(g_WndHandle, GWLP_WNDPROC));
    SetWindowLongPtr(g_WndHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));
}

static void UnhookWndProc() {
    if (g_WndProc)
        SetWindowLongPtr(g_WndHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_WndProc));
}

CKERROR OnCKInit(void *arg) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = "ImGui.ini";
    io.LogFilename = "ImGui.log";

    return CK_OK;
}

CKERROR OnCKEnd(void *arg) {
    ImGui::DestroyContext();

    return CK_OK;
}

CKERROR OnCKPostReset(void *arg) {
    if (!g_IsReady) {

        if (!ImGui_ImplWin32_Init(g_WndHandle)) {
            return CKERR_INVALIDPARAMETER;
        }

        if (!ImGui_ImplCK2_Init(g_CKContext)) {
            return CKERR_INVALIDPARAMETER;
        }

        HookWndProc();

        g_IsReady = true;
    }
    return CK_OK;
}

CKERROR PreClearAll(void *arg) {
    if (g_IsReady) {
        UnhookWndProc();

        ImGui_ImplCK2_Shutdown();
        ImGui_ImplWin32_Shutdown();


        g_WndHandle = nullptr;
        g_IsReady = false;
    }
    return CK_OK;
}

CKERROR PreProcess(void *arg) {
    if (g_IsReady) {
        ImGui_ImplCK2_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    return CK_OK;
}

CKERROR OnPostSpriteRender(CKRenderContext *dev, void *arg) {
    if (g_IsReady) {
        ImGui::Render();
        ImGui_ImplCK2_RenderDrawData(ImGui::GetDrawData());
    }

    return CK_OK;
}

static int OnError(HookModuleErrorCode code, void *data1, void *data2) {
    return HMR_OK;
}

static int OnQuery(HookModuleQueryCode code, void *data1, void *data2) {
    switch (code) {
        case HMQC_ABI:
            *reinterpret_cast<int *>(data2) = HOOKS_ABI_VERSION;
            break;
        case HMQC_CODE:
            *reinterpret_cast<int *>(data2) = 1000;
            break;
        case HMQC_VERSION:
            *reinterpret_cast<int *>(data2) = 1;
            break;
        case HMQC_CK2:
            *reinterpret_cast<int *>(data2) =
                CKHF_OnCKInit |
                CKHF_OnCKEnd |
                CKHF_OnCKPostReset |
                CKHF_PreClearAll |
                CKHF_PreProcess |
                CKHF_OnPostSpriteRender;
            break;
        default:
            return HMR_SKIP;
    }

    return HMR_OK;
}

static int OnPost(HookModulePostCode code, void *data1, void *data2) {
    switch (code) {
        case HMPC_CKCONTEXT:
            g_CKContext = reinterpret_cast<CKContext *>(data2);
            break;
        case HMPC_WINDOW:
            g_WndHandle = static_cast<HWND>(data2);
            break;
        default:
            return HMR_SKIP;
    }
    return HMR_OK;
}

static int OnLoad(size_t code, void * /* handle */) {
    return HMR_OK;
}

static int OnUnload(size_t code, void * /* handle */) {
    return HMR_OK;
}

static int OnSet(size_t code, void **pcb, void **parg) {
    switch (code) {
        case CKHFI_OnCKInit:
            *pcb = reinterpret_cast<void *>(OnCKInit);
            *parg = nullptr;
            break;
        case CKHFI_OnCKEnd:
            *pcb = reinterpret_cast<void *>(OnCKEnd);
            *parg = nullptr;
            break;
        case CKHFI_OnCKPostReset:
            *pcb = reinterpret_cast<void *>(OnCKPostReset);
            *parg = nullptr;
            break;
        case CKHFI_PreClearAll:
            *pcb = reinterpret_cast<void *>(PreClearAll);
            *parg = nullptr;
            break;
        case CKHFI_PreProcess:
            *pcb = reinterpret_cast<void *>(PreProcess);
            *parg = nullptr;
            break;
        case CKHFI_OnPostSpriteRender:
            *pcb = reinterpret_cast<void *>(OnPostSpriteRender);
            *parg = nullptr;
            break;
        default:
            break;
    }
    return HMR_OK;
}

static int OnUnset(size_t code, void **pcb, void **parg) {
    switch (code) {
        case CKHFI_OnCKInit:
            *pcb = reinterpret_cast<void *>(OnCKInit);
            *parg = nullptr;
            break;
        case CKHFI_OnCKEnd:
            *pcb = reinterpret_cast<void *>(OnCKEnd);
            *parg = nullptr;
            break;
        case CKHFI_OnCKPostReset:
            *pcb = reinterpret_cast<void *>(OnCKPostReset);
            *parg = nullptr;
            break;
        case CKHFI_PreClearAll:
            *pcb = reinterpret_cast<void *>(PreClearAll);
            *parg = nullptr;
            break;
        case CKHFI_PreProcess:
            *pcb = reinterpret_cast<void *>(PreProcess);
            *parg = nullptr;
            break;
        case CKHFI_OnPostSpriteRender:
            *pcb = reinterpret_cast<void *>(OnPostSpriteRender);
            *parg = nullptr;
            break;
        default:
            break;
    }
    return HMR_OK;
}

HOOKS_EXPORT int Handler(size_t action, size_t code, uintptr_t param1, uintptr_t param2) {
    switch (action) {
        case HMA_ERROR:
            return OnError(static_cast<HookModuleErrorCode>(code), reinterpret_cast<void *>(param1), reinterpret_cast<void *>(param2));
        case HMA_QUERY:
            return OnQuery(static_cast<HookModuleQueryCode>(code), reinterpret_cast<void *>(param1), reinterpret_cast<void *>(param2));
        case HMA_POST:
            return OnPost(static_cast<HookModulePostCode>(code), reinterpret_cast<void *>(param1), reinterpret_cast<void *>(param2));
        case HMA_LOAD:
            return OnLoad(code, reinterpret_cast<void *>(param2));
        case HMA_UNLOAD:
            return OnUnload(code, reinterpret_cast<void *>(param2));
        case HMA_SET:
            return OnSet(code, reinterpret_cast<void **>(param1), reinterpret_cast<void **>(param2));
        case HMA_UNSET:
            return OnUnset(code, reinterpret_cast<void **>(param1), reinterpret_cast<void **>(param2));
        default:
            return HMR_SKIP;
    }
}
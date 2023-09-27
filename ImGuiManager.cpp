#include "ImGuiManager.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include "imgui_impl_ck2.h"
#include "backends/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef LRESULT (CALLBACK *LPFNWNDPROC)(HWND, UINT, WPARAM, LPARAM);

static LPFNWNDPROC g_WndProc;

static LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 1;
    return g_WndProc(hWnd, msg, wParam, lParam);
}

static void HookWndProc(HWND hwnd) {
    g_WndProc = reinterpret_cast<LPFNWNDPROC>(GetWindowLongPtr(hwnd, GWLP_WNDPROC));
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));
}

static void UnhookWndProc(HWND hwnd) {
    if (g_WndProc)
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_WndProc));
}

ImGuiManager::ImGuiManager(CKContext *context) : CKBaseManager(context, IMGUI_MANAGER_GUID, "ImGui Manager") {
    context->RegisterNewManager(this);
}

CKERROR ImGuiManager::OnCKInit() {
    if (!m_Created) {
        HookWndProc((HWND) m_Context->GetMainWindow());

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        m_Created = true;
    }
    return CK_OK;
}

CKERROR ImGuiManager::OnCKEnd() {
    if (m_Created) {
        ImGui::DestroyContext();

        UnhookWndProc((HWND) m_Context->GetMainWindow());

        m_Created = false;
    }

    return CK_OK;
}

CKERROR ImGuiManager::PreClearAll() {
    if (m_Initialized) {
        ImGui_ImplWin32_Shutdown();
        ImGui_ImplCK2_Shutdown();

        m_Initialized = false;
    }

    return CK_OK;
}

CKERROR ImGuiManager::OnCKPostReset() {
    if (!m_Initialized) {
        ImGui_ImplCK2_Init(m_Context);
        ImGui_ImplWin32_Init(m_Context->GetMainWindow());

        m_Initialized = true;
    }

    return CK_OK;
}

CKERROR ImGuiManager::PreProcess() {
    if (m_Initialized) {
        ImGui_ImplCK2_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    return CK_OK;
}

CKERROR ImGuiManager::OnPostSpriteRender(CKRenderContext *dev) {
    if (m_Initialized) {
        ImGui::Render();
        ImGui_ImplCK2_RenderDrawData(ImGui::GetDrawData());
    }

    return CK_OK;
}

CKDWORD ImGuiManager::GetValidFunctionsMask() {
    return CKMANAGER_FUNC_OnCKInit |
           CKMANAGER_FUNC_OnCKEnd |
           CKMANAGER_FUNC_PreClearAll |
           CKMANAGER_FUNC_OnCKPostReset |
           CKMANAGER_FUNC_PreProcess |
           CKMANAGER_FUNC_OnPostSpriteRender;
}
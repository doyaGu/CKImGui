#include "ImGuiManager.h"

#include "CKRenderContext.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include "imgui_impl_ck2.h"
#include "backends/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef LRESULT (CALLBACK *LPFNWNDPROC)(HWND, UINT, WPARAM, LPARAM);

static LPFNWNDPROC g_MainWndProc = nullptr;
static LPFNWNDPROC g_RenderWndProc = nullptr;

static LRESULT MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 1;
    return g_MainWndProc(hWnd, msg, wParam, lParam);
}

static LRESULT RenderWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 1;
    return g_RenderWndProc(hWnd, msg, wParam, lParam);
}

static void HookWndProc(HWND hMainWnd, HWND hRenderWnd) {
    g_MainWndProc = reinterpret_cast<LPFNWNDPROC>(GetWindowLongPtr(hMainWnd, GWLP_WNDPROC));
    SetWindowLongPtr(hMainWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(MainWndProc));
    if (hMainWnd != hRenderWnd) {
        g_RenderWndProc = reinterpret_cast<LPFNWNDPROC>(GetWindowLongPtr(hRenderWnd, GWLP_WNDPROC));
        SetWindowLongPtr(hRenderWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(RenderWndProc));
    }
}

static void UnhookWndProc(HWND hMainWnd, HWND hRenderWnd) {
    if (g_MainWndProc)
        SetWindowLongPtr(hMainWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_MainWndProc));
    if (g_RenderWndProc)
        SetWindowLongPtr(hRenderWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_RenderWndProc));
}

ImGuiManager::ImGuiManager(CKContext *context) : CKBaseManager(context, IMGUI_MANAGER_GUID, "ImGui Manager") {
    context->RegisterNewManager(this);
}

CKERROR ImGuiManager::OnCKInit() {
    if (!m_Created) {
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

        m_Created = false;
    }

    return CK_OK;
}

CKERROR ImGuiManager::PreClearAll() {
    if (m_Initialized) {
        ImGui_ImplWin32_Shutdown();
        ImGui_ImplCK2_Shutdown();

        UnhookWndProc((HWND) m_Context->GetMainWindow(), (HWND) m_Context->GetPlayerRenderContext()->GetWindowHandle());

        m_Render = false;
        m_Initialized = false;
    }

    return CK_OK;
}

CKERROR ImGuiManager::OnCKPostReset() {
    if (!m_Initialized) {
        HookWndProc((HWND) m_Context->GetMainWindow(), (HWND) m_Context->GetPlayerRenderContext()->GetWindowHandle());

        ImGui_ImplCK2_Init(m_Context);
        ImGui_ImplWin32_Init(m_Context->GetMainWindow());

        m_Initialized = true;
        m_Render = true;
    }

    return CK_OK;
}

CKERROR ImGuiManager::OnPreRender(CKRenderContext *dev) {
    if (m_Render) {
        ImGui_ImplCK2_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    return CK_OK;
}

CKERROR ImGuiManager::OnPostSpriteRender(CKRenderContext *dev) {
    if (m_Render) {
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
           CKMANAGER_FUNC_OnPreRender |
           CKMANAGER_FUNC_OnPostSpriteRender;
}
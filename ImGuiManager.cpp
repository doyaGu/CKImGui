#include "ImGuiManager.h"

#include "imgui_impl_ck2.h"
#include "backends/imgui_impl_win32.h"

#ifndef IMGUI_IMPLEMENTATION
#define IMGUI_IMPLEMENTATION
#endif
#include "ImGuiOverlay.h"

extern bool ImGuiWin32InstallHooks();
extern bool ImGuiWin32UninstallHooks();

ImGuiManager::ImGuiManager(CKContext *context) : CKBaseManager(context, IMGUI_MANAGER_GUID, "ImGui Manager") {
    context->RegisterNewManager(this);
}

CKERROR ImGuiManager::OnCKInit() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    return CK_OK;
}

CKERROR ImGuiManager::OnCKEnd() {
    if (m_Initialized) {
        if (!ImGuiWin32UninstallHooks())
            return CKERR_INVALIDPARAMETER;

        ImGui_ImplWin32_Shutdown();
        ImGui_ImplCK2_Shutdown();
        ImGui::DestroyContext();
        m_Initialized = false;
    }

    return CK_OK;
}

CKERROR ImGuiManager::OnCKPostReset() {
    if (!m_Initialized) {
        ImGui_ImplCK2_Init(m_Context);
        ImGui_ImplWin32_Init(m_Context->GetMainWindow());

        if (!ImGuiWin32InstallHooks())
            return CKERR_INVALIDPARAMETER;

        m_Show = true;
        m_Initialized = true;
    }

    return CK_OK;
}

CKERROR ImGuiManager::OnPostRender(CKRenderContext *dev) {
    if (m_Show) {
        ImGui_ImplCK2_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    return CK_OK;
}

CKERROR ImGuiManager::OnPostSpriteRender(CKRenderContext *dev) {
    if (m_Show) {
        ImGui::Render();
        ImGui_ImplCK2_RenderDrawData(ImGui::GetDrawData());
    }

    return CK_OK;
}

int ImGuiManager::GetFunctionPriority(CKMANAGER_FUNCTIONS Function) {
    switch (Function) {
        case CKMANAGER_FUNC_OnPostRender:
            return 1000;
        case CKMANAGER_FUNC_OnPostSpriteRender:
            return -1000;
        default:
            break;
    }
    return 0;
}

CKDWORD ImGuiManager::GetValidFunctionsMask() {
    return CKMANAGER_FUNC_OnCKInit |
        CKMANAGER_FUNC_OnCKEnd |
        CKMANAGER_FUNC_OnCKPostReset |
        CKMANAGER_FUNC_OnPostRender |
        CKMANAGER_FUNC_OnPostSpriteRender;
}

void ImGuiManager::Show(bool show) {
    m_Show = show;
}

extern "C" __declspec(dllexport) const ImGuiFunctionTable *GetImGuiFunctionTable(unsigned int version);

const ImGuiFunctionTable *ImGuiManager::GetImGuiFunctionTable(unsigned int version, unsigned int reserved) {
    return ::GetImGuiFunctionTable(version);
}

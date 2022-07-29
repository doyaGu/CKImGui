#include "ImGuiManager.h"
#include "CKContext.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_ck2.h"

#ifdef CK_LIB
#define CreateNewManager				CreateNewImGuiManager
#define RemoveManager					RemoveImGuiManager
#define CKGetPluginInfoCount			CKGet_ImGuiManager_PluginInfoCount
#define CKGetPluginInfo					CKGet_ImGuiManager_PluginInfo
#define g_PluginInfo					g_ImGuiManager_PluginInfo
#else
#define CreateNewManager                CreateNewManager
#define RemoveManager                   RemoveManager
#define CKGetPluginInfoCount            CKGetPluginInfoCount
#define CKGetPluginInfo                 CKGetPluginInfo
#define g_PluginInfo                    g_PluginInfo
#endif

CKERROR CreateNewManager(CKContext *context) {
    new ImGuiManager(context);

    return CK_OK;
}

CKERROR RemoveManager(CKContext *context) {
    ImGuiManager *man = (ImGuiManager *) context->GetManagerByGuid(IMGUI_MANAGER_GUID);
    delete man;

    return CK_OK;
}

CKPluginInfo g_PluginInfo;

PLUGIN_EXPORT int CKGetPluginInfoCount() { return 1; }

PLUGIN_EXPORT CKPluginInfo *CKGetPluginInfo(int index) {
    g_PluginInfo.m_Author = "Kakuty";
    g_PluginInfo.m_Description = "ImGui Manager";
    g_PluginInfo.m_Extension = "";
    g_PluginInfo.m_Type = CKPLUGIN_MANAGER_DLL;
    g_PluginInfo.m_Version = 0x000001;
    g_PluginInfo.m_InitInstanceFct = CreateNewManager;
    g_PluginInfo.m_ExitInstanceFct = RemoveManager;
    g_PluginInfo.m_GUID = IMGUI_MANAGER_GUID;
    g_PluginInfo.m_Summary = "ImGui Manager";
    return &g_PluginInfo;
}

static std::vector<std::function<void()>> g_Callbacks;

static void Render() {
    ImGui_ImplCK2_NewFrame();
    ImGui::NewFrame();

    for (auto &cb: g_Callbacks) {
        cb();
    }

    ImGui::Render();
    ImGui_ImplCK2_RenderDrawData(ImGui::GetDrawData());
}

const char *ImGuiManager::Name = "ImGui Manager";

ImGuiManager::ImGuiManager(CKContext *context) : CKBaseManager(context, IMGUI_MANAGER_GUID, (char *) Name) {
    context->RegisterNewManager(this);
}

CKERROR ImGuiManager::OnCKInit() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void) io;
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/simhei.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());

    return CK_OK;
}

CKERROR ImGuiManager::OnCKEnd() {
    if (m_Initialized) {
        ImGui_ImplCK2_Shutdown();
        ImGui::DestroyContext();
        m_Initialized = false;
    }

    return CK_OK;
}

CKERROR ImGuiManager::OnCKPostReset() {
    if (!m_Initialized) {
        ImGui_ImplCK2_Init(m_Context);
        m_Initialized = true;
    }

    return CK_OK;
}

CKERROR ImGuiManager::PreProcess() {
    if (m_Show)
        ImGui_ImplCK2_ProcessInput();

    return CK_OK;
}

CKERROR ImGuiManager::OnPostRender(CKRenderContext *dev) {
    if (m_Show && !m_DrawOnTopMost)
        Render();

    return CK_OK;
}

CKERROR ImGuiManager::OnPostSpriteRender(CKRenderContext *dev) {
    if (m_Show && m_DrawOnTopMost)
        Render();

    return CK_OK;
}

void ImGuiManager::AddToFrame(std::function<void()> callback) {
    g_Callbacks.push_back(std::move(callback));
}

bool ImGuiManager::RemoveFromFrame(std::function<void()> &callback) {
    return std::remove_if(g_Callbacks.begin(), g_Callbacks.end(), [callback](std::function<void()> &cb) {
        return cb.target<std::function<void()>>() == callback.target<std::function<void()>>();
    }) != g_Callbacks.end();
}
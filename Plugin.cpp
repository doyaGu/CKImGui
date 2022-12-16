#include "CKAll.h"

#include "ImGuiManager.h"

#ifdef CK_LIB
#define CreateNewManager        CreateNewImGuiManager
#define RemoveManager           RemoveImGuiManager
#define CKGetPluginInfoCount    CKGet_ImGuiManager_PluginInfoCount
#define CKGetPluginInfo         CKGet_ImGuiManager_PluginInfo
#define g_PluginInfo            g_ImGuiManager_PluginInfo
#else
#define CreateNewManager        CreateNewManager
#define RemoveManager           RemoveManager
#define CKGetPluginInfoCount    CKGetPluginInfoCount
#define CKGetPluginInfo         CKGetPluginInfo
#define g_PluginInfo            g_PluginInfo
#endif

CKERROR CreateNewImGuiManager(CKContext* context) {
    new ImGuiManager(context);
    return CK_OK;
}

CKERROR RemoveImGuiManager(CKContext* context) {
    ImGuiManager* man = ImGuiManager::GetManager(context);
    delete man;
    return CK_OK;
}

CKPluginInfo g_PluginInfo;

PLUGIN_EXPORT int CKGetPluginInfoCount() { return 1; }

PLUGIN_EXPORT CKPluginInfo *CKGetPluginInfo(int Index) {
    g_PluginInfo.m_Author = "Kakuty";
    g_PluginInfo.m_Description = "ImGui Manager";
    g_PluginInfo.m_Extension = "";
    g_PluginInfo.m_Type = CKPLUGIN_MANAGER_DLL;
    g_PluginInfo.m_Version = 0x000001;
    g_PluginInfo.m_InitInstanceFct = CreateNewImGuiManager;
    g_PluginInfo.m_ExitInstanceFct = RemoveImGuiManager;
    g_PluginInfo.m_GUID = IMGUI_MANAGER_GUID;
    g_PluginInfo.m_Summary = "ImGui Manager";
    return &g_PluginInfo;
}
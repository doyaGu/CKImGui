#include "CKImGui.h"

#include <utility>

#include "CKContext.h"
#include "ImGuiManager.h"

static ImGuiManager *GetImGuiManager(CKContext *context) {
    if (!context) return nullptr;
    return (ImGuiManager *) context->GetManagerByGuid(IMGUI_MANAGER_GUID);
}

bool ImGuiManagerIsInitialized(CKContext *context) {
    ImGuiManager *man = GetImGuiManager(context);
    if (man) return man->IsInitialized();
    return false;
}

void ImGuiManagerShow(CKContext *context, bool show) {
    ImGuiManager *man = GetImGuiManager(context);
    if (man) man->Show(show);
}

void ImGuiManagerDrawOnTopMost(CKContext *context, bool topmost) {
    ImGuiManager *man = GetImGuiManager(context);
    if (man) man->DrawOnTopMost(topmost);
}

void ImGuiManagerAddToFrame(CKContext *context, std::function<void()> callback) {
    ImGuiManager *man = GetImGuiManager(context);
    if (man) man->AddToFrame(std::move(callback));
}

bool ImGuiManagerRemoveFromFrame(CKContext *context, std::function<void()> &callback) {
    ImGuiManager *man = GetImGuiManager(context);
    if (man) return man->RemoveFromFrame(callback);
    return false;
}
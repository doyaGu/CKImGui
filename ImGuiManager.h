#ifndef NEMO2_IMGUIMANAGER_H
#define NEMO2_IMGUIMANAGER_H

#include <functional>

#include "CKBaseManager.h"

#define IMGUI_MANAGER_GUID CKGUID(0x19E7A87, 0x95E7972)

class ImGuiManager final : public CKBaseManager {
public :
    explicit ImGuiManager(CKContext *context);

    // Initialization
    CKERROR OnCKInit() override;
    CKERROR OnCKEnd() override;

    CKERROR OnCKPostReset() override;
    CKERROR PreProcess() override;

    CKERROR OnPostRender(CKRenderContext *dev) override;
    CKERROR OnPostSpriteRender(CKRenderContext *dev) override;

    CKDWORD GetValidFunctionsMask() override {
        return CKMANAGER_FUNC_OnCKInit |
               CKMANAGER_FUNC_OnCKEnd |
               CKMANAGER_FUNC_OnCKPostReset |
               CKMANAGER_FUNC_PreProcess |
               CKMANAGER_FUNC_OnPostRender |
               CKMANAGER_FUNC_OnPostSpriteRender;
    }

    bool IsInitialized() const {
        return m_Initialized;
    }

    void Show(bool show = true) {
        m_Show = show;
    }

    void DrawOnTopMost(bool topmost) {
        m_DrawOnTopMost = topmost;
    }

    virtual void AddToFrame(std::function<void()> callback);
    virtual bool RemoveFromFrame(std::function<void()> &callback);

    static const char *Name;

private:
    bool m_Initialized = false;
    bool m_Show = true;
    bool m_DrawOnTopMost = true;
};

#endif // NEMO2_IMGUIMANAGER_H

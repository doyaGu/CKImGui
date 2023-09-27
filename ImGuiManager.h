#ifndef IMGUIMANAGER_H
#define IMGUIMANAGER_H

#include "CKBaseManager.h"
#include "CKContext.h"

#define IMGUI_MANAGER_GUID CKGUID(0x19E7A87, 0x95E7972)

class ImGuiManager : public CKBaseManager {
public :
    explicit ImGuiManager(CKContext *context);

    CKERROR OnCKInit() override;
    CKERROR OnCKEnd() override;

    CKERROR PreClearAll() override;
    CKERROR OnCKPostReset() override;

    CKERROR PreProcess() override;
    CKERROR OnPostSpriteRender(CKRenderContext *dev) override;

    CKDWORD GetValidFunctionsMask() override;

    static ImGuiManager *GetManager(CKContext *context) {
        return (ImGuiManager *)context->GetManagerByGuid(IMGUI_MANAGER_GUID);
    }

private:
    bool m_Created = false;
    bool m_Initialized = false;
};

#endif // IMGUIMANAGER_H

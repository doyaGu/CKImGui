#ifndef IMGUIMANAGER_H
#define IMGUIMANAGER_H

#include "CKBaseManager.h"
#include "CKContext.h"

#define IMGUI_MANAGER_GUID CKGUID(0x19E7A87, 0x95E7972)

struct ImGuiFunctionTable;

class ImGuiManager final : public CKBaseManager {
public :
    explicit ImGuiManager(CKContext *context);

    // Initialization
    CKERROR OnCKInit() override;
    CKERROR OnCKEnd() override;

    CKERROR OnCKPostReset() override;

    CKERROR OnPostRender(CKRenderContext *dev) override;
    CKERROR OnPostSpriteRender(CKRenderContext *dev) override;

    int GetFunctionPriority(CKMANAGER_FUNCTIONS Function) override;

    CKDWORD GetValidFunctionsMask() override;

    virtual void Show(bool show = true);

    virtual const ImGuiFunctionTable *GetImGuiFunctionTable(unsigned int version, unsigned int reserved);

    static ImGuiManager *GetManager(CKContext *context) {
        return (ImGuiManager *)context->GetManagerByGuid(IMGUI_MANAGER_GUID);
    }

private:
    bool m_Initialized = false;
    bool m_Show = false;
};

#endif // IMGUIMANAGER_H

// dear imgui: Renderer Backend for Virtools

// Implemented features:
//  [X] Renderer: User texture binding.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)

#include <stdint.h> // uint64_t
#include <string.h>  // memcpy
#include "imgui.h"
#include "imgui_impl_ck2.h"

// Virtools
#include "CKAll.h"

// CK2 data
struct ImGui_ImplCK2_Data
{
    CKContext *Context;
    CKTexture *FontTexture;

    ImGui_ImplCK2_Data() { memset((void *)this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
static ImGui_ImplCK2_Data *ImGui_ImplCK2_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplCK2_Data *)ImGui::GetIO().BackendRendererUserData : NULL;
}

static void ImGui_ImplCK2_SetupRenderState()
{
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    CKRenderContext *dev = bd->Context->GetPlayerRenderContext();

    // SET STATES
    dev->SetState(VXRENDERSTATE_FILLMODE, VXFILL_SOLID);
    dev->SetState(VXRENDERSTATE_SHADEMODE, VXSHADE_GOURAUD);
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_ALPHATESTENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_CULLMODE, VXCULL_NONE);
    dev->SetState(VXRENDERSTATE_ZENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_ALPHABLENDENABLE, TRUE);
    dev->SetState(VXRENDERSTATE_BLENDOP, VXBLENDOP_ADD);
    dev->SetState(VXRENDERSTATE_SRCBLEND, VXBLEND_SRCALPHA);
    dev->SetState(VXRENDERSTATE_DESTBLEND, VXBLEND_INVSRCALPHA);
    dev->SetState(VXRENDERSTATE_FOGENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_RANGEFOGENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_SPECULARENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_STENCILENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_CLIPPING, TRUE);
    dev->SetState(VXRENDERSTATE_LIGHTING, FALSE);

    dev->SetTextureStageState(CKRST_TSS_OP, CKRST_TOP_MODULATE);
    dev->SetTextureStageState(CKRST_TSS_ARG1, CKRST_TA_TEXTURE);
    dev->SetTextureStageState(CKRST_TSS_ARG2, CKRST_TA_DIFFUSE);

    dev->SetTextureStageState(CKRST_TSS_AOP, CKRST_TOP_MODULATE);
    dev->SetTextureStageState(CKRST_TSS_AARG1, CKRST_TA_TEXTURE);
    dev->SetTextureStageState(CKRST_TSS_AARG2, CKRST_TA_DIFFUSE);

    dev->SetTextureStageState(CKRST_TSS_ADDRESS, VXTEXTURE_ADDRESSCLAMP);
    dev->SetTextureStageState(CKRST_TSS_TEXTUREMAPBLEND, VXTEXTUREBLEND_MODULATEALPHA);

    dev->SetTextureStageState(CKRST_TSS_MINFILTER, VXTEXTUREFILTER_LINEAR);
    dev->SetTextureStageState(CKRST_TSS_MAGFILTER, VXTEXTUREFILTER_LINEAR);
}

bool ImGui_ImplCK2_Init(CKContext *context)
{
    ImGuiIO &io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplCK2_Data *bd = IM_NEW(ImGui_ImplCK2_Data)();
    io.BackendRendererUserData = (void *)bd;
    io.BackendRendererName = "imgui_impl_ck2";

    bd->Context = context;

    // Set platform dependent data in viewport
    ImGui::GetMainViewport()->PlatformHandleRaw = (void *)context->GetPlayerRenderContext()->GetWindowHandle();

    return true;
}

void ImGui_ImplCK2_Shutdown()
{
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    IM_ASSERT(bd != NULL && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplCK2_DestroyDeviceObjects();

    io.BackendRendererName = NULL;
    io.BackendRendererUserData = NULL;
    IM_DELETE(bd);
}

void ImGui_ImplCK2_NewFrame()
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplCK2_Init()?");

    if (!bd->FontTexture)
        ImGui_ImplCK2_CreateDeviceObjects();

    // Setup display size (every frame to accommodate for window resizing)
    VxRect rect;
    bd->Context->GetPlayerRenderContext()->GetWindowRect(rect, TRUE);
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

    // Setup time step
    io.DeltaTime = bd->Context->GetTimeManager()->GetLastDeltaTime() / 1000.0f;
}

// Render function.
void ImGui_ImplCK2_RenderDrawData(ImDrawData *draw_data)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    // Backup CK2 state that will be modified
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    CKRenderContext *dev = bd->Context->GetPlayerRenderContext();

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];
        const ImDrawVert *vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx *idx_buffer = cmd_list->IdxBuffer.Data;

        // Render command lists
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplCK2_SetupRenderState();
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                dev->SetTexture((CKTexture *)pcmd->GetTexID());

                // Setup desired render state
                ImGui_ImplCK2_SetupRenderState();

                const ImDrawVert *vert = vtx_buffer + pcmd->VtxOffset;
                int num_vertices = cmd_list->VtxBuffer.Size - pcmd->VtxOffset;
                int num_indices = pcmd->ElemCount;

                VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure(CKRST_DP_CL_VCT, num_vertices);
                XPtrStrided<VxVector4> positions(data->PositionPtr, data->PositionStride);
                XPtrStrided<CKDWORD> colors(data->ColorPtr, data->ColorStride);
                XPtrStrided<VxUV> uvs(data->TexCoordPtr, data->TexCoordStride);
                for (int v = 0; v < num_vertices; v++)
                {
                    positions->Set(vert->pos.x, vert->pos.y, 0.0f, 1.0f);
                    ++positions;

                    *colors = *(const CKDWORD *)&vert->col;
                    ++colors;

                    uvs->u = vert->uv.x;
                    uvs->v = vert->uv.y;
                    ++uvs;

                    ++vert;
                }

                const ImDrawIdx *idx = (idx_buffer + pcmd->IdxOffset);
                CKWORD *indices = dev->GetDrawPrimitiveIndices(num_indices);
                for (int i = 0; i < num_indices; i++)
                {
                    indices[i] = *idx;
                    ++idx;
                }

                dev->DrawPrimitive(VX_TRIANGLELIST, indices, num_indices, data);
            }
        }
    }
}

bool ImGui_ImplCK2_CreateFontsTexture()
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    CKContext *context = bd->Context;

    // Build texture atlas
    unsigned char *pixels;
    int width, height;
    // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders.
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
    CKTexture *texture = (CKTexture *)context->CreateObject(CKCID_TEXTURE, "ImGuiFontsTexture", CK_OBJECTCREATION_RENAME);
    if (texture == NULL)
        return false;
    texture->Create(width, height, 32);

    VxImageDescEx vxTexDesc;
    texture->GetSystemTextureDesc(vxTexDesc);
    vxTexDesc.Image = texture->LockSurfacePtr();
    if (vxTexDesc.Image)
        memcpy(vxTexDesc.Image, pixels, width * height * 4);

    texture->ReleaseSurfacePtr();

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(intptr_t)texture);
    bd->FontTexture = texture;
    return true;
}

void ImGui_ImplCK2_DestroyFontsTexture()
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    if (bd->FontTexture)
    {
        io.Fonts->SetTexID(NULL);
        bd->FontTexture = NULL;
    }
}

bool ImGui_ImplCK2_CreateDeviceObjects()
{
    return ImGui_ImplCK2_CreateFontsTexture();
}

void ImGui_ImplCK2_DestroyDeviceObjects()
{
    ImGui_ImplCK2_DestroyFontsTexture();
}

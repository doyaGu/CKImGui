// dear imgui: Renderer Backend for Virtools

// Implemented features:
//  [X] Renderer: User texture binding.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)

#include "imgui.h"
#include "imgui_impl_ck2.h"

// Virtools
#include "CKContext.h"
#include "CKRenderContext.h"
#include "CKTexture.h"
//#include "CKRasterizer.h"

// CK2 data
struct ImGui_ImplCK2_Data
{
    CKContext *Context;
    CKRenderContext *RenderContext;
    CKTexture *FontTexture;

    ImGui_ImplCK2_Data() { memset((void *)this, 0, sizeof(*this)); }
};

#ifdef IMGUI_USE_BGRA_PACKED_COLOR
#define IMGUI_COL_TO_ARGB(_COL)     (_COL)
#else
#define IMGUI_COL_TO_ARGB(_COL)     (((_COL) & 0xFF00FF00) | (((_COL) & 0xFF0000) >> 16) | (((_COL) & 0xFF) << 16))
#endif

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplCK2_Data *ImGui_ImplCK2_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplCK2_Data *)ImGui::GetIO().BackendRendererUserData : NULL;
}

static void ImGui_ImplCK2_SetupRenderState(ImDrawData* draw_data)
{
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    CKRenderContext *dev = bd->RenderContext;

    // Setup viewport
    VxRect viewport(0, 0, draw_data->DisplaySize.x, draw_data->DisplaySize.y);
    dev->SetViewRect(viewport);

    // Setup render state: alpha-blending, no face culling, no depth testing, shade mode (for gradient).
    dev->SetState(VXRENDERSTATE_FILLMODE, VXFILL_SOLID);
    dev->SetState(VXRENDERSTATE_SHADEMODE, VXSHADE_GOURAUD);

    dev->SetState(VXRENDERSTATE_CULLMODE, VXCULL_NONE);
    dev->SetState(VXRENDERSTATE_WRAP0, 0);

    dev->SetState(VXRENDERSTATE_SRCBLEND, VXBLEND_SRCALPHA);
    dev->SetState(VXRENDERSTATE_DESTBLEND, VXBLEND_INVSRCALPHA);

    dev->SetState(VXRENDERSTATE_ALPHATESTENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_ZWRITEENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_ZENABLE, FALSE);

    dev->SetState(VXRENDERSTATE_ALPHABLENDENABLE, TRUE);
    dev->SetState(VXRENDERSTATE_BLENDOP, VXBLENDOP_ADD);

    dev->SetState(VXRENDERSTATE_FOGENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_RANGEFOGENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_SPECULARENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_STENCILENABLE, FALSE);
    dev->SetState(VXRENDERSTATE_CLIPPING, TRUE);
    dev->SetState(VXRENDERSTATE_LIGHTING, FALSE);

    dev->SetTextureStageState(CKRST_TSS_ADDRESS, VXTEXTURE_ADDRESSCLAMP);
    dev->SetTextureStageState(CKRST_TSS_TEXTUREMAPBLEND, VXTEXTUREBLEND_MODULATEALPHA);

    dev->SetTextureStageState(CKRST_TSS_STAGEBLEND, 0, 1);

//    CKRasterizerContext *rst = dev->GetRasterizerContext();
//    rst->SetTransformMatrix(VXMATRIX_TEXTURE0, VxMatrix::Identity());

    dev->SetTextureStageState(CKRST_TSS_MINFILTER, VXTEXTUREFILTER_LINEAR);
    dev->SetTextureStageState(CKRST_TSS_MAGFILTER, VXTEXTUREFILTER_LINEAR);

    // Setup orthographic projection matrix
    {
        float l = draw_data->DisplayPos.x + 0.5f;
        float r = draw_data->DisplayPos.x + draw_data->DisplaySize.x + 0.5f;
        float t = draw_data->DisplayPos.y + 0.5f;
        float b = draw_data->DisplayPos.y + draw_data->DisplaySize.y + 0.5f;

        VxMatrix mat_projection;
        mat_projection[0] = {2.0f/(r - l), 0.0f, 0.0f, 0.0f};
        mat_projection[1] = {0.0f, 2.0f/(t - b), 0.0f, 0.0f};
        mat_projection[2] = {0.0f, 0.0f, 0.5f, 0.0f};
        mat_projection[3] = {(l + r) / (l - r), (t + b) / (b - t), 0.5f, 1.0f};

        dev->SetWorldTransformationMatrix(VxMatrix::Identity());
        dev->SetViewTransformationMatrix(VxMatrix::Identity());
        dev->SetProjectionTransformationMatrix(VxMatrix::Identity());
    }
}

// Render function.
void ImGui_ImplCK2_RenderDrawData(ImDrawData *draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;

    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    CKRenderContext *dev = bd->RenderContext;

    // Backup the transform matrices
    VxMatrix last_world = dev->GetWorldTransformationMatrix();
    VxMatrix last_view = dev->GetViewTransformationMatrix();
    VxMatrix last_projection = dev->GetProjectionTransformationMatrix();

    // Setup desired render state
    ImGui_ImplCK2_SetupRenderState(draw_data);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

        const ImDrawVert *vtx_src = vtx_buffer;
        int vtx_count = cmd_list->VtxBuffer.Size;

        // Create the vertex buffer
        VxDrawPrimitiveData *data = dev->GetDrawPrimitiveStructure((CKRST_DPFLAGS)(CKRST_DP_CL_VCT | CKRST_DP_VBUFFER), vtx_count);

        // Copy and convert vertices, convert colors to required format.
        XPtrStrided<VxVector4> positions(data->PositionPtr, data->PositionStride);
        XPtrStrided<CKDWORD> colors(data->ColorPtr, data->ColorStride);
        XPtrStrided<VxUV> uvs(data->TexCoordPtr, data->TexCoordStride);
        for (int vtx_i = 0; vtx_i < vtx_count; vtx_i++)
        {
            positions->Set(vtx_src->pos.x, vtx_src->pos.y, 0.0f, 1.0f);
            *colors = IMGUI_COL_TO_ARGB(vtx_src->col);
            uvs->u = vtx_src->uv.x;
            uvs->v = vtx_src->uv.y;

            ++positions;
            ++colors;
            ++uvs;
            ++vtx_src;
        }

        dev->ReleaseCurrentVB();

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplCK2_SetupRenderState(draw_data);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                dev->SetTexture((CKTexture *)pcmd->GetTexID());
                dev->DrawPrimitive(VX_TRIANGLELIST, (CKWORD *)(idx_buffer + pcmd->IdxOffset), pcmd->ElemCount, data);
            }
        }
    }

    // Restore the transform matrices
    dev->SetWorldTransformationMatrix(last_world);
    dev->SetViewTransformationMatrix(last_view);
    dev->SetProjectionTransformationMatrix(last_projection);
}

bool ImGui_ImplCK2_Init(CKContext *context)
{
    ImGuiIO &io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplCK2_Data *bd = IM_NEW(ImGui_ImplCK2_Data)();
    io.BackendRendererUserData = (void *)bd;
    io.BackendRendererName = "imgui_impl_ck2";

    bd->Context = context;
    bd->RenderContext = context->GetPlayerRenderContext();

    return true;
}

void ImGui_ImplCK2_Shutdown()
{
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    IM_ASSERT(bd != NULL && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplCK2_DestroyDeviceObjects();

    io.BackendRendererName = NULL;
    io.BackendRendererUserData = nullptr;
    IM_DELETE(bd);
}

bool ImGui_ImplCK2_CreateFontsTexture()
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    CKContext *context = bd->Context;

    // Build texture atlas
    unsigned char *pixels;
    int width, height, bytes_per_pixel;
    // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders.
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

    // Convert RGBA32 to BGRA32
#ifndef IMGUI_USE_BGRA_PACKED_COLOR
    if (io.Fonts->TexPixelsUseColors)
    {
        ImU32* dst_start = (ImU32*)ImGui::MemAlloc((size_t)width * height * bytes_per_pixel);
        for (ImU32* src = (ImU32*)pixels, *dst = dst_start, *dst_end = dst_start + (size_t)width * height; dst < dst_end; src++, dst++)
            *dst = IMGUI_COL_TO_ARGB(*src);
        pixels = (unsigned char*)dst_start;
    }
#endif

    // Upload texture to graphics system
    // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
    CKTexture *texture = (CKTexture *)context->CreateObject(CKCID_TEXTURE, "ImGuiFonts", CK_OBJECTCREATION_RENAME);
    if (texture == NULL)
        return false;
    texture->Create(width, height, bytes_per_pixel * 8);

    VxImageDescEx vxTexDesc;
    texture->GetSystemTextureDesc(vxTexDesc);
    vxTexDesc.Image = texture->LockSurfacePtr();
    if (vxTexDesc.Image)
        memcpy(vxTexDesc.Image, pixels, width * height * bytes_per_pixel);

    texture->ReleaseSurfacePtr();

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(intptr_t)texture);
    bd->FontTexture = texture;

#ifndef IMGUI_USE_BGRA_PACKED_COLOR
    if (io.Fonts->TexPixelsUseColors)
        ImGui::MemFree(pixels);
#endif

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

void ImGui_ImplCK2_NewFrame()
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplCK2_Init()?");

    if (!bd->FontTexture)
        ImGui_ImplCK2_CreateDeviceObjects();
}

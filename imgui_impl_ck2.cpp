// dear imgui: Renderer + Platform Backend for Virtools Dev

// Implemented features:
//  [X] Renderer: User texture binding.
//  [X] Platform: Keyboard support.
//  [X] Platform: Clipboard support.
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
// Issues:
//  [ ] Platform: Missing gamepad support.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)

#include <stdint.h> // uint64_t
#include <cstring>  // memcpy
#include <cctype>
#include "imgui.h"
#include "imgui_impl_ck2.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// Virtools
#include "CKAll.h"

// CK2 data
struct ImGui_ImplCK2_Data
{
    CKContext *Context;
    CKRenderContext *RenderContext;
    CKInputManager *InputManager;

    HWND hWnd;
    VxRect WindowRect;
    CKTexture *FontTexture;
    bool UsingIme;
    bool MouseTracked;
    ImGuiMouseCursor LastMouseCursor;

    ImGui_ImplCK2_Data() { memset((void *)this, 0, sizeof(*this)); }
};

HHOOK g_MsgHook;

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
static ImGui_ImplCK2_Data *ImGui_ImplCK2_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplCK2_Data *)ImGui::GetIO().BackendPlatformUserData : NULL;
}

static HMODULE GetSelfModuleHandle()
{
    MEMORY_BASIC_INFORMATION mbi;
    return ((::VirtualQuery(GetSelfModuleHandle, &mbi, sizeof(mbi)) != 0)
                ? (HMODULE)mbi.AllocationBase
                : NULL);
}

static LRESULT CALLBACK ImGui_ImplWin32_InputProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();

    switch (msg)
    {
    case WM_CHAR:
        if (wParam <= 127 && !std::iscntrl(wParam))
        {
            io.AddInputCharacter(wParam);
        }
        break;
    case WM_IME_STARTCOMPOSITION:
        bd->UsingIme = true;
        break;
    case WM_IME_COMPOSITION:
        if (lParam & GCS_RESULTSTR)
        {
            HIMC hIMC = ImmGetContext(hWnd);
            LONG len = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0) / sizeof(WCHAR);
            WCHAR *buf = new WCHAR[len];
            ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, buf, len * sizeof(WCHAR));
            ImmReleaseContext(0, hIMC);

            for (int i = 0; i < len; ++i)
            {
                io.AddInputCharacterUTF16(buf[i]);
            }
            delete[] buf;
            bd->UsingIme = false;
        }
        break;
    default:
        break;
    }

    return 0;
}

static LRESULT CALLBACK ImGui_ImplWin32_GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0) // do not process message
        return CallNextHookEx(g_MsgHook, nCode, wParam, lParam);

    if (nCode == HC_ACTION)
    {
        LPMSG msg = (LPMSG)lParam;
        ImGui_ImplWin32_InputProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
    }

    return CallNextHookEx(g_MsgHook, nCode, wParam, lParam);
}

static void ImGui_ImplWin32_SetHooks(HMODULE hModule)
{
    g_MsgHook = SetWindowsHookEx(WH_GETMESSAGE, ImGui_ImplWin32_GetMsgProc, hModule, 0);
}

static void ImGui_ImplWin32_UnsetHooks()
{
    UnhookWindowsHookEx(g_MsgHook);
}

static void ImGui_ImplCK2_UpdateMouseData()
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    IM_ASSERT(bd->hWnd != 0);

    const bool is_app_focused = (::GetForegroundWindow() == bd->hWnd);
    if (is_app_focused)
    {
        // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        if (io.WantSetMousePos)
        {
            CKPOINT pos = {(int)io.MousePos.x, (int)io.MousePos.y};
            if (VxScreenToClient(bd->hWnd, &pos))
                ::SetCursorPos(pos.x, pos.y);
        }

        // (Optional) Fallback to provide mouse position when focused (WM_MOUSEMOVE already provides this when hovered or captured)
        if (!io.WantSetMousePos && !bd->MouseTracked)
        {
            Vx2DVector pos;
            bd->InputManager->GetMousePosition(pos, FALSE);
            io.AddMousePosEvent(pos.x, pos.y);
        }
    }
}

static bool ImGui_ImplCK2_UpdateMouseCursor()
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return false;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        ::SetCursor(NULL);
    }
    else
    {
        // Show OS mouse cursor
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_Arrow:
            win32_cursor = IDC_ARROW;
            break;
        case ImGuiMouseCursor_TextInput:
            win32_cursor = IDC_IBEAM;
            break;
        case ImGuiMouseCursor_ResizeAll:
            win32_cursor = IDC_SIZEALL;
            break;
        case ImGuiMouseCursor_ResizeEW:
            win32_cursor = IDC_SIZEWE;
            break;
        case ImGuiMouseCursor_ResizeNS:
            win32_cursor = IDC_SIZENS;
            break;
        case ImGuiMouseCursor_ResizeNESW:
            win32_cursor = IDC_SIZENESW;
            break;
        case ImGuiMouseCursor_ResizeNWSE:
            win32_cursor = IDC_SIZENWSE;
            break;
        case ImGuiMouseCursor_Hand:
            win32_cursor = IDC_HAND;
            break;
        case ImGuiMouseCursor_NotAllowed:
            win32_cursor = IDC_NO;
            break;
        }
        ::SetCursor(::LoadCursor(NULL, win32_cursor));
    }

    return true;
}

static ImGuiKey ImGui_ImplCK2_KeyCodeToImGuiKey(int key_code)
{
    switch (key_code)
    {
    case CKKEY_TAB: return ImGuiKey_Tab;
    case CKKEY_LEFT: return ImGuiKey_LeftArrow;
    case CKKEY_RIGHT: return ImGuiKey_RightArrow;
    case CKKEY_UP: return ImGuiKey_UpArrow;
    case CKKEY_DOWN: return ImGuiKey_DownArrow;
    case CKKEY_PRIOR: return ImGuiKey_PageUp;
    case CKKEY_NEXT: return ImGuiKey_PageDown;
    case CKKEY_HOME: return ImGuiKey_Home;
    case CKKEY_END: return ImGuiKey_End;
    case CKKEY_INSERT: return ImGuiKey_Insert;
    case CKKEY_DELETE: return ImGuiKey_Delete;
    case CKKEY_BACK: return ImGuiKey_Backspace;
    case CKKEY_SPACE: return ImGuiKey_Space;
    case CKKEY_RETURN: return ImGuiKey_Enter;
    case CKKEY_ESCAPE: return ImGuiKey_Escape;
    case CKKEY_APOSTROPHE: return ImGuiKey_Apostrophe;
    case CKKEY_COMMA: return ImGuiKey_Comma;
    case CKKEY_MINUS: return ImGuiKey_Minus;
    case CKKEY_PERIOD: return ImGuiKey_Period;
    case CKKEY_SLASH: return ImGuiKey_Slash;
    case CKKEY_SEMICOLON: return ImGuiKey_Semicolon;
    case CKKEY_EQUALS: return ImGuiKey_Equal;
    case CKKEY_LBRACKET: return ImGuiKey_LeftBracket;
    case CKKEY_BACKSLASH: return ImGuiKey_Backslash;
    case CKKEY_RBRACKET: return ImGuiKey_RightBracket;
    case CKKEY_GRAVE: return ImGuiKey_GraveAccent;
    case CKKEY_CAPITAL: return ImGuiKey_CapsLock;
    case CKKEY_SCROLL: return ImGuiKey_ScrollLock;
    case CKKEY_NUMLOCK: return ImGuiKey_NumLock;
    // case CKKEY_PRINTSCREEN: return ImGuiKey_PrintScreen;
    // case CKKEY_PAUSE: return ImGuiKey_Pause;
    case CKKEY_NUMPAD0: return ImGuiKey_Keypad0;
    case CKKEY_NUMPAD1: return ImGuiKey_Keypad1;
    case CKKEY_NUMPAD2: return ImGuiKey_Keypad2;
    case CKKEY_NUMPAD3: return ImGuiKey_Keypad3;
    case CKKEY_NUMPAD4: return ImGuiKey_Keypad4;
    case CKKEY_NUMPAD5: return ImGuiKey_Keypad5;
    case CKKEY_NUMPAD6: return ImGuiKey_Keypad6;
    case CKKEY_NUMPAD7: return ImGuiKey_Keypad7;
    case CKKEY_NUMPAD8: return ImGuiKey_Keypad8;
    case CKKEY_NUMPAD9: return ImGuiKey_Keypad9;
    case CKKEY_DECIMAL: return ImGuiKey_KeypadDecimal;
    case CKKEY_DIVIDE: return ImGuiKey_KeypadDivide;
    case CKKEY_MULTIPLY: return ImGuiKey_KeypadMultiply;
    case CKKEY_SUBTRACT: return ImGuiKey_KeypadSubtract;
    case CKKEY_ADD: return ImGuiKey_KeypadAdd;
    case CKKEY_NUMPADENTER: return ImGuiKey_KeypadEnter;
    case CKKEY_NUMPADEQUALS: return ImGuiKey_KeypadEqual;
    case CKKEY_LCONTROL: return ImGuiKey_LeftCtrl;
    case CKKEY_LSHIFT: return ImGuiKey_LeftShift;
    case CKKEY_LMENU: return ImGuiKey_LeftAlt;
    case CKKEY_LWIN: return ImGuiKey_LeftSuper;
    case CKKEY_RCONTROL: return ImGuiKey_RightCtrl;
    case CKKEY_RSHIFT: return ImGuiKey_RightShift;
    case CKKEY_RMENU: return ImGuiKey_RightAlt;
    case CKKEY_RWIN: return ImGuiKey_RightSuper;
    case CKKEY_APPS: return ImGuiKey_Menu;
    case CKKEY_0: return ImGuiKey_0;
    case CKKEY_1: return ImGuiKey_1;
    case CKKEY_2: return ImGuiKey_2;
    case CKKEY_3: return ImGuiKey_3;
    case CKKEY_4: return ImGuiKey_4;
    case CKKEY_5: return ImGuiKey_5;
    case CKKEY_6: return ImGuiKey_6;
    case CKKEY_7: return ImGuiKey_7;
    case CKKEY_8: return ImGuiKey_8;
    case CKKEY_9: return ImGuiKey_9;
    case CKKEY_A: return ImGuiKey_A;
    case CKKEY_B: return ImGuiKey_B;
    case CKKEY_C: return ImGuiKey_C;
    case CKKEY_D: return ImGuiKey_D;
    case CKKEY_E: return ImGuiKey_E;
    case CKKEY_F: return ImGuiKey_F;
    case CKKEY_G: return ImGuiKey_G;
    case CKKEY_H: return ImGuiKey_H;
    case CKKEY_I: return ImGuiKey_I;
    case CKKEY_J: return ImGuiKey_J;
    case CKKEY_K: return ImGuiKey_K;
    case CKKEY_L: return ImGuiKey_L;
    case CKKEY_M: return ImGuiKey_M;
    case CKKEY_N: return ImGuiKey_N;
    case CKKEY_O: return ImGuiKey_O;
    case CKKEY_P: return ImGuiKey_P;
    case CKKEY_Q: return ImGuiKey_Q;
    case CKKEY_R: return ImGuiKey_R;
    case CKKEY_S: return ImGuiKey_S;
    case CKKEY_T: return ImGuiKey_T;
    case CKKEY_U: return ImGuiKey_U;
    case CKKEY_V: return ImGuiKey_V;
    case CKKEY_W: return ImGuiKey_W;
    case CKKEY_X: return ImGuiKey_X;
    case CKKEY_Y: return ImGuiKey_Y;
    case CKKEY_Z: return ImGuiKey_Z;
    case CKKEY_F1: return ImGuiKey_F1;
    case CKKEY_F2: return ImGuiKey_F2;
    case CKKEY_F3: return ImGuiKey_F3;
    case CKKEY_F4: return ImGuiKey_F4;
    case CKKEY_F5: return ImGuiKey_F5;
    case CKKEY_F6: return ImGuiKey_F6;
    case CKKEY_F7: return ImGuiKey_F7;
    case CKKEY_F8: return ImGuiKey_F8;
    case CKKEY_F9: return ImGuiKey_F9;
    case CKKEY_F10: return ImGuiKey_F10;
    case CKKEY_F11: return ImGuiKey_F11;
    case CKKEY_F12: return ImGuiKey_F12;
    default: return ImGuiKey_None;
    }
}

static void ImGui_ImplCK2_AddKeyEvent(ImGuiKey key, bool down, CKDWORD oKey)
{
    ImGuiIO &io = ImGui::GetIO();
    io.AddKeyEvent(key, down);
    io.SetKeyEventNativeData(key, MapVirtualKey(oKey, MAPVK_VSC_TO_VK), oKey); // To support legacy indexing (<1.87 user code)
}

static void ImGui_ImplCK2_ProcessKeyEventsWorkarounds()
{
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    CKInputManager *im = bd->InputManager;

    // Left & right Shift keys: when both are pressed together, Windows tend to not generate the WM_KEYUP event for the first released one.
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift) && !im->IsKeyDown(CKKEY_LSHIFT))
        ImGui_ImplCK2_AddKeyEvent(ImGuiKey_LeftShift, false, CKKEY_LSHIFT);
    if (ImGui::IsKeyDown(ImGuiKey_RightShift) && !im->IsKeyDown(CKKEY_RSHIFT))
        ImGui_ImplCK2_AddKeyEvent(ImGuiKey_RightShift, false, CKKEY_RSHIFT);

    // Sometimes WM_KEYUP for Win key is not passed down to the app (e.g. for Win+V on some setups, according to GLFW).
    if (ImGui::IsKeyDown(ImGuiKey_LeftSuper) && !im->IsKeyDown(CKKEY_LWIN))
        ImGui_ImplCK2_AddKeyEvent(ImGuiKey_LeftSuper, false, CKKEY_LWIN);
    if (ImGui::IsKeyDown(ImGuiKey_RightSuper) && !im->IsKeyDown(CKKEY_RWIN))
        ImGui_ImplCK2_AddKeyEvent(ImGuiKey_RightSuper, false, CKKEY_RWIN);
}

static void ImGui_ImplCK2_SetupRenderState()
{
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    CKRenderContext *rc = bd->RenderContext;

    // SET STATES
    rc->SetState(VXRENDERSTATE_FILLMODE, VXFILL_SOLID);

    rc->SetState(VXRENDERSTATE_CULLMODE, VXCULL_NONE);
    rc->SetState(VXRENDERSTATE_WRAP0, 0);
    rc->SetState(VXRENDERSTATE_SRCBLEND, VXBLEND_SRCALPHA);
    rc->SetState(VXRENDERSTATE_DESTBLEND, VXBLEND_INVSRCALPHA);

    rc->SetState(VXRENDERSTATE_ALPHABLENDENABLE, TRUE);
    rc->SetState(VXRENDERSTATE_ZWRITEENABLE, FALSE);
    rc->SetState(VXRENDERSTATE_ZENABLE, FALSE);

    rc->SetState(VXRENDERSTATE_SHADEMODE, VXSHADE_FLAT);

    rc->SetTextureStageState(CKRST_TSS_ADDRESS, VXTEXTURE_ADDRESSCLAMP);
    rc->SetTextureStageState(CKRST_TSS_TEXTUREMAPBLEND, VXTEXTUREBLEND_MODULATEALPHA);

    rc->SetTextureStageState(CKRST_TSS_MINFILTER, VXTEXTUREFILTER_LINEAR);
    rc->SetTextureStageState(CKRST_TSS_MAGFILTER, VXTEXTUREFILTER_LINEAR);

    rc->SetState(VXRENDERSTATE_SPECULARENABLE, TRUE);
    rc->SetTextureStageState(CKRST_TSS_STAGEBLEND, 0, 1);
    rc->SetTextureStageState(CKRST_TSS_TEXTURETRANSFORMFLAGS, 0);
    rc->SetTextureStageState(CKRST_TSS_TEXCOORDINDEX, 0);
}

bool ImGui_ImplCK2_Init(CKContext *context)
{
    ImGuiIO &io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == NULL && "Already initialized a platform backend!");

    // Setup backend capabilities flags
    ImGui_ImplCK2_Data *bd = IM_NEW(ImGui_ImplCK2_Data)();
    io.BackendPlatformUserData = (void *)bd;
    io.BackendPlatformName = io.BackendRendererName = "imgui_impl_ck2";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values (optional)

    bd->Context = context;
    bd->RenderContext = context->GetPlayerRenderContext();
    bd->InputManager = (CKInputManager *)context->GetManagerByGuid(INPUT_MANAGER_GUID);

    bd->hWnd = (HWND)bd->RenderContext->GetWindowHandle();
    bd->LastMouseCursor = ImGuiMouseCursor_COUNT;

    ImGui_ImplWin32_SetHooks(GetSelfModuleHandle());

    // Set platform dependent data in viewport
    ImGui::GetMainViewport()->PlatformHandleRaw = (void *)bd->hWnd;

    return true;
}

void ImGui_ImplCK2_Shutdown()
{
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    IM_ASSERT(bd != NULL && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplCK2_DestroyDeviceObjects();

    ImGui_ImplWin32_UnsetHooks();

    io.BackendPlatformUserData = NULL;
    io.BackendPlatformName = io.BackendRendererName = NULL;
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
    bd->RenderContext->GetWindowRect(bd->WindowRect, TRUE);
    io.DisplaySize = ImVec2((float)(bd->WindowRect.right - bd->WindowRect.left), (float)(bd->WindowRect.bottom - bd->WindowRect.top));

    // Setup time step
    io.DeltaTime = bd->Context->GetTimeManager()->GetLastDeltaTime() / 1000.0f;

    // Enable keyboard repetition
    bd->InputManager->EnableKeyboardRepetition();

    // Update OS mouse position
    ImGui_ImplCK2_UpdateMouseData();

    // Process workarounds for known Windows key handling issues
    ImGui_ImplCK2_ProcessKeyEventsWorkarounds();

    // Update OS mouse cursor with the cursor requested by imgui
    ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
    if (bd->LastMouseCursor != mouse_cursor)
    {
        bd->LastMouseCursor = mouse_cursor;
        ImGui_ImplCK2_UpdateMouseCursor();
    }
}

// Render function.
void ImGui_ImplCK2_RenderDrawData(ImDrawData *draw_data)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    // Backup CK2 state that will be modified
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    CKRenderContext *rc = bd->RenderContext;

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
                rc->SetTexture((CKTexture *)pcmd->GetTexID());

                // Setup desired render state
                ImGui_ImplCK2_SetupRenderState();

                const ImDrawVert *vert = vtx_buffer + pcmd->VtxOffset;
                int num_vertices = cmd_list->VtxBuffer.Size - pcmd->VtxOffset;
                int num_indices = pcmd->ElemCount;

                VxDrawPrimitiveData *data = rc->GetDrawPrimitiveStructure(CKRST_DP_CL_VCT, num_vertices);
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
                CKWORD *indices = rc->GetDrawPrimitiveIndices(num_indices);
                for (int i = 0; i < num_indices; i++)
                {
                    indices[i] = *idx;
                    ++idx;
                }

                rc->DrawPrimitive(VX_TRIANGLELIST, indices, num_indices, data);
            }
        }
    }
}

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
bool ImGui_ImplCK2_ProcessInput()
{
    if (ImGui::GetCurrentContext() == NULL)
        return false;

    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplCK2_Data *bd = ImGui_ImplCK2_GetBackendData();
    CKInputManager *im = bd->InputManager;

    Vx2DVector pos;
    im->GetMousePosition(pos, FALSE);
    if (pos.x >= 0 && pos.y >= 0)
    {
        if (!bd->MouseTracked)
            bd->MouseTracked = true;
        io.AddMousePosEvent(pos.x, pos.y);
    }
    else
    {
        bd->MouseTracked = false;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

    VxVector mrp;
    im->GetMouseRelativePosition(mrp);
    if (mrp.z != 0)
    {
        io.AddMouseWheelEvent(0.0f, mrp.z);
    }

    CKBYTE mbs[4];
    im->GetMouseButtonsState(mbs);
    for (int i = CK_MOUSEBUTTON_LEFT; i <= CK_MOUSEBUTTON_4; i++)
    {
        if (mbs[i] & KS_PRESSED)
            io.AddMouseButtonEvent(i, true);
        else
            io.AddMouseButtonEvent(i, false);
    }

    bool ctrl, shift, alt, super;
    CKBYTE *ks = im->GetKeyboardState();
    const int n = im->GetNumberOfKeyInBuffer();
    for (int i = 0; i < n; i++)
    {
        // Submit modifiers
        ctrl = ks[CKKEY_LCONTROL] & KS_PRESSED || ks[CKKEY_RCONTROL] & KS_PRESSED;
        io.AddKeyEvent(ImGuiKey_ModCtrl, ctrl);

        shift = ks[CKKEY_LSHIFT] & KS_PRESSED || ks[CKKEY_RSHIFT] & KS_PRESSED;
        io.AddKeyEvent(ImGuiKey_ModShift, shift);

        alt = ks[CKKEY_LMENU] & KS_PRESSED || ks[CKKEY_RMENU] & KS_PRESSED;
        io.AddKeyEvent(ImGuiKey_ModAlt, alt);

        super = ks[CKKEY_LWIN] & KS_PRESSED || ks[CKKEY_RWIN] & KS_PRESSED;
        io.AddKeyEvent(ImGuiKey_ModSuper, super);

        // Submit key event
        CKDWORD oKey;
        bool keydown = (im->GetKeyFromBuffer(i, oKey) == KEY_PRESSED);
        const ImGuiKey key = ImGui_ImplCK2_KeyCodeToImGuiKey(oKey);
        if (key != ImGuiKey_None)
        {
            ImGui_ImplCK2_AddKeyEvent(key, keydown, oKey);
            if (keydown && !bd->UsingIme)
            {
                char ch = VxScanCodeToAscii(oKey, ks);
                if (std::iscntrl(ch)) {
                    io.AddInputCharacter(ch);
                }
            }
        }

        // Submit individual left/right modifier events
        if (ctrl)
        {
            if (ks[CKKEY_LCONTROL] & KS_PRESSED) { ImGui_ImplCK2_AddKeyEvent(ImGuiKey_LeftCtrl, true, CKKEY_LCONTROL); }
            if (ks[CKKEY_RCONTROL] & KS_PRESSED) { ImGui_ImplCK2_AddKeyEvent(ImGuiKey_RightCtrl, true, CKKEY_RCONTROL); }
        }
        else if (shift)
        {
            if (ks[CKKEY_LSHIFT] & KS_PRESSED) { ImGui_ImplCK2_AddKeyEvent(ImGuiKey_LeftShift, true, CKKEY_LSHIFT); }
            if (ks[CKKEY_RSHIFT] & KS_PRESSED) { ImGui_ImplCK2_AddKeyEvent(ImGuiKey_RightShift, true, CKKEY_RSHIFT); }
        }
        else if (alt)
        {
            if (ks[CKKEY_LMENU] & KS_PRESSED) { ImGui_ImplCK2_AddKeyEvent(ImGuiKey_LeftAlt, true, CKKEY_LMENU); }
            if (ks[CKKEY_RMENU] & KS_PRESSED) { ImGui_ImplCK2_AddKeyEvent(ImGuiKey_RightAlt, true, CKKEY_RMENU); }
        }
    }

    io.AddFocusEvent(::GetForegroundWindow() == bd->hWnd);

    return true;
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
    CKTexture *texture = (CKTexture *)context->CreateObject(CKCID_TEXTURE, "ImGuiFontsTexture");
    if (texture == NULL)
    {
        return false;
    }
    texture->Create(width, height, 32);

    VxImageDescEx vxTexDesc;
    texture->GetSystemTextureDesc(vxTexDesc);
    vxTexDesc.Image = texture->LockSurfacePtr();
    if (vxTexDesc.Image)
    {
        memcpy(vxTexDesc.Image, pixels, width * height * 4);
    }
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
        io.Fonts->SetTexID(0);
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

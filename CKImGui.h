#ifndef NEMO2_CKIMGUI_H
#define NEMO2_CKIMGUI_H

#include <functional>

#ifndef VX_EXPORT
#   ifdef VX_LIB
#       define VX_EXPORT
#   else
#       ifdef VX_API
#           if defined(WIN32)
#               define VX_EXPORT __declspec(dllexport)
#           else
#               define VX_EXPORT
#           endif
#       else
#           if defined(WIN32)
#               define VX_EXPORT __declspec(dllimport)
#           else
#               define VX_EXPORT
#           endif
#       endif // VX_API
#   endif // VX_LIB
#endif // !VX_EXPORT

class CKContext;

VX_EXPORT bool ImGuiManagerIsInitialized(CKContext *context);
VX_EXPORT void ImGuiManagerShow(CKContext *context, bool show = true);
VX_EXPORT void ImGuiManagerDrawOnTopMost(CKContext *context, bool topmost = true);
VX_EXPORT void ImGuiManagerAddToFrame(CKContext *context, std::function<void()> callback);
VX_EXPORT bool ImGuiManagerRemoveFromFrame(CKContext *context, std::function<void()> &callback);

#endif //NEMO2_CKIMGUI_H

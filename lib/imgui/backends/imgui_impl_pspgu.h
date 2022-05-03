// dear imgui: Renderer Backend for psp gsKit
// This needs to be used along with the pspSDK backend

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this. 
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#include <gsKit.h>

IMGUI_IMPL_API bool     ImGui_Implpspgu_Init(GSGLOBAL *global);
IMGUI_IMPL_API void     ImGui_Implpspgu_Shutdown();
IMGUI_IMPL_API void     ImGui_Implpspgu_NewFrame();
IMGUI_IMPL_API void     ImGui_Implpspgu_RenderDrawData(ImDrawData* draw_data, ImVec2 pixelOffset = ImVec2(-0.5f, -0.5f));

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_Implpspgu_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_Implpspgu_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_Implpspgu_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_Implpspgu_DestroyDeviceObjects();

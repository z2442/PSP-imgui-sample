// dear imgui: Platform Backend for PlayStation 2 consoles using pspsdk
// This needs to be used along with the psp gsKit renderer

// Features:
//  [ ] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [ ] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_pspsdk.h"

// TODO: pspsdk includes
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <pspgu.h>
#include <pspgum.h>

// gsKit Data
struct ImGui_ImplpspSdk_Data
{
    GSGLOBAL* Global;
    u64 Time;
    u16 PreviousButtons;

    ImGui_ImplpspSdk_Data()   { memset(this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplpspSdk_Data* ImGui_ImplpspSdk_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplpspSdk_Data*)ImGui::GetIO().BackendPlatformUserData : NULL;
}

// Functions

static bool ImGui_ImplpspSdk_Init(GSGLOBAL* global)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == NULL && "Already initialized a platform backend!");

    // Setup backend capabilities flags
    ImGui_ImplpspSdk_Data* bd = IM_NEW(ImGui_ImplpspSdk_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_pspsdk";
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;        // We can honor io.WantSetMousePos requests (optional, rarely used)

    bd->Global = global;

    io.SetClipboardTextFn = NULL;
    io.GetClipboardTextFn = NULL;
    io.ClipboardUserData = NULL;

    return true;
}

bool ImGui_ImplpspSdk_InitForGsKit(GSGLOBAL* global)
{
    return ImGui_ImplpspSdk_Init(global);
}

void ImGui_ImplpspSdk_Shutdown()
{
    ImGui_ImplpspSdk_Data* bd = ImGui_ImplpspSdk_GetBackendData();
    IM_ASSERT(bd != NULL && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    io.BackendPlatformName = NULL;
    io.BackendPlatformUserData = NULL;
    IM_DELETE(bd);
}

static void ImGui_ImplpspSdk_UpdateGamepads(ImGui_ImplpspSdk_Data* bd)
{
    ImGuiIO& io = ImGui::GetIO();
    memset(io.NavInputs, 0, sizeof(io.NavInputs));
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;

    // Update gamepad state
    int gamepadState = padGetState(0, 0);
    if (gamepadState != PAD_STATE_STABLE && gamepadState != PAD_STATE_FINDCTP1) {
        // Controller is not active, so disable it in imgui
        bd->PreviousButtons = 0;
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
        return;
    }

    // Read gamepad data
    padButtonStatus pad;
    if (padRead(0, 0, &pad)) {
        // Invert active-low button states
        u16 buttons = pad.btns ^ 0xFFFF;
        // u16 newButtons = buttons & ~bd->PreviousButtons;
        // bd->PreviousButtons = buttons;

        #define MAP_DIGITAL_BUTTON(NAV_NO, BUTTON_MASK) { io.NavInputs[NAV_NO] = (buttons & BUTTON_MASK) ? 1.0f : 0.0f; }
        #define MAP_ANALOG(NAV_NO, AXIS_VAL, V0, V1, DZF) { float vn = (float)(AXIS_VAL - V0) / (float)(V1 - V0); if (vn > 1.0f) vn = 1.0f; if (vn > DZF && io.NavInputs[NAV_NO] < vn) io.NavInputs[NAV_NO] = vn; }

        MAP_DIGITAL_BUTTON(ImGuiNavInput_Activate,   PAD_CROSS);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_Cancel,     PAD_CIRCLE);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_Menu,       PAD_TRIANGLE);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_Input,      PAD_SQUARE);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_DpadLeft,   PAD_LEFT);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_DpadRight,  PAD_RIGHT);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_DpadUp,     PAD_UP);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_DpadDown,   PAD_DOWN);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_FocusPrev,  PAD_L1);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_FocusNext,  PAD_R1);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_TweakSlow,  PAD_L2);
        MAP_DIGITAL_BUTTON(ImGuiNavInput_TweakFast,  PAD_R2);

        MAP_ANALOG(ImGuiNavInput_LStickLeft, pad.ljoy_h, 0x7F, 0x00, 0.15);
        MAP_ANALOG(ImGuiNavInput_LStickRight, pad.ljoy_h, 0x80, 0xFF, 0.15);
        MAP_ANALOG(ImGuiNavInput_LStickUp, pad.ljoy_v, 0x7F, 0x00, 0.15);
        MAP_ANALOG(ImGuiNavInput_LStickDown, pad.ljoy_v, 0x80, 0xFF, 0.15);

        #undef MAP_DIGITAL_BUTTON
        #undef MAP_ANALOG

        #define ANALOG_CLAMP_DEADZONE(AXIS_VAL, DEAD_ZONE) ((AXIS_VAL >= DEAD_ZONE || AXIS_VAL <= -DEAD_ZONE) ? AXIS_VAL : 0)

        float rstickX = ANALOG_CLAMP_DEADZONE(pad.rjoy_h / 255.f * 2.f - 1.f, 0.15);
        float rstickY = ANALOG_CLAMP_DEADZONE(pad.rjoy_v / 255.f * 2.f - 1.f, 0.15);

        // Show and adjust the mouse cursor if the right analog stick has been touched
        if (rstickX || rstickY) {
            io.MouseDrawCursor = true;
            io.MousePos = ImVec2(fminf(io.DisplaySize.x - 1, fmaxf(0, io.MousePos.x + rstickX * 8)), fminf(io.DisplaySize.y - 1, fmaxf(0, io.MousePos.y + rstickY * 8)));
        }

        // Only allow emulated mouse clicks if we are in cursor mode
        if (io.MouseDrawCursor) {
            io.MouseDown[0] = (buttons & PAD_R3) | (buttons & PAD_R2);
        }

        // Hide the mouse cursor if the D-pad has been touched
        if (buttons & (PAD_LEFT | PAD_RIGHT | PAD_UP | PAD_DOWN)) {
            io.MouseDrawCursor = false;
        }

        // Indicate that the gamepad is present
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    }
}

void ImGui_ImplpspSdk_NewFrame()
{
    ImGui_ImplpspSdk_Data* bd = ImGui_ImplpspSdk_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplpspSdk_Init()?");
    ImGuiIO& io = ImGui::GetIO();

    // Update the framebuffer to handle resizing
    io.DisplaySize = ImVec2((float)bd->Global->Width, (float)bd->Global->Height);
    io.DisplayFramebufferScale = ImVec2(1.0, 1.0);

    // TODO: Update the imgui DeltaTime from the psp clock
    io.DeltaTime = (float)(1.0f / 60.0f);

    ImGui_ImplpspSdk_UpdateGamepads(bd);
}

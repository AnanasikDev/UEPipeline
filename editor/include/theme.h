#pragma once
#include <imgui.h>

namespace Theme
{
    // --- Font / Zoom -----------------------------------------------------------
    constexpr float FontSizeBase       = 19.0f;   // base px loaded from TTF
    constexpr float FontScaleDefault   = 1.0f;    // initial global scale
    constexpr float FontScaleMin       = 0.75f;
    constexpr float FontScaleMax       = 2.0f;
    constexpr float FontScaleStep      = 0.1f;
    constexpr float FontHeaderScale    = 1.45f;   // multiplied on top of global
    constexpr float FontSubheaderScale = 1.1f;

    // --- Shape ----------------------------------------------------------------
    constexpr float WindowRounding     = 8.0f;
    constexpr float ChildRounding      = 6.0f;
    constexpr float FrameRounding      = 5.0f;
    constexpr float PopupRounding      = 6.0f;
    constexpr float ScrollbarRounding  = 6.0f;
    constexpr float GrabRounding       = 4.0f;
    constexpr float TabRounding        = 5.0f;

    // --- Spacing --------------------------------------------------------------
    constexpr ImVec2 WindowPadding     = { 18.0f, 18.0f };
    constexpr ImVec2 FramePadding      = { 12.0f, 7.0f };
    constexpr ImVec2 CellPadding       = { 8.0f, 4.0f };
    constexpr ImVec2 ItemSpacing       = { 10.0f, 10.0f };
    constexpr ImVec2 ItemInnerSpacing  = { 6.0f, 6.0f };
    constexpr float  ScrollbarSize     = 11.0f;
    constexpr float  GrabMinSize       = 10.0f;
    constexpr float  WindowBorderSize  = 0.0f;
    constexpr float  FrameBorderSize   = 0.0f;

    // --- Buttons (size scales with font, these are the "at 1x" values) --------
    constexpr ImVec2 ButtonMain        = { 210.0f, 40.0f };
    constexpr ImVec2 ButtonSecondary   = { 130.0f, 40.0f };
    constexpr ImVec2 ButtonSmall       = { 36.0f,  0.0f };  // e.g. "..." browse

    // --- Palette --------------------------------------------------------------

    // Background layers  (darkest → lightest)
    constexpr ImVec4 BgDeep            = { 0.086f, 0.090f, 0.102f, 1.0f };
    constexpr ImVec4 BgMid             = { 0.118f, 0.122f, 0.137f, 1.0f };
    constexpr ImVec4 BgRaised          = { 0.149f, 0.153f, 0.173f, 1.0f };
    constexpr ImVec4 BgHover           = { 0.200f, 0.204f, 0.230f, 1.0f };

    // Text
    constexpr ImVec4 TextPrimary       = { 0.95f,  0.95f,  0.97f,  1.0f };
    constexpr ImVec4 TextSecondary     = { 0.62f,  0.63f,  0.68f,  1.0f };
    constexpr ImVec4 TextDisabled      = { 0.42f,  0.43f,  0.47f,  1.0f };

    // Accent
    constexpr ImVec4 Accent            = { 0.302f, 0.498f, 1.000f, 1.0f };
    constexpr ImVec4 AccentDim         = { 0.184f, 0.306f, 0.600f, 1.0f };
    constexpr ImVec4 AccentDark        = { 0.125f, 0.200f, 0.420f, 1.0f };
    constexpr ImVec4 AccentBright      = { 0.502f, 0.698f, 1.000f, 1.0f };

    // Semantic
    constexpr ImVec4 Success           = { 0.35f,  0.85f,  0.35f,  1.0f };
    constexpr ImVec4 Error             = { 1.00f,  0.40f,  0.40f,  1.0f };
    constexpr ImVec4 Warning           = { 1.00f,  0.75f,  0.30f,  1.0f };

    // Stop button
    constexpr ImVec4 StopButton        = { 0.60f,  0.15f,  0.15f,  1.0f };
    constexpr ImVec4 StopButtonHover   = { 0.75f,  0.20f,  0.20f,  1.0f };
    constexpr ImVec4 StopButtonActive  = { 0.90f,  0.25f,  0.25f,  1.0f };

    // Browse "..." button
    constexpr ImVec4 BrowseButton      = { 0.22f,  0.22f,  0.24f,  1.0f };
    constexpr ImVec4 BrowseButtonHover = { 0.30f,  0.30f,  0.33f,  1.0f };
    constexpr ImVec4 BrowseButtonActive= { 0.18f,  0.18f,  0.20f,  1.0f };

    // Console
    constexpr ImU32 ConNormal          = IM_COL32(240, 240, 245, 255);
    constexpr ImU32 ConError           = IM_COL32(255, 100, 100, 255);
    constexpr ImU32 ConAux             = IM_COL32(130, 135, 155, 255);
    constexpr ImU32 ConSelect          = IM_COL32( 51, 102, 204,  80);

    inline void Apply()
    {
        ImGuiStyle& s = ImGui::GetStyle();

        s.WindowRounding    = WindowRounding;
        s.ChildRounding     = ChildRounding;
        s.FrameRounding     = FrameRounding;
        s.PopupRounding     = PopupRounding;
        s.ScrollbarRounding = ScrollbarRounding;
        s.GrabRounding      = GrabRounding;
        s.TabRounding       = TabRounding;

        s.WindowPadding     = WindowPadding;
        s.FramePadding      = FramePadding;
        s.CellPadding       = CellPadding;
        s.ItemSpacing       = ItemSpacing;
        s.ItemInnerSpacing  = ItemInnerSpacing;
        s.ScrollbarSize     = ScrollbarSize;
        s.GrabMinSize       = GrabMinSize;
        s.WindowBorderSize  = WindowBorderSize;
        s.FrameBorderSize   = FrameBorderSize;

        ImVec4* c = s.Colors;

        c[ImGuiCol_WindowBg]            = BgDeep;
        c[ImGuiCol_ChildBg]             = BgMid;
        c[ImGuiCol_PopupBg]             = BgMid;

        c[ImGuiCol_FrameBg]             = BgRaised;
        c[ImGuiCol_FrameBgHovered]      = BgHover;
        c[ImGuiCol_FrameBgActive]       = BgMid;

        c[ImGuiCol_TitleBg]             = BgDeep;
        c[ImGuiCol_TitleBgActive]       = BgDeep;
        c[ImGuiCol_MenuBarBg]           = BgDeep;

        c[ImGuiCol_Button]              = AccentDark;
        c[ImGuiCol_ButtonHovered]       = AccentDim;
        c[ImGuiCol_ButtonActive]        = Accent;

        c[ImGuiCol_Header]              = AccentDark;
        c[ImGuiCol_HeaderHovered]       = AccentDim;
        c[ImGuiCol_HeaderActive]        = Accent;

        c[ImGuiCol_Tab]                 = BgMid;
        c[ImGuiCol_TabHovered]          = AccentDim;
        c[ImGuiCol_TabActive]           = AccentDark;
        c[ImGuiCol_TabUnfocused]        = BgDeep;
        c[ImGuiCol_TabUnfocusedActive]  = AccentDark;

        c[ImGuiCol_SliderGrab]          = Accent;
        c[ImGuiCol_SliderGrabActive]    = AccentBright;
        c[ImGuiCol_CheckMark]           = Accent;

        c[ImGuiCol_ScrollbarBg]         = { BgDeep.x, BgDeep.y, BgDeep.z, 0.0f };
        c[ImGuiCol_ScrollbarGrab]       = { 0.22f, 0.224f, 0.25f, 1.0f };
        c[ImGuiCol_ScrollbarGrabHovered]= { 0.28f, 0.284f, 0.316f, 1.0f };
        c[ImGuiCol_ScrollbarGrabActive] = Accent;

        c[ImGuiCol_SeparatorHovered]    = AccentDim;
        c[ImGuiCol_SeparatorActive]     = Accent;

        c[ImGuiCol_Text]                = TextPrimary;
        c[ImGuiCol_TextDisabled]        = TextSecondary;
    }

    namespace PipelineColors
    {
        // Status fills
        inline ImU32 Awaiting() { return IM_COL32(60, 63, 70, 255); }
        inline ImU32 InProgress() { return IM_COL32(50, 120, 220, 255); }
        inline ImU32 Succeeded() { return IM_COL32(46, 160, 67, 255); }
        inline ImU32 Failed() { return IM_COL32(210, 50, 50, 255); }
        inline ImU32 Skipped() { return IM_COL32(110, 110, 115, 255); }

        // Idle-mode
        inline ImU32 IdleDefault() { return IM_COL32(55, 58, 64, 255); }
        inline ImU32 IdleHovered() { return IM_COL32(72, 76, 85, 255); }
        inline ImU32 IdleSelected() { return IM_COL32(50, 110, 200, 255); }
        inline ImU32 IdleSelectedHov() { return IM_COL32(60, 125, 220, 255); }

        // Connector line
        inline ImU32 Connector() { return IM_COL32(80, 83, 90, 255); }
        inline ImU32 ConnectorDone() { return IM_COL32(46, 160, 67, 255); }

        // Text
        inline ImU32 TextBright() { return IM_COL32(230, 232, 236, 255); }
        inline ImU32 TextDim() { return IM_COL32(160, 163, 170, 255); }

        // Panel text – dark for contrast against colored stage backgrounds
        static ImU32 PanelText() { return IM_COL32(20, 20, 25, 255); }
        static ImU32 PanelTextDim() { return IM_COL32(40, 40, 50, 180); }
        static ImU32 PanelWidgetBg() { return IM_COL32(0, 0, 0, 30); }
        static ImU32 PanelWidgetBorder() { return IM_COL32(0, 0, 0, 50); }
    }
}

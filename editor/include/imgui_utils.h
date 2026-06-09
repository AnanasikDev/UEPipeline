#pragma once

#include <string>
#include <imgui.h>

#include "theme.h"
#include "config.h"
#include "pathpicker.h"

/// <summary>
/// Adds on-hover tooltip to the previous ImGui item
/// </summary>
static void Tooltip(const std::string& str)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40.0f);
        ImGui::TextWrapped(str.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static void ZoomIn()
{
    UserConfig::fontScale += Theme::FontScaleStep;
    if (UserConfig::fontScale > Theme::FontScaleMax) UserConfig::fontScale = Theme::FontScaleMax;
    ImGui::GetIO().FontGlobalScale = UserConfig::fontScale;
}

static void ZoomOut()
{
    UserConfig::fontScale -= Theme::FontScaleStep;
    if (UserConfig::fontScale < Theme::FontScaleMin) UserConfig::fontScale = Theme::FontScaleMin;
    ImGui::GetIO().FontGlobalScale = UserConfig::fontScale;
}

static void ZoomReset()
{
    UserConfig::fontScale = Theme::FontScaleDefault;
    ImGui::GetIO().FontGlobalScale = UserConfig::fontScale;
}

/// <summary>
/// Represents a folder path input/output GUI item. If noinput is set to true, input will be disabled, and instead of rendering a PathInput, a Text will be rendered.
/// Useful for making path input either manual or automatic.
/// </summary>
/// <typeparam name="N">Size of the output path char array (used for safe string copy with c-style strings)</typeparam>
/// <param name="label"></param>
/// <param name="path">Output path</param>
/// <param name="noinput">Determines if PathInput or Text is rendered</param>
/// <returns>(only when input is on) true if path changed, false otherwise</returns>
template <int N>
static bool MaybeAutoFolderInput(const char* const label, char(&path)[N], bool noinput)
{
    ImGui::Spacing();
    if (noinput)
    {
        std::string text = path;
        ImVec4 color = Theme::TextPrimary;
        ImGui::Text(label);
        if (fs::exists(path) && !fs::is_empty(path))
        {
            color = Theme::Success;
        }
        else
        {
            color = Theme::Error;
            text = "Invalid: " + text;
        }
        ImGui::TextColored(color, text.c_str());
    }
    else
    {
        return PathInput(label, path, sizeof(path), PathMode::Folder);
    }
    return false;
}

/// <summary>
/// Represents a file path input/output GUI item. If noinput is set to true, input will be disabled, and instead of rendering a PathInput, a Text will be rendered.
/// Useful for making path input either manual or automatic.
/// </summary>
/// <typeparam name="N">Size of the output path char array (used for safe string copy with c-style strings)</typeparam>
/// <param name="label"></param>
/// <param name="path">Output path</param>
/// <param name="noinput">Determines if PathInput or Text is rendered</param>
/// <param name="filter">If nullptr, will be used PathMode::File, otherwise PathMode::FileFiltered. Example: L"*.exe"</param>
/// <returns>(only when input is on) true if path changed, false otherwise</returns>
template <int N>
static bool MaybeAutoFileInput(const char* const label, char(&path)[N], bool noinput, const wchar_t* filter = nullptr)
{
    if (noinput)
    {
        std::string text = path;
        ImGui::Text(label);
        ImVec4 color = Theme::TextPrimary;
        if (fs::exists(path))
        {
            color = Theme::Success;
        }
        else
        {
            color = Theme::Error;
            text = "Invalid: " + text;
        }
        ImGui::TextColored(color, text.c_str());
    }
    else
    {
        if (filter)
        {
            return PathInput(label, path, sizeof(path), PathMode::FileFiltered, filter);
        }
        else
        {
            return PathInput(label, path, sizeof(path), PathMode::File);
        }
    }
    return false;
}

static void UpdateZoom()
{
    ImGuiIO& io = ImGui::GetIO();

    // --- Zoom: Ctrl+ / Ctrl- / Ctrl0 ---
    if (io.KeyCtrl)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Equal))       ZoomIn();     // Ctrl+=  (+ key)
        if (ImGui::IsKeyPressed(ImGuiKey_Minus))       ZoomOut();    // Ctrl+-
        if (ImGui::IsKeyPressed(ImGuiKey_0))           ZoomReset();  // Ctrl+0
        if (ImGui::IsKeyPressed(ImGuiKey_KeypadAdd))   ZoomIn();
        if (ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract)) ZoomOut();
    }
    // Ctrl+scroll wheel zoom
    if (io.KeyCtrl && io.MouseWheel != 0.0f)
    {
        UserConfig::fontScale += io.MouseWheel * Theme::FontScaleStep;

        if (UserConfig::fontScale < Theme::FontScaleMin)
        {
            UserConfig::fontScale = Theme::FontScaleMin;
        }
        if (UserConfig::fontScale > Theme::FontScaleMax)
        {
            UserConfig::fontScale = Theme::FontScaleMax;
        }
        io.FontGlobalScale = UserConfig::fontScale;
    }
}
#include "console.h"
#include "theme.h"
#include <algorithm>

void Console::Print(const std::string& line)
{
    std::lock_guard<std::mutex> lock(mutex);
    lines.push_back({ line, false });
    pendingScroll = true;
}

void Console::PrintError(const std::string& line)
{
    std::lock_guard<std::mutex> lock(mutex);
    lines.push_back({ line, true });
    pendingScroll = true;
}

void Console::Clear()
{
    std::lock_guard<std::mutex> lock(mutex);
    lines.clear();
    selAnchor = selTail = -1;
    pendingScroll = true;
}

void Console::CopySelection()
{
    if (selAnchor < 0 || selTail < 0) return;

    int lo = std::min(selAnchor, selTail);
    int hi = std::max(selAnchor, selTail);
    hi = std::min(hi, (int)lines.size() - 1);

    std::string clip;
    for (int i = lo; i <= hi; i++)
    {
        clip += lines[i].text;
        if (i < hi) clip += '\n';
    }
    ImGui::SetClipboardText(clip.c_str());
}

void Console::Draw(const char* title, bool* open)
{
    if (!ImGui::Begin(title, open))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear"))
        Clear();

    ImGui::SameLine();
    if (ImGui::Button("Copy All"))
    {
        std::lock_guard<std::mutex> lock(mutex);
        selAnchor = 0;
        selTail = (int)lines.size() - 1;
        CopySelection();
    }

    ImGui::Separator();

    std::lock_guard<std::mutex> lock(mutex);

    ImGui::BeginChild("##scrolling", ImVec2(0, 0), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    // Keyboard shortcuts
    bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (focused)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A))
        {
            selAnchor = 0;
            selTail = (int)lines.size() - 1;
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
        {
            CopySelection();
        }
    }

    int lo = std::min(selAnchor, selTail);
    int hi = std::max(selAnchor, selTail);
    float lineHeight = ImGui::GetTextLineHeightWithSpacing();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImGuiListClipper clipper;
    clipper.Begin((int)lines.size(), lineHeight);
    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            float contentWidth = ImGui::GetContentRegionAvail().x;

            // Selection highlight
            if (selAnchor >= 0 && i >= lo && i <= hi)
            {
                drawList->AddRectFilled(
                    pos,
                    ImVec2(pos.x + contentWidth, pos.y + lineHeight),
                    Theme::ConSelect
                );
            }

            // Pick color from Theme
            ImU32 col = Theme::ConNormal;
            if (lines[i].isError)
                col = Theme::ConError;
            else if (lines[i].text.size() > 1 && lines[i].text[0] == '[')
                col = Theme::ConAux;

            drawList->AddText(pos, col, lines[i].text.c_str());

            // Invisible button for click/drag selection
            ImGui::PushID(i);
            ImGui::InvisibleButton("##line", ImVec2(contentWidth, lineHeight));

            if (ImGui::IsItemClicked(0))
            {
                if (ImGui::GetIO().KeyShift && selAnchor >= 0)
                    selTail = i;
                else
                {
                    selAnchor = i;
                    selTail = i;
                }
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(0))
            {
                selTail = i;
            }

            ImGui::PopID();
        }
    }

    if (pendingScroll)
    {
        ImGui::SetScrollHereY(1.0f);
        pendingScroll = false;
    }

    ImGui::EndChild();
    ImGui::End();
}
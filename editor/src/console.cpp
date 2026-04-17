#include "console.h"
#include <imgui.h>

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
}

void Console::Draw(const char* title, bool* open)
{
    ImGui::SetNextWindowSize(ImVec2(700, 300), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title, open))
    {
        ImGui::End();
        return;
    }

    // Toolbar
    if (ImGui::Button("Clear"))
        Clear();

    ImGui::SameLine();
    ImGui::TextDisabled("%zu lines", lines.size());

    ImGui::Separator();

    // Output area — takes all remaining space
    ImGui::BeginChild("##output", ImVec2(0, 0), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    {
        std::lock_guard<std::mutex> lock(mutex);
        for (const auto& entry : lines)
        {
            if (entry.isError)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.92f, 1.0f));

            ImGui::TextUnformatted(entry.text.c_str());
            ImGui::PopStyleColor();
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
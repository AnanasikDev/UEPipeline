#include <algorithm>

#include "console.h"
#include "theme.h"
#include "strutils.h"

void Console::Print(const std::string& line, Result type)
{
    if (type == Result::Info)
    {
        PrintInfo(line);
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);
    lines.push_back({ line, type });
    pendingScroll = true;
    displaysSkipped = 0;
}

void Console::PrintMultiline(const std::string& text, Result type)
{
    std::vector<std::string> lines = split(text, "\n\r");
    for (const std::string& line : lines)
    {
        printf(line.c_str());
        Result realType = type;
        if (type == Result::AutoFormat)
        {
            realType = GetConsoleResult(line);
        }

        Print(line, realType);
    }
}

void Console::PrintInfo(const std::string& line)
{
    if (!showDisplay)
    {
        if (displaysSkipped > 0)
        {
            if (!lines.empty())
            {
                Console::Entry& l = *(lines.end() - 1); // get last line
                l.text = "Info messages ignored: " + std::to_string(displaysSkipped); // replace it with counter
            }
        }
        else
        {
            std::lock_guard<std::mutex> lock(mutex);
            lines.push_back({ "Info message ignored", Result::Info });
            pendingScroll = true;
        }
        ++displaysSkipped;
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);
    lines.push_back({ line, Result::Info });
    pendingScroll = true;
}

void Console::Clear()
{
    std::lock_guard<std::mutex> lock(mutex);
    lines.clear();
    selectionAnchor = -1;
    selectionTail = -1;
    pendingScroll = true;
    displaysSkipped = 0;
}

void Console::CopySelection()
{
    if (selectionAnchor < 0 || selectionTail < 0) return;

    const int selectionMin = std::min(selectionAnchor, selectionTail);
    const int selectionMax = std::min(
        std::max(selectionAnchor, selectionTail), 
        static_cast<int>(lines.size() - 1)
    );

    std::string clip;
    for (int i = selectionMin; i <= selectionMax; i++)
    {
        clip += lines[i].text;
        if (i < selectionMax) clip += '\n';
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
        selectionAnchor = 0;
        selectionTail = (int)lines.size() - 1;
        CopySelection();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Show info messages", &showDisplay);

    ImGui::Separator();

    std::lock_guard<std::mutex> lock(mutex);

    ImGui::BeginChild("##scrolling", ImVec2(0, 0), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    // Keyboard shortcuts
    const bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (focused)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A))
        {
            selectionAnchor = 0;
            selectionTail = (int)lines.size() - 1;
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
        {
            CopySelection();
        }
    }

    const int selectionMin = std::min(selectionAnchor, selectionTail);
    const int selectionMax = std::max(selectionAnchor, selectionTail);
    const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
    ImDrawList* const drawList = ImGui::GetWindowDrawList();

    ImGuiListClipper clipper;
    clipper.Begin((int)lines.size(), lineHeight);
    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
        {
            const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            const float contentWidth = ImGui::GetContentRegionAvail().x;
            const bool isSelected = selectionAnchor >= 0 && i >= selectionMin && i <= selectionMax;

            // Selection highlight
            if (isSelected)
            {
                drawList->AddRectFilled(
                    cursorPos,
                    ImVec2(cursorPos.x + contentWidth, cursorPos.y + lineHeight),
                    Theme::ConSelect
                );
            }

            // Pick color from Theme
            ImU32 textColor = Theme::ConNormal;
            const Console::Result result = lines[i].type;
            if (result == Result::Error || result == Result::Critical)
            {
                textColor = Theme::ConError;
            }
            else if (result == Result::Warning)
            {
                textColor = Theme::ConWarning;
            }
            else if (result == Result::Success)
            {
                textColor = Theme::ConSuccess;
            }
            else if (lines[i].text.size() > 1 && lines[i].text[0] == '[')
            {
                textColor = Theme::ConAux;
            }

            drawList->AddText(cursorPos, textColor, lines[i].text.c_str());

            // Invisible button for click/drag selection
            ImGui::PushID(i);
            ImGui::InvisibleButton("##line", ImVec2(contentWidth, lineHeight));

            if (ImGui::IsItemClicked(0))
            {
                if (ImGui::GetIO().KeyShift && selectionAnchor >= 0)
                {
                    // select from anchor to tail
                    selectionTail = i;
                }
                else
                {
                    // change anchor, select only this item (from and to it, hence anchor=tail)
                    selectionAnchor = i;
                    selectionTail = i;
                }
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

Console::Result Console::GetConsoleResult(const std::string& line)
{
    std::string checkline = line;

    // lower the line
    std::transform(
        checkline.begin(),
        checkline.end(),
        checkline.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });

    struct Keyword
    {
        const char* keyword;
        Console::Result result;
    };

    static constexpr Keyword keywords[] =
    {
        { "[critical]", Console::Result::Critical },
        { "critical",   Console::Result::Critical },
        { "fatal",      Console::Result::Critical },

        { "[error]",    Console::Result::Error },
        { "error",      Console::Result::Error },

        { "[warning]",  Console::Result::Warning },
        { "warning",    Console::Result::Warning },

        { "[success]",  Console::Result::Success },
        { "success",    Console::Result::Success },
        { "succeed",    Console::Result::Success },
        { "done",       Console::Result::Success },
        { "ok ",         Console::Result::Success },
        { "ok:",         Console::Result::Success },
        { "[ok]",         Console::Result::Success },

        { "[info]",     Console::Result::Info },
        { "info",       Console::Result::Info },

        { "[display]",  Console::Result::Info },
        { "display",    Console::Result::Info },

        { "[log]",      Console::Result::Info },
        { "log:",        Console::Result::Info },
        { "log ",        Console::Result::Info },
    };

    size_t bestPos = std::string::npos;
    Console::Result bestResult = Console::Result::None;

    for (const Keyword& entry : keywords)
    {
        size_t pos = checkline.find(entry.keyword);

        if (pos != std::string::npos && pos < bestPos)
        {
            bestPos = pos;
            bestResult = entry.result;
        }
    }

    return bestResult;
}
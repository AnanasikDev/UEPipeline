#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <imgui.h>

struct Console
{
    void Print(const std::string& line);
    void PrintError(const std::string& line);
    void Clear();
    void Draw(const char* title, bool* open = nullptr);

    bool scrollToBottom = true;

private:
    struct Entry
    {
        std::string text;
        bool        isError = false;
    };

    std::vector<Entry> lines;
    std::mutex         mutex;
    bool               pendingScroll = true;

    int selAnchor = -1;
    int selTail = -1;

    void CopySelection();
};
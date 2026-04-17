#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <imgui.h>

struct Console
{
    // Call these from any thread
    void Print(const std::string& line);
    void PrintError(const std::string& line);
    void Clear();

    // Call these from render thread only
    void Draw(const char* title, bool* open = nullptr);

    // Scroll to bottom next frame
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
};
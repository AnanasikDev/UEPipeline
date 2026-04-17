#pragma once
#include <string>

// That's it. This is all you ever call.
std::string PickFolder();
std::string PickFile(const wchar_t* filter = nullptr); // e.g. L"JSON\0*.json\0"

enum class PathMode { Folder, File, FileFiltered };

bool PathInput(
    const char* label,
    char* buf,
    size_t bufSize,
    PathMode mode = PathMode::Folder,
    const wchar_t* filter = nullptr   // e.g. L"Config Files\0*.json\0All Files\0*.*\0"
);
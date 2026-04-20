#pragma once
#include <string>
#include <windows.h>

void SetPickerOwner(HWND hwnd);

std::string PickFolder();
std::string PickFile(const wchar_t* filter = nullptr);

enum class PathMode { Folder, File, FileFiltered };

bool PathInput(
    const char* label,
    char* buf,
    size_t bufSize,
    PathMode mode = PathMode::Folder,
    const wchar_t* filter = nullptr
);
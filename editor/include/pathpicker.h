#pragma once

#include <string>
#include <windows.h>

enum class PathMode
{ 
    Folder, 
    File, 
    FileFiltered
};

void SetPickerOwner(HWND hwnd);

std::string PickFolder();
std::string PickFile(const wchar_t* filter = nullptr);

/// <summary>
/// GUI input for a path, using win api to open up explorer for visual path selection
/// </summary>
/// <param name="label">GUI label</param>
/// <param name="buf">Output/display path buffer</param>
/// <param name="bufSize">Size (capacity) of the buffer</param>
/// <param name="mode">What to input (folder/file/file filtered)</param>
/// <param name="filter">(optional) if mode is file filtered, what filter to apply</param>
/// <returns></returns>
bool PathInput(
    const char* label,
    char* buf,
    size_t bufSize,
    PathMode mode = PathMode::Folder,
    const wchar_t* filter = nullptr
);
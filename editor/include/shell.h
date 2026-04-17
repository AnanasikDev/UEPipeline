#pragma once

#include <windows.h>
#include <string>

void RunPS1(const std::string& scriptPath)
{
    std::string cmd = "powershell -ExecutionPolicy Bypass -File \"" + scriptPath + "\"";

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // no black console window flashing

    CreateProcessA(
        nullptr, cmd.data(),
        nullptr, nullptr,
        FALSE, 0,
        nullptr, nullptr,
        &si, &pi
    );

    WaitForSingleObject(pi.hProcess, INFINITE); // remove this line to not block
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
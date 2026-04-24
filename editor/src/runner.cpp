#include "runner.h"
#include <cstdio>
#include "app.h"
#include <sstream>
#include <filesystem>
#include "config.h"

namespace fs = std::filesystem;

void Runner::Run(const std::string& cmd, Console& console)
{
    if (running.load())
    {
        console.PrintError("[runner] Already running a command.");
        return;
    }

    if (worker.joinable())
        worker.join();

    running = true;

    worker = std::thread([this, cmd, &console]()
    {
        console.Print("> " + cmd);

        SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
        HANDLE readPipe = nullptr, writePipe = nullptr;
        if (!CreatePipe(&readPipe, &writePipe, &sa, 0))
        {
            console.PrintError("[runner] Failed to create pipe.");
            running = false;
            return;
        }
        SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si = { sizeof(si) };
        si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        si.hStdOutput = writePipe;
        si.hStdError = writePipe;

        PROCESS_INFORMATION pi = {};
        std::string fullCmd = "powershell -NoProfile -ExecutionPolicy Bypass " + cmd;
        console.Print("[runner] Executing: " + fullCmd);

        if (!CreateProcessA(nullptr, fullCmd.data(), nullptr, nullptr,
                            TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        {
            console.PrintError("[runner] Failed to start process.");
            CloseHandle(readPipe);
            CloseHandle(writePipe);
            running = false;
            return;
        }
        CloseHandle(writePipe);

        // Store process handle so Stop() can kill it
        {
            std::lock_guard<std::mutex> lock(processMutex);
            hProcess = pi.hProcess;
        }
        CloseHandle(pi.hThread);

        char buf[512];
        DWORD bytesRead;
        std::string lineBuffer;
        while (ReadFile(readPipe, buf, sizeof(buf) - 1, &bytesRead, nullptr) && bytesRead > 0)
        {
            buf[bytesRead] = '\0';
            lineBuffer += buf;
            size_t pos;
            while ((pos = lineBuffer.find('\n')) != std::string::npos)
            {
                std::string line = lineBuffer.substr(0, pos);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                console.Print(line);
                lineBuffer.erase(0, pos + 1);
            }
        }
        if (!lineBuffer.empty()) console.Print(lineBuffer);

        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        {
            std::lock_guard<std::mutex> lock(processMutex);
            CloseHandle(hProcess);
            hProcess = nullptr;
        }
        CloseHandle(readPipe);

        if (exitCode != 0)
            console.PrintError("[runner] Exited with code " + std::to_string(exitCode));
        else
            console.Print("[runner] Done.");

        running = false;
    });

    worker.detach();
}

void Runner::Stop(Console& console)
{
    std::lock_guard<std::mutex> lock(processMutex);
    if (hProcess)
    {
        // Kill the entire process tree
        DWORD pid = GetProcessId(hProcess);
        std::string killCmd = "taskkill /F /T /PID " + std::to_string(pid);

        STARTUPINFOA si = { sizeof(si) };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi = {};

        if (CreateProcessA(nullptr, killCmd.data(), nullptr, nullptr,
                           FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        {
            WaitForSingleObject(pi.hProcess, 5000);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }

        console.PrintError("[runner] Process killed by user.");
    }
}

Command Runner::BuildCommand(const App& app)
{
    std::string passScriptPath = (fs::path(app.config.paths.GetScriptsDir().path) / "stage2_build.ps1").string();
    std::string passProjectFile = app.config.paths.GetProjectFile().path;
    std::string passOutputDir = (fs::path(app.config.paths.buildOutput)).string();
    std::string passConfig = app.config.GetCurrentBuildConfigString();

    std::stringstream args;
    args << " -UnrealRoot \"" << app.config.paths.unrealRoot << "\"";
    args << " -ProjectPath \"" << passProjectFile << "\"";
    args << " -OutputDir \"" << passOutputDir << "\"";
    args << " -Config " << passConfig;

    return Command{ passScriptPath, args.str() };
}

void Runner::RunFile(const Command& command, Console& console)
{
    Run("-File \"" + command.script + "\" " + command.args, console);
}

void Runner::RunCommand(const std::string& expression, Console& console)
{
    Run("-Command \"" + expression + "\"", console);
}
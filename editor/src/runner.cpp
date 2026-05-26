#include <cstdio>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <string>

#include "runner.h"
#include "app.h"
#include "config.h"
#include "strutils.h"

namespace fs = std::filesystem;

Runner::Runner()
{
    hJob = CreateJobObject(nullptr, nullptr); // default security descriptor, no name
    if (hJob)
    {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &info, sizeof(info));
    }
}

Runner::~Runner()
{
    if (hJob)
    {
        constexpr UINT exitcode = 1;
        TerminateJobObject(hJob, exitcode);
        CloseHandle(hJob);
    }
    if (worker.joinable())
    {
        worker.join();
    }
    Cleanup();
}

void Runner::Cleanup()
{
    if (hProcess)
    { 
        CloseHandle(hProcess); 
        hProcess = nullptr;
    }

    if (hReadPipe)
    { 
        CloseHandle(hReadPipe);
        hReadPipe = nullptr;
    }
}

void Runner::AddRun(std::function<void(void)> fun)
{
    queue.push(fun);
}

void Runner::Run(const std::string& cmd, Console& console, CmdOutputFilter filter, CmdTick tick, CmdOnFinish finish)
{
    if (running.load())
    {
        console.Print("[runner] Already running a command.", Console::Result::Error);
        return;
    }

    if (worker.joinable())
    {
        worker.join();
    }

    Cleanup();
    running = true;
    abortRequested = false;
    abortCode = Console::Result::None;

    worker = std::thread([this, cmd, &console, filter, tick, finish]()
    {
        // 
        // here we create a process for our (windowless) command line,
        // add it to the Job Object (process tree),
        // let it run: we intercept the output via the readpipe
        //      until it cannot be read anymore due to process exit or abortion.
        // Finally we cleanup all handles and pipes

        console.Print("> " + cmd);

        SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
        HANDLE hReadLocal = nullptr;
        HANDLE hWritePipe = nullptr;

        static std::atomic<int> pipeCounter{ 0 };
        char pipeName[128];
        snprintf(pipeName, sizeof(pipeName),
            R"(\\.\pipe\runner_%lu_%d)", GetCurrentProcessId(), pipeCounter++);

        hReadLocal = CreateNamedPipeA(
            pipeName,
            PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,  // overlapped read end
            PIPE_TYPE_BYTE | PIPE_WAIT,
            1, 4096, 4096, 0, &sa);

        hWritePipe = CreateFileA(
            pipeName,
            GENERIC_WRITE,
            0, &sa,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,  // write end stays synchronous, that's fine
            nullptr);

        SetHandleInformation(hReadLocal, HANDLE_FLAG_INHERIT, 0);

        if (!hReadLocal || !hWritePipe)
        {
            console.Print("[runner] Failed to create pipe.", Console::Result::Error);
            running = false;
            return;
        }

        STARTUPINFOA si = { sizeof(si) };
        si.dwFlags = 
            STARTF_USESTDHANDLES | // The hStdInput, hStdOutput, and hStdError members contain additional information. 
            STARTF_USESHOWWINDOW ; // The wShowWindow member contains additional information.  
        si.wShowWindow = SW_HIDE;
        si.hStdOutput = hWritePipe;
        si.hStdError  = hWritePipe;
        si.hStdInput  = nullptr;

        PROCESS_INFORMATION pi = {};
        std::string fullCmd = cmd;
        console.Print("[runner] Executing: " + fullCmd);

        // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa
        BOOL processCreated = CreateProcessA(
            nullptr,         // Application name: For flexibility and ease of use, I embed the app name into the command, hence specify app name as null
            fullCmd.data(),  // Command line: The command line to be executed (max 32,767 characters)
            nullptr,         // Process attributes: Process cannot be inherited
            nullptr,         // Thread attributes: Handle cannot be inherited, default security descriptor 
            TRUE,            // All handles created by the process are inherited
            CREATE_NO_WINDOW |
            CREATE_SUSPENDED , 
            nullptr,         // Use environment of the current process
            nullptr,         // Use the same drive and directory as the calling application
            &si,
            &pi);

        CloseHandle(hWritePipe);

        if (!processCreated)
        {
            console.Print("[runner] Failed to start process.", Console::Result::Error);
            CloseHandle(hReadPipe);
            running = false;
            return;
        }

        {
            std::lock_guard<std::mutex> lock(processMutex);
            hProcess = pi.hProcess;
            hReadPipe = hReadLocal;
            if (hJob)
            {
                AssignProcessToJobObject(hJob, pi.hProcess); // assign process to the job tree
            }
        }
        ResumeThread(pi.hThread);
        CloseHandle(pi.hThread);

        constexpr DWORD TICK_MILLISECONDS = 500; // 500ms tick
        constexpr int CHUNK_SIZE = 512;
        char buf[CHUNK_SIZE];
        std::string lineBuffer;
        
        OVERLAPPED ov = {};
        ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

        bool dotick = true;
        bool stop = false;

        while (!abortRequested.load() && !stop)
        {
            ResetEvent(ov.hEvent);
            BOOL readOk = ReadFile(hReadLocal, buf, sizeof(buf) - 1, nullptr, &ov);
            if (!readOk && GetLastError() != ERROR_IO_PENDING || abortRequested.load())
            {
                abortCode = Console::Result::Critical;
                break;  // pipe closed or cannot read input, process exited or was killed
            }

            while (dotick) // tick loop
            {
                DWORD waitResult = WaitForSingleObject(ov.hEvent, TICK_MILLISECONDS);
                if (waitResult == WAIT_OBJECT_0)
                {
                    DWORD bytesRead = 0;
                    GetOverlappedResult(hReadLocal, &ov, &bytesRead, FALSE);
                    if (bytesRead == 0 || abortRequested.load())
                    {
                        stop = true;
                        break;
                    }
                    buf[bytesRead] = '\0';
                    lineBuffer += buf;
                    size_t pos;
                    while ((pos = lineBuffer.find('\n')) != std::string::npos)
                    {
                        std::string line = lineBuffer.substr(0, pos);
                        std::string checkline = line;
                        trim(checkline);
                        tolower(checkline);

                        if (!line.empty() && line.back() == '\r')
                        {
                            line.pop_back();
                        }

                        Console::Result result = Console::Result::None;
                        if (filter)
                        {
                            result = filter(line);
                            if (result == Console::Result::Critical)
                            {
                                Stop(console, result);
                                dotick = false;
                                stop = true;
                                break;
                            }
                            if (result == Console::Result::Success)
                            {
                                Stop(console, result);
                                dotick = false;
                                stop = true;
                                break;
                            }
                        }

                        Console::Result lineResult = console.GetConsoleResult(checkline);
                        console.Print(line, lineResult);

                        lineBuffer.erase(0, pos + 1);
                    }
                    break;
                }
                else if (waitResult == WAIT_TIMEOUT)
                {
                    if (tick) tick(hProcess);

                    if (abortRequested.load())
                    {
                        CancelIo(hReadLocal);
                        stop = true;
                        break;
                    }
                }
                else
                {
                    break; // error
                }
            }
        }
        CloseHandle(ov.hEvent);
        if (!lineBuffer.empty() && !abortRequested.load())
        {
            if (lineBuffer.back() == '\r') lineBuffer.pop_back();
            console.Print(lineBuffer);
        }

        // wait for a signal from hProcess for 2 seconds, then timeout
        constexpr DWORD signalTimeout = 2000;
        WaitForSingleObject(pi.hProcess, signalTimeout);
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        {
            std::lock_guard<std::mutex> lock(processMutex);
            Cleanup();
        }

        if (abortRequested.load())
        {
            switch (abortCode)
            {
                case Console::Result::Critical:
                {
                    console.Print("[runner] Aborted by user.", Console::Result::Critical);
                } break;
                case Console::Result::Success:
                {
                    console.Print("[runner] Exited early, pass.", Console::Result::Success);
                } break;
                default:
                {
                    console.Print("[runner] Exit for unknown reason.", Console::Result::Critical);
                }
            }
        }
        else if (exitCode != 0)
        {
            console.Print("[runner] Exited with code " + std::to_string(exitCode), Console::Result::Error);
        }
        else
        {
            console.Print("[runner] Done.", Console::Result::Success);
        }

        running = false;

        if (finish != nullptr)
        {
            Console::Result result = abortCode.load();
            queue.push([finish, result]() -> void { finish(result); });
        }
    });
}

bool Runner::Tick()
{
    if (running)
    {
        return false;
    }

    if (queue.empty())
    {
        return false;
    }

    std::function<void(void)> cmd = queue.front();
    queue.pop();
    if (cmd != nullptr)
    {
        cmd();
    }
}

void Runner::Stop(Console& console, Console::Result code)
{
    if (!running.load())
    {
        return;
    }

    abortRequested = true;
    abortCode = code;

    std::lock_guard<std::mutex> lock(processMutex);

    if (hJob)
    {
        // use the Windows Job Object to kill the entire process tree
        // this is why I am not using process handles: they can generate
        // other processes which are impossible to delete
        TerminateJobObject(hJob, 1);
    }

    if (hReadPipe)
    {
        CloseHandle(hReadPipe);
        hReadPipe = nullptr;
    }
}

Command Runner::BuildCommand_CodeAnalysis_Prep(const App& app, std::string target, std::string platform, std::string config)
{
    std::string passUBT = (fs::path(app.config.paths.unrealRoot) / "Engine" / "Build" / "BatchFiles" / "Build.bat").string();
    std::string passProjectFile = app.config.paths.uprojectPath;

    std::stringstream args;
    args << " " << target << " " << platform << " " << config << " ";
    args << " -project=\"" << passProjectFile << "\"";
    args << " -mode=GenerateClangDatabase";

    return Command(passUBT, args.str(), Command::Type::OtherCommand);
}

Command Runner::BuildCommand_CodeAnalysis(const App& app)
{
    std::string passScriptPath = (fs::path(app.config.paths.GetScriptsDir()) / "clang_tidy.ps1").string();
    std::string passProjectFile = app.config.paths.sourcePath;
    std::string passCompileCommands = (fs::path(app.config.paths.unrealRoot) / "compile_commands.json").string();

    std::stringstream args;
    args << " -SourceDir \"" << passProjectFile << "\"";
    args << " --CompileCommandsJson \"" << passCompileCommands << "\"";

    return Command(passScriptPath, args.str(), Command::Type::PowerShellScript);
}

Command Runner::BuildCommand_Verify(const App& app)
{
    std::string passScriptPath = (fs::path(app.config.paths.GetScriptsDir()) / "stage_validate.ps1").string();
    std::string passProjectFile = app.config.paths.uprojectPath;

    std::stringstream args;
    args << " -ProjectFilePath \"" << passProjectFile << "\"";
    args << " -UnrealRoot \"" << app.config.paths.unrealRoot << "\"";

    return Command( passScriptPath, args.str(), Command::Type::PowerShellScript );
}

Command Runner::BuildCommand_BootflowTest(const App& app)
{
    std::string gameExePath = std::string(app.config.paths.buildExePath);

    std::stringstream args;
    args << " -log";

    return Command( gameExePath, args.str(), Command::Type::OtherCommand );
}

bool IsResponding(HWND hwnd)
{
    return IsHungAppWindow(hwnd) == FALSE;
}

struct FindWindowData
{
    DWORD pid;
    HWND  result;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto* data = reinterpret_cast<FindWindowData*>(lParam);
    DWORD windowPid;
    GetWindowThreadProcessId(hwnd, &windowPid);

    if (windowPid == data->pid && IsWindowVisible(hwnd))
    {
        data->result = hwnd;
        return FALSE; // stop enumerating
    }
    return TRUE;
}

HWND FindMainWindow(DWORD pid)
{
    FindWindowData data{ pid, nullptr };
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
    return data.result;
}

void Runner::Run_BootflowTest(App& app, std::string command)
{
    CmdOutputFilter filter = [](const std::string& input) -> Console::Result
        {
            std::string line = input;
            std::transform(line.begin(), line.end(), line.begin(), [](unsigned char c) { return std::tolower(c); });

            if (line.ends_with("game engine initialized.") ||
                line.ends_with("starting game."))
            {
                return Console::Result::Success;
            }
            return Console::Result::None;
        };

    CmdTick tick = [](HANDLE hProcess) -> Console::Result
        {
            DWORD pid = GetProcessId(hProcess);
            HWND hwnd = FindMainWindow(pid);

            if (!hwnd)
            {
                // app hasn't created a visible window yet
            }
            else if (IsHungAppWindow(hwnd))
            {
                // window exists but app is frozen
                return Console::Result::Critical;
            }
            else
            {
                // app is alive and responding
            }
            return Console::Result::None;
        };

    app.runner.Run(command, app.console, filter, tick);
}

void Runner::Run_Verify(App& app, std::string command)
{
    app.runner.Run(command, app.console);
}

void Runner::Run_CodeAnalysis_Prep(App& app, std::string command)
{
    app.runner.Run(command, app.console);
}

void Runner::Run_CodeAnalysis(App& app, std::string command)
{
    app.runner.Run(command, app.console);
}

Command Runner::BuildCommand_Package(const App& app)
{
    std::string passScriptPath = (fs::path(app.config.paths.GetScriptsDir()) / "stage2_build.ps1").string();
    std::string passProjectFile = app.config.paths.uprojectPath;
    std::string passOutputDir = (fs::path(app.config.paths.buildOutput)).string();
    std::string passConfig = app.config.GetCurrentBuildConfigString();

    std::stringstream args;
    args << " -UnrealRoot \"" << app.config.paths.unrealRoot << "\"";
    args << " -ProjectPath \"" << passProjectFile << "\"";
    args << " -OutputDir \"" << passOutputDir << "\"";
    args << " -Config " << passConfig;

    return Command( passScriptPath, args.str(), Command::Type::PowerShellScript );
}
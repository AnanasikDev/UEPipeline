#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <windows.h>
#include <functional>
#include <queue>

#include "console.h"
#include "command.h"

class App;

class Runner
{
public:
    using CmdOutputFilter = std::function<Console::Result(const std::string&)>;
    using CmdTick = std::function<Console::Result(HANDLE)>;
    using CmdOnFinish = std::function<void(Console::Result)>;

    Runner();
    ~Runner();

    /// <summary>
    /// Runs the specified command in powershell with optional callback functions.
    /// </summary>
    /// <param name="cmd">full command to run</param>
    /// <param name="console">console to use to print to</param>
    /// <param name="filter">(optional) per-line filtering function to analyze and modify outputs on fly</param>
    /// <param name="tick">(optional) additional background function that runs on heartbeat when waiting for the command response</param>
    /// <param name="finish">(optional) run additional command after finishing this command. Prefer to use `AddRun` instead</param>
    void Run(const std::string& cmd, Console& console, CmdOutputFilter filter = nullptr, CmdTick tick = nullptr, CmdOnFinish finish = nullptr);

    /// <summary>
    /// Adds the specified function to the queue to run as soon as possible without overlapping.
    /// </summary>
    void AddRun(std::function<void(void)> fun);

    /// <summary>
    /// Sends request to terminate currently executing command and quit forcibly.
    /// </summary>
    /// <param name="console">Console to use</param>
    /// <param name="code">(optional) code to terminate with</param>
    void Stop(Console& console, Console::Result code = Console::Result::None);
    
    inline bool IsRunning() const
    {
        return running.load();
    }

    /// <summary>
    /// Closes Windows handles
    /// </summary>
    void Cleanup();

    /// <summary>
    /// Runs every frame, tried to run commands from the queue.
    /// </summary>
    /// <returns></returns>
    bool Tick();

    static Command BuildCommand_Package(const App& app);
    static Command BuildCommand_Verify(const App& app);
    static Command BuildCommand_CodeAnalysis_Prep(const App& app, std::string target, std::string platform, std::string config);
    static Command BuildCommand_CodeAnalysis(const App& app);
    static Command BuildCommand_BootflowTest(const App& app);

    void Run_BootflowTest(App& app, std::string command);
    void Run_Verify(App& app, std::string command);
    void Run_CodeAnalysis(App& app, std::string command);
    void Run_CodeAnalysis_Prep(App& app, std::string command);

    /// <summary>
    /// Takes a Command object and makes it valid for the integrated powershell to process without a popup terminal and with correct tags. Combines everything together to make a string.
    /// </summary>
    /// <returns>Runnable command as a string</returns>
    static inline std::string MakeCommandHeadless(const Command& cmd)
    {
        std::string type = cmd.type == Command::Type::PowerShellScript ? "-File" : "-Command";
        std::string fullCmd = std::string("powershell -NoProfile -ExecutionPolicy Bypass ") + type + " \"" + cmd.script + "\"" + cmd.args;
        return fullCmd;
    }

private:
    /// <summary>
    /// The only additional thread for running commands. Can only run one command at a time.
    /// </summary>
    std::thread       worker;
    std::atomic<bool> running = false;
    std::atomic<Console::Result> abortCode = Console::Result::None;
    std::atomic<bool> abortRequested = false;
    std::mutex        processMutex;

    HANDLE            hJob = nullptr; // Windows Job Object - a tree of processes for easy recursive kill
    HANDLE            hProcess = nullptr;
    HANDLE            hReadPipe = nullptr;

    /// <summary>
    /// FIFO Queue of commands to run, usually as lambdas that call Run
    /// </summary>
    std::queue<std::function<void(void)>> queue;
};
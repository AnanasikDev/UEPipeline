#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <windows.h>
#include "console.h"

class App;

struct Command
{
    std::string script;
    std::string args;
};

class Runner
{
public:
    void Run(const std::string& cmd, Console& console);
    void RunFile(const Command& command, Console& console);
    void RunCommand(const std::string& expression, Console& console);
    void Stop(Console& console);
    bool IsRunning() const { return running.load(); }
    Command BuildCommand(const App& app);

    Command command;

private:
    std::thread       worker;
    std::atomic<bool> running = false;
    HANDLE            hProcess = nullptr;
    std::mutex        processMutex;
};
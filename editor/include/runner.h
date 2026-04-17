#pragma once
#include <string>
#include <thread>
#include <atomic>
#include "console.h"

struct Command
{
    std::string script;
    std::string args;
};

struct Runner
{
    // Non-blocking — spawns a thread, streams output into console
    void Run(const std::string& cmd, Console& console);
    bool IsRunning() const { return running.load(); }
    void RunFile(const Command& command, Console& console);
    void RunCommand(const std::string& expression, Console& console);

private:
    std::thread     worker;
    std::atomic<bool> running = false;
};
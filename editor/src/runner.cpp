#include "runner.h"
#include <cstdio>

void Runner::Run(const std::string& cmd, Console& console)
{
    if (running.load())
    {
        console.PrintError("[runner] Already running a command.");
        return;
    }

    // Detach previous thread if done
    if (worker.joinable())
        worker.join();

    running = true;

    worker = std::thread([this, cmd, &console]()
    {
        console.Print("> " + cmd);

        std::string fullCmd = "powershell -NoProfile -ExecutionPolicy Bypass " + cmd + " 2>&1";
        console.Print("[runner] Executing: " + fullCmd);
        FILE* pipe = _popen(fullCmd.c_str(), "r");

        if (!pipe)
        {
            console.PrintError("[runner] Failed to open pipe.");
            running = false;
            return;
        }

        char buf[512];
        while (fgets(buf, sizeof(buf), pipe))
        {
            std::string line(buf);
            // Strip trailing newline
            if (!line.empty() && line.back() == '\n') line.pop_back();
            if (!line.empty() && line.back() == '\r') line.pop_back();
            console.Print(line);
        }

        int exitCode = _pclose(pipe);
        if (exitCode != 0)
            console.PrintError("[runner] Exited with code " + std::to_string(exitCode));
        else
            console.Print("[runner] Done.");

        running = false;
    });

    worker.detach();
}

void Runner::RunFile(const Command& command, Console& console)
{
    Run("-File \"" + command.script + "\" " + command.args, console);
}

void Runner::RunCommand(const std::string& expression, Console& console)
{
    Run("-Command \"" + expression + "\"", console);
}
#pragma once

#include "console.h"
#include "runner.h"
#include "config.h"

class GLFWwindow;

class App
{
public:
    int Init();
    void Exit();
    void Run();

    UserConfig config;

private:
    GLFWwindow* window;
    Console console;
    Runner  runner;
    bool    showConsole = true;

    void Tick();
    void SetupImGui();
    void PreRender();
    void Render();
    void PostRender();
};
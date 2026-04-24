#pragma once

#include "console.h"
#include "runner.h"
#include "config.h"
#include "pipeline.h"

class GLFWwindow;

class App
{
public:
    App();
    int Init();
    void Exit();
    void Run();

    UserConfig config;

private:
    GLFWwindow* window;
    Console console;
    Runner  runner;
    Pipeline pipeline;
    bool    showConsole = true;

    void Tick();
    void SetupImGui();
    void PreRender();
    void Render();
    void PostRender();

    void RenderStagePrepare (bool& dirty);
    void RenderStageVerify  (bool& dirty);
    void RenderStagePackage (bool& dirty);
    void RenderStageArchive (bool& dirty);
    void RenderStageDeploy  (bool& dirty);
};
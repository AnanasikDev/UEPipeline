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
    Console console;
    Runner  runner;
    Pipeline pipeline;

private:
    static constexpr int WINDOW_WIDTH{ 1200 };
    static constexpr int WINDOW_HEIGHT{ 700 };
    const char WINDOW_TITLE[14] = "Pipeline Tool";

    GLFWwindow* window = nullptr;
    bool    showConsole = true;

    /// <summary>
    /// Main tick event loop
    /// </summary>
    void Tick();
    
    /// <summary>
    /// Manages all of imgui setup, including creating context, initializing backend, applying theme
    /// </summary>
    void SetupImGui();
    
    /// <summary>
    /// Preparations for the rendering stage: poll events, make new frame
    /// </summary>
    void PreRender();
    
    /// <summary>
    /// Render the app GUI, read GUI interactions in immediate mode
    /// </summary>
    void Render();

    /// <summary>
    /// Finalize frame, swap buffers, send to the screen
    /// </summary>
    void PostRender();

    void RenderStagePrepare (bool& dirty);
    void RenderStageVerify  (bool& dirty);
    void RenderStagePackage (bool& dirty);
    void RenderStageTest    (bool& dirty);
    void RenderStageDeploy  (bool& dirty);

    /// <summary>
    /// Runs path detection and fills found values into the config, and prints them into the console.
    /// </summary>
    void AutoFill();
};
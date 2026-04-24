#include "gl.h"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <windows.h>
#include <shobjidl.h>
#include <string>
#include <iostream>
#include "app.h"
#include "pathpicker.h"
#include "shell.h"
#include "theme.h"
#include <sstream>
#include <vector>
#include <algorithm>
#include "stb_image.h"
#include <imgui_internal.h>
#include "pipeline.h"

App::App() : pipeline(runner)
{
}

static float fontScale = Theme::FontScaleDefault;

static void ZoomIn()
{
    fontScale += Theme::FontScaleStep;
    if (fontScale > Theme::FontScaleMax) fontScale = Theme::FontScaleMax;
    ImGui::GetIO().FontGlobalScale = fontScale;
}

static void ZoomOut()
{
    fontScale -= Theme::FontScaleStep;
    if (fontScale < Theme::FontScaleMin) fontScale = Theme::FontScaleMin;
    ImGui::GetIO().FontGlobalScale = fontScale;
}

static void ZoomReset()
{
    fontScale = Theme::FontScaleDefault;
    ImGui::GetIO().FontGlobalScale = fontScale;
}

void App::SetupImGui()
{
    Theme::Apply();
}

int App::Init()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

    window = glfwCreateWindow(1200, 700, "Pipeline Tool", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    SetPickerOwner(glfwGetWin32Window(window));
    glfwSwapInterval(1);

    if (!gladLoaderLoadGL())
    {
        std::cout << "Failed to initialize OpenGL\n";
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Load font at base size — zoom is handled via FontGlobalScale
    ImFontConfig fontCfg;
    fontCfg.OversampleH = 3;
    fontCfg.OversampleV = 2;
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", Theme::FontSizeBase, &fontCfg);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    SetupImGui();
    config.Load(*this);
    pipeline.Init();

    // Apply loaded zoom level
    io.FontGlobalScale = fontScale;

    return 0;
}

// ---------- Render ----------

void App::Render()
{
    ImGuiIO& io = ImGui::GetIO();

    // --- Zoom: Ctrl+ / Ctrl- / Ctrl0 ---
    if (io.KeyCtrl)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Equal))       ZoomIn();     // Ctrl+=  (+ key)
        if (ImGui::IsKeyPressed(ImGuiKey_Minus))       ZoomOut();    // Ctrl+-
        if (ImGui::IsKeyPressed(ImGuiKey_0))           ZoomReset();  // Ctrl+0
        if (ImGui::IsKeyPressed(ImGuiKey_KeypadAdd))   ZoomIn();
        if (ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract)) ZoomOut();
    }
    // Ctrl+scroll wheel zoom
    if (io.KeyCtrl && io.MouseWheel != 0.0f)
    {
        fontScale += io.MouseWheel * Theme::FontScaleStep;
        if (fontScale < Theme::FontScaleMin) fontScale = Theme::FontScaleMin;
        if (fontScale > Theme::FontScaleMax) fontScale = Theme::FontScaleMax;
        io.FontGlobalScale = fontScale;
    }

    // --- Dockspace ---
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin("##dockspace", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoDocking);

    ImGuiID dockID = ImGui::GetID("MainDock");

    // Build default layout once
    static bool firstTime = true;
    if (firstTime)
    {
        firstTime = false;

        ImGui::DockBuilderRemoveNode(dockID);
        ImGui::DockBuilderAddNode(dockID, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockID, io.DisplaySize);

        // Split: left 66%, right 33%
        ImGuiID dockLeft, dockRight;
        ImGui::DockBuilderSplitNode(dockID, ImGuiDir_Left, 0.66f, &dockLeft, &dockRight);

        // Assign windows to docks — names must match the strings in Begin()
        ImGui::DockBuilderDockWindow("Pipeline", dockLeft);
        ImGui::DockBuilderDockWindow("Console", dockRight);

        ImGui::DockBuilderFinish(dockID);
    }

    ImGui::DockSpace(dockID, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    // Main panel
    ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_FirstUseEver);
    ImGui::Begin("Pipeline");
    // Header
    ImGui::SetWindowFontScale(Theme::FontHeaderScale);
    ImGui::TextColored(Theme::TextPrimary, "Pipeline Tool");
    // Zoom indicator
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80.0f);
    ImGui::TextColored(Theme::TextDisabled, "Zoom: %d%%", (int)(fontScale * 100.0f + 0.5f));
    ImGui::TextColored(Theme::TextSecondary, "Configure and run your build pipeline.");

    int stage = pipeline.RenderPipe();
    // if editing, use clicking to switch settings
    if (stage != -1 && pipeline.status == Pipeline::PipelineStatus::Idle)
    {
        pipeline.stageEditIndex = stage;
    }

    ImGui::Spacing();
    ImGui::Spacing();

    bool dirty = false;

    ImVec2 stageGroupStart = pipeline.PreRenderStage();

    switch (pipeline.stageEditIndex)
    {
        case Pipeline::INDEX_PREPARE: RenderStagePrepare(dirty); break;
        case Pipeline::INDEX_VERIFY:  RenderStageVerify(dirty); break;
        case Pipeline::INDEX_PACKAGE: RenderStagePackage(dirty); break;
        case Pipeline::INDEX_ARCHIVE: RenderStageArchive(dirty); break;
        case Pipeline::INDEX_DEPLOY:  RenderStageDeploy(dirty); break;
    }

    pipeline.PostRenderStage(stageGroupStart);

    // --- Path inputs section ---
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::BgMid);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Unreal Engine
    if (ImGui::Button("Auto-detect"))
    {
        config.paths.AutoDetectUnreal();
        dirty = true;
    }
    ImGui::SameLine();
    dirty |= PathInput("Unreal Engine", config.paths.unrealRoot, sizeof(config.paths.unrealRoot), PathMode::Folder);
    ImGui::Spacing();

    // Workspace
    dirty |= PathInput("P4 Project Path", config.paths.projectRootPath, sizeof(config.paths.projectRootPath), PathMode::Folder);
    ImGui::Spacing();

    // Derived paths
    Paths::Path derivedProject = config.paths.GetProjectFile();
    Paths::Path derivedScriptsPath = config.paths.GetScriptsDir();

    bool projectExists = config.paths.projectRootPath[0] != '\0' && derivedProject.exists;
    bool scriptsExist = config.paths.projectRootPath[0] != '\0' && derivedProject.exists;

    ImGui::SetWindowFontScale(Theme::FontSubheaderScale);
    ImGui::TextColored(Theme::TextSecondary, "Derived .uproject:");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine();
    if (config.paths.projectRootPath[0] != '\0')
    {
        if (projectExists)
            ImGui::TextColored(Theme::Success, "%s", derivedProject.path.c_str());
        else
            ImGui::TextColored(Theme::Error, "%s  (not found)", derivedProject.path.c_str());
    }
    else
    {
        ImGui::TextColored(Theme::TextDisabled, "(set workspace path)");
    }

    ImGui::SetWindowFontScale(Theme::FontSubheaderScale);
    ImGui::TextColored(Theme::TextSecondary, "Derived scripts dir:");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine();
    if (config.paths.projectRootPath[0] != '\0')
    {
        if (scriptsExist)
            ImGui::TextColored(Theme::Success, "%s", derivedScriptsPath.path.c_str());
        else
            ImGui::TextColored(Theme::Error, "%s  (not found)", derivedScriptsPath.path.c_str());
    }
    else
    {
        ImGui::TextColored(Theme::TextDisabled, "(set workspace path)");
    }

    ImGui::Spacing();
    ImGui::PopStyleColor();

    // --- Command preview ---
    ImGui::Spacing();
    ImGui::TextColored(Theme::TextSecondary, "Command parsed:");
    std::string output = "-File \"" + runner.command.script + "\" " + runner.command.args;
    static char cmdPreview[2048];
    strncpy_s(cmdPreview, output.c_str(), sizeof(cmdPreview) - 1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##cmdpreview", cmdPreview, sizeof(cmdPreview), ImGuiInputTextFlags_ReadOnly);
    ImGui::Spacing();

    // --- Buttons ---
    if (runner.IsRunning())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::StopButton);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::StopButtonHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::StopButtonActive);
        if (ImGui::Button("Stop", Theme::ButtonMain))
        {
            runner.Stop(console);
        }
        ImGui::PopStyleColor(3);
    }
    else
    {
        if (ImGui::Button("Run Pipeline", Theme::ButtonMain))
        {
            runner.RunFile(runner.command, console);
        }

        ImGui::SameLine();

        if (ImGui::Button("Save Settings", Theme::ButtonSecondary))
        {
            config.Save(*this);
        }

        ImGui::SameLine();

        if (config.paths.buildOutput[0] != '\0')
        {
            ImGui::SameLine();
            if (ImGui::Button("Open output directory", Theme::ButtonMain))
            {
                ShellExecuteA(nullptr, "explore", config.paths.buildOutput, nullptr, nullptr, SW_SHOWNORMAL);
            }
        }
    }

    ImGui::End();

    // --- Console ---
    console.Draw("Console", &showConsole);

    if (!showConsole)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::MenuItem("Console")) showConsole = true;
            ImGui::EndMainMenuBar();
        }
    }
}

void App::Tick()
{
    PreRender();
    Render();
    PostRender();
}
void App::PreRender()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
void App::PostRender()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

void App::Exit()
{
    config.Save(*this);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
}
void App::Run()
{
    while (!glfwWindowShouldClose(window))
    {
        Tick();
    }
}

void App::RenderStagePrepare(bool& dirty)
{

}

void App::RenderStageVerify(bool& dirty)
{

}

void App::RenderStagePackage(bool& dirty)
{
    // Build output
    dirty |= PathInput("Build Output Folder", config.paths.buildOutput, sizeof(config.paths.buildOutput), PathMode::Folder);
    ImGui::Spacing();

    // Config combo
    {
        int buildConfig = static_cast<int>(config.buildConfig);
        if (ImGui::Combo("Configuration", &buildConfig, config.BuildConfigs, sizeof(config.BuildConfigs) / sizeof(config.BuildConfigs[0])))
        {
            dirty = true;
            config.buildConfig = static_cast<UserConfig::BuildConfig>(buildConfig);
        }
    }

    if (dirty)
    {
        runner.command = runner.BuildCommand(*this);
    }

    ImGui::Spacing();
}

void App::RenderStageArchive(bool& dirty)
{

}

void App::RenderStageDeploy(bool& dirty)
{

}

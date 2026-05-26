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
#include "theme.h"
#include <sstream>
#include <vector>
#include <algorithm>
#include "stb_image.h"
#include <imgui_internal.h>
#include "pipeline.h"
#include <filesystem>
#include "imgui_utils.h"

namespace fs = std::filesystem;

App::App() : pipeline(runner)
{
}


void App::SetupImGui()
{
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

    Theme::Apply();

    // Apply loaded zoom level
    io.FontGlobalScale = UserConfig::fontScale;
}

int App::Init()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (!glfwInit())
    {
        return -1;
    }

    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    SetPickerOwner(glfwGetWin32Window(window));
    glfwSwapInterval(1);

    if (!gladLoaderLoadGL())
    {
        std::cout << "Failed to initialize OpenGL\n";
        return -1;
    }

    if (config.autoLoadSave)
    {
        config.Load(*this);
    }
    SetupImGui();
    pipeline.Init();

    return 0;
}

void App::AutoFill()
{
    Paths::FileStructure structure = config.paths.AnalyzeStructure();
    console.Print("name: " + structure.name);
    console.Print("p4root: " + structure.p4root.string());
    console.Print("project root: " + structure.projectRoot.string());
    console.Print("scripts: " + structure.scripts.string());
    console.Print(".uproject: " + structure.uproject.string());
    console.Print("source: " + structure.source.string());
    console.Print("output: " + structure.output.string());

    config.paths.SetPath(config.paths.scriptsPath, structure.scripts);
    config.paths.SetPath(config.paths.p4root, structure.p4root);
    config.paths.SetPath(config.paths.projectRootPath, structure.projectRoot);
    config.paths.SetPath(config.paths.uprojectPath, structure.uproject);
    config.paths.SetPath(config.paths.sourcePath, structure.source);
    config.loaded = true;
}

void App::Render()
{
    ImGuiIO& io = ImGui::GetIO();

    UpdateZoom();

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

    bool dirty = false;

    // Build default layout once
    static bool firstTime = true;
    if (firstTime)
    {
        firstTime = false;
        dirty = true;

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
    ImGui::TextColored(Theme::TextDisabled, "Zoom: %d%%", (int)(UserConfig::fontScale * 100.0f + 0.5f));
    ImGui::TextColored(Theme::TextSecondary, "Configure and run your build pipeline.");

    int stage = pipeline.RenderPipe();
    // if editing, use clicking to switch settings
    if (stage != -1)
    {
        pipeline.stageEditIndex = stage;
    }

    ImGui::Spacing();
    ImGui::Spacing();

    ImVec2 stageGroupStart = pipeline.PreRenderStage();

    switch (pipeline.stageEditIndex)
    {
        case Pipeline::INDEX_PREPARE: RenderStagePrepare(dirty); break;
        case Pipeline::INDEX_VERIFY:  RenderStageVerify(dirty); break;
        case Pipeline::INDEX_PACKAGE: RenderStagePackage(dirty); break;
        case Pipeline::INDEX_TEST:    RenderStageTest(dirty); break;
        case Pipeline::INDEX_DEPLOY:  RenderStageDeploy(dirty); break;
    }

    pipeline.PostRenderStage(stageGroupStart);

    // --- Path inputs section ---
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::BgMid);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Spacing();

    if (!config.loaded || firstTime)
    {
        AutoFill();
    }

    if (ImGui::CollapsingHeader("Advanced options"))
    {
        if (ImGui::Checkbox("Auto detect paths when possible", &config.autoDetectAll))
        {
            if (config.autoDetectAll)
            {
                AutoFill();
            }
        }

        if (ImGui::Button("Clear cache"))
        {
            config.DeleteTemporaryFiles(&console);
        }
        
        ImGui::SameLine();

        if (ImGui::Button("Load"))
        {
            config.Load(*this);
        }

        ImGui::SameLine();

        if (ImGui::Button("Save"))
        {
            config.Save(*this);
        }

        ImGui::SameLine();

        if (ImGui::Button("Print save file"))
        {
            const std::string output = config.LoadToString();
            console.PrintMultiline(output);
        }
    }

    ImGui::Spacing();

    // Unreal Engine
    if (ImGui::Button("Auto-detect") || config.paths.unrealRoot[0] == 0)
    {
        config.paths.AutoDetectUnreal();
        dirty = true;
    }
    ImGui::SameLine();
    dirty |= PathInput("Unreal Engine", config.paths.unrealRoot, sizeof(config.paths.unrealRoot), PathMode::Folder);
    ImGui::Spacing();

    // Workspace
    dirty |= MaybeAutoFolderInput("P4 Project Path", config.paths.projectRootPath, config.autoDetectAll);
    if (dirty)
    {
        UserConfig::RemoveTrailingSlash(config.paths.projectRootPath);
    }
    ImGui::Spacing();

    // uproject
    dirty |= MaybeAutoFileInput(".uproject file", config.paths.uprojectPath, config.autoDetectAll, L"*.uproject");
    ImGui::Spacing();

    // Derived paths
    std::string derivedProject = config.paths.uprojectPath;
    std::string derivedScriptsPath = config.paths.GetScriptsDir();

    bool projectExists = config.paths.projectRootPath[0] != '\0' && fs::exists(derivedProject);
    bool scriptsExist = config.paths.projectRootPath[0] != '\0' && fs::exists(derivedScriptsPath);

    dirty |= MaybeAutoFolderInput("Scripts", config.paths.scriptsPath, config.autoDetectAll);

    ImGui::Spacing();
    ImGui::PopStyleColor();

    if (runner.IsRunning())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::StopButton);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::StopButtonHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::StopButtonActive);
        if (ImGui::Button("Stop", Theme::ButtonMain))
        {
            runner.Stop(console, Console::Result::Critical);
        }
        ImGui::PopStyleColor(3);
    }
    else
    {

        if (dirty)
        {
            if (config.autoLoadSave)
            {
                config.Save(*this);
            }
            dirty = false;
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
    runner.Tick();

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
    if (config.autoLoadSave)
    {
        config.Save(*this);
    }

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
#include "gl.h"
#include <GLFW/glfw3.h>
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
#include <filesystem>
#include <sstream>

#include <fstream>
#include <json.hpp> 
using json = nlohmann::json;
static const std::string CONFIG_FILE = "pipeline_settings.json";

void App::SetupImGui()
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Shape
    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 5.0f;

    // Spacing
    style.WindowPadding = ImVec2(16.0f, 16.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.CellPadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
    style.ScrollbarSize = 10.0f;
    style.GrabMinSize = 10.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;

    // Palette
    // bg layers:  #16171A    #1E1F23    #26272C
    // accent:     #4D7FFF  (muted blue)
    // accent dim: #2F4F99

    ImVec4* c = style.Colors;

    c[ImGuiCol_WindowBg] = ImVec4(0.086f, 0.090f, 0.102f, 1.00f);
    c[ImGuiCol_ChildBg] = ImVec4(0.118f, 0.122f, 0.137f, 1.00f);
    c[ImGuiCol_PopupBg] = ImVec4(0.118f, 0.122f, 0.137f, 1.00f);

    c[ImGuiCol_FrameBg] = ImVec4(0.149f, 0.153f, 0.173f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.188f, 0.192f, 0.216f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.118f, 0.122f, 0.137f, 1.00f);

    c[ImGuiCol_TitleBg] = ImVec4(0.086f, 0.090f, 0.102f, 1.00f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.086f, 0.090f, 0.102f, 1.00f);
    c[ImGuiCol_MenuBarBg] = ImVec4(0.086f, 0.090f, 0.102f, 1.00f);

    // Accent — muted blue
    ImVec4 accent = ImVec4(0.302f, 0.498f, 1.000f, 1.00f);
    ImVec4 accentDim = ImVec4(0.184f, 0.306f, 0.600f, 1.00f);
    ImVec4 accentDark = ImVec4(0.125f, 0.200f, 0.420f, 1.00f);

    c[ImGuiCol_Button] = accentDark;
    c[ImGuiCol_ButtonHovered] = accentDim;
    c[ImGuiCol_ButtonActive] = accent;

    c[ImGuiCol_Header] = accentDark;
    c[ImGuiCol_HeaderHovered] = accentDim;
    c[ImGuiCol_HeaderActive] = accent;

    c[ImGuiCol_Tab] = ImVec4(0.118f, 0.122f, 0.137f, 1.00f);
    c[ImGuiCol_TabHovered] = accentDim;
    c[ImGuiCol_TabActive] = accentDark;
    c[ImGuiCol_TabUnfocused] = ImVec4(0.086f, 0.090f, 0.102f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive] = accentDark;

    c[ImGuiCol_SliderGrab] = accent;
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.502f, 0.698f, 1.000f, 1.00f);
    c[ImGuiCol_CheckMark] = accent;

    c[ImGuiCol_ScrollbarBg] = ImVec4(0.086f, 0.090f, 0.102f, 0.00f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.220f, 0.224f, 0.250f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.280f, 0.284f, 0.316f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive] = accent;

    c[ImGuiCol_SeparatorHovered] = accentDim;
    c[ImGuiCol_SeparatorActive] = accent;

    c[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.95f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.46f, 0.50f, 1.00f);
}

//  Render — example layout with path inputs

// Persistent state — put these in app.h as members in a real app
static char scriptsDir[512] = "";
static char projectFile[512] = "";
static char buildOutput[512] = "";
static char unrealRoot[512] = "";
static int config = 0;
static Command command;

enum class Config : char
{
    Development,    
    Shipping
};
static const char* const Configs[] = {
    "Development",
    "Shipping"
};

static const COMDLG_FILTERSPEC jsonFilter[] = {
    { L"Config Files", L"*.json;*.yaml;*.yml" },
    { L"All Files",    L"*.*"                 }
};

static Command BuildCommand()
{
    std::string passScriptPath = (std::filesystem::path(scriptsDir) / "stage2_build.ps1").string();
    std::string passOutputDir = (std::filesystem::path(buildOutput)).string();
    std::string passProjectFile = (std::filesystem::path(projectFile)).string();
    std::string passConfig = Configs[config];

    std::stringstream args;
    args << " -UnrealRoot \"" << unrealRoot << "\"";
    args << " -ProjectPath \"" << passProjectFile << "\"";
    args << " -OutputDir \"" << passOutputDir << "\"";
    args << " -Config " << passConfig;

    return Command{ passScriptPath, args.str() };
}

static void SaveSettings()
{
    json j;
    j["unrealRoot"] = unrealRoot;
    j["scriptsDir"] = scriptsDir;
    j["projectDir"] = projectFile;
    j["buildOutput"] = buildOutput;
    j["config"] = config;

    std::ofstream file(CONFIG_FILE);
    if (file.is_open())
    {
        file << j.dump(4);
        file.close();
    }
}

static void LoadSettings()
{
    std::ifstream file(CONFIG_FILE);
    if (!file.is_open()) return;

    try
    {
        json j;
        file >> j;

        if (j.contains("unrealRoot"))
        {
            std::string s = j["unrealRoot"];
            strncpy_s(unrealRoot, s.c_str(), sizeof(unrealRoot) - 1);
        }
        if (j.contains("scriptsDir"))
        {
            std::string s = j["scriptsDir"];
            strncpy_s(scriptsDir, s.c_str(), sizeof(scriptsDir) - 1);
        }
        if (j.contains("projectDir"))
        {
            std::string s = j["projectDir"];
            strncpy_s(projectFile, s.c_str(), sizeof(projectFile) - 1);
        }
        if (j.contains("buildOutput"))
        {
            std::string s = j["buildOutput"];
            strncpy_s(buildOutput, s.c_str(), sizeof(buildOutput) - 1);
        }
        if (j.contains("config"))
        {
            config = j["config"];
        }

        command = BuildCommand();
    }
    catch (const json::exception& e)
    {
        std::cout << "Error parsing JSON: " << e.what() << "\n";
    }
    file.close();
}

int App::Init()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); // required for IFileDialog

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

    window = glfwCreateWindow(900, 600, "Pipeline Tool", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

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

    // Load a nicer font - falls back gracefully if file isn't found
    // Drop any .ttf you like next to the exe and point here
    ImFontConfig fontCfg;
    fontCfg.OversampleH = 2;
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 16.0f, &fontCfg);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    SetupImGui();

    LoadSettings();
    return 0;
}

void App::Render()
{
    ImGuiIO& io = ImGui::GetIO();

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
    ImGui::DockSpace(dockID, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_FirstUseEver);
    ImGui::Begin("Pipeline");

    ImGui::SetWindowFontScale(1.3f);
    ImGui::Text("Pipeline Tool");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::TextDisabled("Configure and run your build pipeline.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.118f, 0.122f, 0.137f, 1.0f));
    //ImGui::BeginChild("##paths", ImVec2(0, 300), false);
    ImGui::Spacing();

    bool dirty = false;

    dirty |= PathInput("Unreal directory", unrealRoot, sizeof(unrealRoot), PathMode::Folder);
    ImGui::Spacing();
    dirty |= PathInput("Scripts Directory", scriptsDir, sizeof(scriptsDir), PathMode::Folder);
    ImGui::Spacing();
    dirty |= PathInput(".uproject File", projectFile, sizeof(projectFile), PathMode::FileFiltered,
                       L"Config Files\0*.uproject\0All Files\0*.*\0");
    ImGui::Spacing();
    dirty |= PathInput("Build Output Folder", buildOutput, sizeof(buildOutput), PathMode::Folder);
    ImGui::Spacing();

    if (ImGui::Combo("Configuration", &config, Configs, sizeof(Configs) / sizeof(Configs[0])))
    {
        dirty = true;
    }

    if (dirty)
    {
        command = BuildCommand();
    }

    ImGui::Spacing();
    //ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::TextDisabled("Command parsed:");
    std::string output = "-File \"" + command.script + "\" " + command.args;
    static char cmdPreview[2048];
    strncpy_s(cmdPreview, output.c_str(), sizeof(cmdPreview) - 1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##cmdpreview", cmdPreview, sizeof(cmdPreview), ImGuiInputTextFlags_ReadOnly);
    ImGui::Spacing();

    if (runner.IsRunning())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.25f, 0.25f, 1.0f));
        if (ImGui::Button("Stop", ImVec2(200.0f, 36.0f)))
        {
            runner.Stop(console);
        }
        ImGui::PopStyleColor(3);
    }
    else
    {
        if (ImGui::Button("Run Pipeline", ImVec2(200.0f, 36.0f)))
        {
            runner.RunFile(command, console);
        }

        ImGui::SameLine();

        if (ImGui::Button("Save Settings", ImVec2(120.0f, 36.0f)))
        {
            SaveSettings();
        }
    }

    ImGui::End();

    console.Draw("Console", &showConsole);

    // Re-open via menu if closed (optional)
    if (!showConsole)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::MenuItem("Console")) showConsole = true;
            ImGui::EndMainMenuBar();
        }
    }
}

// rest of Init / Tick / PreRender / PostRender / Exit / Run unchanged

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
    SaveSettings();

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
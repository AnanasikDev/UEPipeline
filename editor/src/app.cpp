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
#include "theme.h"
#include <filesystem>
#include <sstream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <json.hpp>

using json = nlohmann::json;
static const std::string CONFIG_FILE = "pipeline_settings.json";

namespace fs = std::filesystem;

// ---------- Zoom state ----------

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

// ---------- Setup ----------

void App::SetupImGui()
{
    Theme::Apply();
}

// ---------- State ----------

static char workspacePath[512] = "";
static char buildOutput[512] = "";
static char unrealRoot[512] = "";
static int  config = 0;
static Command command;

// Derivation patterns (saved to JSON so they're editable)
static std::string deriveUproject = "PebbleByPebble\\PebbleByPebble.uproject";
static std::string deriveScripts = "builder\\";

enum class Config : char { Development, Shipping };
static const char* const Configs[] = { "Development", "Shipping" };

// ---------- Derived paths ----------

static std::string GetScriptsDir()
{
    return (fs::path(workspacePath) / deriveScripts).string();
}

static std::string GetProjectFile()
{
    return (fs::path(workspacePath) / deriveUproject).string();
}

// ---------- UE auto-detection ----------

struct UEInstall
{
    std::string path;
    std::string version;
};

static std::vector<UEInstall> DetectUnrealInstalls()
{
    std::vector<UEInstall> found;

    auto scanRegistryHive = [&](HKEY root)
        {
            HKEY ueKey = nullptr;
            if (RegOpenKeyExA(root, "SOFTWARE\\EpicGames\\Unreal Engine", 0,
                              KEY_READ | KEY_ENUMERATE_SUB_KEYS, &ueKey) == ERROR_SUCCESS)
            {
                char subKeyName[256];
                DWORD index = 0;
                DWORD nameLen = sizeof(subKeyName);
                while (RegEnumKeyExA(ueKey, index++, subKeyName, &nameLen,
                                     nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS)
                {
                    HKEY verKey = nullptr;
                    if (RegOpenKeyExA(ueKey, subKeyName, 0, KEY_READ, &verKey) == ERROR_SUCCESS)
                    {
                        char installDir[512] = {};
                        DWORD size = sizeof(installDir);
                        DWORD type = REG_SZ;
                        if (RegQueryValueExA(verKey, "InstalledDirectory", nullptr, &type,
                                             (LPBYTE)installDir, &size) == ERROR_SUCCESS)
                        {
                            std::string dir(installDir);
                            std::string uat = dir + "\\Engine\\Build\\BatchFiles\\RunUAT.bat";
                            if (fs::exists(uat))
                            {
                                found.push_back({ dir, std::string(subKeyName) });
                            }
                        }
                        RegCloseKey(verKey);
                    }
                    nameLen = sizeof(subKeyName);
                }
                RegCloseKey(ueKey);
            }
        };

    scanRegistryHive(HKEY_LOCAL_MACHINE);
    scanRegistryHive(HKEY_CURRENT_USER);

    const char* scanRoots[] = {
        "C:\\Program Files\\Epic Games",
        "D:\\Program Files\\Epic Games",
        "E:\\Program Files\\Epic Games",
        "C:\\Epic Games",
        "D:\\Epic Games",
    };

    for (auto& root : scanRoots)
    {
        if (!fs::exists(root)) continue;
        try
        {
            for (auto& entry : fs::directory_iterator(root))
            {
                if (!entry.is_directory()) continue;
                std::string name = entry.path().filename().string();
                if (name.find("UE") == std::string::npos) continue;

                std::string uat = entry.path().string() + "\\Engine\\Build\\BatchFiles\\RunUAT.bat";
                if (!fs::exists(uat)) continue;

                std::string ver;
                size_t digitStart = name.find_first_of("0123456789");
                if (digitStart != std::string::npos)
                    ver = name.substr(digitStart);
                else
                    ver = name;

                bool dup = false;
                for (auto& f : found)
                {
                    if (fs::equivalent(fs::path(f.path), entry.path()))
                    {
                        dup = true; break;
                    }
                }
                if (!dup)
                    found.push_back({ entry.path().string(), ver });
            }
        }
        catch (...) {}
    }

    std::sort(found.begin(), found.end(), [](const UEInstall& a, const UEInstall& b)
    {
        return a.version > b.version;
    });

    return found;
}

static void AutoDetectUnreal()
{
    auto installs = DetectUnrealInstalls();
    if (!installs.empty())
    {
        strncpy_s(unrealRoot, installs[0].path.c_str(), sizeof(unrealRoot) - 1);
    }
}

// ---------- Build command ----------

static Command BuildCommand()
{
    std::string passScriptPath = (fs::path(GetScriptsDir()) / "stage2_build.ps1").string();
    std::string passProjectFile = GetProjectFile();
    std::string passOutputDir = (fs::path(buildOutput)).string();
    std::string passConfig = Configs[config];

    std::stringstream args;
    args << " -UnrealRoot \"" << unrealRoot << "\"";
    args << " -ProjectPath \"" << passProjectFile << "\"";
    args << " -OutputDir \"" << passOutputDir << "\"";
    args << " -Config " << passConfig;

    return Command{ passScriptPath, args.str() };
}

// ---------- Settings ----------

static void SaveSettings()
{
    json j;
    j["unrealRoot"] = unrealRoot;
    j["workspacePath"] = workspacePath;
    j["buildOutput"] = buildOutput;
    j["config"] = config;
    j["deriveUproject"] = deriveUproject;
    j["deriveScripts"] = deriveScripts;
    j["fontScale"] = fontScale;

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
        if (j.contains("workspacePath"))
        {
            std::string s = j["workspacePath"];
            strncpy_s(workspacePath, s.c_str(), sizeof(workspacePath) - 1);
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
        if (j.contains("deriveUproject"))
        {
            deriveUproject = j["deriveUproject"].get<std::string>();
        }
        if (j.contains("deriveScripts"))
        {
            deriveScripts = j["deriveScripts"].get<std::string>();
        }
        if (j.contains("fontScale"))
        {
            fontScale = j["fontScale"];
            if (fontScale < Theme::FontScaleMin) fontScale = Theme::FontScaleMin;
            if (fontScale > Theme::FontScaleMax) fontScale = Theme::FontScaleMax;
        }

        command = BuildCommand();
    }
    catch (const json::exception& e)
    {
        std::cout << "Error parsing JSON: " << e.what() << "\n";
    }
    file.close();
}

// ---------- Init ----------

int App::Init()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

    window = glfwCreateWindow(900, 600, "Pipeline Tool", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
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
    LoadSettings();

    // Apply loaded zoom level
    io.FontGlobalScale = fontScale;

    if (unrealRoot[0] == '\0')
    {
        AutoDetectUnreal();
        if (unrealRoot[0] != '\0')
            command = BuildCommand();
    }

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
    ImGui::DockSpace(dockID, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    // --- Main panel ---
    ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_FirstUseEver);
    ImGui::Begin("Pipeline");

    // Header
    ImGui::SetWindowFontScale(Theme::FontHeaderScale);
    ImGui::TextColored(Theme::TextPrimary, "Pipeline Tool");
    ImGui::SetWindowFontScale(1.0f);

    ImGui::TextColored(Theme::TextSecondary, "Configure and run your build pipeline.");

    // Zoom indicator
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80.0f);
    ImGui::TextColored(Theme::TextDisabled, "Zoom: %d%%", (int)(fontScale * 100.0f + 0.5f));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // --- Path inputs section ---
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::BgMid);
    ImGui::Spacing();

    bool dirty = false;

    // Unreal Engine
    if (ImGui::Button("Auto-detect"))
    {
        AutoDetectUnreal();
        dirty = true;
    }
    ImGui::SameLine();
    dirty |= PathInput("Unreal Engine", unrealRoot, sizeof(unrealRoot), PathMode::Folder);
    ImGui::Spacing();

    // Workspace
    dirty |= PathInput("P4 Project Path", workspacePath, sizeof(workspacePath), PathMode::Folder);
    ImGui::Spacing();

    // Derived paths
    std::string derivedProject = GetProjectFile();
    std::string derivedScriptsPath = GetScriptsDir();

    bool projectExists = workspacePath[0] != '\0' && fs::exists(derivedProject);
    bool scriptsExist = workspacePath[0] != '\0' && fs::exists(derivedScriptsPath);

    ImGui::SetWindowFontScale(Theme::FontSubheaderScale);
    ImGui::TextColored(Theme::TextSecondary, "Derived .uproject:");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine();
    if (workspacePath[0] != '\0')
    {
        if (projectExists)
            ImGui::TextColored(Theme::Success, "%s", derivedProject.c_str());
        else
            ImGui::TextColored(Theme::Error, "%s  (not found)", derivedProject.c_str());
    }
    else
    {
        ImGui::TextColored(Theme::TextDisabled, "(set workspace path)");
    }

    ImGui::SetWindowFontScale(Theme::FontSubheaderScale);
    ImGui::TextColored(Theme::TextSecondary, "Derived scripts dir:");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine();
    if (workspacePath[0] != '\0')
    {
        if (scriptsExist)
            ImGui::TextColored(Theme::Success, "%s", derivedScriptsPath.c_str());
        else
            ImGui::TextColored(Theme::Error, "%s  (not found)", derivedScriptsPath.c_str());
    }
    else
    {
        ImGui::TextColored(Theme::TextDisabled, "(set workspace path)");
    }

    ImGui::Spacing();

    // Build output
    dirty |= PathInput("Build Output Folder", buildOutput, sizeof(buildOutput), PathMode::Folder);
    ImGui::Spacing();

    // Config combo
    if (ImGui::Combo("Configuration", &config, Configs, sizeof(Configs) / sizeof(Configs[0])))
    {
        dirty = true;
    }

    if (dirty)
    {
        command = BuildCommand();
    }

    ImGui::Spacing();
    ImGui::PopStyleColor();

    // --- Command preview ---
    ImGui::Spacing();
    ImGui::TextColored(Theme::TextSecondary, "Command parsed:");
    std::string output = "-File \"" + command.script + "\" " + command.args;
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
            runner.RunFile(command, console);
        }

        ImGui::SameLine();

        if (ImGui::Button("Save Settings", Theme::ButtonSecondary))
        {
            SaveSettings();
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
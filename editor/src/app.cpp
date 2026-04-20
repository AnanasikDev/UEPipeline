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
#include <vector>
#include <algorithm>

#include <fstream>
#include <json.hpp>
using json = nlohmann::json;
static const std::string CONFIG_FILE = "pipeline_settings.json";

namespace fs = std::filesystem;

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
    std::string version; // e.g. "5.6"
};

static std::vector<UEInstall> DetectUnrealInstalls()
{
    std::vector<UEInstall> found;

    // 1) Scan registry: HKLM\SOFTWARE\EpicGames\Unreal Engine\<version>
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

    // 2) Filesystem scan: common Epic Games install locations
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
                // Match UE_5.x, UE_5.xx, UE5.x etc.
                if (name.find("UE") == std::string::npos) continue;

                std::string uat = entry.path().string() + "\\Engine\\Build\\BatchFiles\\RunUAT.bat";
                if (!fs::exists(uat)) continue;

                // Extract version from folder name (e.g. "UE_5.6" -> "5.6")
                std::string ver;
                size_t digitStart = name.find_first_of("0123456789");
                if (digitStart != std::string::npos)
                    ver = name.substr(digitStart);
                else
                    ver = name;

                // Deduplicate against registry results
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
        catch (...) {} // permission errors etc. 
    }

    // Sort by version descending (lexicographic works for "5.x" style)
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

    ImFontConfig fontCfg;
    fontCfg.OversampleH = 2;
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 16.0f, &fontCfg);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    SetupImGui();

    LoadSettings();

    // Auto-detect UE if not already set from config
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
    ImGui::Spacing();

    bool dirty = false;

    // --- Unreal Engine path with auto-detect button ---
    dirty |= PathInput("Unreal Engine", unrealRoot, sizeof(unrealRoot), PathMode::Folder);
    ImGui::SameLine();
    if (ImGui::Button("Auto-detect"))
    {
        AutoDetectUnreal();
        dirty = true;
    }
    ImGui::Spacing();

    // --- Single workspace path ---
    dirty |= PathInput("P4 Project Path", workspacePath, sizeof(workspacePath), PathMode::Folder);
    ImGui::Spacing();

    // --- Derived paths (read-only display) ---
    std::string derivedProject = GetProjectFile();
    std::string derivedScripts = GetScriptsDir();

    bool projectExists = !std::string(workspacePath).empty() && fs::exists(derivedProject);
    bool scriptsExist = !std::string(workspacePath).empty() && fs::exists(derivedScripts);

    ImGui::TextDisabled("Derived .uproject:");
    ImGui::SameLine();
    if (!std::string(workspacePath).empty())
    {
        if (projectExists)
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "%s", derivedProject.c_str());
        else
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s  (not found)", derivedProject.c_str());
    }
    else
    {
        ImGui::TextDisabled("(set workspace path)");
    }

    ImGui::TextDisabled("Derived scripts dir:");
    ImGui::SameLine();
    if (!std::string(workspacePath).empty())
    {
        if (scriptsExist)
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "%s", derivedScripts.c_str());
        else
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s  (not found)", derivedScripts.c_str());
    }
    else
    {
        ImGui::TextDisabled("(set workspace path)");
    }

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
    ImGui::PopStyleColor();

    // --- Command preview ---
    ImGui::Spacing();
    ImGui::TextDisabled("Command parsed:");
    std::string output = "-File \"" + command.script + "\" " + command.args;
    static char cmdPreview[2048];
    strncpy_s(cmdPreview, output.c_str(), sizeof(cmdPreview) - 1);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##cmdpreview", cmdPreview, sizeof(cmdPreview), ImGuiInputTextFlags_ReadOnly);
    ImGui::Spacing();

    // --- Buttons ---
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

    if (!showConsole)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::MenuItem("Console")) showConsole = true;
            ImGui::EndMainMenuBar();
        }
    }
}

// ---------- Boilerplate ----------

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
#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <cctype>

namespace fs = std::filesystem;

class Paths
{
public:
    /// <summary>
    /// [Common] Absolute Path to the project root, e.g. root\\Y2025D-Y1-ECHO
    /// </summary>
    char projectRootPath[512] = "";

    /// <summary>
    /// [Code analysis] Absolute Path to the Unreal Engine C++ Source directory, e.g. root\\Y2025D-Y1-ECHO\\PebbleByPebble\\Source
    /// </summary>
    char sourcePath[512] = "";

    /// <summary>
    /// [Package] Absolute Path to the desired build output, e.g. User\\Downloads
    /// </summary>
    char buildOutput[512] = "";

    /// <summary>
    /// [Perforce] Absolute Path to the Perforce workspace, e.g. D:\\Perforce
    /// </summary>
    char p4root[512] = "";

    /// <summary>
    /// [Common]: Absolute Path to the Unreal root, e.g. D:\\EpicGames\\UE_5.7
    /// </summary>
    char unrealRoot[512] = "";

    /// <summary>
    /// [Common]: Absolute Path to the game .uproject file, e.g. root\\Y2025D-Y1-ECHO\\PebbleByPebble\\PebbleByPebble.uproject
    /// </summary>
    char uprojectPath[512] = "";

    /// <summary>
    /// [Common]: Absolute Path to the powershell scripts, e.g. root\\Y2025D-Y1-ECHO\\builder
    /// </summary>
    char scriptsPath[512] = "";

    /// <summary>
    /// [Test]: Absolute Path to the executable file that should be tested. NOT SAVED.
    /// </summary>
    char buildExePath[512] = "";

    /// <summary>
    /// [Deploy]: .zip archive of the final game to deploy to itch.io
    /// </summary>
    char zipPath[512] = "";

    struct UEInstall
    {
        std::string path;
        std::string version;
    };

    struct FileStructure
    {
        bool is_valid = false;
        std::string name = "";
        fs::path uproject;
        fs::path projectRoot;
        fs::path source;
        fs::path scripts;
        fs::path p4root;
        fs::path output;
    };

    void Init();

    std::string GetScriptsDir() const;
    std::vector<UEInstall> DetectUnrealInstalls() const;
    void AutoDetectUnreal();
    FileStructure AnalyzeStructure() const;
    inline fs::path GetPath(const char* const pathstr) const
    {
        return fs::path(pathstr);
    }
    template <size_t N>
    inline void SetPath(char (&pathstr)[N], fs::path path)
    {
        strncpy_s(pathstr, path.string().c_str(), sizeof(pathstr) - 1);
    }
};
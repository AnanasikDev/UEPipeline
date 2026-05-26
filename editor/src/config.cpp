#include <iostream>
#include <fstream>
#include <json.hpp>
#include <cstdio>

#include "config.h"

#include "theme.h"
#include "app.h"
#include "console.h"

using json = nlohmann::json;
static const std::string CONFIG_FILE = "pipeline_settings.json";
float UserConfig::fontScale = Theme::FontScaleDefault;

UserConfig::UserConfig()
{
    fontScale = Theme::FontScaleDefault;
}

void UserConfig::Save(App& app)
{
    json j;
    j["unrealRoot"]             = paths.unrealRoot;
    j["projectRoot"]            = paths.projectRootPath;
    j["sourcePath"]             = paths.sourcePath;
    j["buildOutput"]            = paths.buildOutput;
    j["config"]                 = buildConfig;
    j["p4root"]                 = paths.p4root;
    j["uproject"]               = paths.uprojectPath;
    j["scripts"]                = paths.scriptsPath;
    j["fontScale"]              = fontScale;
    j["zip"]                    = paths.zipPath;
    j["usergame"]               = usergame;
    j["autoLoadSave"]           = autoLoadSave;

    std::ofstream file(CONFIG_FILE);
    if (file.is_open())
    {
        file << j.dump(4);
        file.close();
    }
}

template <size_t N>
static bool JLoadString(const json& j, const char* label, char (&dest)[N])
{
    if (j.contains(label))
    {
        strncpy_s(dest, j[label].get<std::string>().c_str(), sizeof(dest) - 1);
        return true;
    }
    return false;
}

template <typename T>
static bool JLoadValue(const json& j, const char* label, T& dest)
{
    if (j.contains(label))
    {
        dest = j[label];
        return true;
    }
    return false;
}

void UserConfig::Load(App& app)
{
    std::ifstream file(CONFIG_FILE);
    if (!file.is_open())
    {
        paths.Init();
        return;
    }

    try
    {
        json j;
        file >> j;

        // paths
        JLoadString(j, "unrealRoot", paths.unrealRoot);
        JLoadString(j, "projectRoot", paths.projectRootPath);
        JLoadString(j, "sourcePath", paths.sourcePath);
        JLoadString(j, "buildOutput", paths.buildOutput);
        JLoadString(j, "scripts", paths.scriptsPath);
        JLoadString(j, "zip", paths.zipPath);
        JLoadString(j, "uproject", paths.uprojectPath);
        JLoadString(j, "p4root", paths.p4root);

        // itch.io user/game
        JLoadString(j, "usergame", usergame);

        // values
        JLoadValue(j, "config", buildConfig);
        JLoadValue(j, "autoLoadSave", autoLoadSave);

        if (JLoadValue(j, "fontScale", fontScale))
        {
            if (fontScale < Theme::FontScaleMin) fontScale = Theme::FontScaleMin;
            if (fontScale > Theme::FontScaleMax) fontScale = Theme::FontScaleMax;
        }

        loaded = true;
    }
    catch (const json::exception& e)
    {
        std::cout << "Error parsing JSON: " << e.what() << "\n";
        paths.Init();
    }
    file.close();
}

static std::string read_file(std::string_view path)
{
    constexpr std::size_t read_size{ 4096 };
    std::ifstream stream(path.data());
    stream.exceptions(std::ios_base::badbit);

    if (!stream)
    {
        throw std::ios_base::failure("file does not exist");
    }

    std::string out;
    std::string buf(read_size, '\0');
    while (stream.read(&buf[0], read_size))
    {
        out.append(buf, 0, stream.gcount());
    }
    out.append(buf, 0, stream.gcount());
    return out;
}

std::string UserConfig::LoadToString() const
{
    std::string path = (fs::path(paths.scriptsPath) / "pipeline_settings.json").string();
    if (!fs::exists(path))
    {
        return "Error parsing config file: file doesn't exist at <" + path + ">";
    }
    return read_file(path);
}

static bool DeleteTempFile(fs::path file, Console* console)
{
    const std::string str = file.string();
    if (!fs::exists(file))
    {
        if (console) console->Print("Error deleting a file. Path doesn't exist: " + str, Console::Result::Error);
        return false;
    }

    if (!fs::is_regular_file(file))
    {
        if (console) console->Print("Error deleting a file. Path leads to a directory instead of a file: " + str, Console::Result::Error);
        return false;
    }

    constexpr int MAX_FILE_SIZE_BYTES = 2 << 20; // 1 MB
    const uintmax_t fileSize = fs::file_size(file);
    if (fileSize > MAX_FILE_SIZE_BYTES)
    {
        if (console) console->Print(std::string("Error deleting a file. File size is too big (") + std::to_string(fileSize) + ") > (" + std::to_string(MAX_FILE_SIZE_BYTES) + " bytes): " + str, Console::Result::Error);
        return false;
    }

    const int status = remove(str.c_str());

    if (status != 0)
    {
        if (console) console->Print("Error deleting a file (" + std::to_string(status) + "): " + str, Console::Result::Error);
        return false;
    }

    if (console) console->Print("Successfully deleted file: " + str, Console::Result::Success);

    return true;
}

void UserConfig::DeleteTemporaryFiles(Console* console)
{
    fs::path settingsFile = (fs::path(paths.scriptsPath) / "pipeline_settings.json");
    fs::path imguiFile = (fs::path(paths.scriptsPath) / "imgui.ini");

    DeleteTempFile(settingsFile, console);
    DeleteTempFile(imguiFile, console);
}

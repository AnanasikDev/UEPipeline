#include "config.h"
#include <iostream>
#include <fstream>
#include <json.hpp>
#include "theme.h"
#include "app.h"

using json = nlohmann::json;
static const std::string CONFIG_FILE = "pipeline_settings.json";

UserConfig::UserConfig()
{
    fontScale = Theme::FontScaleDefault;
}

void UserConfig::Save(App& app)
{
    json j;
    j["unrealRoot"] = paths.unrealRoot;
    j["workspacePath"] = paths.projectRootPath;
    j["buildOutput"] = paths.buildOutput;
    j["config"] = buildConfig;
    j["deriveUproject"] = paths.deriveUproject;
    j["deriveScripts"] = paths.deriveScripts;
    j["fontScale"] = fontScale;

    std::ofstream file(CONFIG_FILE);
    if (file.is_open())
    {
        file << j.dump(4);
        file.close();
    }
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

        if (j.contains("unrealRoot"))
        {
            std::string s = j["unrealRoot"];
            strncpy_s(paths.unrealRoot, s.c_str(), sizeof(paths.unrealRoot) - 1);
        }
        if (j.contains("workspacePath"))
        {
            std::string s = j["workspacePath"];
            strncpy_s(paths.projectRootPath, s.c_str(), sizeof(paths.projectRootPath) - 1);
        }
        if (j.contains("buildOutput"))
        {
            std::string s = j["buildOutput"];
            strncpy_s(paths.buildOutput, s.c_str(), sizeof(paths.buildOutput) - 1);
        }
        if (j.contains("config"))
        {
            buildConfig = j["config"];
        }
        if (j.contains("deriveUproject"))
        {
            paths.deriveUproject = j["deriveUproject"].get<std::string>();
        }
        if (j.contains("deriveScripts"))
        {
            paths.deriveScripts = j["deriveScripts"].get<std::string>();
        }
        if (j.contains("fontScale"))
        {
            fontScale = j["fontScale"];
            if (fontScale < Theme::FontScaleMin) fontScale = Theme::FontScaleMin;
            if (fontScale > Theme::FontScaleMax) fontScale = Theme::FontScaleMax;
        }
    }
    catch (const json::exception& e)
    {
        std::cout << "Error parsing JSON: " << e.what() << "\n";
        paths.Init();
    }
    file.close();
}
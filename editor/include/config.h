#pragma once

#include "paths.h"

class App;

class UserConfig
{
public:
    UserConfig();

    void Save(App& app);
    void Load(App& app);
    
    enum class BuildConfig : char { Development, Shipping };
    const char* const BuildConfigs[2] = { "Development", "Shipping" };
    BuildConfig buildConfig = BuildConfig::Development;
    inline std::string GetCurrentBuildConfigString() const
    {
        return BuildConfigs[static_cast<size_t>(buildConfig)];
    }

    Paths paths;

    float fontScale;
};
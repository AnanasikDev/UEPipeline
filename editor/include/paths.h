#pragma once
#include <vector>
#include <string>

class Paths
{
public:

    enum class InitResult : uint32_t
    {
        Failed = 0,
        UnrealDetected = 1 << 0,
        ProjectDetected = 1 << 1,
    };

    char projectRootPath[512] = "";
    char buildOutput[512] = "";
    char unrealRoot[512] = "";
    std::string deriveUproject = "PebbleByPebble\\PebbleByPebble.uproject";
    std::string deriveScripts = "builder\\";

    struct UEInstall
    {
        std::string path;
        std::string version;
    };

    struct Path
    {
        std::string path;
        bool exists;
    };

    InitResult Init();

    Path GetScriptsDir() const;
    Path GetProjectFile() const;
    std::vector<UEInstall> DetectUnrealInstalls() const;
    void AutoDetectUnreal();
    std::string DetectProjectRoot() const;
};

// OR
constexpr Paths::InitResult operator|(Paths::InitResult a, Paths::InitResult b)
{
    return static_cast<Paths::InitResult>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
    );
}

// AND
constexpr Paths::InitResult operator&(Paths::InitResult a, Paths::InitResult b)
{
    return static_cast<Paths::InitResult>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
    );
}

// OR assignment
constexpr Paths::InitResult& operator|=(Paths::InitResult& a, Paths::InitResult b)
{
    a = a | b;
    return a;
}

// AND assignment
constexpr Paths::InitResult& operator&=(Paths::InitResult& a, Paths::InitResult b)
{
    a = a & b;
    return a;
}

// NOT
constexpr Paths::InitResult operator~(Paths::InitResult a)
{
    return static_cast<Paths::InitResult>(
        ~static_cast<uint32_t>(a)
    );
}
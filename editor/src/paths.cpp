#include "paths.h"
#include "console.h"
#include <Windows.h>

void Paths::Init()
{
    if (unrealRoot[0] == '\0')
    {
        AutoDetectUnreal();
    }
}

std::string Paths::GetScriptsDir() const
{
    std::string dir = (fs::path(projectRootPath) / scriptsPath).string();
    return dir;
}

std::vector<Paths::UEInstall> Paths::DetectUnrealInstalls() const
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

void Paths::AutoDetectUnreal()
{
    auto installs = DetectUnrealInstalls();
    if (!installs.empty())
    {
        strncpy_s(unrealRoot, installs[0].path.c_str(), sizeof(unrealRoot) - 1);
    }
}

static bool IsValidPath(fs::path path)
{
    return path != "" && fs::exists(path);
}

static bool IsValidDirectory(fs::path path)
{
    return IsValidPath(path) && fs::is_directory(path);
}

static bool IsValidFile(fs::path path)
{
    return IsValidPath(path) && fs::is_regular_file(path);
}

static fs::path Validate(fs::path path, Console& console)
{
    if (!IsValidPath(path))
    {
        std::string msg = "Path is invalid: " + path.string();
        console.Print(msg, Console::Result::Error);
        return "";
    }
    return path;
}

static fs::path Parent(fs::path path, Console& console)
{
    if (!IsValidPath(path))
    {
        std::string msg = "Path is invalid: " + path.string();
        console.Print(msg, Console::Result::Error);
        return "";
    }

    const fs::path result = path.parent_path();
    if (!IsValidDirectory(result))
    {
        std::string msg = "Path parent is invalid: " + result.string();
        console.Print(msg, Console::Result::Error);
        return "";
    }

    return result;
}

static fs::path Child(fs::path parent, fs::path child, Console& console)
{
    if (!IsValidDirectory(parent))
    {
        std::string msg = "Parent directory is invalid: " + parent.string();
        console.Print(msg, Console::Result::Error);
        return "";
    }

    const fs::path result = parent / child;
    if (!IsValidDirectory(result))
    {
        std::string msg = "Path parent is invalid: " + result.string();
        console.Print(msg, Console::Result::Error);
        return "";
    }

    return result;
}

Paths::FileStructure Paths::AnalyzeStructure(Console& console) const
{
    Paths::FileStructure result;

    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    const fs::path exeDir = Parent(fs::path(exePath), console);   // .../builder
    const fs::path root = Parent(exeDir, console);               // .../projectroot

    // sanity check
    if (IsValidDirectory(root))
    {
        for (const auto& entry : fs::directory_iterator(root))
        {
            if (!IsValidDirectory(entry))
            {
                continue;
            }

            for (const auto& file : fs::directory_iterator(entry.path()))
            {
                if (fs::exists(file) && file.is_regular_file() && file.path().extension() == ".uproject")
                {
                    result.name = file.path().filename().stem().string(); // get only the filename without extension
                    result.uproject = file.path();
                    break;
                }
            }

            if (!result.uproject.empty())
            {
                break; // found uproject, stop scanning for it
            }
        }
    }

    result.p4root = Parent(root, console);
    result.source = Child(Parent(result.uproject, console), "Source", console);
    result.projectRoot = root;

    result.scripts = "";
    for (const auto& entry : fs::directory_iterator(root))
    {
        if (!IsValidFile(entry))
        {
            continue;
        }
        if (!fs::is_empty(entry) && entry.path().filename().has_extension() && entry.path().filename().extension() == "ps1")
        {
            result.scripts = exeDir;
            break;
        }
    }
    result.is_valid = true;

    return result;
}
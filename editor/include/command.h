#pragma once

#include <string>

/// <summary>
/// Universal command type that separates script from its arguments. Can be executed as a file (PowerShellScript) or as a plain command (OtherCommand)
/// </summary>
struct Command
{
    enum class Type : unsigned char
    {
        PowerShellScript = 0,
        OtherCommand
    };

    std::string script;
    std::string args;
    Type type = Type::PowerShellScript;

    Command() = default;

    Command(const std::string& script, const std::string& args, Type type)
        : script(script), args(args), type(type)
    {
    }
};
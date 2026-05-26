#include "app.h"
#include "imgui_utils.h"

void App::RenderStagePrepare(bool& dirty)
{
    {
        Command command;
        command.script = (fs::path(config.paths.GetScriptsDir()) / "p4check.ps1").string();
        command.args = " -ShowSyncStatus -ProjectRoot \"" + std::string(config.paths.projectRootPath) + "\"";
        std::string cmdstr = Runner::MakeCommandHeadless(command);
        if (ImGui::Button("Check Perforce status"))
        {
            runner.Run(cmdstr, console);
        }
        Tooltip(cmdstr);
    }

    {
        dirty |= MaybeAutoFolderInput("Perforce root", config.paths.p4root, config.autoDetectAll);
        Command command;
        command.script = (fs::path(config.paths.GetScriptsDir()) / "p4setroot.ps1").string();
        command.args = std::string(" \"") + config.paths.p4root + "\"";
        std::string cmdstr = Runner::MakeCommandHeadless(command);
        if (ImGui::Button("Set P4 internal root"))
        {
            runner.Run(cmdstr, console);
        }
        Tooltip(cmdstr);
    }

    ImGui::SameLine();

    {
        if (ImGui::Button("p4 info"))
        {
            runner.Run(Runner::MakeCommandHeadless(Command("p4 info", "", Command::Type::OtherCommand)), console, nullptr, nullptr);
            runner.AddRun([this]() -> void
                {
                    runner.Run(Runner::MakeCommandHeadless(Command("p4 set", "", Command::Type::OtherCommand)), console);
                });
        }
    }
}

void App::RenderStageVerify(bool& dirty)
{
    {
        std::string cmdstr = Runner::MakeCommandHeadless(Runner::BuildCommand_Verify(*this));

        ImGui::Text("Runs Unreal asset validation: blueprints, dependencies, data fields, naming conventions");

        if (ImGui::Button("Run data validation"))
        {
            runner.Run_Verify(*this, cmdstr);
        }
        Tooltip(cmdstr);
    }

    {
        ImGui::Text("C++ Code analysis");

        {
            dirty |= ImGui::InputText("Target", config.target, sizeof(config.target));
            dirty |= ImGui::InputText("Platform", config.platform, sizeof(config.platform));

            std::string cmdstr = Runner::MakeCommandHeadless(Runner::BuildCommand_CodeAnalysis_Prep(*this, config.target, config.platform, config.GetCurrentBuildConfigString()));

            if (ImGui::Button("Generate Clang Database"))
            {
                runner.Run_CodeAnalysis(*this, cmdstr);
            }
            Tooltip(cmdstr);
        }

        dirty |= MaybeAutoFolderInput("C++ Source", config.paths.sourcePath, config.autoDetectAll);

        {
            std::string cmdstr = Runner::MakeCommandHeadless(Runner::BuildCommand_CodeAnalysis(*this));

            if (ImGui::Button("Run code analysis"))
            {
                runner.Run_CodeAnalysis(*this, cmdstr);
            }
            Tooltip(cmdstr);
        }
    }
}

void App::RenderStagePackage(bool& dirty)
{
    // Build output
    dirty |= PathInput("Build Output Folder", config.paths.buildOutput, sizeof(config.paths.buildOutput), PathMode::Folder);
    ImGui::Spacing();

    // Config combo
    {
        int buildConfig = static_cast<int>(config.buildConfig);
        if (ImGui::Combo("Configuration", &buildConfig, config.BuildConfigs, sizeof(config.BuildConfigs) / sizeof(config.BuildConfigs[0])))
        {
            dirty = true;
            config.buildConfig = static_cast<UserConfig::BuildConfig>(buildConfig);
        }
    }

    std::string cmdstr = Runner::MakeCommandHeadless(runner.BuildCommand_Package(*this));
    if (ImGui::Button("Package the project"))
    {
        runner.Run(cmdstr, console);
    }
    Tooltip(cmdstr);

    ImGui::Spacing();
}

void App::RenderStageTest(bool& dirty)
{
    dirty |= PathInput(".exe file", config.paths.buildExePath, sizeof(config.paths.buildExePath), PathMode::File, L"*.exe");
    ImGui::Spacing();

    std::string cmdstr = Runner::MakeCommandHeadless(runner.BuildCommand_BootflowTest(*this));

    if (ImGui::Button("Run bootflow test"))
    {
        runner.Run_BootflowTest(*this, cmdstr);
    }
    ImGui::SetItemTooltip(cmdstr.c_str());
}

void App::RenderStageDeploy(bool& dirty)
{
    {
        static char folder[512] = "";
        dirty |= PathInput("Folder to zip", folder, sizeof(folder), PathMode::Folder);

        Command command;
        std::stringstream str;
        str << "Compress-Archive";
        str << " -LiteralPath \'" << folder << "\'";
        str << " -DestinationPath \'" << folder << ".zip\'";
        command.script = str.str();
        command.type = Command::Type::OtherCommand;
        std::string cmdstr = Runner::MakeCommandHeadless(command);
        if (ImGui::Button("Compress folder to zip"))
        {
            console.Print(cmdstr);
            runner.Run(cmdstr, console);
        }
        Tooltip(cmdstr);
    }

    ImGui::Text("Deployment to Itch.io uses butler script, made by itch.io itself. \nNo need to download anything else, it is shipped along this app!");

    std::string script = (fs::path(config.paths.GetScriptsDir()) / "butler\\butler.exe").string();
    {
        Command command;
        command.script = script;
        command.args = " version";
        command.type = Command::Type::OtherCommand;
        std::string cmdstr = Runner::MakeCommandHeadless(command);
        if (ImGui::Button("Version"))
        {
            console.Print(cmdstr);
            runner.Run(cmdstr, console);
        }
        Tooltip(cmdstr);
    }

    ImGui::SameLine();

    {
        Command command;
        command.script = script;
        command.args = " login";
        command.type = Command::Type::OtherCommand;
        std::string cmdstr = Runner::MakeCommandHeadless(command);
        if (ImGui::Button("Login"))
        {
            console.Print(cmdstr);
            runner.Run(cmdstr, console);
        }
        Tooltip(cmdstr);
    }

    ImGui::SameLine();
    {
        Command command;
        command.script = script;
        command.args = " logout --assume-yes";
        command.type = Command::Type::OtherCommand;
        std::string cmdstr = Runner::MakeCommandHeadless(command);
        if (ImGui::Button("Logout"))
        {
            console.Print(cmdstr);
            runner.Run(cmdstr, console);
        }
        Tooltip(cmdstr);
    }

    dirty |= PathInput("What to push (.zip)", config.paths.zipPath, sizeof(config.paths.zipPath), PathMode::File, L"*.zip");
    dirty |= ImGui::InputText("user/game", config.usergame, sizeof(config.usergame));
    dirty |= ImGui::InputText("Channel", config.itchioChannel, sizeof(config.itchioChannel));

    {
        Command command;
        command.script = script;
        std::string channel = config.itchioChannel[0] == 0 ? "" : (std::string(":") + config.itchioChannel);
        command.args = " push -v " + std::string("\"") + std::string(config.paths.zipPath) + std::string("\"") + " \'" + config.usergame + channel + "\'";
        command.type = Command::Type::OtherCommand;
        std::string cmdstr = Runner::MakeCommandHeadless(command);

        if (ImGui::Button("Push"))
        {
            console.Print(cmdstr);
            runner.Run(cmdstr, console);
        }
        Tooltip(cmdstr);
    }
}
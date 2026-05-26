#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <array>
#include <memory>
#include <functional>

#include "theme.h"
#include "command.h"

class Runner;

class Pipeline
{
public:
    enum class Status
    {
        Awaiting,
        InProgress,
        Succeeded,
        Failed,
        Skipped
    };

    struct Stage
    {
        const char* label;
        Status status = Status::Awaiting;
        Command command;

        Stage(const char* label)
            : label(label)
        {
        }
    };

    static constexpr int INDEX_PREPARE = 0;
    static constexpr int INDEX_VERIFY  = 1;
    static constexpr int INDEX_PACKAGE = 2;
    static constexpr int INDEX_TEST    = 3;
    static constexpr int INDEX_DEPLOY  = 4;

    static constexpr int STAGE_COUNT  = 5;
    std::array<std::unique_ptr<Stage>, STAGE_COUNT> stages;

    Runner& runner;
    int stageEditIndex = 0;
    
    Pipeline(Runner& runner);

    void Init()
    {
        stages[INDEX_PREPARE] = std::make_unique<Stage>("Prepare");
        stages[INDEX_VERIFY]  = std::make_unique<Stage>("Verify");
        stages[INDEX_PACKAGE] = std::make_unique<Stage>("Package");
        stages[INDEX_TEST]    = std::make_unique<Stage>("Test");
        stages[INDEX_DEPLOY]  = std::make_unique<Stage>("Deploy");
    }

    inline Stage& GetStagePrepare () { return *stages[INDEX_PREPARE].get(); };
    inline Stage& GetStageVerify  () { return *stages[INDEX_VERIFY] .get(); };
    inline Stage& GetStagePackage () { return *stages[INDEX_PACKAGE].get(); };
    inline Stage& GetStageTest    () { return *stages[INDEX_TEST].get(); };
    inline Stage& GetStageDeploy  () { return *stages[INDEX_DEPLOY].get(); };
    inline Stage& GetStage   (int i) { return *stages[i] .get(); };

    ImU32 StatusColor(Status s) const;
    const char* StatusIcon(Status s);

    ///  Returns: index of stage clicked, or -1 if none. When running, clicking the first stage calls the "stop" logic (handled by the caller).
    int RenderPipe();
    ImVec2 PreRenderStage();
    void PostRenderStage(ImVec2 groupStart);
};

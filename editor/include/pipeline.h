#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <array>
#include <memory>
#include <functional>
#include "theme.h"

class Runner;

class Pipeline
{
public:
    enum class StageStatus
    {
        Awaiting,
        InProgress,
        Succeeded,
        Failed,
        Skipped
    };

    enum class PipelineStatus
    {
        Idle,
        InProgress,
        Succeeded,
        Failed
    };

    struct Stage
    {
        const char* label;
        StageStatus status = StageStatus::Awaiting;

        Stage(const char* label)
            : label(label)
        {
        }
    };

    static constexpr int INDEX_PREPARE = 0;
    static constexpr int INDEX_VERIFY  = 1;
    static constexpr int INDEX_PACKAGE = 2;
    static constexpr int INDEX_ARCHIVE = 3;
    static constexpr int INDEX_DEPLOY  = 4;

    static constexpr int STAGE_COUNT  = 5;
    std::array<std::unique_ptr<Stage>, STAGE_COUNT> stages;

    Runner& runner;
    PipelineStatus status = PipelineStatus::Idle;
    int stageEditIndex = 0;
    
    Pipeline(Runner& runner);

    void Init()
    {
        stages[INDEX_PREPARE] = std::make_unique<Stage>("Prepare");
        stages[INDEX_VERIFY]  = std::make_unique<Stage>("Verify");
        stages[INDEX_PACKAGE] = std::make_unique<Stage>("Package");
        stages[INDEX_ARCHIVE] = std::make_unique<Stage>("Archive");
        stages[INDEX_DEPLOY]  = std::make_unique<Stage>("Deploy");
    }

    inline Stage& GetStagePrepare () { return *stages[INDEX_PREPARE].get(); };
    inline Stage& GetStageVerify  () { return *stages[INDEX_VERIFY] .get(); };
    inline Stage& GetStagePackage () { return *stages[INDEX_PACKAGE].get(); };
    inline Stage& GetStageArchive () { return *stages[INDEX_ARCHIVE].get(); };
    inline Stage& GetStageDeploy() { return *stages[INDEX_DEPLOY].get(); };
    inline Stage& GetStage  (int i) { return *stages[i] .get(); };


    ImU32 StatusColor(StageStatus s);
    const char* StatusIcon(StageStatus s);
    //  Returns: index of stage clicked, or -1 if none. When running, clicking the first stage calls the "stop" logic (handled by the caller).
    int RenderPipe();
    ImVec2 PreRenderStage();
    void PostRenderStage(ImVec2 groupStart);
};

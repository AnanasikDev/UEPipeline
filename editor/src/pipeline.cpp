#include "pipeline.h"
#include "runner.h"

Pipeline::Pipeline(Runner& runner) : runner(runner)
{

}

int Pipeline::RenderPipe()
{
    int clicked = -1;

    const float boxH = 38.0f;
    const float boxMinW = 100.0f;
    const float connectorW = 24.0f;
    const float rounding = 6.0f;
    const float arrowSize = 6.0f;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 cursor = ImGui::GetCursorScreenPos();

    // Compute box width: fill available space evenly
    float totalAvail = ImGui::GetContentRegionAvail().x;
    float totalConn = connectorW * (STAGE_COUNT - 1);
    float boxW = (totalAvail - totalConn) / (float)STAGE_COUNT;
    if (boxW < boxMinW) boxW = boxMinW;

    float x = cursor.x;
    float y = cursor.y;

    for (int i = 0; i < STAGE_COUNT; i++)
    {
        const Pipeline::Stage& stage = GetStage(i);

        ImVec2 boxMin(x, y);
        ImVec2 boxMax(x + boxW, y + boxH);
        if (i == stageEditIndex)
        {
            boxMax.y += 40;
        }

        ImU32 fillCol;
        bool hovered = false;

        if (status == PipelineStatus::InProgress)
        {
            fillCol = StatusColor(stage.status);

            // Pulsing effect for in-progress stage
            if (stage.status == StageStatus::InProgress)
            {
                float t = (float)ImGui::GetTime();
                float pulse = 0.7f + 0.3f * (0.5f + 0.5f * sinf(t * 4.0f));
                ImVec4 base = ImGui::ColorConvertU32ToFloat4(fillCol);
                base.w = pulse;
                fillCol = ImGui::ColorConvertFloat4ToU32(base);
            }
        }
        else
        {
            // Idle mode: interactive buttons
            ImGui::SetCursorScreenPos(boxMin);
            ImGui::InvisibleButton(stage.label, ImVec2(boxW, boxH));
            hovered = ImGui::IsItemHovered();

            if (ImGui::IsItemClicked(0))
                clicked = i;

            bool selected = (i == stageEditIndex);
            if (selected)
                fillCol = hovered ? Theme::PipelineColors::IdleSelectedHov()
                : Theme::PipelineColors::IdleSelected();
            else
                fillCol = hovered ? Theme::PipelineColors::IdleHovered()
                : Theme::PipelineColors::IdleDefault();
        }

        dl->AddRectFilled(boxMin, boxMax, fillCol, rounding);

        if (status != PipelineStatus::InProgress && (hovered || i == stageEditIndex))
        {
            dl->AddRect(boxMin, boxMax,
                        IM_COL32(255, 255, 255, 40), rounding, 0, 1.5f);
        }

        const char* label = stage.label;
        ImVec2 labelSize = ImGui::CalcTextSize(label);

        if (status == PipelineStatus::InProgress)
        {
            const char* icon = StatusIcon(stage.status);
            ImVec2 iconSize = ImGui::CalcTextSize(icon);

            float contentW = iconSize.x + 6.0f + labelSize.x;
            float tx = boxMin.x + (boxW - contentW) * 0.5f;
            float ty = boxMin.y + (boxH - labelSize.y) * 0.5f;

            ImU32 iconCol = (stage.status == StageStatus::Awaiting)
                ? Theme::PipelineColors::TextDim()
                : Theme::PipelineColors::TextBright();

            dl->AddText(ImVec2(tx, ty), iconCol, icon);
            dl->AddText(ImVec2(tx + iconSize.x + 6.0f, ty),
                        Theme::PipelineColors::TextBright(), label);
        }
        else
        {
            float tx = boxMin.x + (boxW - labelSize.x) * 0.5f;
            float ty = boxMin.y + (boxH - labelSize.y) * 0.5f;

            ImU32 textCol = (i == stageEditIndex)
                ? Theme::PipelineColors::TextBright()
                : Theme::PipelineColors::TextDim();
            dl->AddText(ImVec2(tx, ty), textCol, label);
        }

        // Connector arrow to next box
        if (i < STAGE_COUNT - 1)
        {
            float cx0 = boxMax.x + 4.0f;
            float cx1 = boxMax.x + connectorW - 4.0f;
            float cy = y + boxH * 0.5f;

            ImU32 lineCol = (status == PipelineStatus::InProgress && stage.status == StageStatus::Succeeded)
                ? Theme::PipelineColors::ConnectorDone()
                : Theme::PipelineColors::Connector();

            dl->AddLine(ImVec2(cx0, cy), ImVec2(cx1, cy), lineCol, 2.0f);

            dl->AddTriangleFilled(
                ImVec2(cx1, cy),
                ImVec2(cx1 - arrowSize, cy - arrowSize * 0.6f),
                ImVec2(cx1 - arrowSize, cy + arrowSize * 0.6f),
                lineCol
            );
        }

        x += boxW + connectorW;
    }

    ImGui::SetCursorScreenPos(cursor);
    ImGui::Dummy(ImVec2(totalAvail, boxH));

    return clicked;
}

ImU32 Pipeline::StatusColor(StageStatus s)
{
    switch (s)
    {
        case StageStatus::Awaiting:   return Theme::PipelineColors::Awaiting();
        case StageStatus::InProgress: return Theme::PipelineColors::InProgress();
        case StageStatus::Succeeded:  return Theme::PipelineColors::Succeeded();
        case StageStatus::Failed:     return Theme::PipelineColors::Failed();
        case StageStatus::Skipped:    return Theme::PipelineColors::Skipped();
    }
    return Theme::PipelineColors::Awaiting();
}

const char* Pipeline::StatusIcon(StageStatus s)
{
    switch (s)
    {
        case StageStatus::Awaiting:   return "...";
        case StageStatus::InProgress: return ">>>";
        case StageStatus::Succeeded:  return "OK";
        case StageStatus::Failed:     return "X";
        case StageStatus::Skipped:    return "--";
    }
    return "";
}

static const float rounding = 6.0f;
static const float padding = 8.0f;
ImVec2 Pipeline::PreRenderStage()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->ChannelsSplit(2);
    dl->ChannelsSetCurrent(1);

    ImVec2 groupStart = ImGui::GetCursorScreenPos();
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(0, padding));
    ImGui::Indent(padding);

    return groupStart;
}

void Pipeline::PostRenderStage(ImVec2 groupStart)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::Unindent(padding);
    ImGui::Dummy(ImVec2(0, padding));
    ImGui::EndGroup();

    ImVec2 boxMin = groupStart;
    ImVec2 boxMax = ImVec2(
        groupStart.x + ImGui::GetContentRegionAvail().x + padding,
        ImGui::GetItemRectMax().y
    );

    dl->ChannelsSetCurrent(0);
    ImU32 fillCol = Theme::PipelineColors::IdleSelected();
    dl->AddRectFilled(boxMin, boxMax, fillCol, rounding);

    dl->ChannelsMerge();

}
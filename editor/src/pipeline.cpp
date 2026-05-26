#include "pipeline.h"
#include "runner.h"

Pipeline::Pipeline(Runner& runner) : runner(runner)
{

}

int Pipeline::RenderPipe()
{
    int clicked = -1;

    constexpr float boxH = 38.0f;
    constexpr float boxMinW = 100.0f;
    constexpr float connectorW = 24.0f;
    constexpr float rounding = 6.0f;
    constexpr float arrowSize = 6.0f;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 cursor = ImGui::GetCursorScreenPos();

    // Compute box width: fill available space evenly
    const float totalAvail = ImGui::GetContentRegionAvail().x;
    const float totalConn = connectorW * (STAGE_COUNT - 1);
    float boxW = (totalAvail - totalConn) / static_cast<float>(STAGE_COUNT);
    if (boxW < boxMinW)
    {
        boxW = boxMinW;
    }

    float x = cursor.x;
    const float y = cursor.y;

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

        if (stage.status == Status::InProgress)
        {
            fillCol = StatusColor(stage.status);

            // Pulsing effect for in-progress stage
            float t = (float)ImGui::GetTime();
            float pulse = 0.7f + 0.3f * (0.5f + 0.5f * sinf(t * 4.0f));
            ImVec4 base = ImGui::ColorConvertU32ToFloat4(fillCol);
            base.w = pulse;
            fillCol = ImGui::ColorConvertFloat4ToU32(base);
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

        if (hovered || i == stageEditIndex)
        {
            dl->AddRect(boxMin, boxMax,
                        IM_COL32(255, 255, 255, 40), rounding, 0, 1.5f);
        }

        const char* label = stage.label;
        ImVec2 labelSize = ImGui::CalcTextSize(label);

        if (true)
        {
            const char* icon = StatusIcon(stage.status);
            ImVec2 iconSize = ImGui::CalcTextSize(icon);

            float contentW = iconSize.x + 6.0f + labelSize.x;
            float tx = boxMin.x + (boxW - contentW) * 0.5f;
            float ty = boxMin.y + (boxH - labelSize.y) * 0.5f;

            ImU32 iconCol = (stage.status == Status::Awaiting)
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

            ImU32 lineCol = Theme::PipelineColors::Connector();

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

ImU32 Pipeline::StatusColor(Status s) const
{
    switch (s)
    {
        case Status::Awaiting:   return Theme::PipelineColors::Awaiting();
        case Status::InProgress: return Theme::PipelineColors::InProgress();
        case Status::Succeeded:  return Theme::PipelineColors::Succeeded();
        case Status::Failed:     return Theme::PipelineColors::Failed();
        case Status::Skipped:    return Theme::PipelineColors::Skipped();
    }
    return Theme::PipelineColors::Awaiting();
}

const char* Pipeline::StatusIcon(Status s)
{
    switch (s)
    {
        case Status::Awaiting:   return "";
        case Status::InProgress: return ">>>";
        case Status::Succeeded:  return "OK";
        case Status::Failed:     return "X";
        case Status::Skipped:    return "--";
    }
    return "";
}

static const float rounding = 6.0f;
static const float padding = 8.0f;

ImVec2 Pipeline::PreRenderStage()
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->ChannelsSplit(2);
    drawList->ChannelsSetCurrent(1);

    const ImVec2 groupStart = ImGui::GetCursorScreenPos();
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(0, padding));
    ImGui::Indent(padding);

    return groupStart;
}

void Pipeline::PostRenderStage(ImVec2 groupStart)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImGui::Unindent(padding);
    ImGui::Dummy(ImVec2(0, padding));
    ImGui::EndGroup();

    const ImVec2 boxMin = groupStart;
    const ImVec2 boxMax = ImVec2(
        groupStart.x + ImGui::GetContentRegionAvail().x + padding,
        ImGui::GetItemRectMax().y
    );

    drawList->ChannelsSetCurrent(0);
    ImU32 fillCol = Theme::PipelineColors::IdleSelected();
    drawList->AddRectFilled(boxMin, boxMax, fillCol, rounding);

    drawList->ChannelsMerge();
}
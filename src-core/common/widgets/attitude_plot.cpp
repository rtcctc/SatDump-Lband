#include "attitude_plot.h"
#include "core/style.h"
#include <cmath>
#include <cstdio>

namespace widgets
{
    void AttitudePlotViewer::draw(float pitch, float yaw, bool has_value, float range)
    {
        d_pitch = pitch;
        d_yaw   = yaw;
        d_has_value = has_value;
        d_range = range;

        const float size = 160.0f * ui_scale;
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImDrawList *dl = ImGui::GetWindowDrawList();

        ImVec2 p_min = cursor;
        ImVec2 p_max = ImVec2(cursor.x + size, cursor.y + size);

        float cx = cursor.x + size * 0.5f;
        float cy = cursor.y + size * 0.5f;
        float radius = size * 0.45f;

        dl->AddRectFilled(p_min, p_max, IM_COL32(20, 20, 20, 255), 4.0f);
        dl->AddCircle(ImVec2(cx, cy), radius, IM_COL32(80, 80, 80, 255), 64, 1.5f);

        for (float frac : {0.25f, 0.5f, 0.75f})
            dl->AddCircle(ImVec2(cx, cy), radius * frac, IM_COL32(50, 50, 50, 255), 48, 1.0f);

        dl->AddLine(ImVec2(cx - radius, cy), ImVec2(cx + radius, cy), IM_COL32(70, 70, 70, 255), 1.0f);
        dl->AddLine(ImVec2(cx, cy - radius), ImVec2(cx, cy + radius), IM_COL32(70, 70, 70, 255), 1.0f);

        dl->AddCircleFilled(ImVec2(cx, cy), 2.0f, IM_COL32(120, 120, 120, 255));

        if (has_value)
        {
            float nx = d_yaw   / d_range;
            float ny = d_pitch / d_range;

            float mag = std::sqrt(nx * nx + ny * ny);
            if (mag > 1.0f)
            {
                nx /= mag;
                ny /= mag;
            }

            float px = cx + nx * radius;
            float py = cy - ny * radius;

            ImU32 color = (mag > 1.0f) ? IM_COL32(255, 80, 80, 255)
                                        : IM_COL32(80, 255, 80, 255);
            dl->AddCircleFilled(ImVec2(px, py), 5.0f, color);
            dl->AddLine(ImVec2(cx, cy), ImVec2(px, py), color, 1.5f);
        }

        dl->AddRect(p_min, p_max, IM_COL32(100, 100, 100, 255), 4.0f, 0, 1.5f);

        // Резервируем размер виджета
        ImGui::SetCursorScreenPos(cursor);
        ImGui::Dummy(ImVec2(size, size + 50.0f * ui_scale));
    }
}
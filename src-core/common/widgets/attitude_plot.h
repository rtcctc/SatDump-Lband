#pragma once
#include "imgui/imgui.h"

namespace widgets
{
    class AttitudePlotViewer
    {
    private:
        float d_pitch = 0.0f;
        float d_yaw   = 0.0f;
        bool  d_has_value = false;

        // Границы отображения (градусы)
        float d_range = 30.0f;

    public:
        // pitch, yaw — текущие углы, range — половина размера поля (например, 30° => поле ±30°)
        void draw(float pitch, float yaw, bool has_value, float range = 30.0f);
    };
}
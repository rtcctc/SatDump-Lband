#pragma once

#include "imgui/imgui.h"
#include "complex"

namespace widgets
{
    class CNPlotViewer
    {
    private:
        float snr_history[200] = {0};   // <-- инициализация прямо здесь

    public:
        void draw(float snr = 1, float peak_snr = 1);
    };
}
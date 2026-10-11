#pragma once
#include "common/dsp/utils/cn_estimator.h"
#include "modules/demod/module_demod_base.h"
#include "common/widgets/cn_plot.h"

namespace demod
{
    class BeaconCNModule : public BaseDemodModule
    {
    private:
        std::shared_ptr<dsp::CNEstimator> cn_estimator;
        int d_fft_size = 1024;

        widgets::CNPlotViewer cn_plot;

        // Локальные значения — для логов и HTTP API.
        // Для UI используются базовые snr / peak_snr (их читает snr_plot).
        float d_cn = 0.0f;
        float d_peak_cn = -100.0f;
        float d_avg_cn = 0.0f;
        int update_counter = 0;

    public:
        BeaconCNModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~BeaconCNModule();

        void init();
        void process();
        void stop();
        void drawUI(bool window) override;

        static std::string getID();
        std::string getIDM() override;
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(
            std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}
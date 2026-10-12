#pragma once
#include "common/dsp/utils/cn_estimator.h"
#include "common/dsp/utils/gyro_serial.h"
#include "modules/demod/module_demod_base.h"
#include "common/widgets/cn_plot.h"
#include "common/widgets/attitude_plot.h"
#include <string>

namespace demod
{
    class BeaconCNModule : public BaseDemodModule
    {
    private:
        std::shared_ptr<dsp::CNEstimator> cn_estimator;
        dsp::GyroSerial gyro;

        int d_fft_size = 1024;
        std::string d_serial_port = "/dev/ttyUSB0";
        int d_serial_baud = 115200;

        widgets::CNPlotViewer cn_plot;
        widgets::AttitudePlotViewer attitude_plot;

        float d_cn = 0.0f;
        float d_peak_cn = -100.0f;
        float d_avg_cn = 0.0f;
        int update_counter = 0;

        // Гироскоп
        float d_gyro_pitch = 0.0f;
        float d_gyro_yaw   = 0.0f;
        bool  d_gyro_ok    = false;

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
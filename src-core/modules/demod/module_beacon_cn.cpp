#include "module_beacon_cn.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <thread>
#include <chrono>

namespace demod
{
    BeaconCNModule::BeaconCNModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    : BaseDemodModule(input_file, output_file_hint, parameters)
    {
        if (parameters.count("fft_size") > 0)
            d_fft_size = parameters["fft_size"].get<int>();

        if (parameters.count("serial_port") > 0)
            d_serial_port = parameters["serial_port"].get<std::string>();

        if (parameters.count("serial_baud") > 0)
            d_serial_baud = parameters["serial_baud"].get<int>();

        name = "Beacon C/N Monitor";
        show_freq = false;
    }

    BeaconCNModule::~BeaconCNModule() {}

    void BeaconCNModule::init()
    {
        BaseDemodModule::initb();
        cn_estimator = std::make_shared<dsp::CNEstimator>(d_fft_size);

        if (!gyro.start(d_serial_port, d_serial_baud))
            logger->warn("Gyro not available, continuing without it");
    }

    void BeaconCNModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        logger->info("Beacon C/N Monitor started");
        logger->info("FFT size : %d", d_fft_size);
        logger->info("Serial Port: %s", d_serial_port.c_str());

        BaseDemodModule::start();

        time_t lastTime = 0;

        while (demod_should_run())
        {
            int nsamples = agc->output_stream->read();

            if (nsamples <= 0)
            {
                agc->output_stream->flush();
                continue;
            }

            // Созвездие — базовый виджет, тот же, что у PSK
            constellation.pushComplex(agc->output_stream->readBuf, nsamples);

            // Считаем C/N
            cn_estimator->update(agc->output_stream->readBuf, nsamples);

            if (cn_estimator->has_value())
            {
                d_cn = cn_estimator->cn();

                if (d_cn > d_peak_cn + 2.0f)
                {
                    d_peak_cn = d_cn;
                    gyro.send_reset();
                    logger->info("New C/N peak %.2f dB, gyro reset", d_cn);
                }
                else if (d_cn > d_peak_cn)
                {
                    d_peak_cn = d_cn;
                }

                update_counter++;
                d_avg_cn += (d_cn - d_avg_cn) / update_counter;
            }

            agc->output_stream->flush();

            //Опрос Serial
            static int gyro_div = 0;
            if (++gyro_div >= 20)
            {
                gyro_div = 0;
                d_gyro_ok = gyro.get_last(d_gyro_pitch, d_gyro_yaw);
                module_stats["gyro_pitch"] = d_gyro_pitch;
                module_stats["gyro_yaw"]   = d_gyro_yaw;
                module_stats["gyro_ok"]    = d_gyro_ok;
            }

            // HTTP API
            module_stats["cn"] = d_cn;
            module_stats["peak_cn"] = d_peak_cn;
            module_stats["avg_cn"] = d_avg_cn;

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("C/N : %.2f dB, Peak : %.2f dB, Avg : %.2f dB",
                             d_cn, d_peak_cn, d_avg_cn);
            }
        }

        logger->info("Beacon C/N Monitor finished");

        if (input_data_type == DATA_FILE)
            stop();
    }

    void BeaconCNModule::stop()
    {
        logger->info("BeaconCNModule::stop called");
        gyro.stop();

        // Останавливает AGC, file_source, fft и т.д.
        BaseDemodModule::stop();

        // Разбудить заблокированный read() в process() — аналог PSK-овского rec->output_stream->stopReader()
        if (agc)
            agc->output_stream->stopReader();

        logger->info("BeaconCNModule::stop finished");
    }

    void BeaconCNModule::drawUI(bool window)
    {
        ImGui::SetNextWindowSize(
            ImVec2(ImGui::GetIO().DisplaySize.x,      // ширина — во весь экран
                   500 * ui_scale),                    // высота — с запасом под прицел
            ImGuiCond_FirstUseEver);
        ImGui::Begin(name.c_str(), NULL, window ? 0 : NOWINDOW_FLAGS);

        // Созвездие (то же, что у PSK) — слева
        ImGui::BeginGroup();
        constellation.draw();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Beacon C/N", {200 * ui_scale, 20 * ui_scale});
            cn_plot.draw(d_cn, d_peak_cn);

            if (d_gyro_ok)
            {
                ImGui::TextColored(style::theme.light_green, "Pitch :");
                ImGui::SameLine();
                ImGui::Text("%+7.2f°", d_gyro_pitch);

                ImGui::TextColored(style::theme.light_green, "Yaw   :");
                ImGui::SameLine();
                ImGui::Text("%+7.2f°", d_gyro_yaw);
            }
            else
            {
                ImGui::TextColored(style::theme.red, "No gyro data");
                ImGui::Text(" ");
            }
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Attitude", {160 * ui_scale, 20 * ui_scale});
            attitude_plot.draw(d_gyro_pitch, d_gyro_yaw, d_gyro_ok, 90.0f);
        }
        ImGui::EndGroup();

        // Прогресс — только для файла
        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize,
                               ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        // Кнопка Abort — работает и в live, и в файле
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        {
            ImGuiStyle &style = ImGui::GetStyle();
            ImVec2 cur_pos = ImGui::GetCursorPos();
            cur_pos.x = ImGui::GetContentRegionMax().x
                        - ImGui::CalcTextSize("Abort").x
                        - style.FramePadding.x * 2.0f;
            cur_pos.y -= 20.0f * ui_scale + style.ItemSpacing.y;
            ImGui::SetCursorPos(cur_pos);
            ImGui::PushStyleColor(ImGuiCol_Button, style::theme.red.Value);
            if (ImGui::Button("Abort##beacon_cn_stop", ImVec2(0.0f, 20.0f * ui_scale)))
                demod_should_stop = true;
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Stop monitoring.");
        }

        ImGui::End();
    }

    std::string BeaconCNModule::getID()
    {
        return "beacon_cn";
    }

    std::string BeaconCNModule::getIDM()
    {
        return "Beacon C/N Monitor";
    }

    std::vector<std::string> BeaconCNModule::getParameters()
    {
        std::vector<std::string> params = {"fft_size"};
        params.insert(params.end(),
                      BaseDemodModule::getParameters().begin(),
                      BaseDemodModule::getParameters().end());
        return params;
    }

    std::shared_ptr<ProcessingModule> BeaconCNModule::getInstance(
        std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<BeaconCNModule>(input_file, output_file_hint, parameters);
    }
}
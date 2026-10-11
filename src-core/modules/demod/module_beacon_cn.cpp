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

        name = "Beacon C/N Monitor";
        show_freq = false;
    }

    BeaconCNModule::~BeaconCNModule() {}

    void BeaconCNModule::init()
    {
        BaseDemodModule::initb();
        cn_estimator = std::make_shared<dsp::CNEstimator>(d_fft_size);
    }

    void BeaconCNModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        logger->info("Beacon C/N Monitor started");
        logger->info("FFT size : %d", d_fft_size);

        BaseDemodModule::start();

        time_t lastTime = 0;

        while (demod_should_run())
        {
            int nsamples = agc->output_stream->read();

            if (nsamples <= 0)
            {
                agc->output_stream->flush();
                if (!demod_should_run())
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            // Созвездие — базовый виджет, тот же, что у PSK
            constellation.pushComplex(agc->output_stream->readBuf, nsamples);

            // Считаем C/N
            cn_estimator->update(agc->output_stream->readBuf, nsamples);

            if (cn_estimator->has_value())
            {
                d_cn = cn_estimator->cn();

                // Пишем в БАЗОВЫЕ поля — их читает snr_plot.draw() в drawUI()
                snr = d_cn;
                if (d_cn > peak_snr)
                    peak_snr = d_cn;

                update_counter++;
                d_avg_cn += (d_cn - d_avg_cn) / update_counter;
            }

            agc->output_stream->flush();

            // HTTP API
            module_stats["cn"] = d_cn;
            module_stats["peak_cn"] = peak_snr;
            module_stats["avg_cn"] = d_avg_cn;

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("C/N : %.2f dB, Peak : %.2f dB, Avg : %.2f dB",
                             d_cn, peak_snr, d_avg_cn);
            }
        }

        logger->info("Beacon C/N Monitor finished");
        stop();
    }

    void BeaconCNModule::stop()
    {
        BaseDemodModule::stop();
    }

    void BeaconCNModule::drawUI(bool window)
    {
        ImGui::Begin(name.c_str(), NULL, window ? 0 : NOWINDOW_FLAGS);

        // Созвездие (то же, что у PSK) — слева
        ImGui::BeginGroup();
        constellation.draw();
        ImGui::EndGroup();

        ImGui::SameLine();

        // Наш блок — справа
        ImGui::BeginGroup();
        {
            ImGui::Button("Beacon C/N", {200 * ui_scale, 20 * ui_scale});
            cn_plot.draw(snr, peak_snr);   // вместо snr_plot
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
#pragma once // Защита от повторного включения заголовка

#include "core/module.h"                        // Базовый класс ProcessingModule и ModuleDataType
#include <complex>                              // Для работы с комплексными числами (не используется напрямую здесь)
#include <thread>                               // Потоки (используется базовым классом)
#include <fstream>                              // Файловые потоки ifstream/ofstream
#include "deframer.h"                           // Определение CADUDeframer и CADU_SIZE
#include "common/widgets/constellation.h"       // Виджет отображения созвездия (диаграмма I/Q)

namespace meteor
{
    // Модуль декодирования METEOR HRPT.
    // Вход: мягкие символы (soft symbols, int8_t) из демодулятора.
    // Выход: поток кадров CADU (Channel Access Data Unit), записанный в файл.
    class METEORHRPTDecoderModule : public ProcessingModule
    {
    protected:
        // Дефреймер CADU — ищет синхрослово в мягких символах и нарезает поток на кадры
        std::shared_ptr<CADUDeframer> def;

        // Буфер для мягких символов (soft decision, -127..127).
        // Каждый байт — не «0/1», а уровень уверенности демодулятора в бите.
        int8_t *soft_buffer;

        std::ifstream data_in;  // Входной файл (если вход — файл)
        std::ofstream data_out; // Выходной файл .cadu

        // Размер файла и текущая позиция — атомарные, чтобы UI-поток мог читать безопасно
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        // UI: диаграмма созвездия (визуализация I/Q символов)
        widgets::ConstellationViewer constellation;

    public:
        // Конструктор: инициализирует базовый модуль, дефреймер и буфер символов
        METEORHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~METEORHRPTDecoderModule();

        void process();               // Главный цикл обработки (выполняется в отдельном потоке)
        void drawUI(bool window) override;     // Отрисовка интерфейса ImGui
        std::vector<ModuleDataType> getInputTypes();   // Типы поддерживаемого входа
        std::vector<ModuleDataType> getOutputTypes();  // Типы поддерживаемого выхода

    public:
        // Фабричные методы — используются системой плагинов SatDump
        static std::string getID();                  // Уникальный ID модуля
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters(); // Список параметров, читаемых из JSON
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        bool embed_in_parent = true;
    };
} // namespace meteor
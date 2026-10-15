#pragma once

#include <cstdint>
#include <vector>
#include "common/image/image.h"   // Класс image::Image — внутренний формат изображений SatDump

namespace meteor
{
    namespace msumr   // MSU-MR — сканер Метеора-М (аналог AVHRR), 6 каналов
    {
        // Класс, который принимает один кадр (строку) сырых данных MSU-MR
        // и накапливает данные по 6 спектральным каналам, а также
        // калибровочную информацию.
        class MSUMRReader
        {
        private:
            // Данные по 6 каналам. Каждый элемент — одно 10-битное значение,
            // расширенное (сдвинутое) до 16 бит для удобства хранения.
            std::vector<uint16_t> channels[6];

        public:
            // Калибровочные слова: [канал][0 или 1] — по два слова на канал.
            // Используются калибратором MSUMRCalibrator для перевода
            // «сырых» значений в физические величины.
            std::vector<uint16_t> calibration_info[6][2];

        public:
            MSUMRReader();
            ~MSUMRReader();
            int lines;                      // Число принятых строк (кадров)
            void work(uint8_t *buffer);     // Обработать один кадр
            image::Image getChannel(int channel);   // Получить изображение по каналу
        };
    } // namespace msumr
} // namespace meteor
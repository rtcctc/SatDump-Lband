#pragma once

#include <cstdint>
#include <vector>
#include "common/image/image.h"   // Внутренний формат изображений SatDump

namespace meteor
{
    namespace mtvza   // MTVZA — микроволновый сканер-зондировщик (30 каналов)
    {
        // Класс, принимающий один кадр (сканирование) MTVZA и накапливающий
        // данные по 30 каналам, а также временные метки.
        class MTVZAReader
        {
        private:
            // Данные по 30 каналам. Каждый элемент — 16-битное значение,
            // уже приведённое к знаковому виду (вычтено 32768).
            std::vector<uint16_t> channels[30];

            // Внутренний разбор одной порции сэмплов.
            //   data       — указатель на буфер кадра
            //   ch_start   — индекс первого канала в массиве channels
            //   offset     — смещение в буфере для начала данных этого типа
            //   ch_cnt     — сколько каналов обрабатывать
            //   nsamples   — сколько сэмплов на канал (1, 2 или 4)
            //   counter    — номер позиции сканирования (0..24)
            void parse_samples(uint8_t *data, int ch_start, int offset, int ch_cnt, int nsamples, int counter);

        public:
            MTVZAReader();
            ~MTVZAReader();

            int lines;                          // Число полностью принятых строк (сканов)
            std::vector<double> timestamps;     // Временные метки для каждой строки

            // Временная метка от MSU-MR, которая будет привязана к текущей строке MTVZA.
            // Устанавливается извне (например, модулем синхронизации времени).
            double latest_msumr_timestamp = -1;

            void work(uint8_t *data);           // Обработать один кадр
            image::Image getChannel(int channel); // Получить изображение по каналу

            // Флаг порядка байт во входных данных (big-endian / little-endian).
            // По умолчанию — false (little-endian, как в типичном HRPT).
            bool endian_mode = false;
        };
    }
}
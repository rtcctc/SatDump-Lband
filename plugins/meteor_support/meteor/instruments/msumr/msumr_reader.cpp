#include "msumr_reader.h"
#include <cstdio>

namespace meteor
{
    namespace msumr
    {
        // Конструктор: заранее резервируем место под одну строку в каждом канале.
        // Одна строка = 1572 пикселя (4 пикселя × 393 группы).
        MSUMRReader::MSUMRReader()
        {
            for (int i = 0; i < 6; i++)
                channels[i].resize(1572);
            lines = 0;
        }

        // Деструктор — освобождаем память всех каналов
        MSUMRReader::~MSUMRReader()
        {
            for (int i = 0; i < 6; i++)
                channels[i].clear();
        }

        // Основная функция: разбирает один кадр (строку) MSU-MR из буфера.
        // Формат кадра:
        //   - 35..? байт — калибровочные слова (по 5 байт на 4 слова)
        //   - 50..? байт — данные пикселей, по 5 байт на 4 пикселя × 6 каналов
        // Каждое «слово» — 10-битное значение (0..1023), упакованное по 4 штуки в 5 байт.
        void MSUMRReader::work(uint8_t *buffer)
        {
            // ---- Разбор данных по каналам ----
            // 393 группы по 4 пикселя = 1572 пикселя на канал.
            // Каждый канал сдвинут по смещению channel * 5 байт (по 5 байт на канал).
            for (int channel = 0; channel < 6; channel++)
            {
                for (int l = 0; l < 393; l++)
                {
                    // Адрес начала 5-байтовой группы для данного канала
                    int pixelpos = 50 + l * 30 + channel * 5;

                    // Разбираем 5 байт на 4 10-битных слова.
                    // Затем каждое слово сдвигаем влево на 6 бит —
                    // фактически «выравниваем» 10-битное значение в 16-битной ячейке.
                    //
                    // Схема упаковки (побитово):
                    //   слово 0: bits[7..0] байта 0, затем старшие 2 бита байта 1
                    //   слово 1: младшие 6 бит байта 1, затем старшие 4 бита байта 2
                    //   слово 2: младшие 4 бита байта 2, затем старшие 6 бит байта 3
                    //   слово 3: младшие 2 бита байта 3, затем весь байт 4
                    channels[channel][lines * 1572 + l * 4 + 0] = ((buffer[pixelpos] << 2) | (buffer[pixelpos + 1] >> 6)) << 6;
                    channels[channel][lines * 1572 + l * 4 + 1] = (((buffer[pixelpos + 1] % 64) << 4) | (buffer[pixelpos + 2] >> 4)) << 6;
                    channels[channel][lines * 1572 + l * 4 + 2] = (((buffer[pixelpos + 2] % 16) << 6) | (buffer[pixelpos + 3] >> 2)) << 6;
                    channels[channel][lines * 1572 + l * 4 + 3] = (((buffer[pixelpos + 3] % 4) << 8) | buffer[pixelpos + 4]) << 6;
                }
            }

            // ---- Разбор калибровочных слов ----
            // 3 группы по 5 байт = 12 слов по 10 бит (без сдвига <<6 —
            // калибровочные значения хранятся как есть).
            uint16_t words10_bits[12];
            for (int n = 0; n < 3; n++)
            {
                int pixelpos = 35 + n * 5;
                words10_bits[n * 4 + 0] = ((buffer[pixelpos] << 2) | (buffer[pixelpos + 1] >> 6));
                words10_bits[n * 4 + 1] = (((buffer[pixelpos + 1] % 64) << 4) | (buffer[pixelpos + 2] >> 4));
                words10_bits[n * 4 + 2] = (((buffer[pixelpos + 2] % 16) << 6) | (buffer[pixelpos + 3] >> 2));
                words10_bits[n * 4 + 3] = (((buffer[pixelpos + 3] % 4) << 8) | buffer[pixelpos + 4]);
            }

            // Раскладываем 12 слов по 6 каналам по 2 слова на канал.
            // Эти значения будут использоваться калибратором для каждой строки.
            for (int channel = 0; channel < 6; channel++)
                for (int l = 0; l < 2; l++)
                    calibration_info[channel][l].push_back(words10_bits[channel * 2 + l]);

            // Увеличиваем счётчик строк
            lines++;

            // Расширяем буферы каналов на следующую строку,
            // чтобы при следующем вызове work() было куда писать.
            for (int channel = 0; channel < 6; channel++)
                channels[channel].resize((lines + 1) * 1572);
        }

        // Возвращает изображение по заданному каналу.
        //   - 16 — бит на пиксель (значения уже расширены сдвигом <<6)
        //   - 1572 — ширина
        //   - lines — высота
        //   - 1 — число каналов (grayscale)
        image::Image MSUMRReader::getChannel(int channel)
        {
            return image::Image(channels[channel].data(), 16, 1572, lines, 1);
        }
    } // namespace msumr
} // namespace meteor
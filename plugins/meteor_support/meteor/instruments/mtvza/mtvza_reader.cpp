#include "mtvza_reader.h"

namespace meteor
{
    namespace mtvza
    {
        // Конструктор: выделяем место под одну строку в каждом канале.
        // Строка MTVZA = 100 пикселей, но массив изначально в 2 раза больше (200),
        // так как в процессе работы он расширяется на следующую строку.
        MTVZAReader::MTVZAReader()
        {
            for (int i = 0; i < 30; i++)
                channels[i].resize(100 * 2);
            lines = 0;
        }

        // Деструктор — очищаем память всех каналов
        MTVZAReader::~MTVZAReader()
        {
            for (int i = 0; i < 30; i++)
                channels[i].clear();
        }

        // Разбор одной группы сэмплов для нескольких каналов.
        //
        // MTVZA передаёт данные в разных разрешениях:
        //   - 1 сэмпл на канал (низкое разрешение, 5 каналов)
        //   - 4 сэмпла на канал (полное разрешение, 2 канала)
        //   - 2 сэмпла на канал (среднее разрешение, 23 канала)
        //
        // Внутри буфера данные упакованы по 2 байта на сэмпл (16-битные),
        // причём порядок байт может быть big-endian или little-endian
        // в зависимости от endian_mode.
        //
        // Для каждой группы каналов (ch_cnt) и каждого из 4 сэмплов (i = 0..3)
        // вычисляется позиция sample_position и из буфера читаются два
        // 16-битных значения: одно по смещению 8 (для counter*8+0..3),
        // другое по смещению 128 (для counter*8+4..7).
        void MTVZAReader::parse_samples(uint8_t *data, int ch_start, int offset, int ch_cnt, int nsamples, int counter)
        {
            for (int ch = 0; ch < ch_cnt; ch++)
            {
                for (int i = 0; i < 4; i++)
                {
                    // Позиция сэмпла для текущего канала и «под-сэмпла» i.
                    // В зависимости от nsamples (1, 2 или 4) добавляется
                    // дополнительное смещение, чтобы «растянуть» сэмплы
                    // на все 4 позиции внутри группы.
                    int sample_position = ch * nsamples + offset;
                    if (nsamples == 2)
                        sample_position += i / 2;      // для 2 сэмплов — пара на каждые 2 позиции
                    else if (nsamples == 4)
                        sample_position += i;          // для 4 сэмплов — свой на каждую позицию

                    // Читаем 16-битное значение из буфера. Порядок байт
                    // зависит от endian_mode: если true — big-endian, иначе little-endian.
                    // Затем вычитаем 32768, чтобы перевести беззнаковое
                    // 0..65535 в знаковое -32768..32767 (данные MTVZA знаковые).
                    channels[ch_start + ch][lines * 100 + counter * 8 + 0 + i] =
                        (data[8 + sample_position * 2 + (endian_mode ? 0 : 1)] << 8 |
                         data[8 + sample_position * 2 + (endian_mode ? 1 : 0)]) - 32768;

                    // Второе значение — по смещению 128 в буфере (вторая половина строки)
                    channels[ch_start + ch][lines * 100 + counter * 8 + 4 + i] =
                        (data[128 + sample_position * 2 + (endian_mode ? 0 : 1)] << 8 |
                         data[128 + sample_position * 2 + (endian_mode ? 1 : 0)]) - 32768;
                }
            }
        }

        // Основная функция: обрабатывает один кадр MTVZA.
        void MTVZAReader::work(uint8_t *data)
        {
            // Проверка маркера: байт 4 или 5 (в зависимости от порядка байт)
            // должен быть равен 0xFF. Если нет — кадр невалидный, пропускаем.
            if (data[endian_mode ? 5 : 4] != 255)
                return;

            // Номер позиции сканирования (counter). Допустимы значения 2..26.
            int counter = data[endian_mode ? 4 : 5];

            if (counter > 26 || counter < 2)
                return;

            // ---- Разбор трёх групп каналов ----
            // 1) 5 каналов низкого разрешения (1 сэмпл на канал)
            parse_samples(data, 0, 0, 5, 1, counter - 2);

            // 2) 2 канала полного разрешения (4 сэмпла на канал)
            parse_samples(data, 5, 5, 2, 4, counter - 2);

            // 3) 23 канала среднего разрешения (2 сэмпла на канал)
            parse_samples(data, 7, 13, 23, 2, counter - 2);

            // printf("%d\n", counter);

            // Когда счётчик доходит до 26 — это конец полного сканирования.
            // Добавляем временную метку (от MSU-MR) и увеличиваем счётчик строк.
            if (counter == 26)
            {
                timestamps.push_back(latest_msumr_timestamp);
                lines++;
            }

            // Расширяем буферы всех каналов на две строки вперёд,
            // чтобы при следующем вызове work() было куда писать.
            for (int channel = 0; channel < 30; channel++)
                channels[channel].resize((lines + 2) * 200);
        }

        // Возвращает изображение по каналу.
        //   - 16 бит на пиксель (знаковые значения после вычитания 32768)
        //   - ширина 100
        //   - высота lines
        //   - 1 канал (grayscale)
        image::Image MTVZAReader::getChannel(int channel)
        {
            return image::Image(channels[channel].data(), 16, 100, lines, 1);
        }
    }
}
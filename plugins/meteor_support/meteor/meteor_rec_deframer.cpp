#include "meteor_rec_deframer.h"

namespace meteor
{
    // Расстояние Хэмминга: количество различающихся бит.
    // Алгоритм: XOR даёт 1 там, где биты различаются;
    // затем цикл `diff &= diff - 1` за одну итерацию обнуляет младший установленный бит.
    int KMSS_QPSK_ExtDeframer::compare_8(uint8_t v1, uint8_t v2)
    {
        int cor = 0;
        uint8_t diff = v1 ^ v2;
        for (; diff; cor++)
            diff &= diff - 1;
        return cor;
    }

    // Упаковка 8 отдельных бит в один байт (старший бит первый).
    // Внимание: функция нигде не вызывается в этом файле — вероятно, наследие.
    uint8_t KMSS_QPSK_ExtDeframer::repack_8(uint8_t *bit)
    {
        return bit[0] << 7 |
               bit[1] << 6 |
               bit[2] << 5 |
               bit[3] << 4 |
               bit[4] << 3 |
               bit[5] << 2 |
               bit[6] << 1 |
               bit[7];
    }

    KMSS_QPSK_ExtDeframer::KMSS_QPSK_ExtDeframer()
    {
        // Буфер на (размер_кадра/8)+1 байт — доп. байт для сдвига по битам
        shift_buffer = new uint8_t[KMSS_QPSK_REC_FRM_SIZE];
    }

    KMSS_QPSK_ExtDeframer::~KMSS_QPSK_ExtDeframer()
    {
        delete[] shift_buffer;
    }

    int KMSS_QPSK_ExtDeframer::work(uint8_t *input_bytes, int length, uint8_t *output)
    {
        int frames_out = 0;
        for (int i = 0; i < length; i++)
        {
            // Сдвигаем буфер на 1 байт и добавляем новый входной байт
            memmove(&shift_buffer[0], &shift_buffer[1], ((KMSS_QPSK_REC_FRM_SIZE / 8) + 1) - 1);
            shift_buffer[((KMSS_QPSK_REC_FRM_SIZE / 8) + 1) - 1] = input_bytes[i];

            // Перебираем все 8 битовых сдвигов внутри байта
            for (int s = 0; s < 8; s++)
            {
                // Пропускаем биты уже найденного кадра
                if (skip_bits-- > 0)
                    continue;

                // ---- Первое синхрослово: берём 4 байта со смещения 4 ----
                sync_reg_1[0] = ((shift_buffer[4 + 0] << s) | (shift_buffer[4 + 1] >> (8 - s))) & 0xFF;
                sync_reg_1[1] = ((shift_buffer[5 + 0] << s) | (shift_buffer[5 + 1] >> (8 - s))) & 0xFF;
                sync_reg_1[2] = ((shift_buffer[6 + 0] << s) | (shift_buffer[6 + 1] >> (8 - s))) & 0xFF;
                sync_reg_1[3] = ((shift_buffer[7 + 0] << s) | (shift_buffer[7 + 1] >> (8 - s))) & 0xFF;

                // Из 4 байт собираем 2, объединяя старшие полубайты:
                // sync_reg_1[0] = (старший полубайт байта 0) | (старший полубайт байта 1)
                // sync_reg_1[1] = (старший полубайт байта 2) | (старший полубайт байта 3)
                sync_reg_1[0] = (sync_reg_1[0] >> 4) << 4 | (sync_reg_1[1] >> 4);
                sync_reg_1[1] = (sync_reg_1[2] >> 4) << 4 | (sync_reg_1[3] >> 4);

                // ---- Второе синхрослово: 8 байт со смещения 188+4 ----
                sync_reg_2[0] = ((shift_buffer[188 + 4 + 0] << s) | (shift_buffer[188 + 4 + 1] >> (8 - s))) & 0xFF;
                sync_reg_2[1] = ((shift_buffer[189 + 4 + 0] << s) | (shift_buffer[189 + 4 + 1] >> (8 - s))) & 0xFF;
                sync_reg_2[2] = ((shift_buffer[190 + 4 + 0] << s) | (shift_buffer[190 + 4 + 1] >> (8 - s))) & 0xFF;
                sync_reg_2[3] = ((shift_buffer[191 + 4 + 0] << s) | (shift_buffer[191 + 4 + 1] >> (8 - s))) & 0xFF;
                sync_reg_2[4] = ((shift_buffer[192 + 4 + 0] << s) | (shift_buffer[192 + 4 + 1] >> (8 - s))) & 0xFF;
                sync_reg_2[5] = ((shift_buffer[193 + 4 + 0] << s) | (shift_buffer[193 + 4 + 1] >> (8 - s))) & 0xFF;
                sync_reg_2[6] = ((shift_buffer[194 + 4 + 0] << s) | (shift_buffer[194 + 4 + 1] >> (8 - s))) & 0xFF;
                sync_reg_2[7] = ((shift_buffer[195 + 4 + 0] << s) | (shift_buffer[195 + 4 + 1] >> (8 - s))) & 0xFF;

                // Из 8 байт собираем 4, объединяя старшие полубайты
                sync_reg_2[0] = (sync_reg_2[0] >> 4) << 4 | (sync_reg_2[1] >> 4);
                sync_reg_2[1] = (sync_reg_2[2] >> 4) << 4 | (sync_reg_2[3] >> 4);
                sync_reg_2[2] = (sync_reg_2[4] >> 4) << 4 | (sync_reg_2[5] >> 4);
                sync_reg_2[3] = (sync_reg_2[6] >> 4) << 4 | (sync_reg_2[7] >> 4);

                // Считаем расстояние Хэмминга до эталонных синхрослов:
                // sync1 — ожидаем 0x00, 0x53
                int sync1 = compare_8(sync_reg_1[0], 0x00) +
                            compare_8(sync_reg_1[1], 0x53);

                // sync2 — ожидаем 4 нулевых байта
                int sync2 = compare_8(sync_reg_2[0], 0x00) +
                            compare_8(sync_reg_2[1], 0x00) +
                            compare_8(sync_reg_2[2], 0x00) +
                            compare_8(sync_reg_2[3], 0x00);

                // Порог: мягкий при уже установленной синхронизации,
                // жёсткий — при попытке её захватить.
                if (synced ? (sync1 < 6 && sync2 < 8) : (sync1 < 1 && sync2 < 3))
                {
                    // Копируем кадр из буфера, сдвигая байты на s бит
                    for (int ii = 0; ii < (KMSS_QPSK_REC_FRM_SIZE / 8); ii++)
                        output[frames_out * (KMSS_QPSK_REC_FRM_SIZE / 8) + ii] =
                            ((shift_buffer[ii + 0] << s) | (shift_buffer[ii + 1] >> (8 - s))) & 0xFF;
                    frames_out++;

                    synced = true;
                    // Пропускаем биты текущего кадра, чтобы не искать внутри него
                    skip_bits = KMSS_QPSK_REC_FRM_SIZE - 1;
                }
                else
                {
                    synced = false;
                }
            }
        }
        return frames_out;
    }
}
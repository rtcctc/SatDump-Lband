#pragma once

#include <cstring>   // memmove, memset и т.п.
#include <cstdint>   // uint8_t, uint16_t и т.п.

// Размеры кадров (в битах!) для разных форматов Метеора.
// Обратите внимание: размеры заданы в БИТАХ, а не в байтах,
// поэтому в коде далее делится на 8 там, где нужны байты.
#define MTVZA_REC_FRM_SIZE     3072   // Кадр прибора MTVZA (микроволновый сканер)
#define KMSS_BPSK_REC_FRM_SIZE 12288  // Кадр KMSS при BPSK-модуляции
#define KMSS_QPSK_REC_FRM_SIZE 24576  // Кадр KMSS при QPSK-модуляции

namespace meteor
{
    // =========================================================================
    // MTVZA_ExtDeframer — дефреймер для прибора MTVZA.
    //
    // Работает побитово: на вход приходит поток ОТДЕЛЬНЫХ бит (0/1),
    // упакованных по одному в байт. Ищет синхрослово из 4 частей,
    // разнесённых по кадру. Синхрослова проверяются побитово (точное сравнение).
    // =========================================================================
    class MTVZA_ExtDeframer
    {
    private:
        uint8_t *shifting_buffer;   // Скользящее окно размером ровно в один кадр (биты)

        bool synced = 0;            // Флаг текущей синхронизации
        int runs_since_sync = 0;    // Сколько итераций прошло с момента последней синхронизации

    public:
        MTVZA_ExtDeframer()
        {
            // Буфер на MTVZA_REC_FRM_SIZE элементов (по одному биту в байте)
            shifting_buffer = new uint8_t[MTVZA_REC_FRM_SIZE];
        }

        ~MTVZA_ExtDeframer()
        {
            delete[] shifting_buffer;
        }

        // Основная функция: сдвигает входной поток через буфер,
        // ищет синхрослова и нарезает кадры.
        // input_bits — указатель на массив бит (по одному биту в байте)
        // length     — сколько бит пришло
        // output     — куда писать найденные кадры (упакованные в байты)
        // Возвращает число найденных кадров.
        int work(uint8_t *input_bits, int length, uint8_t *output)
        {
            int nfrm = 0;   // Счётчик найденных кадров

            for (int i = 0; i < length; i++)
            {
                // Сдвигаем буфер влево на 1 бит и добавляем новый бит в конец
                memmove(&shifting_buffer[0], &shifting_buffer[1], MTVZA_REC_FRM_SIZE - 1);
                shifting_buffer[3072 - 1] = input_bits[i];

                // ---- Проверка 4 синхрослов внутри окна ----
                // Ожидаемые биты: 10110011 (0xB3)
                bool sync1 = shifting_buffer[0] == 1 &&
                             shifting_buffer[1] == 0 &&
                             shifting_buffer[2] == 1 &&
                             shifting_buffer[3] == 1 &&
                             shifting_buffer[4] == 0 &&
                             shifting_buffer[5] == 0 &&
                             shifting_buffer[6] == 1 &&
                             shifting_buffer[7] == 1;

                // Ожидаемые биты: 11100011 (0xE3)
                bool sync2 = shifting_buffer[384 + 0] == 1 &&
                             shifting_buffer[384 + 1] == 1 &&
                             shifting_buffer[384 + 2] == 1 &&
                             shifting_buffer[384 + 3] == 0 &&
                             shifting_buffer[384 + 4] == 0 &&
                             shifting_buffer[384 + 5] == 0 &&
                             shifting_buffer[384 + 6] == 1 &&
                             shifting_buffer[384 + 7] == 1;

                // Ожидаемые биты: 01110101 (0x75)
                bool sync3 = shifting_buffer[768 + 0] == 0 &&
                             shifting_buffer[768 + 1] == 1 &&
                             shifting_buffer[768 + 2] == 1 &&
                             shifting_buffer[768 + 3] == 1 &&
                             shifting_buffer[768 + 4] == 0 &&
                             shifting_buffer[768 + 5] == 1 &&
                             shifting_buffer[768 + 6] == 0 &&
                             shifting_buffer[768 + 7] == 1;

                // Ожидаемые биты: 00000000 (0x00), с опечаткой:
                // индекс 1920+8 вместо 1920+7, так что 8-й бит проверяется неверно
                bool sync4 = shifting_buffer[1920 + 0] == 0 &&
                             shifting_buffer[1920 + 1] == 0 &&
                             shifting_buffer[1920 + 2] == 0 &&
                             shifting_buffer[1920 + 3] == 0 &&
                             shifting_buffer[1920 + 4] == 0 &&
                             shifting_buffer[1920 + 5] == 0 &&
                             shifting_buffer[1920 + 6] == 0 &&
                             shifting_buffer[1920 + 8] == 0;

                runs_since_sync++;

                // Порог срабатывания:
                //  - если уже синхронизированы — требуем ВСЕ 4 синхрослова;
                //  - если нет — достаточно, чтобы совпало БОЛЕЕ ДВУХ (т.е. 3 или 4).
                if (synced ? (sync1 && sync2 && sync3 && sync4)
                           : ((int)sync1 + (int)sync2 + (int)sync3 + (int)sync4) > 2)
                {
                    // Пишем кадр: упаковываем биты в байты по 8 штук
                    uint8_t *bytes = &output[nfrm++ * (MTVZA_REC_FRM_SIZE / 8)];
                    for (int i = 0; i < MTVZA_REC_FRM_SIZE; i++)
                        bytes[i / 8] = bytes[i / 8] << 1 | shifting_buffer[i];

                    // «Подтверждаем» синхронизацию только после 2+ подряд удачных кадров
                    if (runs_since_sync > 2 && !synced)
                    {
                        synced = true;
                        runs_since_sync = 0;
                        // logger->critical("SYNC!");
                    }
                }
                else
                {
                    // Сброс — синхронизация потеряна
                    runs_since_sync++;
                    synced = false;
                }
            }

            return nfrm;
        }

        // Публичный геттер для UI / логов
        bool getSynced()
        {
            return synced;
        }
    };

    // =========================================================================
    // KMSS_BPSK_ExtDeframer — дефреймер KMSS при BPSK.
    //
    // На вход — упакованные байты; поиск идёт с учётом возможного битового
    // сдвига s внутри байта (0..7). Используется «мягкий» критерий совпадения:
    // compare_8 считает количество НЕсовпадающих битов (расстояние Хэмминга).
    // =========================================================================
    class KMSS_BPSK_ExtDeframer
    {
    private:
        uint8_t *shift_buffer;      // Скользящее окно в байтах (+1 байт для сдвига)

        bool synced = 0;
        int runs_since_sync = 0;
        int skip_bits = 0;          // Сколько бит пропустить после захвата (чтобы не искать внутри кадра)

        uint8_t sync_reg_1[2];      // Регистр под первое синхрослово (2 байта)
        uint8_t sync_reg_2[4];      // Регистр под второе синхрослово (4 байта)
        // Остальные (sync_reg_3..7) закомментированы — используются в другом режиме

    private:
        // Расстояние Хэмминга: сколько бит различаются между v1 и v2.
        // Реализация через трюк Кернигана: diff &= diff - 1 обнуляет младший установленный бит.
        int compare_8(uint8_t v1, uint8_t v2)
        {
            int cor = 0;
            uint8_t diff = v1 ^ v2;
            for (; diff; cor++)
                diff &= diff - 1;
            return cor;
        }

        // Упаковка 8 отдельных бит в один байт
        uint8_t repack_8(uint8_t *bit)
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

    public:
        KMSS_BPSK_ExtDeframer()
        {
            // Буфер на (размер_кадра/8)+1 байт: дополнительный байт нужен
            // для корректного сдвига при выборке байтов по битовому смещению s
            shift_buffer = new uint8_t[KMSS_BPSK_REC_FRM_SIZE];
        }

        ~KMSS_BPSK_ExtDeframer()
        {
            delete[] shift_buffer;
        }

        int work(uint8_t *input_bytes, int length, uint8_t *output)
        {
            int frames_out = 0;
            for (int i = 0; i < length; i++)
            {
                // Сдвиг буфера на 1 байт влево + новый входной байт
                memmove(&shift_buffer[0], &shift_buffer[1], ((KMSS_BPSK_REC_FRM_SIZE / 8) + 1) - 1);
                shift_buffer[((KMSS_BPSK_REC_FRM_SIZE / 8) + 1) - 1] = input_bytes[i];

                // Пробуем все 8 возможных битовых сдвигов внутри байта
                for (int s = 0; s < 8; s++)
                {
                    // Если мы только что нашли кадр — пропускаем его биты,
                    // чтобы не искать синхрослово внутри самого кадра
                    if (skip_bits-- > 0)
                        continue;

                    // Формируем байты синхрослов с учётом битового сдвига s:
                    // берём старшие биты текущего байта и младшие следующего
                    sync_reg_1[0] = ((shift_buffer[0 + 0] << s) | (shift_buffer[0 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_1[1] = ((shift_buffer[1 + 0] << s) | (shift_buffer[1 + 1] >> (8 - s))) & 0xFF;

                    sync_reg_2[0] = ((shift_buffer[382 + 0] << s) | (shift_buffer[382 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_2[1] = ((shift_buffer[383 + 0] << s) | (shift_buffer[383 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_2[2] = ((shift_buffer[384 + 0] << s) | (shift_buffer[384 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_2[3] = ((shift_buffer[385 + 0] << s) | (shift_buffer[385 + 1] >> (8 - s))) & 0xFF;

#if 0
                    // ---- Заготовки под дополнительные синхрослова (отключены) ----
                    // Они используются в некоторых режимах KMSS, но здесь не задействованы.
                    sync_reg_3[0] = ((shift_buffer[574 + 0] << s) | (shift_buffer[574 + 1] >> (8 - s))) & 0xFF;
                    /* ... и т.д. до sync_reg_7 ... */
#endif

                    // Считаем суммарное расстояние Хэмминга до эталонных синхрослов.
                    // sync1: ожидаем 0x00 0x35
                    int sync1 = compare_8(sync_reg_1[0], 0x00) +
                                compare_8(sync_reg_1[1], 0x35);

                    // sync2: ожидаем 4 нулевых байта (в данном режиме)
                    int sync2 = compare_8(sync_reg_2[0], 0x00) +
                                compare_8(sync_reg_2[1], 0x00) +
                                compare_8(sync_reg_2[2], 0x00) +
                                compare_8(sync_reg_2[3], 0x00);

#if 0
                    // ---- Отключённая расширенная проверка (для других режимов) ----
                    int sync3 = ...;
                    int sync4 = ...;
                    // ...
                    if (synced
                            ? (sync1 < 10 && sync2 < 20 && ...)
                            : (sync1 < 6 && sync2 < 10 && ...))
#endif

                    // Порог: если синхронизированы — мягкий (допускаем ошибки),
                    // если нет — жёсткий (требуем почти точного совпадения).
                    if (synced ? (sync1 < 6 && sync2 < 8) : (sync1 < 1 && sync2 < 3))
                    {
                        // Копируем весь кадр из буфера, сдвигая байты на s бит
                        for (int ii = 0; ii < (KMSS_BPSK_REC_FRM_SIZE / 8); ii++)
                            output[frames_out * (KMSS_BPSK_REC_FRM_SIZE / 8) + ii] =
                                ((shift_buffer[ii + 0] << s) | (shift_buffer[ii + 1] >> (8 - s))) & 0xFF;
                        frames_out++;

                        synced = true;
                        // Пропускаем биты только что найденного кадра
                        skip_bits = KMSS_BPSK_REC_FRM_SIZE - 1;
                    }
                    else
                    {
                        synced = false;
                    }
                }
            }
            return frames_out;
        }
    };

    // =========================================================================
    // KMSS_QPSK_ExtDeframer — дефреймер KMSS при QPSK.
    //
    // Аналог KMSS_BPSK, но кадр в 2 раза больше (24576 бит),
    // а синхрослова расположены на других смещениях. Внутри каждого байта
    // после сдвига берётся старший полубайт (объединение по 4 бита).
    // =========================================================================
    class KMSS_QPSK_ExtDeframer
    {
    private:
        uint8_t *shift_buffer;

        bool synced = 0;
        int runs_since_sync = 0;
        int skip_bits = 0;

        uint8_t sync_reg_1[4];   // 4 байта (собираются в 2 после объединения полубайтов)
        uint8_t sync_reg_2[8];   // 8 байт (собираются в 4)

    private:
        int compare_8(uint8_t v1, uint8_t v2);   // Расстояние Хэмминга
        uint8_t repack_8(uint8_t *bit);          // Упаковка бит → байт

    public:
        KMSS_QPSK_ExtDeframer();
        ~KMSS_QPSK_ExtDeframer();

        int work(uint8_t *input_bytes, int length, uint8_t *output);
    };
}
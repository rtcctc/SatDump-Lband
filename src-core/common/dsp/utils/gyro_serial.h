#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include "libs/serialib.h"

namespace dsp
{
    class GyroSerial
    {
    public:
        GyroSerial();
        ~GyroSerial();

        bool start(const std::string &port, int baud = 115200);
        void stop();

        bool is_running() const { return d_running.load(); }
        bool has_value() const;

        // Последняя фактическая ориентация антенны: pitch и yaw (градусы)
        bool get_last(float &pitch, float &yaw) const;

        // Отправить "RESET\n" — гироскоп сбросит углы в ноль.
        // Потокобезопасно, можно звать из process() или UI.
        bool send_reset();

    private:
        void reader_loop();
        void parse_line(const std::string &line);

        serialib d_serial;
        std::atomic<bool> d_should_run{false};
        std::atomic<bool> d_running{false};
        std::thread d_thread;

        // Данные от гироскопа
        mutable std::mutex d_mutex;
        float d_last_pitch = 0.0f;
        float d_last_yaw   = 0.0f;
        bool  d_has_value  = false;

        // Отдельный мьютекс на запись — чтобы два RESET не перемешались
        std::mutex d_write_mutex;
    };
}
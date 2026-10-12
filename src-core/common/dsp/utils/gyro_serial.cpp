#include "gyro_serial.h"
#include "logger.h"
#include <cstdio>
#include <cstring>
#include <chrono>

namespace dsp
{
    GyroSerial::GyroSerial() {}

    GyroSerial::~GyroSerial()
    {
        stop();
    }

    bool GyroSerial::start(const std::string &port, int baud)
    {
        if (d_running.load())
            return true;

        int status = d_serial.openDevice(port.c_str(), baud);
        if (status != 1)
        {
            logger->error("GyroSerial: could not open %s (err=%d)", port.c_str(), status);
            return false;
        }

        d_serial.flushReceiver();

        d_should_run = true;
        d_running = true;
        d_thread = std::thread(&GyroSerial::reader_loop, this);

        logger->info("GyroSerial: connected on %s @ %d", port.c_str(), baud);
        return true;
    }

    void GyroSerial::stop()
    {
        if (!d_running.load() && !d_thread.joinable())
            return;

        d_should_run = false;
        if (d_thread.joinable())
            d_thread.join();

        if (d_running.load())
            d_serial.closeDevice();

        d_running = false;
        logger->info("GyroSerial: disconnected");
    }

    bool GyroSerial::has_value() const
    {
        std::lock_guard<std::mutex> lk(d_mutex);
        return d_has_value;
    }

    bool GyroSerial::get_last(float &pitch, float &yaw) const
    {
        std::lock_guard<std::mutex> lk(d_mutex);
        if (!d_has_value)
            return false;
        pitch = d_last_pitch;
        yaw   = d_last_yaw;
        return true;
    }

    bool GyroSerial::send_reset()
    {
        if (!d_running.load())
            return false;

        std::lock_guard<std::mutex> lk(d_write_mutex);

        const char cmd[] = "RESET\n";
        int sent = d_serial.writeBytes(cmd, sizeof(cmd) - 1);

        if (sent < 0)
        {
            logger->error("GyroSerial: failed to send RESET");
            return false;
        }

        logger->debug("GyroSerial: RESET sent (%d bytes)", sent);
        return true;
    }

    void GyroSerial::reader_loop()
    {
        char raw[256];
        std::string line_buf;
        line_buf.reserve(256);

        while (d_should_run.load())
        {
            int n = d_serial.readBytes(raw, sizeof(raw) - 1, 100, 50);
            if (n <= 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }

            // Накапливаем байты и режем по '\n'.
            // Один readBytes может вернуть часть строки, несколько строк,
            // или склеить конец одной и начало другой.
            for (int i = 0; i < n; i++)
            {
                char c = raw[i];

                if (c == '\n')
                {
                    parse_line(line_buf);
                    line_buf.clear();
                }
                else if (c != '\r')
                {
                    line_buf.push_back(c);
                    // Защита от мусора без переводов строк
                    if (line_buf.size() > 512)
                        line_buf.clear();
                }
            }
        }
    }

    void GyroSerial::parse_line(const std::string &line)
    {
        if (line.empty())
            return;

        // Формат: "<pitch> <yaw>" — оба числа с 2 знаками после запятой,
        // например "12.34 -56.78".
        float pitch = 0.0f;
        float yaw   = 0.0f;

        if (sscanf(line.c_str(), "%f %f", &pitch, &yaw) == 2)
        {
            std::lock_guard<std::mutex> lk(d_mutex);
            d_last_pitch = pitch;
            d_last_yaw   = yaw;
            d_has_value  = true;
        }
        else
        {
            logger->trace("GyroSerial: bad line '%s'", line.c_str());
        }
    }
}
#pragma once

#include "imgui/imgui.h"
#include "core/style.h"
#include "logger.h"
#include "libs/serialib.h"
#include <string>
#include <nlohmann/json.hpp>
#include "rotator_handler.h"

namespace rotator
{
    class RotctlSerialHandler : public RotatorHandler
    {
    public:
        RotctlSerialHandler();
        ~RotctlSerialHandler();

        std::string command(std::string cmd, int *ret_sz);
        void connect();
        void disconnect();
        bool is_connected();

        std::string get_id();
        void set_settings(nlohmann::json settings);
        nlohmann::json get_settings();

        rotator_status_t get_pos(float *az, float *el);
        rotator_status_t set_pos(float az, float el);
        void render();

    private:
        void l_connect(const char *port, int baud);
        void l_disconnect();

        serialib serial;
        bool connected = false;

        char input_portname[100] = "/dev/ttyUSB0"; // для Linux / macOS
        int input_baudrate = 9600;

        int corrupted_cmd_count = 0;
        static constexpr int MAX_CORRUPTED_CMD = 3;
    };
}

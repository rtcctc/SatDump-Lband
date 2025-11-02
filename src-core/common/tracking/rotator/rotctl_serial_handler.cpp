#include "rotctl_serial_handler.h"
#include <cstdio>
#include <cstring>

namespace rotator
{
    RotctlSerialHandler::RotctlSerialHandler()
    {
    }

    RotctlSerialHandler::~RotctlSerialHandler()
    {
        if (connected)
            serial.closeDevice();
        connected = false;
    }

    std::string RotctlSerialHandler::command(std::string cmd, int *ret_sz)
    {
        if (!connected)
            return "";

        int sent = serial.writeBytes(cmd.data(), cmd.size());
        if (sent < 0)
        {
            logger->error("Serial write error");
            disconnect();
            return "";
        }

        // читаем ответ (ожидаем \n в конце)
        std::string result;
        result.resize(1000);
        int read_bytes = serial.readBytes((char *)result.data(), result.size(), 100, 50); // timeout=50ms, wait=100ms
        if (read_bytes < 0)
        {
            logger->error("Serial read error");
            disconnect();
            return "";
        }

        *ret_sz = read_bytes;
        result.resize(read_bytes);
        return result;
    }

    void RotctlSerialHandler::l_connect(const char *port, int baud)
    {
        int status = serial.openDevice(port, baud);
        if (status != 1)
        {
            logger->error("Could not open serial port %s (err=%d)", port, status);
            connected = false;
            return;
        }

        serial.flushReceiver();
        connected = true;
    }

    void RotctlSerialHandler::l_disconnect()
    {
        if (connected)
            serial.closeDevice();
        connected = false;
    }

    std::string RotctlSerialHandler::get_id()
    {
        return "rotctl_serial";
    }

    void RotctlSerialHandler::set_settings(nlohmann::json settings)
    {
        std::string vport = getValueOrDefault(settings["portname"], std::string(input_portname));
        memcpy(input_portname, vport.data(), vport.size());
        input_portname[vport.size()] = '\0';
        input_baudrate = getValueOrDefault(settings["baudrate"], input_baudrate);
    }

    nlohmann::json RotctlSerialHandler::get_settings()
    {
        nlohmann::json v;
        v["portname"] = std::string(input_portname);
        v["baudrate"] = input_baudrate;
        return v;
    }

    rotator_status_t RotctlSerialHandler::get_pos(float *az, float *el)
    {
        if (!connected)
            return ROT_ERROR_CON;

        float saz = 0, sel = 0;
        int ret_sz = 0;
        std::string cmd = command("p\n", &ret_sz);

        if (sscanf(cmd.c_str(), "%f\n%f", &saz, &sel) == 2)
        {
            corrupted_cmd_count = 0;
            *az = saz;
            *el = sel;
            return ROT_ERROR_OK;
        }

        corrupted_cmd_count++;
        if (corrupted_cmd_count > MAX_CORRUPTED_CMD || ret_sz <= 0)
        {
            disconnect();
            corrupted_cmd_count = 0;
        }

        return ROT_ERROR_CON;
    }

    rotator_status_t RotctlSerialHandler::set_pos(float az, float el)
    {
        if (!connected)
            return ROT_ERROR_CON;

        char command_out[30];
        sprintf(command_out, "P %.2f %.2f\n", az, el);
        int ret_sz = 0;
        std::string cmd = command(std::string(command_out), &ret_sz);

        int result = 0;
        if (sscanf(cmd.c_str(), "RPRT %d", &result) == 1)
        {
            corrupted_cmd_count = 0;
            if (result != 0)
                return ROT_ERROR_CMD;
            else
                return ROT_ERROR_OK;
        }

        corrupted_cmd_count++;
        if (corrupted_cmd_count > MAX_CORRUPTED_CMD || ret_sz <= 0)
        {
            disconnect();
            corrupted_cmd_count = 0;
        }

        return ROT_ERROR_CON;
    }

    void RotctlSerialHandler::render()
    {
        if (connected)
            style::beginDisabled();

        ImGui::InputText("Serial Port##rotctlserialport", input_portname, 100);
        ImGui::InputInt("Baudrate##rotctlserialbaud", &input_baudrate);

        if (connected)
            style::endDisabled();

        if (connected)
        {
            if (ImGui::Button("Disconnect##rotctlserialdisconnect"))
                l_disconnect();
        }
        else
        {
            if (ImGui::Button("Connect##rotctlserialconnect"))
                l_connect(input_portname, input_baudrate);
        }
    }

    bool RotctlSerialHandler::is_connected()
    {
        return connected;
    }

    void RotctlSerialHandler::connect()
    {
        l_connect(input_portname, input_baudrate);
    }

    void RotctlSerialHandler::disconnect()
    {
        l_disconnect();
    }
}

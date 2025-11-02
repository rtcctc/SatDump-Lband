#include "rotator_handler.h"
#include "core/plugin.h"

#include "rotcl_handler.h"
#include "rotctl_serial_handler.h"

namespace rotator
{
    std::vector<RotatorHandlerOption> getRotatorHandlerOptions()
    {
        std::vector<RotatorHandlerOption> rotatorOptions;

        rotatorOptions.push_back({"rotctl", []()
                                  { return std::make_shared<RotctlHandler>(); }});

        rotatorOptions.push_back({"rotctl_serial", []()
                                  { return std::make_shared<RotctlSerialHandler>(); }});

        satdump::eventBus->fire_event<RequestRotatorHandlerOptionsEvent>({rotatorOptions});

        return rotatorOptions;
    }
}

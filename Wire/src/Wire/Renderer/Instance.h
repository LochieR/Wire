#pragma once

#include "Device.h"

namespace wire {

#define WR_FRAMES_IN_FLIGHT 2

    struct InstanceInfo
    {
        RendererAPI API;
    };

    class Instance
    {
    public:
        virtual ~Instance() = default;

        virtual Device* createDevice(const DeviceInfo& deviceInfo, const SwapchainInfo& swapchainInfo) = 0;

        virtual uint32_t getNumFramesInFlight() const = 0;
    };

    Instance* createInstance(const InstanceInfo& instanceInfo);

}

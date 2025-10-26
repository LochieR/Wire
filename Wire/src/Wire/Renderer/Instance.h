#pragma once

#include "Device.h"

#include <memory>

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

        virtual std::shared_ptr<Device> createDevice(const DeviceInfo& deviceInfo, const SwapchainInfo& swapchainInfo) = 0;

        virtual uint32_t getNumFramesInFlight() const = 0;
    };

    std::unique_ptr<Instance> createInstance(const InstanceInfo& instanceInfo);

}

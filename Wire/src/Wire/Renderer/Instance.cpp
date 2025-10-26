#include "Instance.h"

#include "Vulkan/VulkanInstance.h"

namespace wire {

    std::unique_ptr<Instance> createInstance(const InstanceInfo& instanceInfo)
    {
        return std::make_unique<VulkanInstance>(instanceInfo);
    }

}

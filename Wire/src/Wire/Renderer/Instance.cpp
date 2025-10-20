#include "Instance.h"

#include "Vulkan/VulkanInstance.h"

namespace wire {

    Instance* createInstance(const InstanceInfo& instanceInfo)
    {
        return new VulkanInstance(instanceInfo);
    }

}

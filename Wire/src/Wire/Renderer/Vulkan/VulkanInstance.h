#pragma once

#include "VulkanExtensions.h"

#include "Wire/Renderer/Instance.h"

#include <vulkan/vulkan.h>

namespace wire {

#define VK_CHECK(result, message) WR_ASSERT(result == VK_SUCCESS, message)

#ifdef WR_DEBUG
#define VK_DEBUG_NAME(device, type, object, nameCStr)                        \
    {                                                                        \
        VkDebugUtilsObjectNameInfoEXT info{};                                \
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;    \
        info.objectType = VK_OBJECT_TYPE_##type;                            \
        info.objectHandle = (uint64_t)object;                                \
        info.pObjectName = nameCStr;                                        \
                                                                            \
        exts::vkSetDebugUtilsObjectNameEXT(device, &info);                    \
    }
#else
#define VK_DEBUG_NAME(...)
#endif

    class VulkanInstance : public Instance
    {
    public:
        VulkanInstance(const InstanceInfo& instanceInfo);
        virtual ~VulkanInstance();

        virtual std::shared_ptr<Device> createDevice(const DeviceInfo& deviceInfo, const SwapchainInfo& swapchainInfo) override;
        
        virtual uint32_t getNumFramesInFlight() const override { return WR_FRAMES_IN_FLIGHT; }

        VkInstance getInstance() const { return m_Instance; }
        const VkAllocationCallbacks* getAllocator() const { return m_Allocator; }
        VkDebugUtilsMessengerEXT getDebugMessenger() const { return m_DebugMessenger; }
        VkSurfaceKHR getSurface() const { return m_Surface; }
    private:
        void createInstance();
        void createSurface();
    private:
        VkInstance m_Instance = nullptr;
        VkAllocationCallbacks* m_Allocator = nullptr;
        VkDebugUtilsMessengerEXT m_DebugMessenger = nullptr;
        VkSurfaceKHR m_Surface = nullptr;
        
        std::vector<std::shared_ptr<IResource>> m_Resources;
    };

}

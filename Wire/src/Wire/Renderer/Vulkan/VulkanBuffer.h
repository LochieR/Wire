#pragma once

#include "Wire/Renderer/Device.h"

#include <vulkan/vulkan.h>

namespace wire {

    class VulkanBufferBase : public BufferBase
    {
    public:
        VulkanBufferBase(Device* device, BufferType type, size_t size, const void* data = nullptr, std::string_view debugName = {});
        virtual ~VulkanBufferBase();

        virtual void setData(const void* data, size_t size, size_t offset = 0) override;
        virtual void setData(int data, size_t size) override;

        virtual void* map(size_t size) override;
        virtual void unmap() override;

        virtual size_t getSize() const override { return m_Size; }

        VkBuffer getBuffer() const { return m_Buffer; }
    private:
        Device* m_Device;
        BufferType m_Type;

        std::string m_DebugName;

        VkBuffer m_Buffer = nullptr;
        VkDeviceMemory m_Memory = nullptr;
        VkBuffer m_StagingBuffer = nullptr;
        VkDeviceMemory m_StagingMemory = nullptr;

        size_t m_Size;
    };

}

#include "VulkanBuffer.h"

#include "VulkanDevice.h"
#include "Wire/Core/Assert.h"

#include <vulkan/vulkan.h>

namespace wire {

    namespace Utils {

        static VkBufferUsageFlags GetBufferUsage(BufferType type)
        {
            VkBufferUsageFlags flags = 0;

            if (type & VertexBuffer)
                flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            if (type & IndexBuffer)
                flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            if (type & StagingBuffer)
                flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            if (type & UniformBuffer)
                flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            if (type & StorageBuffer)
                flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

            return flags;
        }

        static bool NeedsStagingBuffer(BufferType type)
        {
            return (type & IndexBuffer) || (type & StorageBuffer);
        }

        static VkMemoryPropertyFlags GetBufferMemoryProperties(BufferType type)
        {
            VkMemoryPropertyFlags flags = 0;

            if (NeedsStagingBuffer(type))
                flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            else
                flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            return flags;
        }

        static void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const VkAllocationCallbacks* allocator, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory, const std::string& debugName, bool staging)
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkResult result = vkCreateBuffer(device, &bufferInfo, allocator, &buffer);
            VK_CHECK(result, "Failed to create Vulkan buffer!");

            std::string bufferName(debugName);
            if (staging)
                bufferName += " (staging buffer)";
            else
                bufferName += " (buffer)";

            VK_DEBUG_NAME(device, BUFFER, buffer, bufferName.c_str());

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = Utils::FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

            result = vkAllocateMemory(device, &allocInfo, allocator, &memory);
            VK_CHECK(result, "Failed to allocate Vulkan memory!");

            std::string memoryName(debugName);
            if (staging)
                memoryName += " (staging memory)";
            else
                memoryName += " (memory)";

            VK_DEBUG_NAME(device, DEVICE_MEMORY, memory, memoryName.c_str());

            result = vkBindBufferMemory(device, buffer, memory, 0);
            VK_CHECK(result, "Failed to bind Vulkan buffer memory!");
        }

    }

    VulkanBufferBase::VulkanBufferBase(Device* device, BufferType type, size_t size, const void* data, std::string_view debugName)
        : m_Device(device), m_Type(type), m_Size(size), m_DebugName(debugName)
    {
        VulkanDevice* vk = (VulkanDevice*)device;

        Utils::CreateBuffer(
            vk->getDevice(),
            vk->getPhysicalDevice(),
            vk->getAllocator(),
            size,
            Utils::GetBufferUsage(type),
            Utils::GetBufferMemoryProperties(type),
            m_Buffer,
            m_Memory,
            m_DebugName,
            false
        );

        if (Utils::NeedsStagingBuffer(type))
        {
            Utils::CreateBuffer(
                vk->getDevice(),
                vk->getPhysicalDevice(),
                vk->getAllocator(),
                size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_StagingBuffer,
                m_StagingMemory,
                m_DebugName,
                true
            );
        }

        if (data)
        {
            void* memory = map(size);
            std::memcpy(memory, data, size);
            unmap();
        }
    }

    VulkanBufferBase::~VulkanBufferBase()
    {
        m_Device->submitResourceFree([buffer = m_Buffer, memory = m_Memory, stagingBuffer = m_StagingBuffer, stagingMemory = m_StagingMemory](Device* device)
        {
            VulkanDevice* vk = (VulkanDevice*)device;

            if (buffer)
                vkDestroyBuffer(vk->getDevice(), buffer, vk->getAllocator());
            if (stagingBuffer)
                vkDestroyBuffer(vk->getDevice(), stagingBuffer, vk->getAllocator());
            if (memory)
                vkFreeMemory(vk->getDevice(), memory, vk->getAllocator());
            if (stagingMemory)
                vkFreeMemory(vk->getDevice(), stagingMemory, vk->getAllocator());
        });
    }

    void VulkanBufferBase::setData(const void* data, size_t size, size_t offset)
    {
        void* memory = map(size);
        std::memcpy(memory, data, size);
        unmap();
    }

    void VulkanBufferBase::setData(int data, size_t size)
    {
        void* memory = map(size);
        std::memset(memory, data, size);
        unmap();
    }

    void* VulkanBufferBase::map(size_t size)
    {
        VulkanDevice* vk = (VulkanDevice*)m_Device;

        if (Utils::NeedsStagingBuffer(m_Type))
        {
            void* memory;
            vkMapMemory(vk->getDevice(), m_StagingMemory, 0, size, 0, &memory);
            return memory;
        }
        else
        {
            void* memory;
            vkMapMemory(vk->getDevice(), m_Memory, 0, size, 0, &memory);
            return memory;
        }
    }

    void VulkanBufferBase::unmap()
    {
        VulkanDevice* vk = (VulkanDevice*)m_Device;

        if (Utils::NeedsStagingBuffer(m_Type))
        {
            vkUnmapMemory(vk->getDevice(), m_StagingMemory);

            CommandList commandList = vk->beginSingleTimeCommands();

            std::type_index id = typeid(void);
            std::shared_ptr<CommandListNativeCommand> command = vk->copyBuffer(m_StagingBuffer, m_Buffer, m_Size, 0, 0, id);

            commandList.submitNativeCommand(command, id);

            vk->endSingleTimeCommands(commandList);
        }
        else
            vkUnmapMemory(vk->getDevice(), m_Memory);
    }

}

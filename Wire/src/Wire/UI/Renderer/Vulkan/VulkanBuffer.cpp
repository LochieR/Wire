module;

#include "Wire/Core/Assert.h"
#include <vulkan/vulkan.h>

module wire.ui.renderer.vk:buffer;

import :renderer;

namespace wire {

#define VK_CHECK(result, message) WR_ASSERT(result == VK_SUCCESS, message)

	namespace Utils {

		static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
			{
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
					return i;
			}

			WR_ASSERT(false, "Failed to find suitable memory type!");
			return 0;
		}

		static void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const VkAllocationCallbacks* allocator, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory)
		{
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VkResult result = vkCreateBuffer(device, &bufferInfo, allocator, &buffer);
			VK_CHECK(result, "Failed to create Vulkan buffer!");

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = Utils::FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

			result = vkAllocateMemory(device, &allocInfo, allocator, &memory);
			VK_CHECK(result, "Failed to allocate Vulkan memory!");

			result = vkBindBufferMemory(device, buffer, memory, 0);
			VK_CHECK(result, "Failed to bind Vulkan buffer memory!");
		}

		static void CopyBuffer(CommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, size_t bufferSize)
		{
			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = bufferSize;

			vkCmdCopyBuffer(
				commandBuffer.as<VkCommandBuffer>(),
				srcBuffer,
				dstBuffer,
				1, &copyRegion
			);
		}

		static void CopyBuffer(VulkanRenderer* vk, VkBuffer srcBuffer, VkBuffer dstBuffer, size_t bufferSize)
		{
			CommandBuffer commandBuffer = vk->beginSingleTimeCommands();
			CopyBuffer(commandBuffer, srcBuffer, dstBuffer, bufferSize);
			vk->endSingleTimeCommands(commandBuffer);
		}

	}

	VulkanVertexBuffer::VulkanVertexBuffer(Renderer* renderer, size_t size, const void* data)
		: m_Renderer(renderer), m_Size(size)
	{
		VulkanRenderer* vk = (VulkanRenderer*)renderer;

		Utils::CreateBuffer(vk->getDevice(), vk->getPhysicalDevice(), vk->getAllocator(), size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_Memory);
	
		if (data)
		{
			setData(data, size);
		}
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		m_Renderer->submitResourceFree(
			[buffer = m_Buffer, memory = m_Memory](Renderer* renderer)
			{
				VulkanRenderer* vk = (VulkanRenderer*)renderer;

				vkDestroyBuffer(vk->getDevice(), buffer, vk->getAllocator());
				vkFreeMemory(vk->getDevice(), memory, vk->getAllocator());
			}
		);
	}

	void VulkanVertexBuffer::bind(CommandBuffer commandBuffer) const
	{
		if (((VulkanRenderer*)m_Renderer)->isFrameSkipped())
			return;

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer.as<VkCommandBuffer>(), 0, 1, &m_Buffer, &offset);
	}

	void VulkanVertexBuffer::setData(const void* data, size_t size)
	{
		void* memory = map(size);
		std::memcpy(memory, data, size);
		unmap();
	}

	void VulkanVertexBuffer::setData(int data, size_t size)
	{
		void* memory = map(size);
		std::memset(memory, data, size);
		unmap();
	}

	void* VulkanVertexBuffer::map(size_t size)
	{
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		void* memory;
		vkMapMemory(vk->getDevice(), m_Memory, 0, size, 0, &memory);

		return memory;
	}

	void VulkanVertexBuffer::unmap()
	{
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		vkUnmapMemory(vk->getDevice(), m_Memory);
	}

	size_t VulkanVertexBuffer::getSize() const
	{
		return m_Size;
	}

	VulkanIndexBuffer::VulkanIndexBuffer(Renderer* renderer, size_t size, const void* data)
		: m_Renderer(renderer), m_Size(size)
	{
		VulkanRenderer* vk = (VulkanRenderer*)renderer;

		Utils::CreateBuffer(vk->getDevice(), vk->getPhysicalDevice(), vk->getAllocator(), size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Buffer, m_Memory);
		Utils::CreateBuffer(vk->getDevice(), vk->getPhysicalDevice(), vk->getAllocator(), size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_StagingBuffer, m_StagingMemory);

		if (data)
		{
			setData(data, size);
		}
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		m_Renderer->submitResourceFree(
			[buffer = m_Buffer, memory = m_Memory, stagingBuffer = m_StagingBuffer, stagingMemory = m_StagingMemory](Renderer* renderer)
			{
				VulkanRenderer* vk = (VulkanRenderer*)renderer;

				vkDestroyBuffer(vk->getDevice(), buffer, vk->getAllocator());
				vkFreeMemory(vk->getDevice(), memory, vk->getAllocator());

				vkDestroyBuffer(vk->getDevice(), stagingBuffer, vk->getAllocator());
				vkFreeMemory(vk->getDevice(), stagingMemory, vk->getAllocator());
			}
		);
	}

	void VulkanIndexBuffer::bind(CommandBuffer commandBuffer) const
	{
		if (((VulkanRenderer*)m_Renderer)->isFrameSkipped())
			return;

		vkCmdBindIndexBuffer(commandBuffer.as<VkCommandBuffer>(), m_Buffer, 0, VK_INDEX_TYPE_UINT32);
	}

	void VulkanIndexBuffer::setData(const void* data, size_t size)
	{
		void* memory = map(size);
		std::memcpy(memory, data, size);
		unmap();
	}

	void VulkanIndexBuffer::setData(int data, size_t size)
	{
		void* memory = map(size);
		std::memset(memory, data, size);
		unmap();
	}

	void* VulkanIndexBuffer::map(size_t size)
	{
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		void* memory;
		vkMapMemory(vk->getDevice(), m_StagingMemory, 0, size, 0, &memory);

		return memory;
	}

	void VulkanIndexBuffer::unmap()
	{
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		vkUnmapMemory(vk->getDevice(), m_StagingMemory);
		Utils::CopyBuffer(vk, m_StagingBuffer, m_Buffer, m_Size);
	}

	size_t VulkanIndexBuffer::getSize() const
	{
		return m_Size;
	}

	VulkanStagingBuffer::VulkanStagingBuffer(Renderer* renderer, size_t size, const void* data)
		: m_Renderer(renderer)
	{
		VulkanRenderer* vk = (VulkanRenderer*)renderer;

		Utils::CreateBuffer(
			vk->getDevice(),
			vk->getPhysicalDevice(),
			vk->getAllocator(),
			size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_Buffer,
			m_Memory
		);
	}

	VulkanStagingBuffer::~VulkanStagingBuffer()
	{
		m_Renderer->submitResourceFree([buffer = m_Buffer, memory = m_Memory](Renderer* renderer)
		{
			VulkanRenderer* vk = (VulkanRenderer*)renderer;
			
			vkDestroyBuffer(vk->getDevice(), buffer, vk->getAllocator());
			vkFreeMemory(vk->getDevice(), memory, vk->getAllocator());
		});
	}

	void VulkanStagingBuffer::setData(const void* data, size_t size)
	{
		void* memory = map(size);
		std::memcpy(memory, data, size);
		unmap();
	}

	void VulkanStagingBuffer::setData(int data, size_t size)
	{
		void* memory = map(size);
		std::memset(memory, data, size);
		unmap();
	}

	void* VulkanStagingBuffer::map(size_t size)
	{
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		void* memory;
		vkMapMemory(vk->getDevice(), m_Memory, 0, size, 0, &memory);

		return memory;
	}

	void VulkanStagingBuffer::unmap()
	{
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		vkUnmapMemory(vk->getDevice(), m_Memory);
	}

}

#include "wrpch.h"

#include <vulkan/vulkan.h>
#include "VulkanCommandBuffer.h"

namespace Wire {

	VulkanCommandBuffer::VulkanCommandBuffer(VulkanRenderer* renderer)
		: m_Renderer(renderer)
	{
		auto& vkd = renderer->GetVulkanData();

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = vkd.CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkResult result = vkAllocateCommandBuffers(vkd.Device, &allocInfo, &m_CommandBuffer);
		VK_CHECK(result, "Failed to allocate Vulkan command buffers!");
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
	}

	void VulkanCommandBuffer::Begin()
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
		VK_CHECK(result, "Failed to begin Vulkan command buffer!");
	}

	void VulkanCommandBuffer::End()
	{
		VkResult result = vkEndCommandBuffer(m_CommandBuffer);
		VK_CHECK(result, "Failed to end Vulkan command buffer!");
	}

	void VulkanCommandBuffer::Reset()
	{
		vkResetCommandBuffer(m_CommandBuffer, 0);
	}

}

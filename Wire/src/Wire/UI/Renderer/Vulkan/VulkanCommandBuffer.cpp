#include "VulkanCommandBuffer.h"

#include "VulkanBuffer.h"
#include "VulkanRenderer.h"
#include "VulkanGraphicsPipeline.h"

#include "Wire/Core/Assert.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace wire {

	namespace Utils {

		static VkShaderStageFlags ConvertShaderType(ShaderType type)
		{
			switch (type)
			{
			case ShaderType::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
			case ShaderType::Pixel: return VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			WR_ASSERT(false, "Unknown ShaderType!");
			return VkShaderStageFlags(0);
		}

	}

	VulkanCommandBuffer::VulkanCommandBuffer(Renderer* renderer, VkCommandBuffer commandBuffer, bool isSingleTimeCommands)
		: m_Renderer(renderer), m_CommandBuffer(commandBuffer), m_SingleTimeCommands(isSingleTimeCommands)
	{
	}

	void VulkanCommandBuffer::begin(bool renderPassStarted)
	{
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		if (vk->isFrameSkipped())
			return;

		WR_ASSERT(!m_Begun, "Cannot begin command buffer that hasn't been ended!");

		VkCommandBufferInheritanceInfo inheritanceInfo{};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.renderPass = vk->getRenderPass();
		inheritanceInfo.framebuffer = vk->getCurrentFramebuffer();
		inheritanceInfo.subpass = 0;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = renderPassStarted ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 0;
		beginInfo.pInheritanceInfo = &inheritanceInfo;

		VkCommandBuffer cmd = as<VkCommandBuffer>();
		vkBeginCommandBuffer(cmd, &beginInfo);

		if (renderPassStarted)
			vk->addRenderingCommandBuffer(cmd);
		else
			vk->addNonRenderingCommandBuffer(cmd);

		m_Begun = true;
		m_CurrentPipeline = nullptr;
	}

	void VulkanCommandBuffer::end()
	{
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		if (vk->isFrameSkipped())
			return;

		WR_ASSERT(m_Begun, "Cannot end command buffer that hasn't been started!");

		VkResult result = vkEndCommandBuffer(as<VkCommandBuffer>());
		VK_CHECK(result, "Failed to end Vulkan command buffer!");
		
		m_Begun = false;
	}

	void VulkanCommandBuffer::draw(uint32_t vertexCount, uint32_t vertexOffset)
	{
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		if (vk->isFrameSkipped())
			return;

		WR_ASSERT(m_Begun, "Cannot call draw on command buffer that hasn't been started!");

		vkCmdDraw(as<VkCommandBuffer>(), vertexCount, 1, vertexOffset, 0);
	}

	void VulkanCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t vertexOffset, uint32_t indexOffset)
	{
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		if (vk->isFrameSkipped())
			return;

		WR_ASSERT(m_Begun, "Cannot call drawIndexed on command buffer that hasn't been started!");

		vkCmdDrawIndexed(as<VkCommandBuffer>(), indexCount, 1, indexOffset, (int32_t)vertexOffset, 0);
	}

	void VulkanCommandBuffer::bindPipeline(GraphicsPipeline* pipeline)
	{
		VulkanGraphicsPipeline* vkPipeline = (VulkanGraphicsPipeline*)pipeline;
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		if (vk->isFrameSkipped())
			return;

		WR_ASSERT(m_Begun, "Cannot call bindPipeline on command buffer that hasn't been started!");

		vkCmdBindPipeline(as<VkCommandBuffer>(), VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->getPipeline());

		glm::vec2 extent = vk->getExtent();

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = extent.y;
		viewport.width = extent.x;
		viewport.height = -extent.y;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(as<VkCommandBuffer>(), 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { .width = (uint32_t)extent.x, .height = (uint32_t)extent.y };

		vkCmdSetScissor(as<VkCommandBuffer>(), 0, 1, &scissor);

		m_CurrentPipeline = pipeline;
	}

	void VulkanCommandBuffer::pushConstants(ShaderType stage, size_t size, const void* data, size_t offset)
	{
		VulkanGraphicsPipeline* vkPipeline = (VulkanGraphicsPipeline*)m_CurrentPipeline;
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		if (vk->isFrameSkipped())
			return;

		WR_ASSERT(m_Begun, "Cannot call pushConstants on command buffer that hasn't been started!");
		WR_ASSERT(m_CurrentPipeline, "Cannot call pushConstants without binding a pipeline!");

		vkCmdPushConstants(as<VkCommandBuffer>(), vkPipeline->getPipelineLayout(), Utils::ConvertShaderType(stage), (uint32_t)offset, (uint32_t)size, data);
	}

	void VulkanCommandBuffer::setLineWidth(float lineWidth)
	{
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		if (vk->isFrameSkipped())
			return;

		WR_ASSERT(m_Begun, "Cannot call setLineWidth on command buffer that hasn't been started!");
		WR_ASSERT(m_CurrentPipeline, "Cannot call setLineWidth without binding a pipeline!");

		vkCmdSetLineWidth(as<VkCommandBuffer>(), lineWidth);
	}

	void VulkanCommandBuffer::bindDescriptorSet()
	{
		VulkanGraphicsPipeline* vkPipeline = (VulkanGraphicsPipeline*)m_CurrentPipeline;
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		if (vk->isFrameSkipped())
			return;

		WR_ASSERT(m_Begun, "Cannot call bindDescriptorSet on command buffer that hasn't been started!");
		WR_ASSERT(m_CurrentPipeline, "Cannot call bindDescriptorSet without binding a pipeline!");

		VkDescriptorSet set = vkPipeline->getDescriptorSet();

		vkCmdBindDescriptorSets(
			as<VkCommandBuffer>(),
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vkPipeline->getPipelineLayout(),
			0,
			1,
			&set,
			0,
			nullptr
		);
	}

	void VulkanCommandBuffer::setScissor(const glm::vec2& min, const glm::vec2& max)
	{
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		if (vk->isFrameSkipped())
			return;

		WR_ASSERT(m_Begun, "Cannot call setScissor on command buffer that hasn't been started!");
		WR_ASSERT(m_CurrentPipeline, "Cannot call setScissor without binding a pipeline!");

		VkRect2D rect{};
		rect.extent = { (uint32_t)(max.x - min.x), (uint32_t)(max.y - min.y) };
		rect.offset = { (int)min.x, (int)min.y };
		vkCmdSetScissor(as<VkCommandBuffer>(), 0, 1, &rect);
	}

	void VulkanCommandBuffer::bindBuffer(VertexBuffer* vertexBuffer)
	{
		VulkanVertexBuffer* vkBuffer = (VulkanVertexBuffer*)vertexBuffer;
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		if (vk->isFrameSkipped())
			return;

		WR_ASSERT(m_Begun, "Cannot call bindBuffer on command buffer that hasn't been started!");

		VkBuffer buffer = vkBuffer->getBuffer();
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(as<VkCommandBuffer>(), 0, 1, &buffer, &offset);
	}

	void VulkanCommandBuffer::bindBuffer(IndexBuffer* indexBuffer)
	{
		VulkanIndexBuffer* vkBuffer = (VulkanIndexBuffer*)indexBuffer;
		VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;

		if (vk->isFrameSkipped())
			return;

		WR_ASSERT(m_Begun, "Cannot call bindBuffer on command buffer that hasn't been started!");

		vkCmdBindIndexBuffer(as<VkCommandBuffer>(), vkBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}

}

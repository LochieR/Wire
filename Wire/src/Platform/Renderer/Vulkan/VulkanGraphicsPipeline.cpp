#include "wrpch.h"

#include <vulkan/vulkan.h>
#include "VulkanGraphicsPipeline.h"
#include "VulkanRenderer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanTexture2D.h"
#include "VulkanBuffer.h"

namespace Wire {

	namespace Utils {

		static VkShaderStageFlags GetVkShaderStageFromWireStage(ShaderStage stage)
		{
			switch (stage)
			{
				case ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
				case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			WR_ASSERT(false);
			return 0;
		}

	}

	VulkanGraphicsPipeline::VulkanGraphicsPipeline(Renderer* renderer)
		: m_Renderer(renderer)
	{
	}

	VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
	{
		((VulkanRenderer*)m_Renderer)->SubmitResourceFree([pipeline = m_Pipeline, layout = m_PipelineLayout, setLayout = m_SetLayout](VulkanRenderer* renderer)
		{
			auto& vkd = renderer->GetVulkanData();

			vkDestroyPipeline(vkd.Device, pipeline, vkd.Allocator);
			vkDestroyPipelineLayout(vkd.Device, layout, vkd.Allocator);
			vkDestroyDescriptorSetLayout(vkd.Device, setLayout, vkd.Allocator);
		});
	}

	void VulkanGraphicsPipeline::Bind(rbRef<CommandBuffer> commandBuffer) const
	{
		auto& vkd = ((VulkanRenderer*)m_Renderer)->GetVulkanData();

		VkCommandBuffer cmd = ((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer;

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)vkd.SwapchainExtent.width;
		viewport.height = (float)vkd.SwapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = vkd.SwapchainExtent;
		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}

	void VulkanGraphicsPipeline::PushConstants(rbRef<CommandBuffer> commandBuffer, ShaderStage shaderStage, size_t size, const void* data, size_t offset)
	{
		vkCmdPushConstants(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer, m_PipelineLayout, Utils::GetVkShaderStageFromWireStage(shaderStage), (uint32_t)offset, (uint32_t)size, data);
	}

	void VulkanGraphicsPipeline::SetLineWidth(rbRef<CommandBuffer> commandBuffer, float lineWidth)
	{
		vkCmdSetLineWidth(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer, lineWidth);
	}

	void VulkanGraphicsPipeline::UpdateDescriptor(rbRef<Texture2D> texture2D, uint32_t binding, uint32_t index)
	{
		auto& vkd = ((VulkanRenderer*)m_Renderer)->GetVulkanData();

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = ((VulkanTexture2D*)texture2D.Get())->m_ImageView;
		imageInfo.sampler = ((VulkanTexture2D*)texture2D.Get())->m_Sampler;

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_DescriptorSet;
		descriptorWrite.dstBinding = binding;
		descriptorWrite.dstArrayElement = index;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(vkd.Device, 1, &descriptorWrite, 0, nullptr);
	}

	void VulkanGraphicsPipeline::UpdateDescriptor(rbRef<StorageBuffer> buffer, uint32_t binding, uint32_t index)
	{
		auto& vkd = ((VulkanRenderer*)m_Renderer)->GetVulkanData();

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = ((VulkanStorageBuffer*)buffer.Get())->m_StorageBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = ((VulkanStorageBuffer*)buffer.Get())->m_Size;

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_DescriptorSet;
		descriptorWrite.dstBinding = binding;
		descriptorWrite.dstArrayElement = index;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(vkd.Device, 1, &descriptorWrite, 0, nullptr);
	}

	void VulkanGraphicsPipeline::BindDescriptor(rbRef<CommandBuffer> commandBuffer) const
	{
		vkCmdBindDescriptorSets(
			((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_PipelineLayout,
			0, 1,
			&m_DescriptorSet,
			0, nullptr
		);
	}

}

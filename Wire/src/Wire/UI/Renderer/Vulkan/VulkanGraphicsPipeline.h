#pragma once

#include "VulkanRenderer.h"

#include <vulkan/vulkan.h>
#include <array>

namespace wire {

	class VulkanGraphicsPipeline : public GraphicsPipeline
	{
	public:
		VulkanGraphicsPipeline(Renderer* renderer, const GraphicsPipelineDesc& desc);
		virtual ~VulkanGraphicsPipeline();

		virtual void updateDescriptor(Texture2D* texture, uint32_t binding, uint32_t index) override;
		virtual void updateDescriptor(Sampler* sampler, uint32_t binding, uint32_t index) override;
		virtual void updateDescriptor(Texture2D* texture, Sampler* sampler, uint32_t binding, uint32_t index) override;
		virtual void updateDescriptor(UniformBuffer* uniformBuffer, uint32_t binding, uint32_t index) override;

		virtual void updateFrameDescriptor(Texture2D* texture, uint32_t frameIndex, uint32_t binding, uint32_t index) override;
		virtual void updateFrameDescriptor(Sampler* sampler, uint32_t frameIndex, uint32_t binding, uint32_t index) override;
		virtual void updateFrameDescriptor(Texture2D* texture, Sampler* sampler, uint32_t frameIndex, uint32_t binding, uint32_t index) override;
		virtual void updateFrameDescriptor(UniformBuffer* uniformBuffer, uint32_t frameIndex, uint32_t binding, uint32_t index) override;

		virtual void updateAllDescriptors(Texture2D* texture, uint32_t binding, uint32_t index) override;
		virtual void updateAllDescriptors(Sampler* sampler, uint32_t binding, uint32_t index) override;
		virtual void updateAllDescriptors(Texture2D* texture, Sampler* sampler, uint32_t binding, uint32_t index) override;
		virtual void updateAllDescriptors(UniformBuffer* uniformBuffer, uint32_t binding, uint32_t index) override;

		VkPipeline getPipeline() const { return m_Pipeline; }
		VkPipelineLayout getPipelineLayout() const { return m_PipelineLayout; }
		VkDescriptorSet getDescriptorSet() const { return m_DescriptorSets[m_Renderer->getFrameIndex()]; }
	private:
		VulkanRenderer* m_Renderer;

		VkShaderModule m_VertexShader, m_PixelShader;
		VkDescriptorSetLayout m_SetLayout;
		std::array<VkDescriptorSet, WR_FRAMES_IN_FLIGHT> m_DescriptorSets;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_Pipeline;
	};

}

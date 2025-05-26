module;

#include <vulkan/vulkan.h>

export module wire.ui.renderer.vk:graphicsPipeline;

import wire.ui.renderer;
import :renderer;

namespace wire {

	export class VulkanGraphicsPipeline : public GraphicsPipeline
	{
	public:
		VulkanGraphicsPipeline(Renderer* renderer, const GraphicsPipelineDesc& desc);
		virtual ~VulkanGraphicsPipeline();

		virtual void bind(CommandBuffer commandBuffer) override;
		virtual void pushConstants(CommandBuffer commandBuffer, ShaderType stage, size_t size, const void* data, size_t offset = 0) override;
		virtual void setLineWidth(CommandBuffer commandBuffer, float lineWidth) override;

		virtual void updateDescriptor(Texture2D* texture, uint32_t binding, uint32_t index) override;
		virtual void updateDescriptor(Sampler* sampler, uint32_t binding, uint32_t index) override;
		virtual void bindDescriptorSet(CommandBuffer commandBuffer) override;
	private:
		VulkanRenderer* m_Renderer;

		VkShaderModule m_VertexShader, m_PixelShader;
		VkDescriptorSetLayout m_SetLayout;
		VkDescriptorSet m_DescriptorSet;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_Pipeline;
	};

}

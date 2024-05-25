#pragma once

#include "Wire/Renderer/Renderer.h"
#include "Wire/Renderer/GraphicsPipeline.h"

struct VkPipelineLayout_T; typedef VkPipelineLayout_T* VkPipelineLayout;
struct VkPipeline_T; typedef VkPipeline_T* VkPipeline;
struct VkDescriptorSetLayout_T; typedef VkDescriptorSetLayout_T* VkDescriptorSetLayout;
struct VkDescriptorSet_T; typedef VkDescriptorSet_T* VkDescriptorSet;

namespace Wire {

	class VulkanGraphicsPipeline : public GraphicsPipeline
	{
	public:
		VulkanGraphicsPipeline(Renderer* renderer);
		virtual ~VulkanGraphicsPipeline();

		virtual void Bind(rbRef<CommandBuffer> commandBuffer) const override;
		virtual void PushConstants(rbRef<CommandBuffer> commandBuffer, ShaderStage shaderStage, size_t size, const void* data, size_t offset) override;
		virtual void SetLineWidth(rbRef<CommandBuffer> commandBuffer, float lineWidth) override;

		virtual void UpdateDescriptor(rbRef<Texture2D> texture2D, uint32_t binding, uint32_t index) override;
		virtual void UpdateDescriptor(rbRef<StorageBuffer> buffer, uint32_t binding, uint32_t index) override;
		virtual void BindDescriptor(rbRef<CommandBuffer> commandBuffer) const override;
	private:
		Renderer* m_Renderer;

		VkPipelineLayout m_PipelineLayout = nullptr;
		VkPipeline m_Pipeline = nullptr;
		VkDescriptorSetLayout m_SetLayout = nullptr;
		VkDescriptorSet m_DescriptorSet = nullptr;

		friend class VulkanShader;
	};

}

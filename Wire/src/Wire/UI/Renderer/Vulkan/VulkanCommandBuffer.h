#pragma once

#include "Wire/UI/Renderer/Renderer.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace wire {

	class VulkanCommandBuffer : public CommandBuffer
	{
	public:
		VulkanCommandBuffer(Renderer* renderer, VkCommandBuffer commandBuffer, bool isSingleTimeCommands);

		virtual void begin(bool renderPassStarted = true) override;
		virtual void end() override;
		virtual void draw(uint32_t vertexCount, uint32_t vertexOffset = 0) override;
		virtual void drawIndexed(uint32_t indexCount, uint32_t vertexOffset = 0, uint32_t indexOffset = 0) override;

		virtual void bindPipeline(GraphicsPipeline* pipeline) override;
		virtual void pushConstants(ShaderType stage, size_t size, const void* data, size_t offset = 0) override;
		virtual void setLineWidth(float lineWidth) override;
		virtual void bindDescriptorSet() override;
		virtual void setScissor(const glm::vec2& min, const glm::vec2& max) override;

		virtual void bindBuffer(VertexBuffer* vertexBuffer) override;
		virtual void bindBuffer(IndexBuffer* indexBuffer) override;

		virtual void* get() const override { return m_CommandBuffer; }
	private:
		Renderer* m_Renderer;
		bool m_SingleTimeCommands;
		bool m_Begun = false;

		VkCommandBuffer m_CommandBuffer;

		GraphicsPipeline* m_CurrentPipeline = nullptr;
	};

}

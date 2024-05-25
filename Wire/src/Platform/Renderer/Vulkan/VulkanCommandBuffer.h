#pragma once

#include "VulkanRenderer.h"

#include "Wire/Renderer/CommandBuffer.h"

struct VkCommandBuffer_T; typedef VkCommandBuffer_T* VkCommandBuffer;

namespace Wire {

	class VulkanCommandBuffer : public CommandBuffer
	{
	public:
		VulkanCommandBuffer(VulkanRenderer* renderer);
		virtual ~VulkanCommandBuffer();

		virtual void Begin() override;
		virtual void End() override;

		virtual void Reset() override;
	private:
		VulkanRenderer* m_Renderer = nullptr;
		VkCommandBuffer m_CommandBuffer = nullptr;

		friend class VulkanRenderer;
		friend class VulkanGraphicsPipeline;
		friend class VulkanVertexBuffer;
		friend class VulkanIndexBuffer;
		friend class VulkanImGuiLayer;
		friend class VulkanFramebuffer;
	};

}

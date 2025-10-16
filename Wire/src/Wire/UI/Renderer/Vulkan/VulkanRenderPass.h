#pragma once

#include "VulkanRenderer.h"
#include "Wire/UI/Renderer/RenderPass.h"

#include <vulkan/vulkan.h>

namespace wire {

	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(VulkanRenderer* renderer, const RenderPassDesc& desc);
		virtual ~VulkanRenderPass();
	private:
		std::vector<VkAttachmentDescription> createAttachmentDescriptions();
		std::vector<VkAttachmentReference> createAttachmentRefs();
	private:
		VulkanRenderer* m_Renderer = nullptr;
		RenderPassDesc m_Desc;

		VkRenderPass m_RenderPass = nullptr;
		std::vector<VkFramebuffer> m_Framebuffers;
	};

}

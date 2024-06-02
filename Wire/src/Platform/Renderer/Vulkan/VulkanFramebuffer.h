#pragma once

#include "VulkanRenderer.h"

#include "Wire/Renderer/Framebuffer.h"

#include <vector>

enum VkSampleCountFlagBits;

namespace Wire {

	class VulkanFramebuffer : public Framebuffer
	{
	public:
		VulkanFramebuffer(VulkanRenderer* renderer, const FramebufferSpecification& spec);
		virtual ~VulkanFramebuffer();

		virtual void BeginRenderPass(rbRef<CommandBuffer> commandBuffer) override;
		virtual void EndRenderPass(rbRef<CommandBuffer> commandBuffer) override;

		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual uint32_t GetColorAttachmentCount() const override;
		virtual void CopyAttachmentImageToBuffer(uint32_t attachmentIndex, rbRef<StagingBuffer> buffer) override;

		virtual void* GetRenderHandle() const override { return m_Descriptors[m_ImageIndex]; }

		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }

		virtual bool IsMultiSampled() const override { return m_Specification.MultiSample; }

		VkRenderPass GetRenderPass() const { return m_RenderPass; }
	private:
		VulkanRenderer* m_Renderer = nullptr;
		FramebufferSpecification m_Specification;

		uint32_t m_ImageCount;
		uint32_t m_ImageIndex = 0;

		std::vector<VkImage> m_Images;
		std::vector<VkDeviceMemory> m_ImageMemorys;
		std::vector<VkImageView> m_ImageViews;

		std::vector<std::vector<VkImage>> m_AttachmentImages;
		std::vector<std::vector<VkDeviceMemory>> m_AttachmentImageMemorys;
		std::vector<std::vector<VkImageView>> m_AttachmentImageViews;

		std::vector<VkImage> m_ColorImages;
		std::vector<VkDeviceMemory> m_ColorImageMemorys;
		std::vector<VkImageView> m_ColorImageViews;

		VkImage m_DepthImage = nullptr;
		VkDeviceMemory m_DepthImageMemory = nullptr;
		VkImageView m_DepthImageView = nullptr;

		VkSampleCountFlagBits m_MSAASampleCount;

		std::vector<VkFramebuffer> m_Framebuffers;
		std::vector<VkDescriptorSet> m_Descriptors;

		VkRenderPass m_RenderPass = nullptr;
		VkSampler m_Sampler = nullptr;
	};

}

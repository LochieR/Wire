#include "VulkanRenderPass.h"

namespace wire {

	namespace Utils {

		static VkFormat ConvertFormat(AttachmentFormat format)
		{
			switch (format)
			{
				case AttachmentFormat::R8_UInt: return VK_FORMAT_R8_UINT;
				case AttachmentFormat::R16_UInt: return VK_FORMAT_R16_UINT;
				case AttachmentFormat::R32_UInt: return VK_FORMAT_R32_UINT;
				case AttachmentFormat::R64_UInt: return VK_FORMAT_R64_UINT;
				case AttachmentFormat::R8_SInt: return VK_FORMAT_R8_SINT;
				case AttachmentFormat::R16_SInt: return VK_FORMAT_R16_SINT;
				case AttachmentFormat::R32_SInt: return VK_FORMAT_R32_SINT;
				case AttachmentFormat::R64_SInt: return VK_FORMAT_R64_SINT;
				case AttachmentFormat::R8_UNorm: return VK_FORMAT_R8_UNORM;
				case AttachmentFormat::R16_UNorm: return VK_FORMAT_R16_UNORM;
				case AttachmentFormat::R32_SFloat: return VK_FORMAT_R32_SFLOAT;
				case AttachmentFormat::BGRA8_UNorm: return VK_FORMAT_B8G8R8A8_UNORM;
				case AttachmentFormat::RGBA8_UNorm: return VK_FORMAT_R8G8B8A8_UNORM;
				case AttachmentFormat::RGBA32_SFloat: return VK_FORMAT_R32G32B32A32_SFLOAT;
				case AttachmentFormat::D32_SFloat: return VK_FORMAT_D32_SFLOAT;
				default:
					WR_ASSERT(false, "Unknown attachment format");
					return (VkFormat)0;
			}
		}

		static VkImageLayout GetImageLayout(AttachmentUsage type)
		{
			switch (type)
			{
			case AttachmentUsage::Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			case AttachmentUsage::Depth: return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			case AttachmentUsage::TransferSrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			default:
				WR_ASSERT(false, "Unknown attachment usage");
				return VK_IMAGE_LAYOUT_UNDEFINED;
			}
		}

		static std::vector<VkAttachmentReference> GetColorAttachments(const std::vector<VkAttachmentReference>& refs)
		{
			std::vector<VkAttachmentReference> attachments;

			for (const auto& ref : refs)
			{
				if (ref.layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
					attachments.push_back(ref);
			}

			return attachments;
		}

		static VkAttachmentReference GetDepthAttachment(const std::vector<VkAttachmentReference>& refs)
		{
			for (const auto& ref : refs)
			{
				if (ref.layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL || ref.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
					return ref;
			}

			WR_ASSERT(false, "could not get depth attachment");
			return {};
		}

	}

	VulkanRenderPass::VulkanRenderPass(VulkanRenderer* renderer, const RenderPassDesc& desc)
		: m_Renderer(renderer), m_Desc(desc)
	{
		std::vector<VkAttachmentDescription> attachments = createAttachmentDescriptions();
		std::vector<VkAttachmentReference> refs = createAttachmentRefs();

		std::vector<VkAttachmentReference> colorRefs = Utils::GetColorAttachments(refs);
		VkAttachmentReference depthRef = Utils::GetDepthAttachment(refs);

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = (uint32_t)colorRefs.size();
		subpass.pColorAttachments = colorRefs.data();
		subpass.pDepthStencilAttachment = &depthRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = (uint32_t)attachments.size();
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkResult result = vkCreateRenderPass(renderer->getDevice(), &renderPassInfo, renderer->getAllocator(), &m_RenderPass);
		VK_CHECK(result, "Failed to create Vulkan render pass!");

		m_Framebuffers.resize(renderer->getSwapchainImageCount());

		for (size_t i = 0; i < renderer->getSwapchainImageCount(); i++)
		{
			
		}
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
	}

	std::vector<VkAttachmentDescription> VulkanRenderPass::createAttachmentDescriptions()
	{
		std::vector<VkAttachmentDescription> result;

		for (const auto& attachment : m_Desc.Attachments)
		{
			VkAttachmentDescription desc{};
			desc.format = Utils::ConvertFormat(attachment.Format);
			desc.samples = (VkSampleCountFlagBits)attachment.Samples;
			desc.loadOp = (VkAttachmentLoadOp)attachment.LoadOp;
			desc.storeOp = (VkAttachmentStoreOp)attachment.StoreOp;
			desc.stencilLoadOp = (VkAttachmentLoadOp)attachment.StencilLoadOp;
			desc.stencilStoreOp = (VkAttachmentStoreOp)attachment.StencilStoreOp;
			desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.finalLayout = Utils::GetImageLayout(attachment.Usage);
			
			result.push_back(desc);
		}

		return result;
	}

	std::vector<VkAttachmentReference> VulkanRenderPass::createAttachmentRefs()
	{
		std::vector<VkAttachmentReference> refs;

		for (uint32_t i = 0; i < m_Desc.Attachments.size(); i++)
		{
			VkAttachmentReference ref{};
			ref.attachment = i;
			
			if (m_Desc.Attachments[i].Format != AttachmentFormat::D32_SFloat)
				ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			else
				ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}

		return refs;
	}

}

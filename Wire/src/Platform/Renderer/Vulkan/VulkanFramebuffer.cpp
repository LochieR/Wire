#include "wrpch.h"

#include <vulkan/vulkan.h>
#include "VulkanBuffer.h"
#include "VulkanFramebuffer.h"
#include "VulkanCommandBuffer.h"

#include <backends/imgui_impl_vulkan.h>

namespace Wire {

	namespace Utils {

		static VkFormat AttachmentFormatToVkFormat(AttachmentFormat format)
		{
			switch (format)
			{
				case AttachmentFormat::RGBA: return VK_FORMAT_R32G32B32A32_SFLOAT;
				case AttachmentFormat::R32_SInt: return VK_FORMAT_R32_SINT;
				case AttachmentFormat::R32_UInt: return VK_FORMAT_R32_UINT;
				default:
					return (VkFormat)0;
			}
		}

	}

	VulkanFramebuffer::VulkanFramebuffer(VulkanRenderer* renderer, const FramebufferSpecification& spec)
		: m_Renderer(renderer), m_Specification(spec)
	{
		auto& vkd = renderer->GetVulkanData();

		m_ImageCount = (uint32_t)vkd.SwapchainImages.size();

		m_Images.resize(m_ImageCount);
		m_ImageMemorys.resize(m_ImageCount);
		m_ImageViews.resize(m_ImageCount);
		m_Framebuffers.resize(m_ImageCount);
		m_Descriptors.resize(m_ImageCount);

		uint32_t attachmentCount = 0;
		for (uint32_t i = 0; i < spec.Attachments.size(); i++)
		{
			if (i == 0)
				continue;
			if (spec.Attachments[i] == AttachmentFormat::Depth)
				continue;
			attachmentCount++;
		}

		m_AttachmentImages.resize((size_t)attachmentCount);
		m_AttachmentImageMemorys.resize((size_t)attachmentCount);
		m_AttachmentImageViews.resize((size_t)attachmentCount);
		for (uint32_t i = 0; i < m_AttachmentImages.size(); i++)
		{
			m_AttachmentImages[i].resize(m_ImageCount);
			m_AttachmentImageMemorys[i].resize(m_ImageCount);
			m_AttachmentImageViews[i].resize(m_ImageCount);
		}

		for (uint32_t i = 0; i < m_ImageCount; i++)
		{
			uint32_t j = 0, k = 0;
			for (AttachmentFormat attachmentFormat : spec.Attachments)
			{
				VkFormat format = Utils::AttachmentFormatToVkFormat(attachmentFormat);
				if (format == (VkFormat)0 && attachmentFormat != AttachmentFormat::Depth)
				{
					format = vkd.SwapchainImageFormat;
				}
				else if (attachmentFormat == AttachmentFormat::Depth)
				{
					format = renderer->FindDepthFormat();

					if (m_DepthImage)
					{
						j++;
						continue;
					}

					renderer->CreateImage(
						spec.Width,
						spec.Height,
						VK_SAMPLE_COUNT_1_BIT,
						format,
						VK_IMAGE_TILING_OPTIMAL,
						VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						m_DepthImage,
						m_DepthImageMemory
					);

					m_DepthImageView = renderer->CreateImageView(m_DepthImage, format, VK_IMAGE_ASPECT_DEPTH_BIT);
					renderer->TransitionImageLayout(m_DepthImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
				
					j++;
					continue;
				}

				if (j == 0)
				{
					renderer->CreateImage(
						spec.Width, spec.Height,
						VK_SAMPLE_COUNT_1_BIT,
						format,
						VK_IMAGE_TILING_OPTIMAL,
						VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						m_Images[i],
						m_ImageMemorys[i]
					);

					m_ImageViews[i] = renderer->CreateImageView(m_Images[i], vkd.SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
				}
				else
				{
					renderer->CreateImage(
						spec.Width,
						spec.Height,
						VK_SAMPLE_COUNT_1_BIT,
						format,
						VK_IMAGE_TILING_OPTIMAL,
						VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
						m_AttachmentImages[k][i],
						m_AttachmentImageMemorys[k][i]
					);
					m_AttachmentImageViews[k][i] = renderer->CreateImageView(m_AttachmentImages[k][i], format, VK_IMAGE_ASPECT_COLOR_BIT);
					k++;
				}
				j++;
			}
		}

		{
			std::vector<VkAttachmentDescription> attachments;
			std::vector<VkAttachmentReference> colorRefs;
			VkAttachmentReference depthRef{};

			for (uint32_t i = 0; i < spec.Attachments.size(); i++)
			{
				AttachmentFormat attachmentFormat = spec.Attachments[i];

				bool depth = false;

				VkFormat format = Utils::AttachmentFormatToVkFormat(attachmentFormat);
				if (format == (VkFormat)0 && attachmentFormat != AttachmentFormat::Depth)
				{
					format = vkd.SwapchainImageFormat;
				}
				else if (attachmentFormat == AttachmentFormat::Depth)
				{
					format = renderer->FindDepthFormat();
					depth = true;
				}

				VkAttachmentDescription attachment{};
				attachment.format = format;
				attachment.samples = VK_SAMPLE_COUNT_1_BIT;
				attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				
				if (i == 0)
					attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				else if (depth)
					attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				else
					attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				attachments.push_back(attachment);

				VkAttachmentReference attachmentRef{};
				attachmentRef.attachment = i;
				attachmentRef.layout = depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				if (!depth)
					colorRefs.push_back(attachmentRef);
				else
					depthRef = attachmentRef;
			}

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = (uint32_t)colorRefs.size();
			subpass.pColorAttachments = colorRefs.data();
			subpass.pDepthStencilAttachment = &depthRef;

			std::array<VkSubpassDependency, 2> dependencies;

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_NONE_KHR;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = (uint32_t)attachments.size();
			renderPassInfo.pAttachments = attachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
			renderPassInfo.pDependencies = dependencies.data();

			VkResult result = vkCreateRenderPass(vkd.Device, &renderPassInfo, vkd.Allocator, &m_RenderPass);
			VK_CHECK(result, "Failed to create Vulkan render pass!");
		}

		for (uint32_t i = 0; i < m_ImageCount; i++)
		{
			std::vector<VkImageView> attachments;

			for (uint32_t j = 0, k = 0; j < spec.Attachments.size(); j++)
			{
				if (j == 0)
				{
					attachments.push_back(m_ImageViews[i]);
				}
				else if (spec.Attachments[j] == AttachmentFormat::Depth)
				{
					attachments.push_back(m_DepthImageView);
				}
				else
				{
					attachments.push_back(m_AttachmentImageViews[k][i]);
					k++;
				}
			}

			VkFramebufferCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.renderPass = m_RenderPass;
			createInfo.attachmentCount = (uint32_t)attachments.size();
			createInfo.pAttachments = attachments.data();
			createInfo.width = spec.Width;
			createInfo.height = spec.Height;
			createInfo.layers = 1;

			VkResult result = vkCreateFramebuffer(vkd.Device, &createInfo, vkd.Allocator, &m_Framebuffers[i]);
			VK_CHECK(result, "Failed to create Vulkan framebuffer!");
		}

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(vkd.PhysicalDevice, &properties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		VkResult result = vkCreateSampler(vkd.Device, &samplerInfo, vkd.Allocator, &m_Sampler);
		VK_CHECK(result, "Failed to create Vulkan sampler!");

		for (uint32_t i = 0; i < m_ImageCount; i++)
		{
			m_Descriptors[i] = ImGui_ImplVulkan_AddTexture(m_Sampler, m_ImageViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
		m_Renderer->SubmitResourceFree(
		[imageCount = m_ImageCount, framebuffers = m_Framebuffers, views = m_ImageViews, 
			images = m_Images, memorys = m_ImageMemorys, renderPass = m_RenderPass, sampler = m_Sampler,
			depthImage = m_DepthImage, depthView = m_DepthImageView, depthMemory = m_DepthImageMemory,
			attachments = m_AttachmentImages, attachmentMemorys = m_AttachmentImageMemorys, attachmentViews = m_AttachmentImageViews](VulkanRenderer* renderer)
		{
			auto& vkd = renderer->GetVulkanData();

			vkDestroySampler(vkd.Device, sampler, vkd.Allocator);

			for (uint32_t i = 0; i < attachments.size(); i++)
			{
				for (uint32_t j = 0; j < imageCount; j++)
				{
					vkDestroyImage(vkd.Device, attachments[i][j], vkd.Allocator);
					vkFreeMemory(vkd.Device, attachmentMemorys[i][j], vkd.Allocator);
					vkDestroyImageView(vkd.Device, attachmentViews[i][j], vkd.Allocator);
				}
			}

			vkDestroyImageView(vkd.Device, depthView, vkd.Allocator);
			vkDestroyImage(vkd.Device, depthImage, vkd.Allocator);
			vkFreeMemory(vkd.Device, depthMemory, vkd.Allocator);

			for (uint32_t i = 0; i < imageCount; i++)
			{
				vkDestroyFramebuffer(vkd.Device, framebuffers[i], vkd.Allocator);
				vkDestroyImageView(vkd.Device, views[i], vkd.Allocator);
				vkDestroyImage(vkd.Device, images[i], vkd.Allocator);
				vkFreeMemory(vkd.Device, memorys[i], vkd.Allocator);
			}

			vkDestroyRenderPass(vkd.Device, renderPass, vkd.Allocator);
		});

	}

	void VulkanFramebuffer::BeginRenderPass(rbRef<CommandBuffer> commandBuffer)
	{
		auto& vkd = m_Renderer->GetVulkanData();
		m_ImageIndex = vkd.ImageIndex;

		for (uint32_t i = 0; i < m_AttachmentImages.size(); i++)
		{

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_AttachmentImages[i][m_ImageIndex];
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			vkCmdPipelineBarrier(
				((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_RenderPass;
		renderPassInfo.renderArea.extent = { m_Specification.Width, m_Specification.Height };
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.framebuffer = m_Framebuffers[m_ImageIndex];

		VkClearValue clearValues[3] = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
		clearValues[2].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = 3;
		renderPassInfo.pClearValues = clearValues;
		
		vkCmdBeginRenderPass(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VulkanFramebuffer::EndRenderPass(rbRef<CommandBuffer> commandBuffer)
	{
		vkCmdEndRenderPass(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer);
	}

	void VulkanFramebuffer::Resize(uint32_t width, uint32_t height)
	{
		std::vector<VkFramebuffer> tempFramebuffers;
		for (VkFramebuffer framebuffer : m_Framebuffers)
			tempFramebuffers.push_back(framebuffer);
		std::vector<VkImage> tempImages;
		for (VkImage image : m_Images)
			tempImages.push_back(image);
		std::vector<VkImageView> tempImageViews;
		for (VkImageView imageView : m_ImageViews)
			tempImageViews.push_back(imageView);
		std::vector<VkDeviceMemory> tempDeviceMemorys;
		for (VkDeviceMemory memory : m_ImageMemorys)
			tempDeviceMemorys.push_back(memory);
		std::vector<VkDescriptorSet> tempDescriptors;
		for (VkDescriptorSet descriptor : m_Descriptors)
			tempDescriptors.push_back(descriptor);
		std::vector<std::vector<VkImage>> tempAttachments;
		for (std::vector<VkImage> attachment : m_AttachmentImages)
		{
			std::vector<VkImage> tempImageAttachment;
			for (VkImage image : attachment)
				tempImageAttachment.push_back(image);

			tempAttachments.push_back(tempImageAttachment);
		}
		std::vector<std::vector<VkDeviceMemory>> tempAttachmentMemorys;
		for (std::vector<VkDeviceMemory> attachment : m_AttachmentImageMemorys)
		{
			std::vector<VkDeviceMemory> tempImageAttachment;
			for (VkDeviceMemory memory : attachment)
				tempImageAttachment.push_back(memory);

			tempAttachmentMemorys.push_back(tempImageAttachment);
		}
		std::vector<std::vector<VkImageView>> tempAttachmentViews;
		for (std::vector<VkImageView> attachment : m_AttachmentImageViews)
		{
			std::vector<VkImageView> tempImageAttachment;
			for (VkImageView image : attachment)
				tempImageAttachment.push_back(image);

			tempAttachmentViews.push_back(tempImageAttachment);
		}

		m_Renderer->SubmitResourceFree(
		[imageCount = m_ImageCount, framebuffers = tempFramebuffers,
		images = tempImages, views = tempImageViews, memorys = tempDeviceMemorys, descriptors = tempDescriptors,
		depthImage = m_DepthImage, depthView = m_DepthImageView, depthMemory = m_DepthImageMemory,
		attachments = tempAttachments, attachmentMemorys = tempAttachmentMemorys, attachmentViews = tempAttachmentViews](VulkanRenderer* renderer)
		{
			auto& vkd = renderer->GetVulkanData();

			vkDeviceWaitIdle(vkd.Device);

			for (uint32_t i = 0; i < attachments.size(); i++)
			{
				for (uint32_t j = 0; j < imageCount; j++)
				{
					vkDestroyImage(vkd.Device, attachments[i][j], vkd.Allocator);
					vkFreeMemory(vkd.Device, attachmentMemorys[i][j], vkd.Allocator);
					vkDestroyImageView(vkd.Device, attachmentViews[i][j], vkd.Allocator);
				}
			}

			vkDestroyImageView(vkd.Device, depthView, vkd.Allocator);
			vkDestroyImage(vkd.Device, depthImage, vkd.Allocator);
			vkFreeMemory(vkd.Device, depthMemory, vkd.Allocator);

			for (uint32_t i = 0; i < imageCount; i++)
			{
				vkDestroyFramebuffer(vkd.Device, framebuffers[i], vkd.Allocator);
				vkDestroyImageView(vkd.Device, views[i], vkd.Allocator);
				vkDestroyImage(vkd.Device, images[i], vkd.Allocator);
				vkFreeMemory(vkd.Device, memorys[i], vkd.Allocator);
				
				ImGui_ImplVulkan_RemoveTexture(descriptors[i]);
			}
		});

		auto& vkd = m_Renderer->GetVulkanData();

		m_Framebuffers.clear();
		m_ImageViews.clear();
		m_Images.clear();
		m_ImageMemorys.clear();
		m_Framebuffers.resize((size_t)m_ImageCount);
		m_ImageViews.resize((size_t)m_ImageCount);
		m_Images.resize((size_t)m_ImageCount);
		m_ImageMemorys.resize((size_t)m_ImageCount);

		m_Specification.Width = width;
		m_Specification.Height = height;

		uint32_t attachmentCount = 0;
		for (uint32_t i = 0; i < m_Specification.Attachments.size(); i++)
		{
			if (i == 0)
				continue;
			if (m_Specification.Attachments[i] == AttachmentFormat::Depth)
				continue;
			attachmentCount++;
		}

		m_AttachmentImages.resize((size_t)attachmentCount);
		m_AttachmentImageMemorys.resize((size_t)attachmentCount);
		m_AttachmentImageViews.resize((size_t)attachmentCount);
		for (uint32_t i = 0; i < m_AttachmentImages.size(); i++)
		{
			m_AttachmentImages[i].resize(m_ImageCount);
			m_AttachmentImageMemorys[i].resize(m_ImageCount);
			m_AttachmentImageViews[i].resize(m_ImageCount);
		}

		m_DepthImage = nullptr;

		for (uint32_t i = 0; i < m_ImageCount; i++)
		{
			uint32_t j = 0, k = 0;
			for (AttachmentFormat attachmentFormat : m_Specification.Attachments)
			{
				VkFormat format = Utils::AttachmentFormatToVkFormat(attachmentFormat);
				if (format == (VkFormat)0 && attachmentFormat != AttachmentFormat::Depth)
				{
					format = vkd.SwapchainImageFormat;
				}
				else if (attachmentFormat == AttachmentFormat::Depth)
				{
					format = m_Renderer->FindDepthFormat();

					if (m_DepthImage)
					{
						j++;
						continue;
					}

					m_Renderer->CreateImage(
						m_Specification.Width,
						m_Specification.Height,
						VK_SAMPLE_COUNT_1_BIT,
						format,
						VK_IMAGE_TILING_OPTIMAL,
						VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						m_DepthImage,
						m_DepthImageMemory
					);

					m_DepthImageView = m_Renderer->CreateImageView(m_DepthImage, format, VK_IMAGE_ASPECT_DEPTH_BIT);
					m_Renderer->TransitionImageLayout(m_DepthImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

					j++;
					continue;
				}

				if (j == 0)
				{
					m_Renderer->CreateImage(
						m_Specification.Width,
						m_Specification.Height,
						VK_SAMPLE_COUNT_1_BIT,
						format,
						VK_IMAGE_TILING_OPTIMAL,
						VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						m_Images[i],
						m_ImageMemorys[i]
					);

					m_ImageViews[i] = m_Renderer->CreateImageView(m_Images[i], vkd.SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
				}
				else
				{
					m_Renderer->CreateImage(
						m_Specification.Width,
						m_Specification.Height,
						VK_SAMPLE_COUNT_1_BIT,
						format,
						VK_IMAGE_TILING_OPTIMAL,
						VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
						m_AttachmentImages[k][i],
						m_AttachmentImageMemorys[k][i]
					);
					m_AttachmentImageViews[k][i] = m_Renderer->CreateImageView(m_AttachmentImages[k][i], format, VK_IMAGE_ASPECT_COLOR_BIT);
					k++;
				}
				j++;
			}
		}

		for (uint32_t i = 0; i < m_ImageCount; i++)
		{
			std::vector<VkImageView> attachments;

			for (uint32_t j = 0, k = 0; j < m_Specification.Attachments.size(); j++)
			{
				if (j == 0)
				{
					attachments.push_back(m_ImageViews[i]);
				}
				else if (m_Specification.Attachments[j] == AttachmentFormat::Depth)
				{
					attachments.push_back(m_DepthImageView);
				}
				else
				{
					attachments.push_back(m_AttachmentImageViews[k][i]);
					k++;
				}
			}

			VkFramebufferCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.renderPass = m_RenderPass;
			createInfo.attachmentCount = (uint32_t)attachments.size();
			createInfo.pAttachments = attachments.data();
			createInfo.width = width;
			createInfo.height = height;
			createInfo.layers = 1;

			VkResult result = vkCreateFramebuffer(vkd.Device, &createInfo, vkd.Allocator, &m_Framebuffers[i]);
			VK_CHECK(result, "Failed to create Vulkan framebuffer!");

			rbRef<CommandBuffer> commandBuffer = m_Renderer->BeginSingleTimeCommands();

			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageMemoryBarrier.image = m_Images[i];
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier
			);

			m_Renderer->EndSingleTimeCommands(commandBuffer);

			m_Descriptors[i] = ImGui_ImplVulkan_AddTexture(m_Sampler, m_ImageViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}

	uint32_t VulkanFramebuffer::GetColorAttachmentCount() const
	{
		uint32_t count = 0;

		for (AttachmentFormat format : m_Specification.Attachments)
		{
			if (format != AttachmentFormat::Depth)
				count++;
		}

		return count;
	}

	void VulkanFramebuffer::CopyAttachmentImageToBuffer(uint32_t attachmentIndex, rbRef<StagingBuffer> buffer)
	{
		WR_ASSERT(attachmentIndex > 0 && "Can't copy main color attachment to buffer!");

		rbRef<CommandBuffer> commandBuffer = m_Renderer->BeginSingleTimeCommands();

		attachmentIndex--; // account for first color attachment

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_AttachmentImages[attachmentIndex][m_ImageIndex];
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(
			((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		VkImageSubresourceLayers subresource{};
		subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource.mipLevel = 0;
		subresource.baseArrayLayer = 0;
		subresource.layerCount = 1;

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource = subresource;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { m_Specification.Width, m_Specification.Height };
		region.imageExtent.depth = 1;

		vkCmdCopyImageToBuffer(
			((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer,
			m_AttachmentImages[attachmentIndex][m_ImageIndex],
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			((VulkanStagingBuffer*)buffer.Get())->m_StagingBuffer,
			1, &region
		);

		m_Renderer->EndSingleTimeCommands(commandBuffer);
	}

}

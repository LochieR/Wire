#include "VulkanRenderer.h"

#include "VulkanFont.h"
#include "VulkanBuffer.h"
#include "VulkanTexture2D.h"
#include "VulkanCommandBuffer.h"
#include "VulkanGraphicsPipeline.h"

#include "Wire/Core/Assert.h"
#include "Wire/Core/Application.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <set>
#include <array>
#include <vector>
#include <iostream>
#include <algorithm>

namespace wire {
	
	constexpr bool s_EnableValidationLayers = true;

	const static std::vector<const char*> s_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	const static std::vector<const char*> s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

	struct QueueFamilyIndices
	{
		uint32_t GraphicsFamily = static_cast<uint32_t>(-1);
		uint32_t PresentFamily = static_cast<uint32_t>(-1);

		bool IsComplete() const { return GraphicsFamily != static_cast<uint32_t>(-1) && PresentFamily != static_cast<uint32_t>(-1); }
	};

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	namespace Utils {

		static std::vector<const char*> GetRequiredExtensions()
		{
			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

			std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

			if (s_EnableValidationLayers)
			{
				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			return extensions;
		}

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)
		{
			if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				return VK_FALSE;

			if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
				std::cerr << "\u001b[31m[Validation Layer] " << pCallbackData->pMessage << "\u001b[0m" << std::endl;
			//else
			//	std::cerr << "\033[38;5;208m[Validation Layer] " << pCallbackData->pMessage << "\u001b[0m" << std::endl;

			return VK_FALSE;
		}

		static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
		{
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr)
			{
				return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
		{
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr)
			{
				func(instance, debugMessenger, pAllocator);
			}
		}

		static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
		{
			createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = DebugCallback;
		}

		static bool CheckValidationLayerSupport()
		{
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (const char* layerName : s_ValidationLayers)
			{
				bool layerFound = false;

				for (const auto& layerProperties : availableLayers)
				{
					if (strcmp(layerName, layerProperties.layerName) == 0)
					{
						layerFound = true;
						break;
					}
				}

				if (!layerFound)
				{
					return false;
				}
			}

			return true;
		}

		static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
		{
			QueueFamilyIndices indices{};

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			int i = 0;
			for (const auto& queueFamily : queueFamilies)
			{
				if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					indices.GraphicsFamily = i;

				VkBool32 presentSupport = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

				if (presentSupport)
					indices.PresentFamily = i;

				if (indices.IsComplete())
					break;

				i++;
			}

			return indices;
		}

		static bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
		{
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

			std::set<std::string> requiredExtensions;

			for (const char* ext : s_DeviceExtensions)
				requiredExtensions.insert(std::string(ext));

			for (const auto& extension : availableExtensions)
			{
				requiredExtensions.erase(std::string(extension.extensionName));
			}

			return requiredExtensions.empty();
		}

		static SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
		{
			SwapchainSupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

			if (formatCount != 0)
			{
				details.Formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

			if (presentModeCount != 0)
			{
				details.PresentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
			}

			return details;
		}

		static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
			{
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
					return i;
			}

			WR_ASSERT(false, "Failed to find suitable memory type!");
			return 0;
		}

		static bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
		{
			QueueFamilyIndices indices = FindQueueFamilies(device, surface);

			bool extensionsSupported = CheckDeviceExtensionSupport(device);

			bool swapchainAdequate = false;
			if (extensionsSupported)
			{
				SwapchainSupportDetails swapChainSupport = QuerySwapchainSupport(device, surface);
				swapchainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
			}

			VkPhysicalDeviceFeatures supportedFeatures;
			vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

			return
				indices.IsComplete() &&
				extensionsSupported &&
				swapchainAdequate &&
				supportedFeatures.sampleRateShading &&
				supportedFeatures.samplerAnisotropy;
		}

		static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
		{
			for (const auto& availableFormat : availableFormats)
			{
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					return availableFormat;
				}
			}

			return availableFormats[0];
		}

		static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
		{
			for (const auto& availablePresentMode : availablePresentModes)
			{
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					return availablePresentMode;
				}
			}

			return VK_PRESENT_MODE_FIFO_KHR;
		}

		static VkExtent2D ChooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities)
		{
			if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			{
				return capabilities.currentExtent;
			}
			else
			{
				int width, height;
				glfwGetFramebufferSize(window, &width, &height);

				VkExtent2D actualExtent = {
					static_cast<uint32_t>(width),
					static_cast<uint32_t>(height)
				};

				actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

				return actualExtent;
			}
		}

		static VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
		{
			for (VkFormat format : candidates)
			{
				VkFormatProperties properties;
				vkGetPhysicalDeviceFormatProperties(device, format, &properties);

				if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
					return format;
				else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
					return format;
			}

			WR_ASSERT(false, "Failed to find supported Vulkan format!");
			return candidates[0];
		}

		static VkFormat FindDepthFormat(VkPhysicalDevice device)
		{
			return FindSupportedFormat(
				device,
				{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
			);
		}

		static bool HasStencilComponent(VkFormat format)
		{
			return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
		}

		static VkShaderStageFlags ConvertShaderType(ShaderType type)
		{
			switch (type)
			{
			case ShaderType::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
			case ShaderType::Pixel: return VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			WR_ASSERT(false, "Unknown ShaderType!");
			return VkShaderStageFlags(0);
		}
	}

	VulkanRenderer::VulkanRenderer(const RendererDesc& desc)
	{
		m_FrameIndex = 0;
		m_ResourceFreeQueue.resize(WR_FRAMES_IN_FLIGHT);

		createInstance();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createCommandPool();
		createDescriptorPool();

		createSwapchain();
		createRenderPass();
		createSyncObject();

		m_ShaderCache = ShaderCache::createOrGetShaderCache(desc.ShaderCache);
		m_FontCache = FontCache::createOrGetFontCache(desc.FontCache);
	}

	VulkanRenderer::~VulkanRenderer()
	{
		vkDeviceWaitIdle(m_Device);

		m_FontCache.release();

		for (auto& queue : m_ResourceFreeQueue)
		{
			for (auto& func : queue)
				func(this);
			queue.clear();
		}
		m_ResourceFreeQueue.clear();

		for (CommandBuffer* commandBuffer : m_AllocatedCommandBuffers)
			delete commandBuffer;
		m_AllocatedCommandBuffers.clear();

		vkDestroyRenderPass(m_Device, m_RenderPass, m_Allocator);

		for (VkFramebuffer framebuffer : m_Framebuffers)
			vkDestroyFramebuffer(m_Device, framebuffer, m_Allocator);
		
		vkDestroyImageView(m_Device, m_DepthImageView, m_Allocator);
		vkDestroyImage(m_Device, m_DepthImage, m_Allocator);
		vkFreeMemory(m_Device, m_DepthImageMemory, m_Allocator);

		for (VkImageView view : m_SwapchainImageViews)
			vkDestroyImageView(m_Device, view, m_Allocator);
		m_SwapchainImageViews.clear();

		vkDestroySwapchainKHR(m_Device, m_Swapchain, m_Allocator);

		for (VkFence fence : m_InFlightFences)
			vkDestroyFence(m_Device, fence, m_Allocator);

		for (VkSemaphore semaphore : m_ImageAvailableSemaphores)
			vkDestroySemaphore(m_Device, semaphore, m_Allocator);
		m_ImageAvailableSemaphores.clear();

		for (VkSemaphore semaphore : m_RenderFinishedSemaphores)
			vkDestroySemaphore(m_Device, semaphore, m_Allocator);
		m_RenderFinishedSemaphores.clear();

		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, m_Allocator);
		vkDestroyCommandPool(m_Device, m_CommandPool, m_Allocator);
		m_FrameCommandBuffers.clear();

		vkDestroyDevice(m_Device, m_Allocator);
		vkDestroySurfaceKHR(m_Instance, m_Surface, m_Allocator);
		Utils::DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, m_Allocator);
		vkDestroyInstance(m_Instance, m_Allocator);
	}

	void VulkanRenderer::beginFrame()
	{
		VkResult result = vkWaitForFences(m_Device, 1, &m_InFlightFences[m_FrameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());
		VK_CHECK(result, "Failed to wait for Vulkan fence!");

		result = vkAcquireNextImageKHR(m_Device, m_Swapchain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphores[m_FrameIndex], 0, &m_ImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || Application::get().wasWindowResized())
		{
			Application::get().resetWindowResized();
			createSwapchain();
			createRenderPass();

			m_SkipFrame = true;

			submitResourceFree(
				[semaphore = m_ImageAvailableSemaphores[m_FrameIndex]](Renderer* renderer)
				{
					VulkanRenderer* vk = (VulkanRenderer*)renderer;

					vkDestroySemaphore(vk->getDevice(), semaphore, vk->getAllocator());
				}
			);

			VkSemaphoreCreateInfo semaphoreInfo{};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			result = vkCreateSemaphore(m_Device, &semaphoreInfo, m_Allocator, &m_ImageAvailableSemaphores[m_FrameIndex]);
			VK_CHECK(result, "Failed to create Vulkan semaphore!");

			return;
		}
		VK_CHECK(result, "Failed to acquire next Vulkan image!");

		result = vkResetFences(m_Device, 1, &m_InFlightFences[m_FrameIndex]);
		VK_CHECK(result, "Failed to reset Vulkan fence!");
	}

	void VulkanRenderer::endFrame()
	{
		if (m_SkipFrame)
		{
			m_SkipFrame = false;

			m_CurrentNonRenderingCommandBuffers.clear();
			m_CurrentRenderingCommandBuffers.clear();
			m_SubmittedNonRenderingCommandBuffers.clear();
			m_SubmittedRenderingCommandBuffers.clear();

			vkDeviceWaitIdle(m_Device);

			for (auto& func : m_ResourceFreeQueue[m_FrameIndex])
			{
				func(this);
			}
			m_ResourceFreeQueue[m_FrameIndex].clear();

			return;
		}

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkResult result = vkBeginCommandBuffer(m_FrameCommandBuffers[m_FrameIndex], &beginInfo);
		VK_CHECK(result, "Failed to begin Vulkan command buffer!");

		for (const auto& listInfo : m_SubmittedCommandLists[m_FrameIndex])
		{
			for (size_t i = 0; i < listInfo.Types.size(); i++)
			{
				CommandScope::Type type = listInfo.Types[i];
				
				if (type == CommandScope::RenderPass)
				{
					std::array<VkClearValue, 2> clearValues = {};
					clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
					clearValues[1].depthStencil = { 1.0f, 0 };

					VkRenderPassBeginInfo renderPassInfo{};
					renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassInfo.renderArea.extent = m_Extent;
					renderPassInfo.renderArea.offset = { 0, 0 };
					renderPassInfo.renderPass = m_RenderPass;
					renderPassInfo.framebuffer = m_Framebuffers[m_ImageIndex];
					renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
					renderPassInfo.pClearValues = clearValues.data();

					vkCmdBeginRenderPass(m_FrameCommandBuffers[m_FrameIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
				}

				vkCmdExecuteCommands(m_FrameCommandBuffers[m_FrameIndex], 1, &listInfo.Buffers[i]);

				if (type == CommandScope::RenderPass)
				{
					vkCmdEndRenderPass(m_FrameCommandBuffers[m_FrameIndex]);
				}
			}
		}

		// old code ---------------------------
		/*if (!m_SubmittedNonRenderingCommandBuffers.empty())
			vkCmdExecuteCommands(m_FrameCommandBuffers[m_FrameIndex], (uint32_t)m_SubmittedNonRenderingCommandBuffers.size(), m_SubmittedNonRenderingCommandBuffers.data());

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderArea.extent = m_Extent;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderPass = m_RenderPass;
		renderPassInfo.framebuffer = m_Framebuffers[m_ImageIndex];
		renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_FrameCommandBuffers[m_FrameIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		if (!m_SubmittedRenderingCommandBuffers.empty())
			vkCmdExecuteCommands(m_FrameCommandBuffers[m_FrameIndex], (uint32_t)m_SubmittedRenderingCommandBuffers.size(), m_SubmittedRenderingCommandBuffers.data());

		vkCmdEndRenderPass(m_FrameCommandBuffers[m_FrameIndex]);*/
		// old code ---------------------------

		result = vkEndCommandBuffer(m_FrameCommandBuffers[m_FrameIndex]);
		VK_CHECK(result, "Failed to end Vulkan command buffer!");

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &m_FrameCommandBuffers[m_FrameIndex];
		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores = &m_ImageAvailableSemaphores[m_FrameIndex];
		submit.pWaitDstStageMask = waitStages;
		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &m_RenderFinishedSemaphores[m_ImageIndex];

		result = vkQueueSubmit(m_GraphicsQueue, 1, &submit, m_InFlightFences[m_FrameIndex]);
		VK_CHECK(result, "Failed to submit to Vulkan queue!");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_ImageIndex];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.pImageIndices = &m_ImageIndex;

		result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || Application::get().wasWindowResized())
		{
			Application::get().resetWindowResized();

			createSwapchain();
			createRenderPass();
		}
		VK_CHECK(result, "Failed to present to Vulkan queue!");

		m_CurrentNonRenderingCommandBuffers.clear();
		m_CurrentRenderingCommandBuffers.clear();
		m_SubmittedNonRenderingCommandBuffers.clear();
		m_SubmittedRenderingCommandBuffers.clear();

		m_SubmittedCommandLists[m_FrameIndex].clear();
		m_UsedSecondaryCommandBufferCount[m_FrameIndex] = 0;

		for (auto& func : m_ResourceFreeQueue[m_FrameIndex])
		{
			func(this);
		}
		m_ResourceFreeQueue[m_FrameIndex].clear();

		m_FrameIndex = (m_FrameIndex + 1) % WR_FRAMES_IN_FLIGHT;
	}

	uint32_t VulkanRenderer::getNumFramesInFlight() const
	{
		return WR_FRAMES_IN_FLIGHT;
	}

	CommandBuffer& VulkanRenderer::allocateCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.commandBufferCount = 1;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		
		VkCommandBuffer commandBuffer;
		VkResult result = vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);
		VK_CHECK(result, "Failed to allocate Vulkan command buffer!");

		m_AllocatedCommandBuffers.push_back(new VulkanCommandBuffer(this, commandBuffer, false));

		return *m_AllocatedCommandBuffers[m_AllocatedCommandBuffers.size() - 1];
	}

	CommandBuffer& VulkanRenderer::beginSingleTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.commandBufferCount = 1;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VkCommandBuffer commandBuffer;
		VkResult result = vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);
		VK_CHECK(result, "Failed to allocate Vulkan command buffer!");

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		VK_CHECK(result, "Failed to begin Vulkan command buffer!");

		m_CurrentSingleTimeCommands.push_back(new VulkanCommandBuffer(this, commandBuffer, true));

		return *m_CurrentSingleTimeCommands[m_CurrentSingleTimeCommands.size() - 1];
	}

	void VulkanRenderer::endSingleTimeCommands(CommandBuffer& commandBuffer)
	{
		VkCommandBuffer cmd = commandBuffer.as<VkCommandBuffer>();

		VkResult result = vkEndCommandBuffer(cmd);
		VK_CHECK(result, "Failed to end Vulkan command buffer!");

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmd;

		result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, 0);
		VK_CHECK(result, "Failed to submit to Vulkan queue!");

		result = vkQueueWaitIdle(m_GraphicsQueue);
		VK_CHECK(result, "An error occurred while waiting for Vulkan queue!");

		uint32_t commandBufferIndex = -1;
		for (uint32_t i = 0; i < m_CurrentSingleTimeCommands.size(); i++)
		{
			if ((VkCommandBuffer)m_CurrentSingleTimeCommands[i]->get() == cmd)
				commandBufferIndex = i;
		}
		WR_ASSERT(commandBufferIndex != -1, "Unknown command buffer for endSingleTimeCommands!");

		delete m_CurrentSingleTimeCommands[commandBufferIndex];
		m_CurrentSingleTimeCommands.erase(m_CurrentSingleTimeCommands.begin() + commandBufferIndex);

		vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &cmd);
	}

	void VulkanRenderer::submitCommandBuffer(CommandBuffer& commandBuffer)
	{
		if (m_SkipFrame)
			return;

		auto it1 = std::find(m_CurrentRenderingCommandBuffers.begin(), m_CurrentRenderingCommandBuffers.end(), commandBuffer.as<VkCommandBuffer>());
		auto it2 = std::find(m_CurrentNonRenderingCommandBuffers.begin(), m_CurrentNonRenderingCommandBuffers.end(), commandBuffer.as<VkCommandBuffer>());
		if (it1 != m_CurrentRenderingCommandBuffers.end())
		{
			m_SubmittedRenderingCommandBuffers.push_back(commandBuffer.as<VkCommandBuffer>());
		}
		else if (it2 != m_CurrentNonRenderingCommandBuffers.end())
		{
			m_SubmittedNonRenderingCommandBuffers.push_back(commandBuffer.as<VkCommandBuffer>());
		}
		else // assume rendering command buffer
		{
			m_SubmittedRenderingCommandBuffers.push_back(commandBuffer.as<VkCommandBuffer>());
		}
	}

	CommandList VulkanRenderer::createCommandList()
	{
		return CommandList(this);
	}

	void VulkanRenderer::submitCommandList(const CommandList& commandList)
	{
		WR_ASSERT(!commandList.isRecording(), "cannot submit a CommandList that is currently recording!");

		CommandListData listData{};

		for (const auto& scope : commandList.getScopes())
		{
			if (scope.Commands.empty())
				continue;

			VkCommandBuffer commandBuffer;

			if (m_UsedSecondaryCommandBufferCount[m_FrameIndex] < m_SecondaryCommandBufferPool[m_FrameIndex].size())
				commandBuffer = m_SecondaryCommandBufferPool[m_FrameIndex][m_UsedSecondaryCommandBufferCount[m_FrameIndex]++];
			else
			{
				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = m_CommandPool;
				allocInfo.commandBufferCount = 1;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

				VkResult result = vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);
				VK_CHECK(result, "Failed to allocate Vulkan command buffer!");

				m_SecondaryCommandBufferPool[m_FrameIndex].push_back(commandBuffer);
				m_UsedSecondaryCommandBufferCount[m_FrameIndex]++;
			}
			
			VkCommandBufferInheritanceInfo inheritanceInfo{};
			inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.pInheritanceInfo = &inheritanceInfo;
			
			if (scope.ScopeType == CommandScope::RenderPass)
			{
				inheritanceInfo.renderPass = m_RenderPass;
				inheritanceInfo.framebuffer = getCurrentFramebuffer();

				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
			}
			
			VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
			VK_CHECK(result, "Failed to begin Vulkan command buffer!");

			executeCommandScope(commandBuffer, scope);

			result = vkEndCommandBuffer(commandBuffer);
			VK_CHECK(result, "Failed to end Vulkan command buffer!");

			listData.Buffers.push_back(commandBuffer);
			listData.Types.push_back(scope.ScopeType);
		}

		m_SubmittedCommandLists[m_FrameIndex].push_back(listData);
	}

	void VulkanRenderer::executeCommandScope(VkCommandBuffer commandBuffer, const CommandScope& commandScope)
	{
		for (const auto& command : commandScope.Commands)
		{
			switch (command.Type)
			{
			case CommandType::BeginRenderPass:
			case CommandType::EndRenderPass:
				break; // don't need to handle this as scopes handle it
			case CommandType::BindPipeline:
			{
				const auto& args = std::get<CommandEntry::BindPipelineArgs>(command.Args);

				const VulkanGraphicsPipeline* vkPipeline = static_cast<const VulkanGraphicsPipeline*>(args.Pipeline);

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->getPipeline());
				break;
			}
			case CommandType::PushConstants:
			{
				const auto& args = std::get<CommandEntry::PushConstantsArgs>(command.Args);

				const VulkanGraphicsPipeline* vkPipeline = static_cast<const VulkanGraphicsPipeline*>(args.Pipeline);

				vkCmdPushConstants(commandBuffer, vkPipeline->getPipelineLayout(), Utils::ConvertShaderType(args.Stage), args.Offset, args.Size, args.Data);
				break;
			}
			case CommandType::BindDescriptorSet:
			{
				const auto& args = std::get<CommandEntry::BindDescriptorSetArgs>(command.Args);

				const VulkanGraphicsPipeline* vkPipeline = static_cast<const VulkanGraphicsPipeline*>(args.Pipeline);

				VkDescriptorSet set = vkPipeline->getDescriptorSet();
				vkCmdBindDescriptorSets(
					commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					vkPipeline->getPipelineLayout(),
					0,
					1,
					&set,
					0,
					nullptr
				);
				break;
			}
			case CommandType::SetViewport:
			{
				const auto& args = std::get<CommandEntry::SetViewportArgs>(command.Args);

				VkViewport viewport{};
				viewport.x = args.Position.x;
				viewport.y = args.Position.y;
				viewport.width = args.Size.x;
				viewport.height = args.Size.y;
				viewport.minDepth = args.MinDepth;
				viewport.maxDepth = args.MaxDepth;

				vkCmdSetViewport(
					commandBuffer,
					0,
					1,
					&viewport
				);
				break;
			}
			case CommandType::SetScissor:
			{
				const auto& args = std::get<CommandEntry::SetScissorArgs>(command.Args);

				VkRect2D rect{};
				rect.extent = { (uint32_t)(args.Max.x - args.Min.x), (uint32_t)(args.Max.y - args.Min.y) };
				rect.offset = { (int)args.Min.x, (int)args.Min.y };

				vkCmdSetScissor(
					commandBuffer,
					0,
					1,
					&rect
				);
				break;
			}
			case CommandType::SetLineWidth:
			{
				const auto& args = std::get<CommandEntry::SetLineWidthArgs>(command.Args);

				vkCmdSetLineWidth(commandBuffer, args.LineWidth);
				break;
			}
			case CommandType::BindVertexBuffers:
			{
				const auto& args = std::get<CommandEntry::BindVertexBuffersArgs>(command.Args);

				std::vector<VkBuffer> buffers(args.Buffers.size());
				std::vector<VkDeviceSize> offsets(args.Buffers.size());
				for (size_t i = 0; i < args.Buffers.size(); i++)
				{
					buffers[i] = static_cast<const VulkanVertexBuffer*>(args.Buffers[i])->getBuffer();
					offsets[i] = 0;
				}

				vkCmdBindVertexBuffers(
					commandBuffer,
					0,
					static_cast<uint32_t>(args.Buffers.size()),
					buffers.data(),
					offsets.data()
				);
				break;
			}
			case CommandType::BindIndexBuffer:
			{
				const auto& args = std::get<CommandEntry::BindIndexBufferArgs>(command.Args);

				vkCmdBindIndexBuffer(commandBuffer, static_cast<const VulkanIndexBuffer*>(args.Buffer)->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
				break;
			}
			case CommandType::Draw:
			{
				const auto& args = std::get<CommandEntry::DrawArgs>(command.Args);
				
				vkCmdDraw(commandBuffer, args.VertexCount, 1, args.VertexOffset, 0);
				break;
			}
			case CommandType::DrawIndexed:
			{
				const auto& args = std::get<CommandEntry::DrawIndexedArgs>(command.Args);

				vkCmdDrawIndexed(commandBuffer, args.IndexCount, 1, args.IndexOffset, args.VertexOffset, 0);
				break;
			}
			case CommandType::Dispatch:
			{
				break;
			}
			default:
				WR_ASSERT(false, "Unknown command in CommandList!");
				break;
			}
		}
	}

	void VulkanRenderer::submitResourceFree(std::function<void(Renderer*)>&& func)
	{
		m_ResourceFreeQueue[m_FrameIndex].emplace_back(func);
	}

	GraphicsPipeline* VulkanRenderer::createGraphicsPipeline(const GraphicsPipelineDesc& desc)
	{
		return new VulkanGraphicsPipeline(this, desc);
	}

	VertexBuffer* VulkanRenderer::createVertexBuffer(size_t size, const void* data)
	{
		return new VulkanVertexBuffer(this, size, data);
	}

	IndexBuffer* VulkanRenderer::createIndexBuffer(size_t size, const void* data)
	{
		return new VulkanIndexBuffer(this, size, data);
	}

	StagingBuffer* VulkanRenderer::createStagingBuffer(size_t size, const void* data)
	{
		return new VulkanStagingBuffer(this, size, data);
	}

	UniformBuffer* VulkanRenderer::createUniformBuffer(size_t size, const void* data)
	{
		return new VulkanUniformBuffer(this, size, data);
	}

	Texture2D* VulkanRenderer::createTexture2D(const std::filesystem::path& path)
	{
		return new VulkanTexture2D(this, path);
	}

	Texture2D* VulkanRenderer::createTexture2D(uint32_t* data, uint32_t width, uint32_t height)
	{
		return new VulkanTexture2D(this, data, width, height);
	}

	Sampler* VulkanRenderer::createSampler(const SamplerDesc& desc)
	{
		return new VulkanSampler(this, desc);
	}

	Font* VulkanRenderer::createFont(const std::filesystem::path& path, uint32_t minChar, uint32_t maxChar)
	{
		return new VulkanFont(this, path, minChar, maxChar);
	}

	Font* VulkanRenderer::getFontFromCache(const std::filesystem::path& path)
	{
		std::string pathStr = path.string();

		WR_ASSERT(pathStr.contains("fontcache://"), "Invalid font cache path! (must begin with fontcache://)");

		std::string name = pathStr.substr(12);

		for (const auto& font : m_FontCache)
		{
			if (font.Name == name)
				return new VulkanFont(this, font);
		}

		WR_ASSERT(false, "Font not found in cache!");
		return nullptr;
	}

	float VulkanRenderer::getMaxAnisotropy() const
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

		return properties.limits.maxSamplerAnisotropy;
	}

	void VulkanRenderer::createInstance()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "cNom";
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo instanceInfo{};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pApplicationInfo = &appInfo;

		std::vector<const char*> extensions = Utils::GetRequiredExtensions();
		instanceInfo.enabledExtensionCount = (uint32_t)extensions.size();
		instanceInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if constexpr (s_EnableValidationLayers)
		{
			WR_ASSERT(Utils::CheckValidationLayerSupport(), "Validation layers enables but are not supported!");

			instanceInfo.enabledLayerCount = (uint32_t)s_ValidationLayers.size();
			instanceInfo.ppEnabledLayerNames = s_ValidationLayers.data();

			Utils::PopulateDebugMessengerCreateInfo(debugCreateInfo);
			instanceInfo.pNext = &debugCreateInfo;
		}
		else
		{
			instanceInfo.enabledLayerCount = 0;
			instanceInfo.ppEnabledLayerNames = nullptr;
		}

		VkResult result = vkCreateInstance(&instanceInfo, m_Allocator, &m_Instance);
		VK_CHECK(result, "Failed to create Vulkan instance!");

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> instanceExtensions(extensionCount);

		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instanceExtensions.data());

		WR_INFO("Available extensions:");
		for (const auto& extension : instanceExtensions)
		{
			WR_INFO("\t{}", extension.extensionName);
		}

		if constexpr (s_EnableValidationLayers)
		{
			result = Utils::CreateDebugUtilsMessengerEXT(m_Instance, &debugCreateInfo, m_Allocator, &m_DebugMessenger);
			VK_CHECK(result, "Failed to create Vulkan debug messenger!");
		}
	}

	void VulkanRenderer::createSurface()
	{
		VkResult result = glfwCreateWindowSurface(m_Instance, Application::get().getWindow(), m_Allocator, &m_Surface);
		VK_CHECK(result, "Failed to create Vulkan window surface!");
	}

	void VulkanRenderer::pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

		for (const auto& device : devices)
		{
			if (Utils::IsDeviceSuitable(device, m_Surface))
			{
				m_PhysicalDevice = device;
				break;
			}
		}

		WR_ASSERT(m_PhysicalDevice, "Failed to find a suitable GPU!");
	}

	void VulkanRenderer::createLogicalDevice()
	{
		QueueFamilyIndices indices = Utils::FindQueueFamilies(m_PhysicalDevice, m_Surface);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.GraphicsFamily, indices.PresentFamily };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo& queueCreateInfo = queueCreateInfos.emplace_back();
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.sampleRateShading = VK_TRUE;
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = (uint32_t)s_DeviceExtensions.size();
		createInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();

		if (s_EnableValidationLayers)
		{
			createInfo.enabledLayerCount = (uint32_t)s_ValidationLayers.size();
			createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		VkResult result = vkCreateDevice(m_PhysicalDevice, &createInfo, m_Allocator, &m_Device);
		VK_CHECK(result, "Failed to create Vulkan device!");

		vkGetDeviceQueue(m_Device, indices.GraphicsFamily, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, indices.PresentFamily, 0, &m_PresentQueue);
	}

	void VulkanRenderer::createCommandPool()
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VkResult result = vkCreateCommandPool(m_Device, &poolInfo, m_Allocator, &m_CommandPool);
		VK_CHECK(result, "Failed to create Vulkan command pool!");

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandBufferCount = WR_FRAMES_IN_FLIGHT;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		m_FrameCommandBuffers.resize(WR_FRAMES_IN_FLIGHT);

		result = vkAllocateCommandBuffers(m_Device, &allocInfo, m_FrameCommandBuffers.data());
		VK_CHECK(result, "Failed to allocate Vulkan command buffers!");

		constexpr uint32_t secondaryCommandBufferCount = 10;

		allocInfo.commandBufferCount = secondaryCommandBufferCount;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

		m_SecondaryCommandBufferPool.resize(WR_FRAMES_IN_FLIGHT);
		m_UsedSecondaryCommandBufferCount.resize(WR_FRAMES_IN_FLIGHT);
		m_SubmittedCommandLists.resize(WR_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
		{
			m_SecondaryCommandBufferPool[i].resize(secondaryCommandBufferCount);
			m_UsedSecondaryCommandBufferCount[i] = 0;

			result = vkAllocateCommandBuffers(m_Device, &allocInfo, m_SecondaryCommandBufferPool[i].data());
			VK_CHECK(result, "Failed to allocate Vulkan command buffers!");
		}
	}

	void VulkanRenderer::createDescriptorPool()
	{
		VkDescriptorPoolSize combinedSamplers{};
		combinedSamplers.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		combinedSamplers.descriptorCount = 1000;

		VkDescriptorPoolSize uniforms{};
		uniforms.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniforms.descriptorCount = 1000;

		VkDescriptorPoolSize sampledImages{};
		sampledImages.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		sampledImages.descriptorCount = 1000;

		VkDescriptorPoolSize samplers{};
		samplers.type = VK_DESCRIPTOR_TYPE_SAMPLER;
		samplers.descriptorCount = 1000;

		VkDescriptorPoolSize storage{};
		storage.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		storage.descriptorCount = 1000;

		std::array poolSizes = { combinedSamplers, uniforms, sampledImages, samplers, storage };

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 1000 * (uint32_t)poolSizes.size();

		VkResult result = vkCreateDescriptorPool(m_Device, &poolInfo, m_Allocator, &m_DescriptorPool);
		VK_CHECK(result, "Failed to create Vulkan descriptor pool!");
	}

	void VulkanRenderer::createSyncObject()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		m_ImageAvailableSemaphores.resize(WR_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(m_SwapchainImageCount);
		m_InFlightFences.resize(WR_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < m_SwapchainImageCount; i++)
		{
			VkResult result = vkCreateSemaphore(m_Device, &semaphoreInfo, m_Allocator, &m_RenderFinishedSemaphores[i]);
			VK_CHECK(result, "Failed to create Vulkan semaphore!");
		}

		for (size_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
		{
			VkResult result = vkCreateSemaphore(m_Device, &semaphoreInfo, m_Allocator, &m_ImageAvailableSemaphores[i]);
			VK_CHECK(result, "Failed to create Vulkan semaphore!");

			result = vkCreateFence(m_Device, &fenceInfo, m_Allocator, &m_InFlightFences[i]);
			VK_CHECK(result, "Failed to create Vulkan fence!");
		}
	}

	void VulkanRenderer::createSwapchain()
	{
		VkSwapchainKHR oldSwapchain = m_Swapchain;
		std::vector<VkImageView> oldImageViews = m_SwapchainImageViews;
		VkImage oldDepthImage = m_DepthImage;
		VkImageView oldDepthImageView = m_DepthImageView;
		VkDeviceMemory oldDepthImageMemory = m_DepthImageMemory;

		submitResourceFree(
			[oldSwapchain, oldImageViews, oldDepthImage, oldDepthImageView, oldDepthImageMemory](Renderer* renderer)
			{
				VulkanRenderer* vk = (VulkanRenderer*)renderer;
				
				if (oldSwapchain)
					vkDestroySwapchainKHR(vk->getDevice(), oldSwapchain, vk->getAllocator());
				if (oldDepthImageView)
					vkDestroyImageView(vk->getDevice(), oldDepthImageView, vk->getAllocator());
				if (oldDepthImage)
					vkDestroyImage(vk->getDevice(), oldDepthImage, vk->getAllocator());
				if (oldDepthImageMemory)
					vkFreeMemory(vk->getDevice(), oldDepthImageMemory, vk->getAllocator());

				for (VkImageView view : oldImageViews)
					vkDestroyImageView(vk->getDevice(), view, vk->getAllocator());
			}
		);

		m_SwapchainImages.clear();
		m_SwapchainImageViews.clear();
		m_DepthImage = nullptr;
		m_DepthImageMemory = nullptr;
		m_DepthImageView = nullptr;

		SwapchainSupportDetails swapchainSupport = Utils::QuerySwapchainSupport(m_PhysicalDevice, m_Surface);
		VkSurfaceFormatKHR surfaceFormat = Utils::ChooseSwapSurfaceFormat(swapchainSupport.Formats);
		VkPresentModeKHR presentMode = Utils::ChooseSwapPresentMode(swapchainSupport.PresentModes);
		VkExtent2D extent = Utils::ChooseSwapExtent(Application::get().getWindow(), swapchainSupport.Capabilities);

		m_SwapchainImageFormat = surfaceFormat.format;
		m_Extent = extent;

		m_SwapchainImageCount = swapchainSupport.Capabilities.minImageCount + 1;

		if (swapchainSupport.Capabilities.maxImageCount > 0 && m_SwapchainImageCount > swapchainSupport.Capabilities.maxImageCount)
			m_SwapchainImageCount = swapchainSupport.Capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;
		createInfo.minImageCount = m_SwapchainImageCount;
		createInfo.imageFormat = m_SwapchainImageFormat;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.oldSwapchain = oldSwapchain;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		QueueFamilyIndices indices = Utils::FindQueueFamilies(m_PhysicalDevice, m_Surface);
		uint32_t queueFamilyIndices[] = { indices.GraphicsFamily, indices.PresentFamily };

		if (indices.GraphicsFamily != indices.PresentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = swapchainSupport.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		VkResult result = vkCreateSwapchainKHR(m_Device, &createInfo, m_Allocator, &m_Swapchain);
		VK_CHECK(result, "Failed to create Vulkan swapchain!");

		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &m_SwapchainImageCount, nullptr);
		m_SwapchainImages.resize(m_SwapchainImageCount);
		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &m_SwapchainImageCount, m_SwapchainImages.data());

		m_SwapchainImageViews.resize(m_SwapchainImageCount);

		for (size_t i = 0; i < m_SwapchainImageCount; i++)
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_SwapchainImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_SwapchainImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			result = vkCreateImageView(m_Device, &viewInfo, m_Allocator, &m_SwapchainImageViews[i]);
			VK_CHECK(result, "Failed to create Vulkan image view!");
		}

		VkFormat depthFormat = Utils::FindDepthFormat(m_PhysicalDevice);

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = extent.width;
		imageInfo.extent.height = extent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = depthFormat;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		result = vkCreateImage(m_Device, &imageInfo, m_Allocator, &m_DepthImage);
		VK_CHECK(result, "Failed to create Vulkan image!");

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(m_Device, m_DepthImage, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = Utils::FindMemoryType(m_PhysicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = vkAllocateMemory(m_Device, &allocInfo, m_Allocator, &m_DepthImageMemory);
		VK_CHECK(result, "Failed to allocate Vulkan memory!");

		vkBindImageMemory(m_Device, m_DepthImage, m_DepthImageMemory, 0);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_DepthImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = depthFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		result = vkCreateImageView(m_Device, &viewInfo, m_Allocator, &m_DepthImageView);
		VK_CHECK(result, "Failed to create Vulkan image view!");
	}

	void VulkanRenderer::createRenderPass()
	{
		VkRenderPass oldRenderPass = m_RenderPass;
		std::vector<VkFramebuffer> oldFramebuffers = m_Framebuffers;

		submitResourceFree(
			[oldRenderPass, oldFramebuffers](Renderer* renderer)
			{
				VulkanRenderer* vk = (VulkanRenderer*)renderer;

				for (VkFramebuffer framebuffer : oldFramebuffers)
					vkDestroyFramebuffer(vk->getDevice(), framebuffer, vk->getAllocator());
				if (oldRenderPass)
					vkDestroyRenderPass(vk->getDevice(), oldRenderPass, vk->getAllocator());
			}
		);

		m_RenderPass = nullptr;
		m_Framebuffers.clear();

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_SwapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = Utils::FindDepthFormat(m_PhysicalDevice);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array attachments = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkResult result = vkCreateRenderPass(m_Device, &renderPassInfo, m_Allocator, &m_RenderPass);
		VK_CHECK(result, "Failed to create Vulkan render pass!");

		m_Framebuffers.resize(m_SwapchainImageCount);

		for (size_t i = 0; i < m_SwapchainImageCount; i++)
		{
			std::array attachmentViews = {
				m_SwapchainImageViews[i],
				m_DepthImageView
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = (uint32_t)attachmentViews.size();
			framebufferInfo.pAttachments = attachmentViews.data();
			framebufferInfo.width = m_Extent.width;
			framebufferInfo.height = m_Extent.height;
			framebufferInfo.layers = 1;

			result = vkCreateFramebuffer(m_Device, &framebufferInfo, m_Allocator, &m_Framebuffers[i]);
			VK_CHECK(result, "Failed to create Vulkan framebuffer!");
		}
	}

}

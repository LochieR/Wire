#include "wrpch.h"

#include <vulkan/vulkan.h>
#include "VulkanRenderer.h"

#include "VulkanShader.h"
#include "VulkanBuffer.h"
#include "VulkanTexture2D.h"
#include "VulkanFramebuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanImGuiLayer.h"

#include <glfw/glfw3.h>

#include <set>

namespace Wire {

	constexpr bool g_ValidationLayers =
#ifdef WR_DEBUG
		true;
#else
		false;
#endif

	static VulkanData s_VkD;

	const static std::vector<const char*> s_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	namespace Utils {

		static std::vector<const char*> GetRequiredExtensions()
		{
			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

			std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

			if (g_ValidationLayers)
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
			else
				std::cerr << "\033[38;5;208m[Validation Layer] " << pCallbackData->pMessage << "\u001b[0m" << std::endl;

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

		static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
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
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, s_VkD.Surface, &presentSupport);

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

		static SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device)
		{
			SwapchainSupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, s_VkD.Surface, &details.Capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, s_VkD.Surface, &formatCount, nullptr);

			if (formatCount != 0)
			{
				details.Formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, s_VkD.Surface, &formatCount, details.Formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, s_VkD.Surface, &presentModeCount, nullptr);

			if (presentModeCount != 0)
			{
				details.PresentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, s_VkD.Surface, &presentModeCount, details.PresentModes.data());
			}

			return details;
		}

		static bool IsDeviceSuitable(VkPhysicalDevice device)
		{
			QueueFamilyIndices indices = FindQueueFamilies(device);

			bool extensionsSupported = CheckDeviceExtensionSupport(device);

			bool swapchainAdequate = false;
			if (extensionsSupported)
			{
				SwapchainSupportDetails swapChainSupport = QuerySwapchainSupport(device);
				swapchainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
			}

			VkPhysicalDeviceFeatures supportedFeatures;
			vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

			return 
				indices.IsComplete() && 
				extensionsSupported && 
				swapchainAdequate && 
				supportedFeatures.samplerAnisotropy && 
				supportedFeatures.wideLines && 
				supportedFeatures.fragmentStoresAndAtomics &&
				supportedFeatures.independentBlend;
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

		static VkExtent2D ChooseSwapExtent(Window& window, const VkSurfaceCapabilitiesKHR& capabilities)
		{
			if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			{
				return capabilities.currentExtent;
			}
			else
			{
				int width, height;
				glfwGetFramebufferSize((GLFWwindow*)window.GetNativeHandle(), &width, &height);

				VkExtent2D actualExtent = {
					static_cast<uint32_t>(width),
					static_cast<uint32_t>(height)
				};

				actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

				return actualExtent;
			}
		}

		static VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDevice device)
		{
			VkPhysicalDeviceProperties physicalDeviceProperties;
			vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);

			VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

			if (counts & VK_SAMPLE_COUNT_64_BIT)
				return VK_SAMPLE_COUNT_64_BIT;
			if (counts & VK_SAMPLE_COUNT_32_BIT)
				return VK_SAMPLE_COUNT_32_BIT;
			if (counts & VK_SAMPLE_COUNT_16_BIT)
				return VK_SAMPLE_COUNT_16_BIT;
			if (counts & VK_SAMPLE_COUNT_8_BIT)
				return VK_SAMPLE_COUNT_8_BIT;
			if (counts & VK_SAMPLE_COUNT_4_BIT)
				return VK_SAMPLE_COUNT_4_BIT;
			if (counts & VK_SAMPLE_COUNT_2_BIT)
				return VK_SAMPLE_COUNT_2_BIT;

			return VK_SAMPLE_COUNT_1_BIT;
		}

	}

	VulkanRenderer::VulkanRenderer(Window& window)
		: m_Window(window)
	{
		WR_TAG("Renderer");

		s_VkD = {};

		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapchain();
		CreateRenderPass();
		CreateColorResources();
		CreateFramebuffers();
		CreateCommandPool();
		CreateDescriptorPool();
		CreateSyncObjects();

		m_Renderer2D = Renderer2D(this);
	}

	VulkanRenderer::~VulkanRenderer()
	{
		vkDeviceWaitIdle(s_VkD.Device);

		m_Renderer2D.Release();

		for (auto it = s_VkD.Resources.rbegin(); it != s_VkD.Resources.rend(); ++it)
		{
			delete *it;
		}
		s_VkD.Resources.clear();

		for (auto& queue : s_VkD.ResourceFreeQueue)
		{
			for (auto& func : queue)
				func(this);
			queue.clear();
		}

		for (size_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyFence(s_VkD.Device, s_VkD.InFlightFences[i], s_VkD.Allocator);
			vkDestroySemaphore(s_VkD.Device, s_VkD.RenderFinishedSemaphores[i], s_VkD.Allocator);
			vkDestroySemaphore(s_VkD.Device, s_VkD.ImageAvailableSemaphores[i], s_VkD.Allocator);
		}

		vkDestroyDescriptorPool(s_VkD.Device, s_VkD.DescriptorPool, s_VkD.Allocator);
		vkDestroyCommandPool(s_VkD.Device, s_VkD.CommandPool, s_VkD.Allocator);

		vkDestroyRenderPass(s_VkD.Device, s_VkD.RenderPass, s_VkD.Allocator);
		
		CleanupSwapchain();

		vkDestroySurfaceKHR(s_VkD.Instance, s_VkD.Surface, s_VkD.Allocator);
		vkDestroyDevice(s_VkD.Device, s_VkD.Allocator);

		if (g_ValidationLayers)
			Utils::DestroyDebugUtilsMessengerEXT(s_VkD.Instance, s_VkD.DebugMessenger, s_VkD.Allocator);

		vkDestroyInstance(s_VkD.Instance, s_VkD.Allocator);

		// To avoid any confusing errors after shutdown
		memset(&s_VkD, 0, sizeof(VulkanData));
	}

	void VulkanRenderer::Release()
	{
		delete this;
	}

	bool VulkanRenderer::BeginFrame()
	{
		if (!s_VkD.FrameFences[s_VkD.CurrentFrame].empty())
			vkWaitForFences(s_VkD.Device, (uint32_t)s_VkD.FrameFences[s_VkD.CurrentFrame].size(), s_VkD.FrameFences[s_VkD.CurrentFrame].data(), VK_TRUE, std::numeric_limits<uint64_t>::max());

		VkResult result = vkAcquireNextImageKHR(s_VkD.Device, s_VkD.Swapchain, std::numeric_limits<uint64_t>::max(), s_VkD.ImageAvailableSemaphores[s_VkD.CurrentFrame], nullptr, &s_VkD.ImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window.WasWindowResized())
		{
			m_Window.ResetResizedFlag();
			RecreateSwapchain();
			return false;
		}
		VK_CHECK(result, "Failed to acquire Vulkan swapchain image!");

		vkResetFences(s_VkD.Device, 1, &s_VkD.InFlightFences[s_VkD.CurrentFrame]);
		if (!s_VkD.FrameFences[s_VkD.CurrentFrame].empty())
			vkResetFences(s_VkD.Device, (uint32_t)s_VkD.FrameFences[s_VkD.CurrentFrame].size(), s_VkD.FrameFences[s_VkD.CurrentFrame].data());

		s_VkD.FrameRenderFinishedSemaphores[s_VkD.CurrentFrame].clear();
		s_VkD.ImageAcquiredSemaphoreUsed[s_VkD.CurrentFrame] = false;
		s_VkD.FrameFences[s_VkD.CurrentFrame].clear();

		return true;
	}

	void VulkanRenderer::EndFrame()
	{
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = (uint32_t)s_VkD.FrameRenderFinishedSemaphores[s_VkD.CurrentFrame].size();
		presentInfo.pWaitSemaphores = s_VkD.FrameRenderFinishedSemaphores[s_VkD.CurrentFrame].data();
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &s_VkD.Swapchain;
		presentInfo.pImageIndices = &s_VkD.ImageIndex;

		VkResult result = vkQueuePresentKHR(s_VkD.PresentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			RecreateSwapchain();
		else
			VK_CHECK(result, "Failed to present Vulkan queue!");

		for (auto& func : s_VkD.ResourceFreeQueue[s_VkD.CurrentFrame])
		{
			func(this);
		}
		s_VkD.ResourceFreeQueue[s_VkD.CurrentFrame].clear();

		s_VkD.CurrentFrame = (s_VkD.CurrentFrame + 1) % WR_FRAMES_IN_FLIGHT;
	}

	uint32_t VulkanRenderer::GetFrameIndex() const
	{
		return s_VkD.CurrentFrame;
	}

	void VulkanRenderer::Draw(rbRef<CommandBuffer> commandBuffer, uint32_t vertexCount)
	{
		vkCmdDraw(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer, vertexCount, 1, 0, 0);
	}

	void VulkanRenderer::DrawIndexed(rbRef<CommandBuffer> commandBuffer, uint32_t indexCount)
	{
		vkCmdDrawIndexed(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer, indexCount, 1, 0, 0, 0);
	}

	VulkanData& VulkanRenderer::GetVulkanData() const
	{
		return s_VkD;
	}

	void VulkanRenderer::SubmitResourceFree(std::function<void(VulkanRenderer*)>&& func)
	{
		s_VkD.ResourceFreeQueue[s_VkD.CurrentFrame].emplace_back(func);
	}

	rbRef<Shader> VulkanRenderer::CreateShader(std::string_view filepath)
	{
		rbRef<Shader> ref = new VulkanShader(this, filepath);
		s_VkD.Resources.push_back(ref.GetResource());

		return ref;
	}

	rbRef<VertexBuffer> VulkanRenderer::CreateVertexBuffer(size_t size)
	{
		rbRef<VertexBuffer> ref = new VulkanVertexBuffer(this, size);
		s_VkD.Resources.push_back(ref.GetResource());

		return ref;
	}

	rbRef<IndexBuffer> VulkanRenderer::CreateIndexBuffer(uint32_t* indices, uint32_t indexCount)
	{
		rbRef<IndexBuffer> ref = new VulkanIndexBuffer(this, indices, indexCount);
		s_VkD.Resources.push_back(ref.GetResource());

		return ref;
	}

	rbRef<StorageBuffer> VulkanRenderer::CreateStorageBuffer(size_t size)
	{
		rbRef<StorageBuffer> ref = new VulkanStorageBuffer(this, size);
		s_VkD.Resources.push_back(ref.GetResource());

		return ref;
	}

	rbRef<StagingBuffer> VulkanRenderer::CreateStagingBuffer(size_t size)
	{
		rbRef<StagingBuffer> ref = new VulkanStagingBuffer(this, size);
		s_VkD.Resources.push_back(ref.GetResource());

		return ref;
	}

	rbRef<Texture2D> VulkanRenderer::CreateTexture2D(std::string_view path)
	{
		rbRef<Texture2D> ref = new VulkanTexture2D(this, path);
		s_VkD.Resources.push_back(ref.GetResource());

		return ref;
	}

	rbRef<Texture2D> VulkanRenderer::CreateTexture2D(uint32_t* data, uint32_t width, uint32_t height)
	{
		rbRef<Texture2D> ref = new VulkanTexture2D(this, data, width, height);
		s_VkD.Resources.push_back(ref.GetResource());

		return ref;
	}

	rbRef<Font> VulkanRenderer::CreateFont(std::string_view path, uint32_t minChar, uint32_t maxChar)
	{
		rbRef<Font> ref = new Font(this, path, minChar, maxChar);
		s_VkD.Resources.push_back(ref.GetResource());

		return ref;
	}

	rbRef<Framebuffer> VulkanRenderer::CreateFramebuffer(const FramebufferSpecification& spec)
	{
		rbRef<Framebuffer> ref = new VulkanFramebuffer(this, spec);
		s_VkD.Resources.push_back(ref.GetResource());

		return ref;
	}

	ImGuiLayer* VulkanRenderer::CreateImGuiLayer()
	{
		return new VulkanImGuiLayer(this);
	}

	rbRef<CommandBuffer> VulkanRenderer::AllocateCommandBuffer()
	{
		rbRef<CommandBuffer> ref = new VulkanCommandBuffer(this);
		s_VkD.Resources.push_back(ref.GetResource());

		return ref;
	}

	rbRef<CommandBuffer> VulkanRenderer::BeginSingleTimeCommands()
	{
		rbRef<CommandBuffer> commandBuffer = AllocateCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkResult result = vkBeginCommandBuffer(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer, &beginInfo);
		VK_CHECK(result, "Failed to begin Vulkan command buffer!");

		return commandBuffer;
	}

	void VulkanRenderer::EndSingleTimeCommands(rbRef<CommandBuffer> commandBuffer)
	{
		commandBuffer->End();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer;

		VkResult result = vkQueueSubmit(s_VkD.GraphicsQueue, 1, &submitInfo, nullptr);
		VK_CHECK(result, "Failed to submit Vulkan command buffer (single time commands)!");

		vkQueueWaitIdle(s_VkD.GraphicsQueue);

		vkFreeCommandBuffers(s_VkD.Device, s_VkD.CommandPool, 1, &((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer);

		((Renderer*)this)->Free(commandBuffer);
	}

	void VulkanRenderer::BeginCommandBufferAndRenderPass(rbRef<CommandBuffer> commandBuffer)
	{
		commandBuffer->Begin();

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = s_VkD.RenderPass;
		renderPassInfo.framebuffer = s_VkD.SwapchainFramebuffers[s_VkD.ImageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = s_VkD.SwapchainExtent;

		VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		s_VkD.CurrentSubpass = 0;

		vkCmdBeginRenderPass(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VulkanRenderer::EndCommandBufferAndRenderPass(rbRef<CommandBuffer> commandBuffer)
	{
		vkCmdEndRenderPass(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer);

		VkResult result = vkEndCommandBuffer(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer);
		VK_CHECK(result, "Failed to end Vulkan command buffer!");
	}

	void VulkanRenderer::BeginRenderPass(rbRef<CommandBuffer> commandBuffer)
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = s_VkD.RenderPass;
		renderPassInfo.framebuffer = s_VkD.SwapchainFramebuffers[s_VkD.ImageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = s_VkD.SwapchainExtent;

		VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		s_VkD.CurrentSubpass = 0;

		vkCmdBeginRenderPass(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VulkanRenderer::EndRenderPass(rbRef<CommandBuffer> commandBuffer)
	{
		vkCmdEndRenderPass(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer);
	}

	void VulkanRenderer::SubmitCommandBuffer(rbRef<CommandBuffer> commandBuffer)
	{
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = s_VkD.ImageAcquiredSemaphoreUsed[s_VkD.CurrentFrame] ? s_VkD.FrameRenderFinishedSemaphores[s_VkD.CurrentFrame].data() : &s_VkD.ImageAvailableSemaphores[s_VkD.CurrentFrame];
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &s_VkD.RenderFinishedSemaphores[s_VkD.CurrentFrame];

		if (!s_VkD.ImageAcquiredSemaphoreUsed[s_VkD.CurrentFrame])
			s_VkD.ImageAcquiredSemaphoreUsed[s_VkD.CurrentFrame] = true;

		s_VkD.FrameRenderFinishedSemaphores[s_VkD.CurrentFrame].clear();

		VkResult result = vkQueueSubmit(s_VkD.GraphicsQueue, 1, &submitInfo, s_VkD.InFlightFences[s_VkD.CurrentFrame]);
		VK_CHECK(result, "Failed to submit Vulkan command buffer!");

		s_VkD.FrameRenderFinishedSemaphores[s_VkD.CurrentFrame].push_back(s_VkD.RenderFinishedSemaphores[s_VkD.CurrentFrame]);
	}

	void VulkanRenderer::SubmitCommandBuffer(rbRef<CommandBuffer> commandBuffer, VkFence fence, VkSemaphore semaphore)
	{
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = s_VkD.ImageAcquiredSemaphoreUsed[s_VkD.CurrentFrame] ? s_VkD.FrameRenderFinishedSemaphores[s_VkD.CurrentFrame].data() : &s_VkD.ImageAvailableSemaphores[s_VkD.CurrentFrame];
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &semaphore;

		if (!s_VkD.ImageAcquiredSemaphoreUsed[s_VkD.CurrentFrame])
			s_VkD.ImageAcquiredSemaphoreUsed[s_VkD.CurrentFrame] = true;

		s_VkD.FrameRenderFinishedSemaphores[s_VkD.CurrentFrame].clear();

		VkResult result = vkQueueSubmit(s_VkD.GraphicsQueue, 1, &submitInfo, fence);
		VK_CHECK(result, "Failed to submit Vulkan command buffer!");

		s_VkD.FrameRenderFinishedSemaphores[s_VkD.CurrentFrame].push_back(semaphore);
		s_VkD.FrameFences[s_VkD.CurrentFrame].push_back(fence);
	}

	void VulkanRenderer::NextSubpass(rbRef<CommandBuffer> commandBuffer)
	{
		if (s_VkD.CurrentSubpass < s_VkD.SubpassCount - 1)
		{
			vkCmdNextSubpass(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer, VK_SUBPASS_CONTENTS_INLINE);
			s_VkD.CurrentSubpass++;
		}
	}

	void VulkanRenderer::CopyBuffer(rbRef<CommandBuffer> commandBuffer, rbRef<StagingBuffer> srcBuffer, rbRef<StorageBuffer> dstBuffer)
	{
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = dstBuffer->GetSize();

		vkCmdCopyBuffer(
			((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer,
			((VulkanStagingBuffer*)srcBuffer.Get())->m_Buffer,
			((VulkanStorageBuffer*)dstBuffer.Get())->m_StorageBuffer,
			1, &copyRegion
		);
	}

	void VulkanRenderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult result = vkCreateBuffer(s_VkD.Device, &bufferInfo, s_VkD.Allocator, &buffer);
		VK_CHECK(result, "Failed to create Vulkan buffer!");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(s_VkD.Device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		result = vkAllocateMemory(s_VkD.Device, &allocInfo, s_VkD.Allocator, &bufferMemory);
		VK_CHECK(result, "Failed to allocate Vulkan memory!");

		vkBindBufferMemory(s_VkD.Device, buffer, bufferMemory, 0);
	}

	void VulkanRenderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		rbRef<CommandBuffer> commandBuffer = BeginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		EndSingleTimeCommands(commandBuffer);
	}

	void VulkanRenderer::CreateImage(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = numSamples;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult result = vkCreateImage(s_VkD.Device, &imageInfo, s_VkD.Allocator, &image);
		VK_CHECK(result, "Failed to create Vulkan image!");

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(s_VkD.Device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		result = vkAllocateMemory(s_VkD.Device, &allocInfo, s_VkD.Allocator, &imageMemory);
		VK_CHECK(result, "Failed to allocate Vulkan memory!");

		vkBindImageMemory(s_VkD.Device, image, imageMemory, 0);
	}

	VkImageView VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		VkResult result = vkCreateImageView(s_VkD.Device, &viewInfo, s_VkD.Allocator, &imageView);
		VK_CHECK(result, "Failed to create Vulkan image view!");

		return imageView;
	}

	void VulkanRenderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		rbRef<CommandBuffer> commandBuffer = BeginSingleTimeCommands();

		VkCommandBuffer cmd = ((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer;

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		
		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (HasStencilComponent(format))
			{
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			WR_ASSERT(false && "Unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			cmd,
			sourceStage,
			destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
			
		EndSingleTimeCommands(commandBuffer);
	}

	void VulkanRenderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		rbRef<CommandBuffer> commandBuffer = BeginSingleTimeCommands();

		VkCommandBuffer cmd = ((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer;

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(
			cmd,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		EndSingleTimeCommands(commandBuffer);
	}

	uint32_t VulkanRenderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(s_VkD.PhysicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		WR_ASSERT(false && "Failed to find suitable memory type!");
		return 0;
	}

	VkFormat VulkanRenderer::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(s_VkD.PhysicalDevice, format, &props);
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}

		WR_ASSERT(false && "Failed to find supported format!");
		return candidates[0];
	}

	VkFormat VulkanRenderer::FindDepthFormat()
	{
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool VulkanRenderer::HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	QueueFamilyIndices VulkanRenderer::FindQueueFamilies()
	{
		return Utils::FindQueueFamilies(s_VkD.PhysicalDevice);
	}

	void VulkanRenderer::Free(rbIRef* ref)
	{
		IResource* resource = ref->GetResource();
		
		auto it = std::find(s_VkD.Resources.begin(), s_VkD.Resources.end(), resource);
		WR_ASSERT(it != s_VkD.Resources.end());

		s_VkD.Resources.erase(it);

		delete resource;
	}

	void VulkanRenderer::CreateInstance()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Wire";
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		
		std::vector<const char*> extensions = Utils::GetRequiredExtensions();

		createInfo.enabledExtensionCount = (uint32_t)extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (g_ValidationLayers)
		{
			WR_ASSERT(CheckValidationLayerSupport() && "Validation layers enabled, but are not supported!");

			createInfo.enabledLayerCount = (uint32_t)s_ValidationLayers.size();
			createInfo.ppEnabledLayerNames = s_ValidationLayers.data();

			Utils::PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = &debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		VkResult result = vkCreateInstance(&createInfo, s_VkD.Allocator, &s_VkD.Instance);
		VK_CHECK(result, "Failed to create Vulkan instance!");

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> instanceExtensions(extensionCount);

		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instanceExtensions.data());

		WR_INFO("Available extensions:");
		for (const auto& extension : instanceExtensions)
		{
			WR_INFO("\t", extension.extensionName);
		}
	}

	void VulkanRenderer::SetupDebugMessenger()
	{
		if (!g_ValidationLayers)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		Utils::PopulateDebugMessengerCreateInfo(createInfo);

		VkResult result = Utils::CreateDebugUtilsMessengerEXT(s_VkD.Instance, &createInfo, s_VkD.Allocator, &s_VkD.DebugMessenger);
		VK_CHECK(result, "Failed to create Vulkan debug messenger!");
	}

	void VulkanRenderer::PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(s_VkD.Instance, &deviceCount, nullptr);

		WR_ASSERT(deviceCount && "Failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(s_VkD.Instance, &deviceCount, devices.data());

		for (const auto& device : devices)
		{
			if (Utils::IsDeviceSuitable(device))
			{
				s_VkD.PhysicalDevice = device;
				s_VkD.MultisampleCount = Utils::GetMaxUsableSampleCount(device);
				break;
			}
		}

		WR_ASSERT(s_VkD.PhysicalDevice && "Failed to find a suitable GPU!");
	}

	void VulkanRenderer::CreateLogicalDevice()
	{
		QueueFamilyIndices indices = Utils::FindQueueFamilies(s_VkD.PhysicalDevice);

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
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.wideLines = VK_TRUE;
		deviceFeatures.sampleRateShading = VK_TRUE;
		deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
		deviceFeatures.independentBlend = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = (uint32_t)s_DeviceExtensions.size();
		createInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();

		if (g_ValidationLayers)
		{
			createInfo.enabledLayerCount = (uint32_t)s_ValidationLayers.size();
			createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		VkResult result = vkCreateDevice(s_VkD.PhysicalDevice, &createInfo, s_VkD.Allocator, &s_VkD.Device);
		VK_CHECK(result, "Failed to create Vulkan device!");
		
		vkGetDeviceQueue(s_VkD.Device, indices.GraphicsFamily, 0, &s_VkD.GraphicsQueue);
		vkGetDeviceQueue(s_VkD.Device, indices.PresentFamily, 0, &s_VkD.PresentQueue);
	}

	void VulkanRenderer::CreateSurface()
	{
		VkResult result = glfwCreateWindowSurface(s_VkD.Instance, (GLFWwindow*)m_Window.GetNativeHandle(), s_VkD.Allocator, &s_VkD.Surface);
		VK_CHECK(result, "Failed to create Vulkan surface!");
	}

	void VulkanRenderer::CreateSwapchain()
	{
		SwapchainSupportDetails swapchainSupport = Utils::QuerySwapchainSupport(s_VkD.PhysicalDevice);

		VkSurfaceFormatKHR surfaceFormat = Utils::ChooseSwapSurfaceFormat(swapchainSupport.Formats);
		VkPresentModeKHR presentMode = Utils::ChooseSwapPresentMode(swapchainSupport.PresentModes);
		VkExtent2D extent = Utils::ChooseSwapExtent(m_Window, swapchainSupport.Capabilities);

		uint32_t imageCount = swapchainSupport.Capabilities.minImageCount + 1;

		if (swapchainSupport.Capabilities.maxImageCount > 0 && imageCount > swapchainSupport.Capabilities.maxImageCount)
		{
			imageCount = swapchainSupport.Capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = s_VkD.Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = Utils::FindQueueFamilies(s_VkD.PhysicalDevice);
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
		createInfo.oldSwapchain = nullptr;

		VkResult result = vkCreateSwapchainKHR(s_VkD.Device, &createInfo, s_VkD.Allocator, &s_VkD.Swapchain);
		VK_CHECK(result, "Failed to create Vulkan swapchain!");

		vkGetSwapchainImagesKHR(s_VkD.Device, s_VkD.Swapchain, &imageCount, nullptr);
		s_VkD.SwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(s_VkD.Device, s_VkD.Swapchain, &imageCount, s_VkD.SwapchainImages.data());

		s_VkD.SwapchainImageFormat = surfaceFormat.format;
		s_VkD.SwapchainExtent = extent;

		s_VkD.SwapchainImageViews.resize(s_VkD.SwapchainImages.size());

		for (size_t i = 0; i < s_VkD.SwapchainImages.size(); i++)
		{
			s_VkD.SwapchainImageViews[i] = CreateImageView(s_VkD.SwapchainImages[i], s_VkD.SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	void VulkanRenderer::CreateRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = s_VkD.SwapchainImageFormat;
		colorAttachment.samples = s_VkD.MultisampleCount;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = s_VkD.SwapchainImageFormat;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentResolveRef{};
		colorAttachmentResolveRef.attachment = 1;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, colorAttachmentResolve };
		std::array<VkSubpassDescription, 1> subpasses = { subpass };
		std::array<VkSubpassDependency, 1> dependencies = { dependency };

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = (uint32_t)attachments.size();
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = (uint32_t)subpasses.size();
		renderPassInfo.pSubpasses = subpasses.data();
		renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
		renderPassInfo.pDependencies = dependencies.data();

		s_VkD.SubpassCount = renderPassInfo.subpassCount;

		VkResult result = vkCreateRenderPass(s_VkD.Device, &renderPassInfo, s_VkD.Allocator, &s_VkD.RenderPass);
		VK_CHECK(result, "Failed to create Vulkan render pass!");
	}

	void VulkanRenderer::CreateFramebuffers()
	{
		s_VkD.SwapchainFramebuffers.resize(s_VkD.SwapchainImageViews.size());

		for (size_t i = 0; i < s_VkD.SwapchainImageViews.size(); i++)
		{
			std::array<VkImageView, 2> attachments = {
				s_VkD.ColorImageView,
				s_VkD.SwapchainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = s_VkD.RenderPass;
			framebufferInfo.attachmentCount = (uint32_t)attachments.size();
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = s_VkD.SwapchainExtent.width;
			framebufferInfo.height = s_VkD.SwapchainExtent.height;
			framebufferInfo.layers = 1;

			VkResult result = vkCreateFramebuffer(s_VkD.Device, &framebufferInfo, s_VkD.Allocator, &s_VkD.SwapchainFramebuffers[i]);
			VK_CHECK(result, "Failed to create Vulkan framebuffer!");
		}
	}

	void VulkanRenderer::CreateCommandPool()
	{
		QueueFamilyIndices indices = Utils::FindQueueFamilies(s_VkD.PhysicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = indices.GraphicsFamily;

		VkResult result = vkCreateCommandPool(s_VkD.Device, &poolInfo, s_VkD.Allocator, &s_VkD.CommandPool);
		VK_CHECK(result, "Failed to create Vulkan command pool!");
	}

	void VulkanRenderer::CreateDescriptorPool()
	{
		VkDescriptorPoolSize samplers{};
		samplers.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplers.descriptorCount = 1000;

		VkDescriptorPoolSize uniforms{};
		uniforms.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniforms.descriptorCount = 1000;
		
		VkDescriptorPoolSize storage{};
		storage.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		storage.descriptorCount = 1000;

		std::array poolSizes = { samplers, uniforms, storage };

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 1000 * (uint32_t)poolSizes.size();

		VkResult result = vkCreateDescriptorPool(s_VkD.Device, &poolInfo, s_VkD.Allocator, &s_VkD.DescriptorPool);
		VK_CHECK(result, "Failed to create Vulkan descriptor pool!");
	}

	void VulkanRenderer::CreateColorResources()
	{
		VkFormat colorFormat = s_VkD.SwapchainImageFormat;

		CreateImage(
			s_VkD.SwapchainExtent.width,
			s_VkD.SwapchainExtent.height,
			s_VkD.MultisampleCount,
			colorFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			s_VkD.ColorImage,
			s_VkD.ColorImageMemory
		);
		s_VkD.ColorImageView = CreateImageView(s_VkD.ColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void VulkanRenderer::CreateSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkResult result;
		
		for (size_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
		{
			result = vkCreateSemaphore(s_VkD.Device, &semaphoreInfo, s_VkD.Allocator, &s_VkD.ImageAvailableSemaphores[i]);
			VK_CHECK(result, "Failed to create Vulkan semaphore!");
			result = vkCreateSemaphore(s_VkD.Device, &semaphoreInfo, s_VkD.Allocator, &s_VkD.RenderFinishedSemaphores[i]);
			VK_CHECK(result, "Failed to create Vulkan semaphore!");
			result = vkCreateFence(s_VkD.Device, &fenceInfo, s_VkD.Allocator, &s_VkD.InFlightFences[i]);
			VK_CHECK(result, "Failed to create Vulkan fence!");
		}
	}

	bool VulkanRenderer::CheckValidationLayerSupport()
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

	void VulkanRenderer::CleanupSwapchain()
	{
		vkDestroyImageView(s_VkD.Device, s_VkD.ColorImageView, s_VkD.Allocator);
		vkDestroyImage(s_VkD.Device, s_VkD.ColorImage, s_VkD.Allocator);
		vkFreeMemory(s_VkD.Device, s_VkD.ColorImageMemory, s_VkD.Allocator);

		for (VkFramebuffer framebuffer : s_VkD.SwapchainFramebuffers)
			vkDestroyFramebuffer(s_VkD.Device, framebuffer, s_VkD.Allocator);

		for (VkImageView imageView : s_VkD.SwapchainImageViews)
			vkDestroyImageView(s_VkD.Device, imageView, s_VkD.Allocator);

		vkDestroySwapchainKHR(s_VkD.Device, s_VkD.Swapchain, s_VkD.Allocator);
	}

	void VulkanRenderer::RecreateSwapchain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize((GLFWwindow*)m_Window.GetNativeHandle(), &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize((GLFWwindow*)m_Window.GetNativeHandle(), &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(s_VkD.Device);

		CleanupSwapchain();

		CreateSwapchain();
		CreateColorResources();
		CreateFramebuffers();
	}

}

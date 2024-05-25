#include "wrpch.h"

#include <vulkan/vulkan.h>
#include "VulkanImGuiLayer.h"
#include "VulkanCommandBuffer.h"

#include "Wire/Core/Application.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <glfw/glfw3.h>

namespace Wire {

	static void CheckVkResult(VkResult result)
	{
		VK_CHECK(result, "ImGui Vulkan operation failed!");
	}

	VulkanImGuiLayer::VulkanImGuiLayer(VulkanRenderer* renderer)
		: m_Renderer(renderer)
	{
	}

	VulkanImGuiLayer::~VulkanImGuiLayer()
	{
	}

	void VulkanImGuiLayer::OnAttach()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		
		m_FontLibrary = new FontLibrary("Resources/fonts/opensans/OpenSans-Regular.ttf", 18.0f);

		ImGui::StyleColorsDark();
		
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ConfigureStyle();

		Application& app = Application::Get();
		GLFWwindow* window = (GLFWwindow*)app.GetWindow().GetNativeHandle();

		auto& vkd = m_Renderer->GetVulkanData();

		CreateVulkanObjects();

		ImGui_ImplVulkan_InitInfo initInfo{};
		initInfo.Instance = vkd.Instance;
		initInfo.PhysicalDevice = vkd.PhysicalDevice;
		initInfo.Device = vkd.Device;
		initInfo.QueueFamily = m_Renderer->FindQueueFamilies().GraphicsFamily;
		initInfo.Queue = vkd.GraphicsQueue;
		initInfo.PipelineCache = nullptr;
		initInfo.DescriptorPool = m_DescriptorPool;
		initInfo.Subpass = 0;
		initInfo.MinImageCount = (uint32_t)vkd.SwapchainImages.size();
		initInfo.ImageCount = (uint32_t)vkd.SwapchainImages.size();
		initInfo.MSAASamples = vkd.MultisampleCount;
		initInfo.Allocator = vkd.Allocator;
		initInfo.CheckVkResultFn = &CheckVkResult;

		for (int i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
		{
			m_CommandBuffers[i] = m_Renderer->AllocateCommandBuffer();
		}

		ImGui_ImplGlfw_InitForVulkan(window, true);
		ImGui_ImplVulkan_Init(&initInfo, vkd.RenderPass);
	}

	void VulkanImGuiLayer::OnDetach()
	{
		delete m_FontLibrary;

		m_Renderer->SubmitResourceFree([pool = m_DescriptorPool, fences = m_InFlightFences, semaphores = m_RenderFinishedSemaphores](VulkanRenderer* renderer)
		{
			auto& vkd = renderer->GetVulkanData();

			for (int i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
			{
				vkDestroySemaphore(vkd.Device, semaphores[i], vkd.Allocator);
				vkDestroyFence(vkd.Device, fences[i], vkd.Allocator);
			}

			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::Shutdown();

			vkDestroyDescriptorPool(vkd.Device, pool, vkd.Allocator);
		});
	}

	void VulkanImGuiLayer::Begin()
	{
		auto& vkd = m_Renderer->GetVulkanData();

		//vkWaitForFences(vkd.Device, 1, &m_InFlightFences[vkd.CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
		//vkResetFences(vkd.Device, 1, &m_InFlightFences[vkd.CurrentFrame]);

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void VulkanImGuiLayer::End()
	{
		auto& vkd = m_Renderer->GetVulkanData();

		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = { (float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight() };

		ImGui::Render();

		rbRef<CommandBuffer> commandBuffer = m_CommandBuffers[vkd.CurrentFrame];

		m_Renderer->BeginCommandBufferAndRenderPass(commandBuffer);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), ((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer);

		m_Renderer->EndCommandBufferAndRenderPass(commandBuffer);
		m_Renderer->SubmitCommandBuffer(commandBuffer, m_InFlightFences[vkd.CurrentFrame], m_RenderFinishedSemaphores[vkd.CurrentFrame]);
	}

	void VulkanImGuiLayer::UpdateViewports()
	{
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void VulkanImGuiLayer::ConfigureStyle()
	{
	}

	void VulkanImGuiLayer::CreateVulkanObjects()
	{
		auto& vkd = m_Renderer->GetVulkanData();

		VkDescriptorPoolSize poolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }
		};

		VkDescriptorPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		createInfo.maxSets = 100;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = poolSizes;

		VkResult result = vkCreateDescriptorPool(vkd.Device, &createInfo, vkd.Allocator, &m_DescriptorPool);
		VK_CHECK(result, "Failed to create Vulkan descriptor pool!");

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = 0;

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (int i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
		{
			result = vkCreateFence(vkd.Device, &fenceInfo, vkd.Allocator, &m_InFlightFences[i]);
			VK_CHECK(result, "Failed to create Vulkan fence!");

			result = vkCreateSemaphore(vkd.Device, &semaphoreInfo, vkd.Allocator, &m_RenderFinishedSemaphores[i]);
			VK_CHECK(result, "Failed to create Vulkan semaphore!");
		}
	}

}

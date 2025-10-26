#include "ImGuiLayer.h"

#include "Wire/Core/Application.h"
#include "Wire/Renderer/Vulkan/VulkanDevice.h"
#include "Wire/Renderer/Vulkan/VulkanRenderPass.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace wire {

    void ImGuiLayer::onAttach()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		VulkanDevice* vk = (VulkanDevice*)Application::get().getDevice().get();
		m_Device = vk;

		std::shared_ptr<Swapchain> swapchain = vk->getSwapchain();

		RenderPassDesc renderPassDesc{};
		renderPassDesc.Attachments = {
			AttachmentDesc{
				.Format = AttachmentFormat::SwapchainColorDefault,
				.Usage = AttachmentLayout::Color,
				.PreviousAttachmentUsage = AttachmentLayout::Undefined,
				.Samples = AttachmentDesc::Count1Bit,
				.LoadOp = LoadOperation::Clear,
				.StoreOp = StoreOperation::Store,
				.StencilLoadOp = LoadOperation::DontCare,
				.StencilStoreOp = StoreOperation::DontCare
			}
		};

		m_RenderPass = m_Device->createRenderPass(renderPassDesc, swapchain);

		VulkanRenderPass* vkRenderPass = (VulkanRenderPass*)m_RenderPass.get();

		ImGui_ImplVulkan_InitInfo initInfo{};
		initInfo.Instance = static_cast<VulkanInstance&>(vk->getInstance()).getInstance();
		initInfo.PhysicalDevice = vk->getPhysicalDevice();
		initInfo.Device = vk->getDevice();
		initInfo.QueueFamily = vk->getGraphicsQueueFamily();
		initInfo.Queue = vk->getGraphicsQueue();
		initInfo.PipelineCache = nullptr;
		initInfo.DescriptorPool = vk->getDescriptorPool();
		initInfo.Subpass = 0;
		initInfo.ImageCount = swapchain->getImageCount();
		initInfo.MinImageCount = swapchain->getImageCount();
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.Allocator = vk->getAllocator();
		initInfo.CheckVkResultFn = [](VkResult result) { VK_CHECK(result, "ImGui Vulkan failure") };

		ImGui_ImplGlfw_InitForVulkan(Application::get().getWindow(), true);
		ImGui_ImplVulkan_Init(&initInfo, vkRenderPass->getRenderPass());
	}

	void ImGuiLayer::onDetach()
	{
		m_Device->submitResourceFree([](Device* device)
		{
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		});
	}

	void ImGuiLayer::begin()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::end()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = { m_Device->getExtent().x, m_Device->getExtent().y };

		ImGui::Render();

		if (!m_Device->skipFrame())
		{
			VkCommandBuffer commandBuffer = ((VulkanDevice*)m_Device)->beginCommandListOverride(m_RenderPass);

			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

			((VulkanDevice*)m_Device)->endCommandListOverride();
		}

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		if (m_Device->didSwapchainResize())
			m_RenderPass->recreateFramebuffers();
	}

	void ImGuiLayer::onUpdate(float timestep)
	{
	}

	void ImGuiLayer::onEvent(Event& event)
	{
	}

}

#pragma once

#include "VulkanRenderer.h"
#include "Wire/ImGui/ImGuiLayer.h"

#include <array>

struct VkDescriptorPool_T; typedef VkDescriptorPool_T* VkDescriptorPool;

namespace Wire {

	class VulkanImGuiLayer : public ImGuiLayer
	{
	public:
		VulkanImGuiLayer(VulkanRenderer* renderer);
		virtual ~VulkanImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void Begin() override;
		virtual void End() override;
		virtual void UpdateViewports() override;

		void ConfigureStyle();

		virtual FontLibrary& GetFontLibrary() const override { return *m_FontLibrary; }
	private:
		void CreateVulkanObjects();
	private:
		VulkanRenderer* m_Renderer = nullptr;
		FontLibrary* m_FontLibrary = nullptr;

		std::array<rbRef<CommandBuffer>, WR_FRAMES_IN_FLIGHT> m_CommandBuffers;

		VkDescriptorPool m_DescriptorPool = nullptr;
		
		std::array<VkFence, WR_FRAMES_IN_FLIGHT> m_InFlightFences;
		std::array<VkSemaphore, WR_FRAMES_IN_FLIGHT> m_RenderFinishedSemaphores;
	};

}

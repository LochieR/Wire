#pragma once

#include "Wire/Core/Assert.h"
#include "Wire/UI/Renderer/Renderer.h"

#include <vulkan/vulkan.h>

#include <map>
#include <vector>

#define VK_CHECK(result, message) WR_ASSERT(result == VK_SUCCESS, message)

namespace wire {

	class VulkanRenderer : public Renderer
	{
	public:
		VulkanRenderer(const RendererDesc& desc);
		virtual ~VulkanRenderer();

		virtual void beginFrame() override;
		virtual void endFrame() override;

		virtual glm::vec2 getExtent() const override { return { (float)m_Extent.width, (float)m_Extent.height }; }

		virtual uint32_t getFrameIndex() const override { return m_FrameIndex; }
		virtual uint32_t getNumFramesInFlight() const override;

		virtual CommandBuffer& allocateCommandBuffer() override;
		virtual CommandBuffer& beginSingleTimeCommands() override;
		virtual void endSingleTimeCommands(CommandBuffer& commandBuffer) override;
		virtual void submitCommandBuffer(CommandBuffer& commandBuffer) override;

		virtual CommandList createCommandList() override;
		virtual void submitCommandList(const CommandList& commandList) override;

		virtual void submitResourceFree(std::function<void(Renderer*)>&& func) override;

		virtual GraphicsPipeline* createGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
		virtual VertexBuffer* createVertexBuffer(size_t size, const void* data = nullptr) override;
		virtual IndexBuffer* createIndexBuffer(size_t size, const void* data = nullptr) override;
		virtual StagingBuffer* createStagingBuffer(size_t size, const void* data = nullptr) override;
		virtual UniformBuffer* createUniformBuffer(size_t size, const void* data = nullptr) override;
		virtual Texture2D* createTexture2D(const std::filesystem::path& path) override;
		virtual Texture2D* createTexture2D(uint32_t* data, uint32_t width, uint32_t height) override;
		virtual Sampler* createSampler(const SamplerDesc& desc) override;
		virtual Font* createFont(const std::filesystem::path& path, uint32_t minChar = 0x0020, uint32_t maxChar = 0x00FF) override;
		virtual Font* getFontFromCache(const std::filesystem::path& path) override;

		virtual ShaderCache& getShaderCache() override { return m_ShaderCache; }
		virtual const ShaderCache& getShaderCache() const override { return m_ShaderCache; }
		virtual FontCache& getFontCache() override { return m_FontCache; }
		virtual const FontCache& getFontCache() const override { return m_FontCache; }

		virtual float getMaxAnisotropy() const override;

		VkDevice getDevice() const { return m_Device; }
		VkPhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice; }
		const VkAllocationCallbacks* getAllocator() const { return m_Allocator; }
		VkDescriptorPool getDescriptorPool() const { return m_DescriptorPool; }
		VkRenderPass getRenderPass() const { return m_RenderPass; }
		VkFramebuffer getCurrentFramebuffer() const { return m_Framebuffers[m_ImageIndex]; }

		uint32_t getSwapchainImageCount() const { return m_SwapchainImageCount; }

		void addRenderingCommandBuffer(VkCommandBuffer commandBuffer) { m_CurrentRenderingCommandBuffers.push_back(commandBuffer); }
		void addNonRenderingCommandBuffer(VkCommandBuffer commandBuffer) { m_CurrentNonRenderingCommandBuffers.push_back(commandBuffer); }

		bool isFrameSkipped() const { return m_SkipFrame; }
	private:
		void createInstance();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createCommandPool();
		void createDescriptorPool();
		void createSyncObject();

		void createSwapchain();
		void createRenderPass();

		void executeCommandScope(VkCommandBuffer commandBuffer, const CommandScope& commandScope);
	private:
		struct CommandListData
		{
			std::vector<VkCommandBuffer> Buffers;
			std::vector<CommandScope::Type> Types;
		};
	private:
		VkInstance m_Instance = nullptr;
		VkAllocationCallbacks* m_Allocator = nullptr;
		VkDebugUtilsMessengerEXT m_DebugMessenger = nullptr;
		VkSurfaceKHR m_Surface = nullptr;
		VkPhysicalDevice m_PhysicalDevice = nullptr;
		VkDevice m_Device = nullptr;
		VkQueue m_GraphicsQueue = nullptr;
		VkQueue m_PresentQueue = nullptr;
		VkCommandPool m_CommandPool = nullptr;
		VkDescriptorPool m_DescriptorPool = nullptr;

		std::vector<VkCommandBuffer> m_FrameCommandBuffers;

		VkExtent2D m_Extent;
		VkSwapchainKHR m_Swapchain = nullptr;
		uint32_t m_SwapchainImageCount;
		VkFormat m_SwapchainImageFormat;

		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;

		VkImage m_DepthImage = nullptr;
		VkDeviceMemory m_DepthImageMemory = nullptr;
		VkImageView m_DepthImageView = nullptr;

		VkRenderPass m_RenderPass = nullptr;

		std::vector<VkFramebuffer> m_Framebuffers;

		uint32_t m_ImageIndex = 0;
		uint32_t m_FrameIndex;

		bool m_SkipFrame = false;

		std::vector<VkCommandBuffer> m_CurrentRenderingCommandBuffers;
		std::vector<VkCommandBuffer> m_CurrentNonRenderingCommandBuffers;
		std::vector<VkCommandBuffer> m_SubmittedRenderingCommandBuffers;
		std::vector<VkCommandBuffer> m_SubmittedNonRenderingCommandBuffers;

		std::vector<CommandBuffer*> m_AllocatedCommandBuffers;
		std::vector<CommandBuffer*> m_CurrentSingleTimeCommands;

		std::vector<std::vector<VkCommandBuffer>> m_SecondaryCommandBufferPool;
		std::vector<uint32_t> m_UsedSecondaryCommandBufferCount;

		std::vector<std::vector<CommandListData>> m_SubmittedCommandLists;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;

		ShaderCache m_ShaderCache;
		FontCache m_FontCache;

		std::vector<std::vector<std::function<void(Renderer*)>>> m_ResourceFreeQueue;
	};

}

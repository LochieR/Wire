#pragma once

#include "Wire/Core/Base.h"
#include "Wire/Renderer/Renderer.h"

#include <array>
#include <functional>

#define WR_VK_DEF(name) struct name##_T; typedef name##_T* name

WR_VK_DEF(VkInstance);
WR_VK_DEF(VkDebugUtilsMessengerEXT);
WR_VK_DEF(VkPhysicalDevice);
WR_VK_DEF(VkDevice);
WR_VK_DEF(VkQueue);
WR_VK_DEF(VkSurfaceKHR);
WR_VK_DEF(VkSwapchainKHR);
WR_VK_DEF(VkImage);
WR_VK_DEF(VkImageView);
WR_VK_DEF(VkRenderPass);
WR_VK_DEF(VkFramebuffer);
WR_VK_DEF(VkCommandPool);
WR_VK_DEF(VkSemaphore);
WR_VK_DEF(VkFence);
WR_VK_DEF(VkBuffer);
WR_VK_DEF(VkDeviceMemory);
WR_VK_DEF(VkDescriptorPool);

struct VkAllocationCallbacks;
struct VkSurfaceFormatKHR;
enum VkPresentModeKHR;
enum VkSurfaceTransformFlagBitsKHR;
enum VkFormat;
enum VkImageTiling;
enum VkImageLayout;
enum VkSampleCountFlagBits;

typedef uint32_t VkFlags;
typedef VkFlags VkSurfaceTransformFlagsKHR;
typedef VkFlags VkCompositeAlphaFlagsKHR;
typedef VkFlags VkImageUsageFlags;
typedef VkFlags VkMemoryPropertyFlags;
typedef VkFlags VkBufferUsageFlags;
typedef VkFlags VkFormatFeatureFlags;
typedef VkFlags VkImageAspectFlags;
typedef uint64_t VkDeviceSize;

#ifndef VULKAN_H_

typedef struct VkExtent2D
{
	uint32_t    width;
	uint32_t    height;
} VkExtent2D;

typedef struct VkSurfaceCapabilitiesKHR
{
	uint32_t                         minImageCount;
	uint32_t                         maxImageCount;
	VkExtent2D                       currentExtent;
	VkExtent2D                       minImageExtent;
	VkExtent2D                       maxImageExtent;
	uint32_t                         maxImageArrayLayers;
	VkSurfaceTransformFlagsKHR       supportedTransforms;
	VkSurfaceTransformFlagBitsKHR    currentTransform;
	VkCompositeAlphaFlagsKHR         supportedCompositeAlpha;
	VkImageUsageFlags                supportedUsageFlags;
} VkSurfaceCapabilitiesKHR;

#endif

#define WR_FRAMES_IN_FLIGHT 2

namespace Wire {

	class VulkanRenderer;

	struct VulkanData
	{
		VkInstance Instance = nullptr;
		VkAllocationCallbacks* Allocator = nullptr;
		VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
		VkPhysicalDevice PhysicalDevice = nullptr;
		VkDevice Device = nullptr;
		VkQueue GraphicsQueue = nullptr;
		VkQueue PresentQueue = nullptr;
		VkSurfaceKHR Surface = nullptr;
		VkSwapchainKHR Swapchain = nullptr;
		std::vector<VkImage> SwapchainImages;
		std::vector<VkImageView> SwapchainImageViews;
		std::vector<VkFramebuffer> SwapchainFramebuffers;
		VkFormat SwapchainImageFormat;
		VkExtent2D SwapchainExtent;
		VkRenderPass RenderPass = nullptr;
		VkCommandPool CommandPool = nullptr;
		VkDescriptorPool DescriptorPool = nullptr;
		std::array<VkSemaphore, WR_FRAMES_IN_FLIGHT> ImageAvailableSemaphores;
		std::array<VkSemaphore, WR_FRAMES_IN_FLIGHT> RenderFinishedSemaphores;
		std::array<VkFence, WR_FRAMES_IN_FLIGHT> InFlightFences;

		VkSampleCountFlagBits MultisampleCount;
		VkImage ColorImage = nullptr;
		VkDeviceMemory ColorImageMemory = nullptr;
		VkImageView ColorImageView = nullptr;

		uint32_t ImageIndex = 0;
		uint32_t CurrentFrame = 0;

		uint32_t SubpassCount = 0;
		uint32_t CurrentSubpass = 0;

		std::array<std::vector<std::function<void(VulkanRenderer*)>>, WR_FRAMES_IN_FLIGHT> ResourceFreeQueue;
		std::array<std::vector<VkSemaphore>, WR_FRAMES_IN_FLIGHT> FrameRenderFinishedSemaphores;
		std::array<bool, WR_FRAMES_IN_FLIGHT> ImageAcquiredSemaphoreUsed;
		std::array<std::vector<VkFence>, WR_FRAMES_IN_FLIGHT> FrameFences;

		std::vector<IResource*> Resources;
	};

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	struct QueueFamilyIndices
	{
		uint32_t GraphicsFamily = static_cast<uint32_t>(-1);
		uint32_t PresentFamily = static_cast<uint32_t>(-1);

		bool IsComplete() const { return GraphicsFamily != static_cast<uint32_t>(-1) && PresentFamily != static_cast<uint32_t>(-1); }
	};

	class VulkanRenderer : public Renderer
	{
	public:
		VulkanRenderer(Window& window);
		virtual ~VulkanRenderer();

		virtual void Release() override;

		virtual bool BeginFrame() override;
		virtual void EndFrame() override;

		virtual uint32_t GetFrameIndex() const override;
		virtual uint32_t GetMaxFramesInFlight() const override { return WR_FRAMES_IN_FLIGHT; }

		virtual Renderer2D& GetRenderer2D() override { return m_Renderer2D; }

		virtual void Draw(rbRef<CommandBuffer> commandBuffer, uint32_t vertexCount) override;
		virtual void DrawIndexed(rbRef<CommandBuffer> commandBuffer, uint32_t indexCount) override;

		VulkanData& GetVulkanData() const;
		void SubmitResourceFree(std::function<void(VulkanRenderer*)>&& func);

		virtual rbRef<Shader> CreateShader(std::string_view filepath) override;
		virtual rbRef<VertexBuffer> CreateVertexBuffer(size_t size) override;
		virtual rbRef<IndexBuffer> CreateIndexBuffer(uint32_t* indices, uint32_t indexCount) override;
		virtual rbRef<StorageBuffer> CreateStorageBuffer(size_t size) override;
		virtual rbRef<StagingBuffer> CreateStagingBuffer(size_t size) override;
		virtual rbRef<Texture2D> CreateTexture2D(std::string_view path) override;
		virtual rbRef<Texture2D> CreateTexture2D(uint32_t* data, uint32_t width, uint32_t height) override;
		virtual rbRef<Font> CreateFont(std::string_view path, uint32_t minChar = 0x0020, uint32_t maxChar = 0x00FF) override;
		virtual rbRef<Framebuffer> CreateFramebuffer(const FramebufferSpecification& spec) override;
		virtual ImGuiLayer* CreateImGuiLayer() override;

		virtual rbRef<CommandBuffer> AllocateCommandBuffer() override;
		virtual rbRef<CommandBuffer> BeginSingleTimeCommands() override;
		virtual void EndSingleTimeCommands(rbRef<CommandBuffer> commandBuffer) override;
		virtual void BeginCommandBufferAndRenderPass(rbRef<CommandBuffer> commandBuffer) override;
		virtual void EndCommandBufferAndRenderPass(rbRef<CommandBuffer> commandBuffer) override;
		virtual void BeginRenderPass(rbRef<CommandBuffer> commandBuffer) override;
		virtual void EndRenderPass(rbRef<CommandBuffer> commandBuffer) override;
		virtual void SubmitCommandBuffer(rbRef<CommandBuffer> commandBuffer) override;
		void SubmitCommandBuffer(rbRef<CommandBuffer> commandBuffer, VkFence fence, VkSemaphore semaphore);
		virtual void NextSubpass(rbRef<CommandBuffer> commandBuffer) override;
		virtual void CopyBuffer(rbRef<CommandBuffer> commandBuffer, rbRef<StagingBuffer> srcBuffer, rbRef<StorageBuffer> dstBuffer) override;

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		
		void CreateImage(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat FindDepthFormat();
		bool HasStencilComponent(VkFormat format);

		QueueFamilyIndices FindQueueFamilies();
	protected:
		virtual void Free(rbIRef* ref) override;
	private:
		#pragma region VulkanInit

		void CreateInstance();
		void SetupDebugMessenger();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSurface();
		void CreateSwapchain();
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreateCommandPool();
		void CreateDescriptorPool();
		void CreateColorResources();
		void CreateSyncObjects();
		void CleanupSwapchain();
		void RecreateSwapchain();

		bool CheckValidationLayerSupport();

		#pragma endregion
	private:
		Window& m_Window;
		Renderer2D m_Renderer2D;

		const std::vector<const char*> s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	};

}

#define VK_CHECK(result, msg) WR_ASSERT(result == VK_SUCCESS && msg)

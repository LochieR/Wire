#pragma once

#include "VulkanExtensions.h"

#include "Wire/Core/Assert.h"
#include "Wire/UI/Renderer/Renderer.h"

#include <vulkan/vulkan.h>
#include <glfw/glfw3.h>

#include <map>
#include <vector>
#include <typeindex>

#define VK_CHECK(result, message) WR_ASSERT(result == VK_SUCCESS, message)

#ifdef WR_DEBUG
#define VK_DEBUG_NAME(device, type, object, nameCStr)                        \
    {                                                                        \
        VkDebugUtilsObjectNameInfoEXT info{};                                \
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;    \
        info.objectType = VK_OBJECT_TYPE_##type;                            \
        info.objectHandle = (uint64_t)object;                                \
        info.pObjectName = nameCStr;                                        \
                                                                            \
        exts::vkSetDebugUtilsObjectNameEXT(device, &info);                    \
    }
#else
#define VK_DEBUG_NAME(...)
#endif

namespace wire {

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

    struct VulkanCopyBufferNativeCommand : public CommandListNativeCommand
    {
        VkBuffer SrcBuffer;
        VkBuffer DstBuffer;
        size_t Size;
        size_t SrcOffset, DstOffset;

        virtual ~VulkanCopyBufferNativeCommand() = default;
    };

    struct VulkanPipelineBarrierNativeCommand : public CommandListNativeCommand
    {
        VkPipelineStageFlags SrcStage, DstStage;
        VkImageMemoryBarrier Barrier;
    };

    struct VulkanCopyBufferToImageNativeCommand : public CommandListNativeCommand
    {
        VkBuffer SrcBuffer;
        VkImage DstImage;
        VkImageLayout DstImageLayout;
        VkBufferImageCopy Region;
    };

    class VulkanRenderer : public Renderer
    {
    public:
        VulkanRenderer(const RendererDesc& desc, const SwapchainDesc& swapchainDesc);
        virtual ~VulkanRenderer();

        virtual void beginFrame() override;
        virtual void endFrame() override;

        virtual glm::vec2 getExtent() const override { return m_Swapchain->getExtent(); }

        virtual uint32_t getFrameIndex() const override { return m_FrameIndex; }
        virtual uint32_t getNumFramesInFlight() const override;

        virtual CommandList beginSingleTimeCommands() override;
        virtual void endSingleTimeCommands(CommandList& commandList) override;

        virtual CommandList createCommandList() override;
        virtual void submitCommandList(const CommandList& commandList) override;

        virtual void submitResourceFree(std::function<void(Renderer*)>&& func) override;

        virtual bool didSwapchainResize() const override { return m_DidSwapchainResize; }
        virtual bool skipFrame() const override { return m_SkipFrame; }

        virtual Swapchain* getSwapchain() const override { return m_Swapchain; }

        virtual Swapchain* createSwapchain(const SwapchainDesc& desc, std::string_view debugName = {}) override;
        virtual Framebuffer* createFramebuffer(const FramebufferDesc& desc, std::string_view debugName = {}) override;
        virtual RenderPass* createRenderPass(const RenderPassDesc& desc, Swapchain* swapchain, std::string_view debugName = {}) override;
        virtual RenderPass* createRenderPass(const RenderPassDesc& desc, Framebuffer* framebuffer, std::string_view debugName = {}) override;
        virtual ShaderResourceLayout* createShaderResourceLayout(const ShaderResourceLayoutInfo& layoutInfo) override;
        virtual ShaderResource* createShaderResource(uint32_t set, ShaderResourceLayout* layout) override;
        virtual GraphicsPipeline* createGraphicsPipeline(const GraphicsPipelineDesc& desc, std::string_view debugName = {}) override;
        virtual ComputePipeline* createComputePipeline(const ComputePipelineDesc& desc, std::string_view debugName = {}) override;
        virtual Texture2D* createTexture2D(const std::filesystem::path& path, std::string_view debugName = {}) override;
        virtual Texture2D* createTexture2D(uint32_t* data, uint32_t width, uint32_t height, std::string_view debugName = {}) override;
        virtual Sampler* createSampler(const SamplerDesc& desc, std::string_view debugName = {}) override;
        virtual Font* createFont(const std::filesystem::path& path, std::string_view debugName = {}, uint32_t minChar = 0x0020, uint32_t maxChar = 0x00FF) override;
        virtual Font* getFontFromCache(const std::filesystem::path& path) override;

        virtual ShaderCache& getShaderCache() override { return m_ShaderCache; }
        virtual const ShaderCache& getShaderCache() const override { return m_ShaderCache; }
        virtual FontCache& getFontCache() override { return m_FontCache; }
        virtual const FontCache& getFontCache() const override { return m_FontCache; }

        virtual float getMaxAnisotropy() const override;

        VkCommandBuffer beginCommandListOverride(RenderPass* renderPass = nullptr);
        void endCommandListOverride();

        VkInstance getInstance() const { return m_Instance; }
        VkPhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice; }
        VkSurfaceKHR getSurface() const { return m_Surface; }
        VkDevice getDevice() const { return m_Device; }
        uint32_t getGraphicsQueueFamily() const;
        VkQueue getGraphicsQueue() const { return m_GraphicsQueue; }
        const VkAllocationCallbacks* getAllocator() const { return m_Allocator; }
        VkDescriptorPool getDescriptorPool() const { return m_DescriptorPool; }

        VkSemaphore getCurrentImageAvailableSemaphore() const { return m_ImageAvailableSemaphores[m_FrameIndex]; }
        void setImageAvailableSemaphore(uint32_t frameIndex, VkSemaphore semaphore) { m_ImageAvailableSemaphores[frameIndex] = semaphore; }

        bool isFrameSkipped() const { return m_SkipFrame; }

        std::shared_ptr<CommandListNativeCommand> copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, size_t size, size_t srcOffset, size_t dstOffset, std::type_index& outType);
        std::shared_ptr<CommandListNativeCommand> pipelineBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, const VkImageMemoryBarrier& barrier, std::type_index& outType);
        std::shared_ptr<CommandListNativeCommand> copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, const VkBufferImageCopy& copy, std::type_index& outType);
    protected:
        virtual BufferBase* createBufferBase(BufferType type, size_t size, const void* data = nullptr, std::string_view debugName = {}) override;
    private:
        void createInstance();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createCommandPool();
        void createDescriptorPool();
        void createSyncObject();

        void loadExtensions();

        void executeCommandScope(VkCommandBuffer commandBuffer, const CommandScope& commandScope);
    private:
        struct CommandListData
        {
            std::vector<VkCommandBuffer> Buffers;
            std::vector<CommandScope::Type> Types;
            std::vector<RenderPass*> RenderPasses;
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

        Swapchain* m_Swapchain = nullptr;

        uint32_t m_ImageIndex;
        uint32_t m_FrameIndex;

        bool m_SkipFrame = false;
        bool m_DidSwapchainResize = false;

        std::vector<std::vector<VkCommandBuffer>> m_SecondaryCommandBufferPool;
        std::vector<uint32_t> m_UsedSecondaryCommandBufferCount;

        std::vector<std::vector<CommandListData>> m_SubmittedCommandLists;
        CommandListData* m_CurrentOverrideCommandList = nullptr;

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_InFlightFences;

        ShaderCache m_ShaderCache;
        FontCache m_FontCache;

        std::vector<std::vector<std::function<void(Renderer*)>>> m_ResourceFreeQueue;
    };

    namespace Utils {

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
        SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D ChooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);
        VkFormat FindDepthFormat(VkPhysicalDevice device);
        uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
        VkImageLayout GetImageLayout(AttachmentLayout type);

    }

}

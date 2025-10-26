#pragma once

#include "VulkanInstance.h"
#include "Wire/Renderer/Device.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

struct GLFWwindow;

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

    class VulkanDevice : public Device
    {
    public:
        VulkanDevice(VulkanInstance* instance, const DeviceInfo& deviceInfo, const SwapchainInfo& swapchainInfo);
        virtual ~VulkanDevice();

        virtual glm::vec2 getExtent() const override { return m_Swapchain->getExtent(); }

        virtual void beginFrame() override;
        virtual void endFrame() override;

        virtual uint32_t getFrameIndex() const override { return m_FrameIndex; }

        virtual CommandList beginSingleTimeCommands() override;
        virtual void endSingleTimeCommands(CommandList& commandList) override;

        virtual CommandList createCommandList() override;
        virtual void submitCommandList(const CommandList& commandList) override;

        virtual void submitResourceFree(std::function<void(Device*)>&& func) override;

        virtual bool skipFrame() const override { return m_SkipFrame; }
        virtual bool didSwapchainResize() const override { return m_DidSwapchainResize; }
        
        virtual Instance& getInstance() const override { return *m_Instance; }
        virtual std::shared_ptr<Swapchain> getSwapchain() const override { return m_Swapchain; }
        
        virtual void drop(const std::shared_ptr<IResource>& resource) override;
        virtual std::shared_ptr<IResource> getResource(IResource* resource) const override;

        virtual std::shared_ptr<Swapchain> createSwapchain(const SwapchainInfo& info, std::string_view debugName = {}) override;
        virtual std::shared_ptr<Framebuffer> createFramebuffer(const FramebufferDesc& desc, std::string_view debugName = {}) override;
        virtual std::shared_ptr<RenderPass> createRenderPass(const RenderPassDesc& desc, const std::shared_ptr<Swapchain>& swapchain, std::string_view debugName = {}) override;
        virtual std::shared_ptr<RenderPass> createRenderPass(const RenderPassDesc& desc, const std::shared_ptr<Framebuffer>& framebuffer, std::string_view debugName = {}) override;
        virtual std::shared_ptr<Buffer> createBuffer(BufferType type, size_t size, const void* data = nullptr, std::string_view debugName = {}) override;
        virtual std::shared_ptr<ShaderResourceLayout> createShaderResourceLayout(const ShaderResourceLayoutInfo& layoutInfo, std::string_view debugName = {}) override;
        virtual std::shared_ptr<ShaderResource> createShaderResource(uint32_t set, const std::shared_ptr<ShaderResourceLayout>& layout, std::string_view debugName = {}) override;
        virtual std::shared_ptr<GraphicsPipeline> createGraphicsPipeline(const GraphicsPipelineDesc& desc, std::string_view debugName = {}) override;
        virtual std::shared_ptr<ComputePipeline> createComputePipeline(const ComputePipelineDesc& desc, std::string_view debugName = {}) override;
        virtual std::shared_ptr<Texture2D> createTexture2D(const std::filesystem::path& path, std::string_view debugName = {}) override;
        virtual std::shared_ptr<Texture2D> createTexture2D(uint32_t* data, uint32_t width, uint32_t height, std::string_view debugName = {}) override;
        virtual std::shared_ptr<Sampler> createSampler(const SamplerDesc& desc, std::string_view debugName = {}) override;
        virtual std::shared_ptr<Font> createFont(const std::filesystem::path& path, std::string_view debugName = {}, uint32_t minChar = 0x0020, uint32_t maxChar = 0x00FF) override;
        virtual std::shared_ptr<Font> getFontFromCache(const std::filesystem::path& path) override;

        virtual ShaderCache& getShaderCache() override { return m_ShaderCache; }
        virtual const ShaderCache& getShaderCache() const override { return m_ShaderCache; }
        virtual FontCache& getFontCache() override { return m_FontCache; }
        virtual const FontCache& getFontCache() const override { return m_FontCache; }

        virtual float getMaxAnisotropy() const override;

        VkCommandBuffer beginCommandListOverride(const std::shared_ptr<RenderPass>& renderPass = nullptr);
        void endCommandListOverride();

        VkPhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice; }
        VkDevice getDevice() const { return m_Device; }
        uint32_t getGraphicsQueueFamily() const;
        VkQueue getGraphicsQueue() const { return m_GraphicsQueue; }
        const VkAllocationCallbacks* getAllocator() const { return m_Instance->getAllocator(); }
        VkDescriptorPool getDescriptorPool() const { return m_DescriptorPool; }

        VkSurfaceKHR getSurface() const { return m_Instance->getSurface(); }

        VkSemaphore getCurrentImageAvailableSemaphore() const { return m_ImageAvailableSemaphores[m_FrameIndex]; }
        void setImageAvailableSemaphore(uint32_t frameIndex, VkSemaphore semaphore) { m_ImageAvailableSemaphores[frameIndex] = semaphore; }

        std::shared_ptr<CommandListNativeCommand> copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, size_t size, size_t srcOffset, size_t dstOffset, std::type_index& outType);
        std::shared_ptr<CommandListNativeCommand> pipelineBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, const VkImageMemoryBarrier& barrier, std::type_index& outType);
        std::shared_ptr<CommandListNativeCommand> copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, const VkBufferImageCopy& copy, std::type_index& outType);
        
        virtual void destroy() override;
        virtual void invalidate() noexcept override;
    private:
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
            std::vector<std::shared_ptr<RenderPass>> RenderPasses;
        };
    private:
        VulkanInstance* m_Instance = nullptr;

        VkPhysicalDevice m_PhysicalDevice = nullptr;
        VkDevice m_Device = nullptr;
        VkQueue m_GraphicsQueue = nullptr;
        VkQueue m_PresentQueue = nullptr;
        VkCommandPool m_CommandPool = nullptr;
        VkDescriptorPool m_DescriptorPool = nullptr;

        std::vector<VkCommandBuffer> m_FrameCommandBuffers;

        std::shared_ptr<Swapchain> m_Swapchain = nullptr;
        std::vector<std::shared_ptr<IResource>> m_Resources;

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

        std::vector<std::vector<std::function<void(Device*)>>> m_ResourceFreeQueue;
    };

    namespace Utils {

        bool CheckValidationLayerSupport();
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

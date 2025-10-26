#pragma once

#include "IResource.h"
#include "CommandList.h"
#include "ComputePipeline.h"
#include "GraphicsPipeline.h"
#include "Buffer.h"
#include "Texture2D.h"
#include "FontCache.h"
#include "Font.h"
#include "ShaderCache.h"
#include "RenderPass.h"
#include "Swapchain.h"
#include "Framebuffer.h"
#include "ShaderResource.h"

#include <memory>
#include <concepts>

namespace wire {

    class Instance;

    struct DeviceInfo
    {
        ShaderCacheDesc ShaderCache;
        FontCacheDesc FontCache;
    };

    class Device : public IResource
    {
    public:
        virtual ~Device() = default;

        virtual glm::vec2 getExtent() const = 0;

        virtual void beginFrame() = 0;
        virtual void endFrame() = 0;

        virtual uint32_t getFrameIndex() const = 0;

        virtual CommandList beginSingleTimeCommands() = 0;
        virtual void endSingleTimeCommands(CommandList& commandList) = 0;

        virtual CommandList createCommandList() = 0;
        virtual void submitCommandList(const CommandList& commandList) = 0;

        virtual void submitResourceFree(std::function<void(Device*)>&& func) = 0;

        virtual bool skipFrame() const = 0;
        virtual bool didSwapchainResize() const = 0;
        
        virtual Instance& getInstance() const = 0;
        virtual std::shared_ptr<Swapchain> getSwapchain() const = 0;
        
        virtual void drop(const std::shared_ptr<IResource>& resource) = 0;
        virtual std::shared_ptr<IResource> getResource(IResource* resource) const = 0;

        virtual std::shared_ptr<Swapchain> createSwapchain(const SwapchainInfo& info, std::string_view debugName = {}) = 0;
        virtual std::shared_ptr<Framebuffer> createFramebuffer(const FramebufferDesc& desc, std::string_view debugName = {}) = 0;
        virtual std::shared_ptr<RenderPass> createRenderPass(const RenderPassDesc& desc, const std::shared_ptr<Swapchain>& swapchain, std::string_view debugName = {}) = 0;
        virtual std::shared_ptr<RenderPass> createRenderPass(const RenderPassDesc& desc, const std::shared_ptr<Framebuffer>& framebuffer, std::string_view debugName = {}) = 0;
        virtual std::shared_ptr<Buffer> createBuffer(BufferType type, size_t size, const void* data = nullptr, std::string_view debugName = {}) = 0;
        virtual std::shared_ptr<ShaderResourceLayout> createShaderResourceLayout(const ShaderResourceLayoutInfo& layoutInfo, std::string_view debugName = {}) = 0;
        virtual std::shared_ptr<ShaderResource> createShaderResource(uint32_t set, const std::shared_ptr<ShaderResourceLayout>& layout, std::string_view debugName = {}) = 0;
        virtual std::shared_ptr<GraphicsPipeline> createGraphicsPipeline(const GraphicsPipelineDesc& desc, std::string_view debugName = {}) = 0;
        virtual std::shared_ptr<ComputePipeline> createComputePipeline(const ComputePipelineDesc& desc, std::string_view debugName = {}) = 0;
        virtual std::shared_ptr<Texture2D> createTexture2D(const std::filesystem::path& path, std::string_view debugName = {}) = 0;
        virtual std::shared_ptr<Texture2D> createTexture2D(uint32_t* data, uint32_t width, uint32_t height, std::string_view debugName = {}) = 0;
        virtual std::shared_ptr<Sampler> createSampler(const SamplerDesc& desc, std::string_view debugName = {}) = 0;
        virtual std::shared_ptr<Font> createFont(const std::filesystem::path& path, std::string_view debugName = {}, uint32_t minChar = 0x0020, uint32_t maxChar = 0x00FF) = 0;
        virtual std::shared_ptr<Font> getFontFromCache(const std::filesystem::path& path) = 0;

        virtual ShaderCache& getShaderCache() = 0;
        virtual const ShaderCache& getShaderCache() const = 0;
        virtual FontCache& getFontCache() = 0;
        virtual const FontCache& getFontCache() const = 0;

        virtual float getMaxAnisotropy() const = 0;
        
        template<typename T>
        requires std::is_base_of_v<IResource, T>
        std::shared_ptr<T> getResource(T* resource) const
        {
            return std::static_pointer_cast<T>(getResource(resource));
        }
    };

}

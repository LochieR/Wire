#pragma once

#include "Buffer.h"
#include "IResource.h"
#include "Texture2D.h"
#include "Framebuffer.h"
#include "ShaderCache.h"

#include <cstdint>
#include <vector>

namespace wire {

    enum class ShaderResourceType
    {
        UniformBuffer = 0,
        CombinedImageSampler,
        SampledImage,
        Sampler,
        StorageBuffer,
        StorageImage
    };

    struct ShaderResourceInfo
    {
        uint32_t Binding;
        ShaderResourceType Type;
        uint32_t ArrayCount;
        ShaderType Stage;
    };

    struct ShaderResourceSetInfo
    {
        std::vector<ShaderResourceInfo> Resources;
    };

    struct ShaderResourceLayoutInfo
    {
        std::vector<ShaderResourceSetInfo> Sets;
    };
    
    class ShaderResourceLayout : public IResource
    {
    public:
        virtual ~ShaderResourceLayout() = default;
    };

    class ShaderResource : public IResource
    {
    public:
        virtual ~ShaderResource() = default;
        
        virtual void update(const std::shared_ptr<Texture2D>& texture, uint32_t binding, uint32_t index) = 0;
        virtual void update(const std::shared_ptr<Texture2D>& texture, uint32_t binding, uint32_t index, uint32_t mipLevel) = 0;
        virtual void update(const std::shared_ptr<Sampler>& sampler, uint32_t binding, uint32_t index) = 0;
        virtual void update(const std::shared_ptr<Texture2D>& texture, const std::shared_ptr<Sampler>& sampler, uint32_t binding, uint32_t index) = 0;
        virtual void update(const std::shared_ptr<Buffer>& uniformBuffer, uint32_t binding, uint32_t index) = 0;
        virtual void update(const std::shared_ptr<Framebuffer>& storageImage, uint32_t binding, uint32_t index) = 0;
        virtual void update(const std::shared_ptr<Framebuffer>& storageImage, uint32_t binding, uint32_t index, uint32_t mipLevel) = 0;
    };

}

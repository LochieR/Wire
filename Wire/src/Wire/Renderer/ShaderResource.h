#pragma once

#include "Buffer.h"
#include "Texture2D.h"
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
    
    class ShaderResourceLayout
    {
    public:
        virtual ~ShaderResourceLayout() = default;
    };

    class ShaderResource
    {
    public:
        virtual ~ShaderResource() = default;
        
        virtual void update(Texture2D* texture, uint32_t binding, uint32_t index) = 0;
        virtual void update(Sampler* sampler, uint32_t binding, uint32_t index) = 0;
        virtual void update(Texture2D* texture, Sampler* sampler, uint32_t binding, uint32_t index) = 0;
        virtual void update(BufferBase* uniformBuffer, uint32_t binding, uint32_t index) = 0;
        
        template<BufferType Type>
        std::enable_if_t<(Type & UniformBuffer) == UniformBuffer, void> update(Buffer<Type>* uniformBuffer, uint32_t binding, uint32_t index)
        {
            update(uniformBuffer->getBase(), binding, index);
        }
    };

}

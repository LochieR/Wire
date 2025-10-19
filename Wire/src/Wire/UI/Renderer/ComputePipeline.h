#pragma once

#include "GraphicsPipeline.h"

#include <string>

namespace wire {

    struct ComputeInputLayout
    {
        uint32_t SlotCount;
        uint32_t SetsPerSlot;
        std::vector<PushConstantInfo> PushConstantInfos;
        std::vector<ShaderResourceInfo> ShaderResources;
    };

    struct ComputePipelineDesc
    {
        std::string ShaderPath;
        ComputeInputLayout Layout;
    };
    
    class ComputePipeline
    {
    public:
        virtual ~ComputePipeline() = default;

        virtual void updateDescriptor(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateDescriptor(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t mipLevel, uint32_t binding, uint32_t index) = 0;
        virtual void updateDescriptor(Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateDescriptor(Texture2D* texture, Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateDescriptor(BufferBase* buffer, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateDescriptor(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateDescriptor(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t mipLevel, uint32_t binding, uint32_t index) = 0;

        virtual void updateFrameDescriptor(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateFrameDescriptor(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t mipLevel, uint32_t binding, uint32_t index) = 0;
        virtual void updateFrameDescriptor(Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateFrameDescriptor(Texture2D* texture, Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateFrameDescriptor(BufferBase* buffer, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateFrameDescriptor(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateFrameDescriptor(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t mipLevel, uint32_t binding, uint32_t index) = 0;

        virtual void updateAllDescriptors(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateAllDescriptors(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t mipLevel, uint32_t binding, uint32_t index) = 0;
        virtual void updateAllDescriptors(Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateAllDescriptors(Texture2D* texture, Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateAllDescriptors(BufferBase* buffer, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateAllDescriptors(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) = 0;
        virtual void updateAllDescriptors(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t mipLevel, uint32_t binding, uint32_t index) = 0;

        template<BufferType Type>
        std::enable_if_t<Type & (UniformBuffer | StorageBuffer), void> updateDescriptor(Buffer<Type>* buffer, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index)
        {
            updateDescriptor(buffer->getBase(), set, setIndex, binding, index);
        }

        template<BufferType Type>
        std::enable_if_t<Type & (UniformBuffer | StorageBuffer), void> updateFrameDescriptor(Buffer<Type>* buffer, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index)
        {
            updateFrameDescriptor(buffer->getBase(), set, setIndex, frameIndex, binding, index);
        }

        template<BufferType Type>
        std::enable_if_t<Type & (UniformBuffer | StorageBuffer), void> updateAllDescriptors(Buffer<Type>* buffer, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index)
        {
            updateAllDescriptors(buffer->getBase(), set, setIndex, binding, index);
        }
    };

}

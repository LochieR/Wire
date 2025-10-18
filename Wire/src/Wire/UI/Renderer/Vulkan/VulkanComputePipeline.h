#pragma once

#include "VulkanRenderer.h"

#include <vulkan/vulkan.h>

#include <array>
#include <vector>

namespace wire {

    class VulkanComputePipeline : public ComputePipeline
    {
    public:
        VulkanComputePipeline(Renderer* renderer, const ComputePipelineDesc& desc, std::string_view debugName = {});
        virtual ~VulkanComputePipeline();

        virtual void updateDescriptor(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) override;
        virtual void updateDescriptor(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t mipLevel, uint32_t binding, uint32_t index) override;
        virtual void updateDescriptor(Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) override;
        virtual void updateDescriptor(Texture2D* texture, Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) override;
        virtual void updateDescriptor(BufferBase* buffer, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) override;
        virtual void updateDescriptor(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) override;
        virtual void updateDescriptor(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t mipLevel, uint32_t binding, uint32_t index) override;

        virtual void updateFrameDescriptor(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index) override;
        virtual void updateFrameDescriptor(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t mipLevel, uint32_t binding, uint32_t index) override;
        virtual void updateFrameDescriptor(Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index) override;
        virtual void updateFrameDescriptor(Texture2D* texture, Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index) override;
        virtual void updateFrameDescriptor(BufferBase* buffer, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index) override;
        virtual void updateFrameDescriptor(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index) override;
        virtual void updateFrameDescriptor(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t mipLevel, uint32_t binding, uint32_t index) override;;

        virtual void updateAllDescriptors(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) override;
        virtual void updateAllDescriptors(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t mipLevel, uint32_t binding, uint32_t index) override;
        virtual void updateAllDescriptors(Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) override;
        virtual void updateAllDescriptors(Texture2D* texture, Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) override;
        virtual void updateAllDescriptors(BufferBase* buffer, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) override;
        virtual void updateAllDescriptors(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index) override;
        virtual void updateAllDescriptors(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t mipLevel, uint32_t binding, uint32_t index) override;

        VkPipeline getPipeline() const { return m_Pipeline; }
        VkPipelineLayout getPipelineLayout() const { return m_Layout; }
        VkDescriptorSet getDescriptorSet(uint32_t set, uint32_t index) const { return m_DescriptorSets[set][index][m_Renderer->getFrameIndex()]; }
    private:
        Renderer* m_Renderer = nullptr;

        std::string m_DebugName;

        const ComputeInputLayout m_InputLayout;

        VkShaderModule m_ComputeShader = nullptr;
        std::vector<VkDescriptorSetLayout> m_SetLayouts;
        std::vector<std::vector<std::array<VkDescriptorSet, WR_FRAMES_IN_FLIGHT>>> m_DescriptorSets;
        VkPipelineLayout m_Layout = nullptr;
        VkPipeline m_Pipeline = nullptr;
    };

}

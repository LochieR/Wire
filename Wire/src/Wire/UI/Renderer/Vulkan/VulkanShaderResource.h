#pragma once

#include "VulkanRenderer.h"
#include "Wire/UI/Renderer/ShaderResource.h"

#include <vulkan/vulkan.h>

namespace wire {

    class VulkanShaderResourceLayout : public ShaderResourceLayout
    {
    public:
        VulkanShaderResourceLayout(VulkanRenderer* renderer, const ShaderResourceLayoutInfo& layoutInfo);
        virtual ~VulkanShaderResourceLayout();
        
        VkDescriptorSetLayout getLayout(uint32_t set) const { return m_SetLayouts[set]; }
        const std::vector<VkDescriptorSetLayout>& getLayouts() const { return m_SetLayouts; }
    private:
        VulkanRenderer* m_Renderer = nullptr;
        
        std::vector<VkDescriptorSetLayout> m_SetLayouts;
    };

    class VulkanShaderResource : public ShaderResource
    {
    public:
        VulkanShaderResource(VulkanRenderer* renderer, uint32_t set, ShaderResourceLayout* layout);
        virtual ~VulkanShaderResource();
        
        virtual void update(Texture2D* texture, uint32_t binding, uint32_t index) override;
        virtual void update(Sampler* sampler, uint32_t binding, uint32_t index) override;
        virtual void update(Texture2D* texture, Sampler* sampler, uint32_t binding, uint32_t index) override;
        virtual void update(BufferBase* uniformBuffer, uint32_t binding, uint32_t index) override;
        
        VkDescriptorSet getSet() const { return m_Set; }
    private:
        VulkanRenderer* m_Renderer;

        VkDescriptorSet m_Set;
    };

}

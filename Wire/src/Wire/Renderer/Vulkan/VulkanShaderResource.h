#pragma once

#include "VulkanDevice.h"
#include "Wire/Renderer/ShaderResource.h"

#include <vulkan/vulkan.h>

#include <string>

namespace wire {

    class VulkanShaderResourceLayout : public ShaderResourceLayout
    {
    public:
        VulkanShaderResourceLayout(VulkanDevice* device, const ShaderResourceLayoutInfo& layoutInfo, std::string_view debugName = {});
        virtual ~VulkanShaderResourceLayout();
        
        VkDescriptorSetLayout getLayout(uint32_t set) const { return m_SetLayouts[set]; }
        const std::vector<VkDescriptorSetLayout>& getLayouts() const { return m_SetLayouts; }
    protected:
        virtual void destroy() override;
        virtual void invalidate() noexcept override;
    private:
        VulkanDevice* m_Device = nullptr;
        
        std::string m_DebugName;
        
        std::vector<VkDescriptorSetLayout> m_SetLayouts;
    };

    class VulkanShaderResource : public ShaderResource
    {
    public:
        VulkanShaderResource(VulkanDevice* device, uint32_t set, const std::shared_ptr<ShaderResourceLayout>& layout, std::string_view debugName = {});
        virtual ~VulkanShaderResource();
        
        virtual void update(const std::shared_ptr<Texture2D>& texture, uint32_t binding, uint32_t index) override;
        virtual void update(const std::shared_ptr<Sampler>& sampler, uint32_t binding, uint32_t index) override;
        virtual void update(const std::shared_ptr<Texture2D>& texture, const std::shared_ptr<Sampler>& sampler, uint32_t binding, uint32_t index) override;
        virtual void update(const std::shared_ptr<Buffer>& uniformBuffer, uint32_t binding, uint32_t index) override;
        
        VkDescriptorSet getSet() const { return m_Set; }
    protected:
        virtual void destroy() override;
        virtual void invalidate() noexcept override;
    private:
        VulkanDevice* m_Device;
        
        std::string m_DebugName;

        VkDescriptorSet m_Set;
    };

}

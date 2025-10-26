#pragma once

#include "VulkanDevice.h"

#include <vulkan/vulkan.h>

#include <array>
#include <string>

namespace wire {

    class VulkanGraphicsPipeline : public GraphicsPipeline
    {
    public:
        VulkanGraphicsPipeline(Device* device, const GraphicsPipelineDesc& desc, std::string_view debugName);
        virtual ~VulkanGraphicsPipeline();

        VkPipeline getPipeline() const { return m_Pipeline; }
        VkPipelineLayout getPipelineLayout() const { return m_PipelineLayout; }
    protected:
        virtual void destroy() override;
        virtual void invalidate() noexcept override;
    private:
        VulkanDevice* m_Device;

        std::string m_DebugName;

        std::shared_ptr<RenderPass> m_RenderPass;

        VkShaderModule m_VertexShader, m_PixelShader;
        std::shared_ptr<ShaderResourceLayout> m_ShaderResourceLayout;
        
        VkPipelineLayout m_PipelineLayout;
        VkPipeline m_Pipeline;
    };

    namespace Utils {

        VkShaderStageFlags ConvertShaderType(ShaderType type);
        VkDescriptorType ConvertDescriptorType(ShaderResourceType type);

    }

}

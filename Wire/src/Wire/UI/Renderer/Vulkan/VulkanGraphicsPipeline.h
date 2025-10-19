#pragma once

#include "VulkanRenderer.h"

#include <vulkan/vulkan.h>

#include <array>
#include <string>

namespace wire {

    class VulkanGraphicsPipeline : public GraphicsPipeline
    {
    public:
        VulkanGraphicsPipeline(Renderer* renderer, const GraphicsPipelineDesc& desc, std::string_view debugName);
        virtual ~VulkanGraphicsPipeline();

        VkPipeline getPipeline() const { return m_Pipeline; }
        VkPipelineLayout getPipelineLayout() const { return m_PipelineLayout; }
    private:
        VulkanRenderer* m_Renderer;

        std::string m_DebugName;

        RenderPass* m_RenderPass;

        VkShaderModule m_VertexShader, m_PixelShader;
        ShaderResourceLayout* m_ShaderResourceLayout;
        
        VkPipelineLayout m_PipelineLayout;
        VkPipeline m_Pipeline;
    };

    namespace Utils {

        VkShaderStageFlags ConvertShaderType(ShaderType type);
        VkDescriptorType ConvertDescriptorType(ShaderResourceType type);

    }

}

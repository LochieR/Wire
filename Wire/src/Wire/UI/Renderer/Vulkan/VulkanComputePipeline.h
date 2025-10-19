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

        VkPipeline getPipeline() const { return m_Pipeline; }
        VkPipelineLayout getPipelineLayout() const { return m_Layout; }
    private:
        Renderer* m_Renderer = nullptr;

        std::string m_DebugName;

        const ComputeInputLayout m_InputLayout;

        VkShaderModule m_ComputeShader = nullptr;
        
        VkPipelineLayout m_Layout = nullptr;
        VkPipeline m_Pipeline = nullptr;
    };

}

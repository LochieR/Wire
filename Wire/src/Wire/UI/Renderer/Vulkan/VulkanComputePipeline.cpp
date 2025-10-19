#include "VulkanComputePipeline.h"

#include "VulkanBuffer.h"
#include "VulkanTexture2D.h"
#include "VulkanFramebuffer.h"
#include "VulkanShaderResource.h"
#include "VulkanGraphicsPipeline.h"

#include "Wire/UI/Renderer/ComputePipeline.h"

#include <array>

namespace wire {

    VulkanComputePipeline::VulkanComputePipeline(Renderer* renderer, const ComputePipelineDesc& desc, std::string_view debugName)
        : m_Renderer(renderer), m_InputLayout(desc.Layout), m_DebugName(debugName)
    {
        ShaderCache& cache = renderer->getShaderCache();

        ShaderResult shaderResult = cache.getShaderFromURL(desc.ShaderPath, RendererAPI::Vulkan, false);
        
        WR_ASSERT(!shaderResult.IsGraphics, "compute pipeline shader must be a compute shader");
        
        VkShaderModuleCreateInfo moduleInfo{};
        moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleInfo.codeSize = shaderResult.VertexOrCompute.Bytecode.size();
        moduleInfo.pCode = reinterpret_cast<uint32_t*>(shaderResult.VertexOrCompute.Bytecode.data());
        
        VulkanRenderer* vk = (VulkanRenderer*)renderer;
        m_Renderer = vk;

        VkResult result = vkCreateShaderModule(vk->getDevice(), &moduleInfo, vk->getAllocator(), &m_ComputeShader);
        VK_CHECK(result, "failed to create Vulkan shader module");

        std::string workingDebugName = m_DebugName;
        workingDebugName += " (compute shader module)";
        VK_DEBUG_NAME(vk->getDevice(), SHADER_MODULE, m_ComputeShader, workingDebugName.c_str());

        VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
        computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        computeShaderStageInfo.module = m_ComputeShader;
        computeShaderStageInfo.pName = "CShader";

        std::array shaderStages = { computeShaderStageInfo };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 0;
        dynamicState.pDynamicStates = nullptr;

        const std::vector<PushConstantInfo>& pushConstantInfos = desc.Layout.PushConstantInfos;
        std::vector<VkPushConstantRange> pushConstantRanges;

        for (const auto& pushConstant : pushConstantInfos)
        {
            VkPushConstantRange& range = pushConstantRanges.emplace_back();
            range.size = (uint32_t)pushConstant.Size;
            range.offset = (uint32_t)pushConstant.Offset;
            range.stageFlags = Utils::ConvertShaderType(pushConstant.Shader);
        }

        VulkanShaderResourceLayout* vkShaderResourceLayout = static_cast<VulkanShaderResourceLayout*>(m_InputLayout.ResourceLayout);
        const std::vector<VkDescriptorSetLayout>& setLayouts = vkShaderResourceLayout->getLayouts();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        pipelineLayoutInfo.pSetLayouts = setLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

        result = vkCreatePipelineLayout(vk->getDevice(), &pipelineLayoutInfo, vk->getAllocator(), &m_Layout);
        VK_CHECK(result, "failed to create create Vulkan pipeline layout");

        workingDebugName = m_DebugName;
        workingDebugName += " (pipeline layout)";
        VK_DEBUG_NAME(vk->getDevice(), PIPELINE_LAYOUT, m_Layout, workingDebugName.c_str());

        VkComputePipelineCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        createInfo.layout = m_Layout;
        createInfo.stage = computeShaderStageInfo;
        
        result = vkCreateComputePipelines(vk->getDevice(), nullptr, 1, &createInfo, vk->getAllocator(), &m_Pipeline);
        VK_CHECK(result, "failed to create Vulkan compute pipeline");

        workingDebugName = m_DebugName;
        workingDebugName += " (pipeline)";
        VK_DEBUG_NAME(vk->getDevice(), PIPELINE, m_Pipeline, workingDebugName.c_str());
    }

    VulkanComputePipeline::~VulkanComputePipeline()
    {
        m_Renderer->submitResourceFree([pipeline = m_Pipeline, pipelineLayout = m_Layout, computeShader = m_ComputeShader](Renderer* renderer)
        {
            VulkanRenderer* vk = (VulkanRenderer*)renderer;

            vkDestroyPipeline(vk->getDevice(), pipeline, vk->getAllocator());
            vkDestroyPipelineLayout(vk->getDevice(), pipelineLayout, vk->getAllocator());
            vkDestroyShaderModule(vk->getDevice(), computeShader, vk->getAllocator());
        });
    }

}

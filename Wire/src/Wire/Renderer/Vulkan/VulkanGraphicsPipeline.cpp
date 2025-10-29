#include "VulkanGraphicsPipeline.h"

#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanTexture2D.h"
#include "VulkanRenderPass.h"
#include "VulkanShaderResource.h"

#include "Wire/Core/Assert.h"

#include <vulkan/vulkan.h>

#include <array>
#include <vector>

namespace wire {

    namespace Utils {

        static VkFormat ConvertFormat(ShaderDataType type)
        {
            switch (type)
            {
            case ShaderDataType::Float: return VK_FORMAT_R32_SFLOAT;
            case ShaderDataType::Float2: return VK_FORMAT_R32G32_SFLOAT;
            case ShaderDataType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
            case ShaderDataType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case ShaderDataType::Int: return VK_FORMAT_R32_SINT;
            case ShaderDataType::Int2: return VK_FORMAT_R32G32_SINT;
            case ShaderDataType::Int3: return VK_FORMAT_R32G32B32_SINT;
            case ShaderDataType::Int4: return VK_FORMAT_R32G32B32A32_SINT;
            case ShaderDataType::UInt: return VK_FORMAT_R32_UINT;
            case ShaderDataType::UInt2: return VK_FORMAT_R32G32_UINT;
            case ShaderDataType::UInt3: return VK_FORMAT_R32G32B32_UINT;
            case ShaderDataType::UInt4: return VK_FORMAT_R32G32B32A32_UINT;
            case ShaderDataType::Mat3: return VK_FORMAT_R32G32B32_SFLOAT;
            case ShaderDataType::Mat4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            default:
                break;
            }

            WR_ASSERT(false, "Unknown ShaderDataType!");
            return VkFormat(0);
        }

        VkShaderStageFlags ConvertShaderType(ShaderType type)
        {
            switch (type)
            {
            case ShaderType::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderType::Pixel: return VK_SHADER_STAGE_FRAGMENT_BIT;
            case ShaderType::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
            }

            WR_ASSERT(false, "Unknown ShaderType!");
            return VkShaderStageFlags(0);
        }

        VkDescriptorType ConvertDescriptorType(ShaderResourceType type)
        {
            switch (type)
            {
            case ShaderResourceType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case ShaderResourceType::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case ShaderResourceType::SampledImage: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case ShaderResourceType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
            case ShaderResourceType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case ShaderResourceType::StorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            }

            WR_ASSERT(false, "Unknown ShaderResourceType!");
            return VkDescriptorType(0);
        }

        static VkPrimitiveTopology ConvertPrimitiveTopology(PrimitiveTopology topology)
        {
            switch (topology)
            {
            case PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case PrimitiveTopology::LineList:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PrimitiveTopology::LineStrip:    return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            }

            WR_ASSERT(false, "Unknown ShaderResourceType!");
            return VkPrimitiveTopology(0);
        }

        static VkVertexInputBindingDescription GetBindingDescription(const InputLayout& layout)
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = (uint32_t)layout.Stride;
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions(const InputLayout& layout)
        {
            const auto& elements = layout.VertexBufferLayout;

            std::vector<VkVertexInputAttributeDescription> attributes(elements.size());

            for (uint32_t i = 0; i < elements.size(); i++)
            {
                attributes[i].binding = 0;
                attributes[i].location = i;
                attributes[i].format = ConvertFormat(elements[i].Type);
                attributes[i].offset = (uint32_t)elements[i].Offset;
            }

            return attributes;
        }

    }

    VulkanGraphicsPipeline::VulkanGraphicsPipeline(Device* device, const GraphicsPipelineDesc& desc, std::string_view debugName)
        : m_Device((VulkanDevice*)device), m_RenderPass(desc.RenderPass), m_ShaderResourceLayout(desc.Layout.ResourceLayout), m_DebugName(debugName)
    {
        ShaderCache& cache = device->getShaderCache();

        ShaderResult shaderResult = cache.getShaderFromURL(desc.ShaderPath, RendererAPI::Vulkan, true);

        VkShaderModuleCreateInfo moduleInfo{};
        moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleInfo.codeSize = shaderResult.VertexOrCompute.Bytecode.size();
        moduleInfo.pCode = reinterpret_cast<uint32_t*>(shaderResult.VertexOrCompute.Bytecode.data());

        VulkanDevice* vk = m_Device;

        VkResult result = vkCreateShaderModule(vk->getDevice(), &moduleInfo, vk->getAllocator(), &m_VertexShader);
        VK_CHECK(result, "Failed to create Vulkan shader module!");

        std::string workingDebugName = m_DebugName;
        workingDebugName += " (vertex shader module)";
        VK_DEBUG_NAME(vk->getDevice(), SHADER_MODULE, m_VertexShader, workingDebugName.c_str());

        moduleInfo.codeSize = shaderResult.Pixel.Bytecode.size();
        moduleInfo.pCode = reinterpret_cast<uint32_t*>(shaderResult.Pixel.Bytecode.data());

        result = vkCreateShaderModule(vk->getDevice(), &moduleInfo, vk->getAllocator(), &m_PixelShader);
        VK_CHECK(result, "Failed to create Vulkan shader module!");

        workingDebugName = m_DebugName;
        workingDebugName += " (pixel shader module)";
        VK_DEBUG_NAME(vk->getDevice(), SHADER_MODULE, m_PixelShader, workingDebugName.c_str());

        VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
        vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = m_VertexShader;
        vertexShaderStageInfo.pName = "VShader";

        VkPipelineShaderStageCreateInfo pixelShaderStageInfo{};
        pixelShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pixelShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pixelShaderStageInfo.module = m_PixelShader;
        pixelShaderStageInfo.pName = "PShader";

        std::array shaderStages = { vertexShaderStageInfo, pixelShaderStageInfo };

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        if (desc.Topology == PrimitiveTopology::LineList || desc.Topology == PrimitiveTopology::LineStrip)
        {
            dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
        }

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        if (desc.Layout.VertexBufferLayout.size() != 0)
            WR_ASSERT_OR_ERROR(desc.Layout.Stride != 0, "InputLayout stride has size 0");

        VkVertexInputBindingDescription bindingDescription = Utils::GetBindingDescription(desc.Layout);
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Utils::GetAttributeDescriptions(desc.Layout);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = desc.Layout.VertexBufferLayout.size() != 0 ? 1 : 0;
        vertexInputInfo.pVertexBindingDescriptions = desc.Layout.VertexBufferLayout.size() != 0 ? &bindingDescription : nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = desc.Layout.VertexBufferLayout.size() != 0 ? (uint32_t)attributeDescriptions.size() : 0;
        vertexInputInfo.pVertexAttributeDescriptions = desc.Layout.VertexBufferLayout.size() != 0 ? attributeDescriptions.data() : nullptr;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = Utils::ConvertPrimitiveTopology(desc.Topology);
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.sampleShadingEnable = VK_TRUE;
        multisampling.minSampleShading = 0.2f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = ((VulkanRenderPass*)desc.RenderPass.get())->getBlendAttachmentStates();

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = (uint32_t)colorBlendAttachments.size();
        colorBlending.pAttachments = colorBlendAttachments.data();
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        const std::vector<PushConstantInfo>& pushConstantInfos = desc.Layout.PushConstantInfos;
        std::vector<VkPushConstantRange> pushConstantRanges;

        for (const auto& pushConstant : pushConstantInfos)
        {
            VkPushConstantRange& range = pushConstantRanges.emplace_back();
            range.size = (uint32_t)pushConstant.Size;
            range.offset = (uint32_t)pushConstant.Offset;
            range.stageFlags = Utils::ConvertShaderType(pushConstant.Shader);
        }
        
        VulkanShaderResourceLayout* vkShaderResourceLayout = static_cast<VulkanShaderResourceLayout*>(m_ShaderResourceLayout.get());
        std::vector<VkDescriptorSetLayout> setLayouts;
        if (vkShaderResourceLayout)
            setLayouts = vkShaderResourceLayout->getLayouts();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        pipelineLayoutInfo.pSetLayouts = setLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

        result = vkCreatePipelineLayout(vk->getDevice(), &pipelineLayoutInfo, vk->getAllocator(), &m_PipelineLayout);
        VK_CHECK(result, "Failed to create Vulkan pipeline layout!");

        workingDebugName = m_DebugName;
        workingDebugName += " (pipeline layout)";
        VK_DEBUG_NAME(vk->getDevice(), PIPELINE_LAYOUT, m_PipelineLayout, workingDebugName.c_str());

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = (uint32_t)shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_PipelineLayout;
        pipelineInfo.renderPass = ((VulkanRenderPass*)m_RenderPass.get())->getRenderPass();
        pipelineInfo.subpass = 0;

        result = vkCreateGraphicsPipelines(vk->getDevice(), 0, 1, &pipelineInfo, vk->getAllocator(), &m_Pipeline);
        VK_CHECK(result, "Failed to create Vulkan graphics pipeline!");

        workingDebugName = m_DebugName;
        workingDebugName += " (pipeline)";
        VK_DEBUG_NAME(vk->getDevice(), PIPELINE, m_Pipeline, workingDebugName.c_str());
    }

    VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
    {
        destroy();
    }

    void VulkanGraphicsPipeline::destroy()
    {
        if (m_Valid && m_Device)
        {
            m_Device->submitResourceFree(
                [pipeline = m_Pipeline, pipelineLayout = m_PipelineLayout,
                pixelShader = m_PixelShader, vertexShader = m_VertexShader](Device* device)
                {
                    VulkanDevice* vk = (VulkanDevice*)device;

                    vkDestroyPipeline(vk->getDevice(), pipeline, vk->getAllocator());
                    vkDestroyPipelineLayout(vk->getDevice(), pipelineLayout, vk->getAllocator());
                    vkDestroyShaderModule(vk->getDevice(), pixelShader, vk->getAllocator());
                    vkDestroyShaderModule(vk->getDevice(), vertexShader, vk->getAllocator());
                }
            );
        }
    }

    void VulkanGraphicsPipeline::invalidate() noexcept
    {
        m_Valid = false;
        m_Device = nullptr;
    }

}

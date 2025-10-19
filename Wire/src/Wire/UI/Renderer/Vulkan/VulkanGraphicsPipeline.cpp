#include "VulkanGraphicsPipeline.h"

#include "VulkanBuffer.h"
#include "VulkanRenderer.h"
#include "VulkanTexture2D.h"
#include "VulkanRenderPass.h"

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

    VulkanGraphicsPipeline::VulkanGraphicsPipeline(Renderer* renderer, const GraphicsPipelineDesc& desc, std::string_view debugName)
        : m_RenderPass(desc.RenderPass), m_DebugName(debugName)
    {
        ShaderCache& cache = renderer->getShaderCache();

        ShaderResult shaderResult = cache.getShaderFromURL(desc.ShaderPath, RendererAPI::Vulkan, true);

        VkShaderModuleCreateInfo moduleInfo{};
        moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleInfo.codeSize = shaderResult.VertexOrCompute.Bytecode.size();
        moduleInfo.pCode = reinterpret_cast<uint32_t*>(shaderResult.VertexOrCompute.Bytecode.data());

        VulkanRenderer* vk = (VulkanRenderer*)renderer;
        m_Renderer = vk;

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

        WR_ASSERT_OR_ERROR(desc.Layout.Stride != 0, "InputLayout stride has size 0");

        VkVertexInputBindingDescription bindingDescription = Utils::GetBindingDescription(desc.Layout);
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Utils::GetAttributeDescriptions(desc.Layout);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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

        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        colorBlendAttachments.push_back(colorBlendAttachment);

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

        const std::vector<ShaderResourceInfo>& shaderResourceInfos = desc.Layout.ShaderResources;
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        uint32_t i = 0;
        for (const auto& shaderResource : shaderResourceInfos)
        {
            VkDescriptorType descriptorType = Utils::ConvertDescriptorType(shaderResource.ResourceType);

            VkDescriptorSetLayoutBinding binding{};
            binding.binding = shaderResource.Binding;
            binding.descriptorType = descriptorType;
            binding.descriptorCount = shaderResource.ResourceCount;
            binding.stageFlags = Utils::ConvertShaderType(shaderResource.Shader);
            binding.pImmutableSamplers = nullptr;

            bindings.push_back(binding);

            i++;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = (uint32_t)bindings.size();
        layoutInfo.pBindings = bindings.data();

        result = vkCreateDescriptorSetLayout(vk->getDevice(), &layoutInfo, vk->getAllocator(), &m_SetLayout);
        VK_CHECK(result, "Failed to create Vulkan descriptor set layout!");

        workingDebugName = m_DebugName;
        workingDebugName += " (set layout)";
        VK_DEBUG_NAME(vk->getDevice(), DESCRIPTOR_SET_LAYOUT, m_SetLayout, workingDebugName.c_str());

        std::vector<VkDescriptorSetLayout> setLayouts(WR_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
            setLayouts[i] = m_SetLayout;

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = vk->getDescriptorPool();
        allocInfo.descriptorSetCount = WR_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = setLayouts.data();

        result = vkAllocateDescriptorSets(vk->getDevice(), &allocInfo, m_DescriptorSets.data());
        VK_CHECK(result, "Failed to allocate Vulkan descriptor set!");

        for (size_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
        {
            workingDebugName = m_DebugName;
            workingDebugName += " (frame " + std::to_string(i) + " descriptor set)";
            VK_DEBUG_NAME(vk->getDevice(), DESCRIPTOR_SET, m_DescriptorSets[i], workingDebugName.c_str());
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_SetLayout;
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
        pipelineInfo.renderPass = ((VulkanRenderPass*)m_RenderPass)->getRenderPass();
        pipelineInfo.subpass = 0;

        result = vkCreateGraphicsPipelines(vk->getDevice(), 0, 1, &pipelineInfo, vk->getAllocator(), &m_Pipeline);
        VK_CHECK(result, "Failed to create Vulkan graphics pipeline!");

        workingDebugName = m_DebugName;
        workingDebugName += " (pipeline)";
        VK_DEBUG_NAME(vk->getDevice(), PIPELINE, m_Pipeline, workingDebugName.c_str());
    }

    VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
    {
        m_Renderer->submitResourceFree(
            [pipeline = m_Pipeline, pipelineLayout = m_PipelineLayout, setLayout = m_SetLayout,
            pixelShader = m_PixelShader, vertexShader = m_VertexShader](Renderer* renderer)
            {
                VulkanRenderer* vk = (VulkanRenderer*)renderer;

                vkDestroyPipeline(vk->getDevice(), pipeline, vk->getAllocator());
                vkDestroyPipelineLayout(vk->getDevice(), pipelineLayout, vk->getAllocator());
                vkDestroyDescriptorSetLayout(vk->getDevice(), setLayout, vk->getAllocator());
                vkDestroyShaderModule(vk->getDevice(), pixelShader, vk->getAllocator());
                vkDestroyShaderModule(vk->getDevice(), vertexShader, vk->getAllocator());
            }
        );
    }

    void VulkanGraphicsPipeline::updateDescriptor(Texture2D* texture, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanTexture2D* vkTexture = (VulkanTexture2D*)texture;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = vkTexture->getImageView();
        imageInfo.sampler = nullptr;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = getDescriptorSet();
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanGraphicsPipeline::updateDescriptor(Sampler* sampler, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanSampler* vkSampler = (VulkanSampler*)sampler;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = nullptr;
        imageInfo.sampler = vkSampler->getSampler();

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = getDescriptorSet();
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanGraphicsPipeline::updateDescriptor(Texture2D* texture, Sampler* sampler, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanTexture2D* vkTexture = (VulkanTexture2D*)texture;
        VulkanSampler* vkSampler = (VulkanSampler*)sampler;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = vkTexture->getImageView();
        imageInfo.sampler = vkSampler->getSampler();

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = getDescriptorSet();
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanGraphicsPipeline::updateDescriptor(BufferBase* uniformBuffer, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanBufferBase* vkBuffer = (VulkanBufferBase*)uniformBuffer;

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = vkBuffer->getBuffer();
        bufferInfo.range = vkBuffer->getSize();
        bufferInfo.offset = 0;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = getDescriptorSet();
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanGraphicsPipeline::updateFrameDescriptor(Texture2D* texture, uint32_t frameIndex, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanTexture2D* vkTexture = (VulkanTexture2D*)texture;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = vkTexture->getImageView();
        imageInfo.sampler = nullptr;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSets[frameIndex];
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanGraphicsPipeline::updateFrameDescriptor(Sampler* sampler, uint32_t frameIndex, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanSampler* vkSampler = (VulkanSampler*)sampler;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = nullptr;
        imageInfo.sampler = vkSampler->getSampler();

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSets[frameIndex];
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanGraphicsPipeline::updateFrameDescriptor(Texture2D* texture, Sampler* sampler, uint32_t frameIndex, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanTexture2D* vkTexture = (VulkanTexture2D*)texture;
        VulkanSampler* vkSampler = (VulkanSampler*)sampler;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = vkTexture->getImageView();
        imageInfo.sampler = vkSampler->getSampler();

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSets[frameIndex];
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanGraphicsPipeline::updateFrameDescriptor(BufferBase* uniformBuffer, uint32_t frameIndex, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanBufferBase* vkBuffer = (VulkanBufferBase*)uniformBuffer;

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = vkBuffer->getBuffer();
        bufferInfo.range = vkBuffer->getSize();
        bufferInfo.offset = 0;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSets[frameIndex];
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanGraphicsPipeline::updateAllDescriptors(Texture2D* texture, uint32_t binding, uint32_t index)
    {
        for (uint32_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
            updateFrameDescriptor(texture, i, binding, index);
    }

    void VulkanGraphicsPipeline::updateAllDescriptors(Sampler* sampler, uint32_t binding, uint32_t index)
    {
        for (uint32_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
            updateFrameDescriptor(sampler, i, binding, index);
    }

    void VulkanGraphicsPipeline::updateAllDescriptors(Texture2D* texture, Sampler* sampler, uint32_t binding, uint32_t index)
    {
        for (uint32_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
            updateFrameDescriptor(texture, sampler, i, binding, index);
    }

    void VulkanGraphicsPipeline::updateAllDescriptors(BufferBase* uniformBuffer, uint32_t binding, uint32_t index)
    {
        for (uint32_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
            updateFrameDescriptor(uniformBuffer, i, binding, index);
    }

}

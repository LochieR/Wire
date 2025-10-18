#include "VulkanComputePipeline.h"

#include "VulkanBuffer.h"
#include "VulkanTexture2D.h"
#include "VulkanFramebuffer.h"
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

        const std::vector<ShaderResourceInfo>& shaderResourceInfos = desc.Layout.ShaderResources;
        std::vector<std::vector<VkDescriptorSetLayoutBinding>> bindings(desc.Layout.SlotCount);

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

            bindings[shaderResource.Set].push_back(binding);

            i++;
        }

        m_SetLayouts.resize(desc.Layout.SlotCount);
        m_DescriptorSets.resize(desc.Layout.SlotCount);

        for (size_t set = 0; set < desc.Layout.SlotCount; set++)
        {
            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = (uint32_t)bindings[set].size();
            layoutInfo.pBindings = bindings[set].data();

            result = vkCreateDescriptorSetLayout(vk->getDevice(), &layoutInfo, vk->getAllocator(), &m_SetLayouts[set]);
            VK_CHECK(result, "Failed to create Vulkan descriptor set layout!");

            workingDebugName = m_DebugName;
            workingDebugName += " (slot " + std::to_string(set) + " set layout)";
            VK_DEBUG_NAME(vk->getDevice(), DESCRIPTOR_SET_LAYOUT, m_SetLayouts[set], workingDebugName.c_str());
            
            std::vector<VkDescriptorSetLayout> setLayouts(WR_FRAMES_IN_FLIGHT);
            for (size_t j = 0; j < WR_FRAMES_IN_FLIGHT; j++)
                setLayouts[j] = m_SetLayouts[set];

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = vk->getDescriptorPool();
            allocInfo.descriptorSetCount = (uint32_t)setLayouts.size();
            allocInfo.pSetLayouts = setLayouts.data();

            m_DescriptorSets[set].resize(desc.Layout.SetsPerSlot);
            for (size_t index = 0; index < desc.Layout.SetsPerSlot; index++)
            {
                result = vkAllocateDescriptorSets(vk->getDevice(), &allocInfo, m_DescriptorSets[set][index].data());
                VK_CHECK(result, "Failed to allocate Vulkan descriptor set!");

                for (size_t frame = 0; frame < WR_FRAMES_IN_FLIGHT; frame++)
                {
                    workingDebugName = m_DebugName;
                    workingDebugName += " (slot " + std::to_string(set) + " index " + std::to_string(index) + " frame " + std::to_string(frame) + " descriptor set)";
                    VK_DEBUG_NAME(vk->getDevice(), DESCRIPTOR_SET, m_DescriptorSets[set][index][frame], workingDebugName.c_str());
                }
            }
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = (uint32_t)m_SetLayouts.size();
        pipelineLayoutInfo.pSetLayouts = m_SetLayouts.data();
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
        m_Renderer->submitResourceFree([pipeline = m_Pipeline, pipelineLayout = m_Layout, setLayouts = m_SetLayouts, computeShader = m_ComputeShader](Renderer* renderer)
        {
            VulkanRenderer* vk = (VulkanRenderer*)renderer;

            vkDestroyPipeline(vk->getDevice(), pipeline, vk->getAllocator());
            vkDestroyPipelineLayout(vk->getDevice(), pipelineLayout, vk->getAllocator());
            for (VkDescriptorSetLayout setLayout : setLayouts)
                vkDestroyDescriptorSetLayout(vk->getDevice(), setLayout, vk->getAllocator());
            vkDestroyShaderModule(vk->getDevice(), computeShader, vk->getAllocator());
        });
    }

    void VulkanComputePipeline::updateDescriptor(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanTexture2D* vkTexture = (VulkanTexture2D*)texture;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = vkTexture->getImageView();
        imageInfo.sampler = nullptr;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = getDescriptorSet(set, setIndex);
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateDescriptor(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t mipLevel, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanTexture2D* vkTexture = (VulkanTexture2D*)texture;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = vkTexture->getMip(mipLevel);
        imageInfo.sampler = nullptr;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = getDescriptorSet(set, setIndex);
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateDescriptor(Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanSampler* vkSampler = (VulkanSampler*)sampler;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = nullptr;
        imageInfo.sampler = vkSampler->getSampler();

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = getDescriptorSet(set, setIndex);
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateDescriptor(Texture2D* texture, Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index)
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
        descriptorWrite.dstSet = getDescriptorSet(set, setIndex);
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateDescriptor(BufferBase* buffer, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanBufferBase* vkBuffer = (VulkanBufferBase*)buffer;

        VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        
        for (const auto& resource : m_InputLayout.ShaderResources)
        {
            if (resource.Binding == binding)
            {
                WR_ASSERT(resource.ResourceType == ShaderResourceType::StorageBuffer || resource.ResourceType == ShaderResourceType::UniformBuffer, "corresponding shader resource in layout must be uniform or storage buffer for updateDescriptor* (binding = {})", binding);
                type = Utils::ConvertDescriptorType(resource.ResourceType);
            }
        }

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = vkBuffer->getBuffer();
        bufferInfo.range = vkBuffer->getSize();
        bufferInfo.offset = 0;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = getDescriptorSet(set, setIndex);
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = type;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateDescriptor(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanFramebuffer* vkFramebuffer = (VulkanFramebuffer*)storageImage;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = vkFramebuffer->getColorView();
        imageInfo.sampler = nullptr;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = getDescriptorSet(set, setIndex);
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateDescriptor(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t mipLevel, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanFramebuffer* vkFramebuffer = (VulkanFramebuffer*)storageImage;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = vkFramebuffer->getMip(mipLevel);
        imageInfo.sampler = nullptr;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = getDescriptorSet(set, setIndex);
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateFrameDescriptor(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanTexture2D* vkTexture = (VulkanTexture2D*)texture;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = vkTexture->getImageView();
        imageInfo.sampler = nullptr;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSets[set][setIndex][frameIndex];
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateFrameDescriptor(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t mipLevel, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanTexture2D* vkTexture = (VulkanTexture2D*)texture;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = vkTexture->getMip(mipLevel);
        imageInfo.sampler = nullptr;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSets[set][setIndex][frameIndex];
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateFrameDescriptor(Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanSampler* vkSampler = (VulkanSampler*)sampler;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = nullptr;
        imageInfo.sampler = vkSampler->getSampler();

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSets[set][setIndex][frameIndex];
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateFrameDescriptor(Texture2D* texture, Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index)
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
        descriptorWrite.dstSet = m_DescriptorSets[set][setIndex][frameIndex];
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateFrameDescriptor(BufferBase* buffer, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanBufferBase* vkBuffer = (VulkanBufferBase*)buffer;

        VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        for (const auto& resource : m_InputLayout.ShaderResources)
        {
            if (resource.Binding == binding)
            {
                WR_ASSERT(resource.ResourceType == ShaderResourceType::StorageBuffer || resource.ResourceType == ShaderResourceType::UniformBuffer, "corresponding shader resource in layout must be uniform or storage buffer for updateDescriptor* (binding = {})", binding);
                type = Utils::ConvertDescriptorType(resource.ResourceType);
            }
        }

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = vkBuffer->getBuffer();
        bufferInfo.range = vkBuffer->getSize();
        bufferInfo.offset = 0;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSets[set][setIndex][frameIndex];
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = type;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateFrameDescriptor(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanFramebuffer* vkFramebuffer = (VulkanFramebuffer*)storageImage;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = vkFramebuffer->getColorView();
        imageInfo.sampler = nullptr;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSets[set][setIndex][frameIndex];
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateFrameDescriptor(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t frameIndex, uint32_t mipLevel, uint32_t binding, uint32_t index)
    {
        VulkanRenderer* vk = (VulkanRenderer*)m_Renderer;
        VulkanFramebuffer* vkFramebuffer = (VulkanFramebuffer*)storageImage;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = vkFramebuffer->getMip(mipLevel);
        imageInfo.sampler = nullptr;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSets[set][setIndex][frameIndex];
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanComputePipeline::updateAllDescriptors(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index)
    {
        for (uint32_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
            updateFrameDescriptor(texture, set, setIndex, i, binding, index);
    }

    void VulkanComputePipeline::updateAllDescriptors(Texture2D* texture, uint32_t set, uint32_t setIndex, uint32_t mipLevel, uint32_t binding, uint32_t index)
    {
        for (uint32_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
            updateFrameDescriptor(texture, set, setIndex, i, mipLevel, binding, index);
    }

    void VulkanComputePipeline::updateAllDescriptors(Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index)
    {
        for (uint32_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
            updateFrameDescriptor(sampler, set, setIndex, i, binding, index);
    }

    void VulkanComputePipeline::updateAllDescriptors(Texture2D* texture, Sampler* sampler, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index)
    {
        for (uint32_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
            updateFrameDescriptor(texture, sampler, set, setIndex, i, binding, index);
    }

    void VulkanComputePipeline::updateAllDescriptors(BufferBase* buffer, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index)
    {
        for (uint32_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
            updateFrameDescriptor(buffer, set, setIndex, i, binding, index);
    }

    void VulkanComputePipeline::updateAllDescriptors(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t binding, uint32_t index)
    {
        for (uint32_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
            updateFrameDescriptor(storageImage, set, setIndex, i, binding, index);
    }

    void VulkanComputePipeline::updateAllDescriptors(Framebuffer* storageImage, uint32_t set, uint32_t setIndex, uint32_t mipLevel, uint32_t binding, uint32_t index)
    {
        for (uint32_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
            updateFrameDescriptor(storageImage, set, setIndex, i, mipLevel, binding, index);
    }

}

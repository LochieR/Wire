#include "VulkanShaderResource.h"

#include "VulkanBuffer.h"
#include "VulkanTexture2D.h"
#include "VulkanGraphicsPipeline.h"

namespace wire {

    VulkanShaderResourceLayout::VulkanShaderResourceLayout(VulkanDevice* device, const ShaderResourceLayoutInfo& layoutInfo)
        : m_Device(device)
    {
        for (const auto& set : layoutInfo.Sets)
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings;
            
            for (const auto& resource : set.Resources)
            {
                VkDescriptorSetLayoutBinding binding{};
                binding.binding = resource.Binding;
                binding.descriptorType = Utils::ConvertDescriptorType(resource.Type);
                binding.descriptorCount = resource.ArrayCount;
                binding.stageFlags = Utils::ConvertShaderType(resource.Stage);
                binding.pImmutableSamplers = nullptr;
                
                bindings.push_back(binding);
            }
            
            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();
            
            VkDescriptorSetLayout setLayout;
            VkResult result = vkCreateDescriptorSetLayout(m_Device->getDevice(), &createInfo, m_Device->getAllocator(), &setLayout);
            VK_CHECK(result, "failed to create Vulkan descriptor set layout");
            
            m_SetLayouts.push_back(setLayout);
        }
    }

    VulkanShaderResourceLayout::~VulkanShaderResourceLayout()
    {
        m_Device->submitResourceFree([setLayouts = m_SetLayouts](Device* device)
        {
            VulkanDevice* vk = (VulkanDevice*)device;

            for (VkDescriptorSetLayout setLayout : setLayouts)
                vkDestroyDescriptorSetLayout(vk->getDevice(), setLayout, vk->getAllocator());
        });
    }

    VulkanShaderResource::VulkanShaderResource(VulkanDevice* device, uint32_t set, ShaderResourceLayout* layout)
        : m_Device(device)
    {
        VulkanShaderResourceLayout* vkResourceLayout = static_cast<VulkanShaderResourceLayout*>(layout);
        
        VkDescriptorSetLayout setLayout = vkResourceLayout->getLayout(set);
        
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = device->getDescriptorPool();
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &setLayout;
        
        VkResult result = vkAllocateDescriptorSets(device->getDevice(), &allocInfo, &m_Set);
        VK_CHECK(result, "failed to allocate Vulkan descriptor sets");
    }

    VulkanShaderResource::~VulkanShaderResource()
    {
    }

    void VulkanShaderResource::update(Texture2D* texture, uint32_t binding, uint32_t index)
    {
        VulkanDevice* vk = (VulkanDevice*)m_Device;
        VulkanTexture2D* vkTexture = (VulkanTexture2D*)texture;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = vkTexture->getImageView();
        imageInfo.sampler = nullptr;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_Set;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanShaderResource::update(Sampler* sampler, uint32_t binding, uint32_t index)
    {
        VulkanDevice* vk = (VulkanDevice*)m_Device;
        VulkanSampler* vkSampler = (VulkanSampler*)sampler;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = nullptr;
        imageInfo.sampler = vkSampler->getSampler();

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_Set;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanShaderResource::update(Texture2D* texture, Sampler* sampler, uint32_t binding, uint32_t index)
    {
        VulkanDevice* vk = (VulkanDevice*)m_Device;
        VulkanTexture2D* vkTexture = (VulkanTexture2D*)texture;
        VulkanSampler* vkSampler = (VulkanSampler*)sampler;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = vkTexture->getImageView();
        imageInfo.sampler = vkSampler->getSampler();

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_Set;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanShaderResource::update(BufferBase* uniformBuffer, uint32_t binding, uint32_t index)
    {
        VulkanDevice* vk = (VulkanDevice*)m_Device;
        VulkanBufferBase* vkBuffer = (VulkanBufferBase*)uniformBuffer;

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = vkBuffer->getBuffer();
        bufferInfo.range = vkBuffer->getSize();
        bufferInfo.offset = 0;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_Set;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = index;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(vk->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

}

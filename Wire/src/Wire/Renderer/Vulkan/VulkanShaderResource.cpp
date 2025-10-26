#include "VulkanShaderResource.h"

#include "VulkanBuffer.h"
#include "VulkanTexture2D.h"
#include "VulkanGraphicsPipeline.h"

namespace wire {

    VulkanShaderResourceLayout::VulkanShaderResourceLayout(VulkanDevice* device, const ShaderResourceLayoutInfo& layoutInfo, std::string_view debugName)
        : m_Device(device), m_DebugName(debugName)
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
            
            std::string debugName = m_DebugName;
            debugName += " (" + std::to_string(m_SetLayouts.size() - 1) + ")";
            VK_DEBUG_NAME(m_Device->getDevice(), DESCRIPTOR_SET_LAYOUT, setLayout, debugName.c_str());
        }
    }

    VulkanShaderResourceLayout::~VulkanShaderResourceLayout()
    {
        destroy();
    }

    void VulkanShaderResourceLayout::destroy()
    {
        if (m_Valid && m_Device)
        {
            m_Device->submitResourceFree([setLayouts = m_SetLayouts](Device* device)
            {
                VulkanDevice* vk = (VulkanDevice*)device;

                for (VkDescriptorSetLayout setLayout : setLayouts)
                    vkDestroyDescriptorSetLayout(vk->getDevice(), setLayout, vk->getAllocator());
            });
        }
    }

    void VulkanShaderResourceLayout::invalidate() noexcept
    {
        m_Valid = false;
        m_Device = nullptr;
    }

    VulkanShaderResource::VulkanShaderResource(VulkanDevice* device, uint32_t set, const std::shared_ptr<ShaderResourceLayout>& layout, std::string_view debugName)
        : m_Device(device), m_DebugName(debugName)
    {
        VulkanShaderResourceLayout* vkResourceLayout = static_cast<VulkanShaderResourceLayout*>(layout.get());
        
        VkDescriptorSetLayout setLayout = vkResourceLayout->getLayout(set);
        
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = device->getDescriptorPool();
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &setLayout;
        
        VkResult result = vkAllocateDescriptorSets(device->getDevice(), &allocInfo, &m_Set);
        VK_CHECK(result, "failed to allocate Vulkan descriptor sets");
        
        VK_DEBUG_NAME(device->getDevice(), DESCRIPTOR_SET, m_Set, m_DebugName.c_str());
    }

    VulkanShaderResource::~VulkanShaderResource()
    {
        destroy();
    }

    void VulkanShaderResource::update(const std::shared_ptr<Texture2D>& texture, uint32_t binding, uint32_t index)
    {
        if (!m_Valid)
        {
            WR_ASSERT_OR_WARN(false, "ShaderResource used after destroyed ({})", m_DebugName);
            return;
        }
        
        VulkanDevice* vk = (VulkanDevice*)m_Device;
        VulkanTexture2D* vkTexture = (VulkanTexture2D*)texture.get();

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

    void VulkanShaderResource::update(const std::shared_ptr<Sampler>& sampler, uint32_t binding, uint32_t index)
    {
        if (!m_Valid)
        {
            WR_ASSERT_OR_WARN(false, "ShaderResource used after destroyed ({})", m_DebugName);
            return;
        }
        
        VulkanDevice* vk = (VulkanDevice*)m_Device;
        VulkanSampler* vkSampler = (VulkanSampler*)sampler.get();

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

    void VulkanShaderResource::update(const std::shared_ptr<Texture2D>& texture, const std::shared_ptr<Sampler>& sampler, uint32_t binding, uint32_t index)
    {
        if (!m_Valid)
        {
            WR_ASSERT_OR_WARN(false, "ShaderResource used after destroyed ({})", m_DebugName);
            return;
        }
        
        VulkanDevice* vk = (VulkanDevice*)m_Device;
        VulkanTexture2D* vkTexture = (VulkanTexture2D*)texture.get();
        VulkanSampler* vkSampler = (VulkanSampler*)sampler.get();

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

    void VulkanShaderResource::update(const std::shared_ptr<Buffer>& uniformBuffer, uint32_t binding, uint32_t index)
    {
        if (!m_Valid)
        {
            WR_ASSERT_OR_WARN(false, "ShaderResource used after destroyed ({})", m_DebugName);
            return;
        }
        
        VulkanDevice* vk = (VulkanDevice*)m_Device;
        VulkanBuffer* vkBuffer = (VulkanBuffer*)uniformBuffer.get();

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

    void VulkanShaderResource::destroy()
    {
        if (m_Valid && m_Device)
        {
        }
    }

    void VulkanShaderResource::invalidate() noexcept
    {
        m_Valid = false;
        m_Device = nullptr;
    }

}

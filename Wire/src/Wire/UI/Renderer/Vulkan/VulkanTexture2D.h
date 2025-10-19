#pragma once

#include "VulkanRenderer.h"

#include <vulkan/vulkan.h>

#include <string>
#include <filesystem>

namespace wire {

    class VulkanTexture2D : public Texture2D
    {
    public:
        VulkanTexture2D(Renderer* renderer, const std::filesystem::path& path, std::string_view debugName);
        VulkanTexture2D(Renderer* renderer, uint32_t* data, uint32_t width, uint32_t height, std::string_view debugName);
        VulkanTexture2D(VkImage image, VkDeviceMemory memory, VkImageView view, const std::vector<VkImageView>& mips, uint32_t width, uint32_t height);
        virtual ~VulkanTexture2D();

        virtual uint32_t getWidth() const override { return m_Width; }
        virtual uint32_t getHeight() const override { return m_Height; }

        virtual UUID getUUID() const { return m_UUID; }

        VkImageView getImageView() const { return m_ImageView; }
        VkImageView getMip(uint32_t level) const { return m_Mips[level]; }
    private:
        Renderer* m_Renderer = nullptr;

        std::string m_DebugName;

        UUID m_UUID;

        VkImage m_Image = nullptr;
        VkDeviceMemory m_Memory = nullptr;
        VkImageView m_ImageView = nullptr;

        std::vector<VkImageView> m_Mips;

        uint32_t m_Width = 1, m_Height = 1;

        bool m_NoFree = false;
    };

    class VulkanSampler : public Sampler
    {
    public:
        VulkanSampler(Renderer* renderer, const SamplerDesc& desc, std::string_view debugName);
        virtual ~VulkanSampler();

        VkSampler getSampler() const { return m_Sampler; }
    private:
        Renderer* m_Renderer = nullptr;
        SamplerDesc m_Desc;

        std::string m_DebugName;

        VkSampler m_Sampler = nullptr;
    };

}

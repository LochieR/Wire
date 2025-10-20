#pragma once

#include "VulkanDevice.h"

#include "Wire/Renderer/Framebuffer.h"

#include <vulkan/vulkan.h>

#include <string>

namespace wire {

    class VulkanFramebuffer : public Framebuffer
    {
    public:
        VulkanFramebuffer(Device* device, const FramebufferDesc& desc, std::string_view debugName = {});
        virtual ~VulkanFramebuffer();

        virtual void resize(const glm::vec2& extent) override;
        virtual glm::vec2 getExtent() const override { return m_Desc.Extent; }
        virtual uint32_t getNumMips() const override { return m_Desc.MipCount; }

        virtual Texture2D* asTexture2D() const override;

        VkImage getColorImage() const { return m_Image; }
        VkImage getDepthImage() const { return m_DepthImage; }
        VkImageView getColorView() const { return m_View; }
        VkImageView getDepthView() const { return m_DepthView; }

        VkImageView getMip(uint32_t level) const { return m_Mips.empty() && level == 0 ? m_View : m_Mips[level]; }
    private:
        Device* m_Device;
        FramebufferDesc m_Desc;

        std::string m_DebugName;

        VkImage m_Image = nullptr;
        VkImageView m_View = nullptr;
        VkDeviceMemory m_Memory = nullptr;

        std::vector<VkImageView> m_Mips;

        VkImage m_DepthImage = nullptr;
        VkImageView m_DepthView = nullptr;
        VkDeviceMemory m_DepthMemory = nullptr;
    };

}


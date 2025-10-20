#pragma once

#include "VulkanDevice.h"

#include "Wire/Renderer/Swapchain.h"

#include <vulkan/vulkan.h>

namespace wire {

    class VulkanSwapchain : public Swapchain
    {
    public:
        VulkanSwapchain(VulkanDevice* device, const SwapchainInfo& info, std::string_view debugName);
        virtual ~VulkanSwapchain();

        virtual bool acquireNextImage(uint32_t& imageIndex) override;

        virtual void recreateSwapchain() override;

        virtual glm::vec2 getExtent() const override { return { (float)m_Extent.width, (float)m_Extent.height }; }
        virtual uint32_t getImageCount() const override { return m_SwapchainImageCount; }

        VkSwapchainKHR getSwapchain() const { return m_Swapchain; }
        const std::vector<VkImageView> getAttachmentViews(uint32_t imageIndex) const;

        std::vector<VkImageView> getAttachmentViews(const std::vector<uint32_t>& indices, uint32_t imageIndex) const;

        VkFormat getDefaultColorAttachmentFormat() const;
        VkFormat getDefaultDepthAttachmentFormat() const;

        virtual const SwapchainInfo& getInfo() const override { return m_Info; }
    private:
        void disposeSwapchain();
    private:
        VulkanDevice* m_Device = nullptr;
        SwapchainInfo m_Info;

        std::string m_DebugName;

        VkSwapchainKHR m_Swapchain = nullptr;
        VkFormat m_SwapchainImageFormat;
        VkExtent2D m_Extent;

        uint32_t m_SwapchainImageCount;

        struct Attachment
        {
            std::vector<VkImage> Images;
            std::vector<VkImageView> Views;
            std::vector<VkDeviceMemory> Memory;
            VkFormat Format;
            VkImageUsageFlags Usage;

            Attachment() = default;
            Attachment(const Attachment& other)
            {
                Images = std::vector<VkImage>(other.Images.size());
                Views = std::vector<VkImageView>(other.Views.size());
                Memory = std::vector<VkDeviceMemory>(other.Memory.size());
                Format = other.Format;
                Usage = other.Usage;

                for (size_t i = 0; i < Images.size(); i++)
                    Images[i] = other.Images[i];
                for (size_t i = 0; i < Views.size(); i++)
                    Views[i] = other.Views[i];
                for (size_t i = 0; i < Memory.size(); i++)
                    Memory[i] = other.Memory[i];
            }
        };

        std::vector<Attachment> m_Attachments;
    };

    namespace Utils {

        VkFormat ConvertFormat(AttachmentFormat format);

    }

}

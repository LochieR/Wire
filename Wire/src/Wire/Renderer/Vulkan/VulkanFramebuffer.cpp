#include "VulkanFramebuffer.h"

#include "VulkanSwapchain.h"
#include "VulkanTexture2D.h"

namespace wire {

    namespace Utils {

        static VkImageUsageFlags GetImageUsage(AttachmentUsage usage)
        {
            return (VkImageUsageFlags)usage;
        }

    }

    VulkanFramebuffer::VulkanFramebuffer(Device* device, const FramebufferDesc& desc, std::string_view debugName)
        : m_Device(device), m_Desc(desc), m_DebugName(debugName)
    {
        resize(desc.Extent);
    }

    VulkanFramebuffer::~VulkanFramebuffer()
    {
        destroy();
    }

    void VulkanFramebuffer::resize(const glm::vec2& extent)
    {
        if (!m_Valid)
        {
            WR_ASSERT_OR_WARN(false, "Framebuffer used after destroyed ({})", m_DebugName);
            return;
        }
        
        m_Device->submitResourceFree([image = m_Image, view = m_View, memory = m_Memory, mips = m_Mips,
            depthImage = m_DepthImage, depthView = m_DepthView, depthMemory = m_DepthMemory](Device* device)
        {
            VulkanDevice* vk = (VulkanDevice*)device;

            if (depthView)
                vkDestroyImageView(vk->getDevice(), depthView, vk->getAllocator());
            if (depthImage)
                vkDestroyImage(vk->getDevice(), depthImage, vk->getAllocator());
            if (depthMemory)
                vkFreeMemory(vk->getDevice(), depthMemory, vk->getAllocator());

            for (VkImageView view : mips)
                vkDestroyImageView(vk->getDevice(), view, vk->getAllocator());

            if (view)
                vkDestroyImageView(vk->getDevice(), view, vk->getAllocator());
            if (image)
                vkDestroyImage(vk->getDevice(), image, vk->getAllocator());
            if (memory)
                vkFreeMemory(vk->getDevice(), memory, vk->getAllocator());
        });

        m_Desc.Extent = extent;

        VulkanDevice* vk = (VulkanDevice*)m_Device;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = (uint32_t)extent.x;
        imageInfo.extent.height = (uint32_t)extent.y;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = m_Desc.MipCount;
        imageInfo.arrayLayers = 1;
        imageInfo.format = Utils::ConvertFormat(m_Desc.Format);
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = Utils::GetImageUsage(m_Desc.Usage);
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateImage(vk->getDevice(), &imageInfo, vk->getAllocator(), &m_Image);
        VK_CHECK(result, "failed to create Vulkan image");

        std::string imageDebugName = m_DebugName + " (color image)";
        VK_DEBUG_NAME(vk->getDevice(), IMAGE, m_Image, imageDebugName.c_str());

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(vk->getDevice(), m_Image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = Utils::FindMemoryType(vk->getPhysicalDevice(), memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        result = vkAllocateMemory(vk->getDevice(), &allocInfo, vk->getAllocator(), &m_Memory);
        VK_CHECK(result, "failed to allocate Vulkan memory");

        std::string memoryDebugName = m_DebugName + " (memory)";
        VK_DEBUG_NAME(vk->getDevice(), DEVICE_MEMORY, m_Memory, memoryDebugName.c_str());

        result = vkBindImageMemory(vk->getDevice(), m_Image, m_Memory, 0);
        VK_CHECK(result, "failed to bind Vulkan image memory");

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_Image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = imageInfo.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(vk->getDevice(), &viewInfo, vk->getAllocator(), &m_View);
        VK_CHECK(result, "failed to create Vulkan image view");

        std::string viewDebugName = m_DebugName + " (main view)";
        VK_DEBUG_NAME(vk->getDevice(), IMAGE_VIEW, m_View, viewDebugName.c_str());

        if (m_Desc.MipCount != 1)
        {
            m_Mips.resize(m_Desc.MipCount);

            for (uint32_t i = 0; i < m_Desc.MipCount; i++)
            {
                viewInfo.subresourceRange.baseMipLevel = i;
                viewInfo.subresourceRange.levelCount = 1;

                result = vkCreateImageView(vk->getDevice(), &viewInfo, vk->getAllocator(), &m_Mips[i]);
                VK_CHECK(result, "failed to create Vulkan image view");

                viewDebugName = m_DebugName + " (view mip " + std::to_string(i) + ")";
                VK_DEBUG_NAME(vk->getDevice(), IMAGE_VIEW, m_Mips[i], viewDebugName.c_str());
            }
        }

        if (m_Desc.HasDepth)
        {
            imageInfo.format = Utils::ConvertFormat(m_Desc.DepthFormat);
            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

            result = vkCreateImage(vk->getDevice(), &imageInfo, vk->getAllocator(), &m_DepthImage);
            VK_CHECK(result, "failed to create Vulkan image");

            std::string depthImageDebugName = m_DebugName + " (depth image)";
            VK_DEBUG_NAME(vk->getDevice(), IMAGE, m_DepthImage, depthImageDebugName.c_str());

            vkGetImageMemoryRequirements(vk->getDevice(), m_DepthImage, &memRequirements);
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = Utils::FindMemoryType(vk->getPhysicalDevice(), memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            result = vkAllocateMemory(vk->getDevice(), &allocInfo, vk->getAllocator(), &m_DepthMemory);
            VK_CHECK(result, "failed to allocate Vulkan memory");

            std::string depthMemoryDebugName = m_DebugName + " (depth memory)";
            VK_DEBUG_NAME(vk->getDevice(), DEVICE_MEMORY, m_DepthMemory, depthMemoryDebugName.c_str());

            result = vkBindImageMemory(vk->getDevice(), m_DepthImage, m_DepthMemory, 0);
            VK_CHECK(result, "failed to bind Vulkan image memory");

            viewInfo.image = m_DepthImage;
            viewInfo.format = imageInfo.format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            result = vkCreateImageView(vk->getDevice(), &viewInfo, vk->getAllocator(), &m_DepthView);
            VK_CHECK(result, "failed to create Vulkan image view");

            std::string depthViewDebugName = m_DebugName + " (depth view)";
            VK_DEBUG_NAME(vk->getDevice(), IMAGE_VIEW, m_DepthView, depthViewDebugName.c_str());
        }

        if (m_Desc.Layout != AttachmentLayout::Undefined)
        {
            std::shared_ptr<VulkanFramebuffer> framebuffer = m_Device->getResource<VulkanFramebuffer>(this);
            
            CommandList list = m_Device->beginSingleTimeCommands();
            list.imageMemoryBarrier(framebuffer, AttachmentLayout::Undefined, m_Desc.Layout, 0, m_Desc.MipCount);
            m_Device->endSingleTimeCommands(list);
        }
    }

    Texture2D* VulkanFramebuffer::asTexture2D() const
    {
        if (!m_Valid)
        {
            WR_ASSERT_OR_WARN(false, "Framebuffer used after destroyed ({})", m_DebugName);
            return nullptr;
        }
        
        return new VulkanTexture2D(m_Image, m_Memory, m_View, m_Mips, (uint32_t)m_Desc.Extent.x, (uint32_t)m_Desc.Extent.y);
    }

    void VulkanFramebuffer::destroy()
    {
        if (m_Valid && m_Device)
        {
            m_Device->submitResourceFree([image = m_Image, view = m_View, memory = m_Memory, mips = m_Mips,
                depthImage = m_DepthImage, depthView = m_DepthView, depthMemory = m_DepthMemory](Device* device)
            {
                VulkanDevice* vk = (VulkanDevice*)device;

                if (depthView)
                    vkDestroyImageView(vk->getDevice(), depthView, vk->getAllocator());
                if (depthImage)
                    vkDestroyImage(vk->getDevice(), depthImage, vk->getAllocator());
                if (depthMemory)
                    vkFreeMemory(vk->getDevice(), depthMemory, vk->getAllocator());

                for (VkImageView view : mips)
                    vkDestroyImageView(vk->getDevice(), view, vk->getAllocator());

                if (view)
                    vkDestroyImageView(vk->getDevice(), view, vk->getAllocator());
                if (image)
                    vkDestroyImage(vk->getDevice(), image, vk->getAllocator());
                if (memory)
                    vkFreeMemory(vk->getDevice(), memory, vk->getAllocator());
            });
        }
    }

    void VulkanFramebuffer::invalidate() noexcept
    {
        m_Valid = false;
        m_Device = nullptr;
    }

}

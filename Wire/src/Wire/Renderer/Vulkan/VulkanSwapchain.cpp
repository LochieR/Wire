#include "VulkanSwapchain.h"

#include "VulkanRenderPass.h"

#include "Wire/Core/Application.h"

namespace wire {

    namespace Utils {

        VkFormat ConvertFormat(AttachmentFormat format)
        {
            switch (format)
            {
            case AttachmentFormat::R8_UInt: return VK_FORMAT_R8_UINT;
            case AttachmentFormat::R16_UInt: return VK_FORMAT_R16_UINT;
            case AttachmentFormat::R32_UInt: return VK_FORMAT_R32_UINT;
            case AttachmentFormat::R64_UInt: return VK_FORMAT_R64_UINT;
            case AttachmentFormat::R8_SInt: return VK_FORMAT_R8_SINT;
            case AttachmentFormat::R16_SInt: return VK_FORMAT_R16_SINT;
            case AttachmentFormat::R32_SInt: return VK_FORMAT_R32_SINT;
            case AttachmentFormat::R64_SInt: return VK_FORMAT_R64_SINT;
            case AttachmentFormat::R8_UNorm: return VK_FORMAT_R8_UNORM;
            case AttachmentFormat::R16_UNorm: return VK_FORMAT_R16_UNORM;
            case AttachmentFormat::R32_SFloat: return VK_FORMAT_R32_SFLOAT;
            case AttachmentFormat::BGRA8_UNorm: return VK_FORMAT_B8G8R8A8_UNORM;
            case AttachmentFormat::RGBA8_UNorm: return VK_FORMAT_R8G8B8A8_UNORM;
            case AttachmentFormat::RGBA16_SFloat: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case AttachmentFormat::RGBA32_SFloat: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case AttachmentFormat::D32_SFloat: return VK_FORMAT_D32_SFLOAT;
            default:
                WR_ASSERT(false, "Unknown attachment format");
                return (VkFormat)0;
            }
        }

        static bool IsPresentModeSupported(const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR mode)
        {
            for (const auto& availablePresentMode : availablePresentModes)
            {
                if (availablePresentMode == mode)
                    return true;
            }

            return false;
        }

    }

    VulkanSwapchain::VulkanSwapchain(VulkanDevice* device, const SwapchainInfo& info, std::string_view debugName)
        : m_Device(device), m_Info(info), m_DebugName(debugName)
    {
        recreateSwapchain();
    }

    VulkanSwapchain::~VulkanSwapchain()
    {
        disposeSwapchain();
    }

    bool VulkanSwapchain::acquireNextImage(uint32_t& imageIndex)
    {
        VkSemaphore currentSemaphore = m_Device->getCurrentImageAvailableSemaphore();
        VkResult result = vkAcquireNextImageKHR(m_Device->getDevice(), m_Swapchain, std::numeric_limits<uint32_t>::max(), currentSemaphore, nullptr, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || Application::get().wasWindowResized())
        {
            Application::get().resetWindowResized();

            recreateSwapchain();

            m_Device->submitResourceFree([currentSemaphore](Device* device)
            {
                VulkanDevice* vk = (VulkanDevice*)device;

                vkDestroySemaphore(vk->getDevice(), currentSemaphore, vk->getAllocator());
            });

            VkSemaphore newSemaphore;

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            result = vkCreateSemaphore(m_Device->getDevice(), &semaphoreInfo, m_Device->getAllocator(), &newSemaphore);
            VK_CHECK(result, "Failed to create Vulkan semaphore!");

            m_Device->setImageAvailableSemaphore(m_Device->getFrameIndex(), newSemaphore);

            return false;
        }
        VK_CHECK(result, "Failed to acquire next Vulkan swapchain image!");

        return true;
    }

    void VulkanSwapchain::recreateSwapchain()
    {
        disposeSwapchain();

        VkSwapchainKHR oldSwapchain = m_Swapchain; // device shouldn't call the resource free queue before now so still valid

        m_Swapchain = nullptr;
        m_Attachments.clear();

        SwapchainSupportDetails swapchainSupport = Utils::QuerySwapchainSupport(m_Device->getPhysicalDevice(), m_Device->getSurface());
        VkSurfaceFormatKHR surfaceFormat = Utils::ChooseSwapSurfaceFormat(swapchainSupport.Formats);
        VkExtent2D extent = Utils::ChooseSwapExtent(Application::get().getWindow(), swapchainSupport.Capabilities);

        VkPresentModeKHR presentMode;

        switch (m_Info.PresentMode)
        {
        case PresentMode::SwapchainDefault:
        case PresentMode::MailboxOrFifo:
            presentMode = Utils::ChooseSwapPresentMode(swapchainSupport.PresentModes);
            break;
        case PresentMode::Mailbox:
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            WR_ASSERT(Utils::IsPresentModeSupported(swapchainSupport.PresentModes, presentMode), "Mailbox present mode is not supported!");
            break;
        case PresentMode::Fifo:
            presentMode = VK_PRESENT_MODE_FIFO_KHR;
            WR_ASSERT(Utils::IsPresentModeSupported(swapchainSupport.PresentModes, presentMode), "FIFO present mode is not supported!");
            break;
        case PresentMode::Immediate:
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            WR_ASSERT(Utils::IsPresentModeSupported(swapchainSupport.PresentModes, presentMode), "Immediate present mode is not supported!");
            break;
        }

        m_SwapchainImageFormat = surfaceFormat.format;
        m_Extent = extent;

        m_SwapchainImageCount = swapchainSupport.Capabilities.minImageCount + 1;

        if (swapchainSupport.Capabilities.maxImageCount > 0 && m_SwapchainImageCount > swapchainSupport.Capabilities.maxImageCount)
            m_SwapchainImageCount = swapchainSupport.Capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_Device->getSurface();
        createInfo.minImageCount = m_SwapchainImageCount;
        createInfo.imageFormat = m_SwapchainImageFormat;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.oldSwapchain = oldSwapchain;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        QueueFamilyIndices indices = Utils::FindQueueFamilies(m_Device->getPhysicalDevice(), m_Device->getSurface());
        uint32_t queueFamilyIndices[] = { indices.GraphicsFamily, indices.PresentFamily };

        if (indices.GraphicsFamily != indices.PresentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapchainSupport.Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        VkResult result = vkCreateSwapchainKHR(m_Device->getDevice(), &createInfo, m_Device->getAllocator(), &m_Swapchain);
        VK_CHECK(result, "Failed to create Vulkan swapchain!");

        std::string workingDebugName = m_DebugName + " (swapchain)";
        VK_DEBUG_NAME(m_Device->getDevice(), SWAPCHAIN_KHR, m_Swapchain, workingDebugName.c_str());

        vkGetSwapchainImagesKHR(m_Device->getDevice(), m_Swapchain, &m_SwapchainImageCount, nullptr);
        std::vector<VkImage> swapchainImages(m_SwapchainImageCount);
        vkGetSwapchainImagesKHR(m_Device->getDevice(), m_Swapchain, &m_SwapchainImageCount, swapchainImages.data());

        for (size_t i = 0; i < m_SwapchainImageCount; i++)
        {
            workingDebugName = m_DebugName + " (swapchain image " + std::to_string(i) + ")";
            VK_DEBUG_NAME(m_Device->getDevice(), IMAGE, swapchainImages[i], workingDebugName.c_str());
        }

        for (size_t attachmentIndex = 0; attachmentIndex < m_Info.Attachments.size(); attachmentIndex++)
        {
            AttachmentFormat format = m_Info.Attachments[attachmentIndex];
            Attachment attachment{};

            switch (format)
            {
            case AttachmentFormat::SwapchainColorDefault:
            {
                attachment.Format = surfaceFormat.format;
                attachment.Images = swapchainImages;
                attachment.Views.resize(attachment.Images.size());
                attachment.Memory.clear();
                attachment.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                for (size_t i = 0; i < attachment.Images.size(); i++)
                {
                    VkImageViewCreateInfo viewInfo{};
                    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    viewInfo.image = attachment.Images[i];
                    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    viewInfo.format = attachment.Format;
                    viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
                    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    viewInfo.subresourceRange.baseMipLevel = 0;
                    viewInfo.subresourceRange.levelCount = 1;
                    viewInfo.subresourceRange.baseArrayLayer = 0;
                    viewInfo.subresourceRange.layerCount = 1;

                    result = vkCreateImageView(m_Device->getDevice(), &viewInfo, m_Device->getAllocator(), &attachment.Views[i]);
                    VK_CHECK(result, "Failed to create Vulkan image view");

                    workingDebugName = m_DebugName + " (swapchain view " + std::to_string(i) + ")";
                    VK_DEBUG_NAME(m_Device->getDevice(), IMAGE_VIEW, attachment.Views[i], workingDebugName.c_str());
                }

                break;
            }
            case AttachmentFormat::SwapchainDepthDefault:
            {
                attachment.Format = Utils::FindDepthFormat(m_Device->getPhysicalDevice());
                attachment.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

                attachment.Images.resize(m_SwapchainImageCount);
                attachment.Views.resize(m_SwapchainImageCount);
                attachment.Memory.resize(m_SwapchainImageCount);

                for (size_t i = 0; i < m_SwapchainImageCount; i++)
                {
                    VkImageCreateInfo imageInfo{};
                    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    imageInfo.imageType = VK_IMAGE_TYPE_2D;
                    imageInfo.extent.width = extent.width;
                    imageInfo.extent.height = extent.height;
                    imageInfo.extent.depth = 1;
                    imageInfo.mipLevels = 1;
                    imageInfo.arrayLayers = 1;
                    imageInfo.format = attachment.Format;
                    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    imageInfo.usage = attachment.Usage;
                    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                    result = vkCreateImage(m_Device->getDevice(), &imageInfo, m_Device->getAllocator(), &attachment.Images[i]);
                    VK_CHECK(result, "Failed to create Vulkan image!");

                    workingDebugName = m_DebugName + " (depth image attachment " + std::to_string(attachmentIndex) + " index " + std::to_string(i) + ")";
                    VK_DEBUG_NAME(m_Device->getDevice(), IMAGE, attachment.Images[i], workingDebugName.c_str());

                    VkMemoryRequirements memRequirements;
                    vkGetImageMemoryRequirements(m_Device->getDevice(), attachment.Images[i], &memRequirements);

                    VkMemoryAllocateInfo allocInfo{};
                    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                    allocInfo.allocationSize = memRequirements.size;
                    allocInfo.memoryTypeIndex = Utils::FindMemoryType(m_Device->getPhysicalDevice(), memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                    result = vkAllocateMemory(m_Device->getDevice(), &allocInfo, m_Device->getAllocator(), &attachment.Memory[i]);
                    VK_CHECK(result, "Failed to allocate Vulkan memory!");

                    workingDebugName = m_DebugName + " (depth memory attachment " + std::to_string(attachmentIndex) + " index " + std::to_string(i) + ")";
                    VK_DEBUG_NAME(m_Device->getDevice(), DEVICE_MEMORY, attachment.Memory[i], workingDebugName.c_str());

                    result = vkBindImageMemory(m_Device->getDevice(), attachment.Images[i], attachment.Memory[i], 0);
                    VK_CHECK(result, "Failed to bind Vulkan image to memory!");

                    VkImageViewCreateInfo viewInfo{};
                    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    viewInfo.image = attachment.Images[i];
                    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    viewInfo.format = attachment.Format;
                    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                    viewInfo.subresourceRange.baseMipLevel = 0;
                    viewInfo.subresourceRange.levelCount = 1;
                    viewInfo.subresourceRange.baseArrayLayer = 0;
                    viewInfo.subresourceRange.layerCount = 1;

                    result = vkCreateImageView(m_Device->getDevice(), &viewInfo, m_Device->getAllocator(), &attachment.Views[i]);
                    VK_CHECK(result, "Failed to create Vulkan image view!");

                    workingDebugName = m_DebugName + " (depth view attachment " + std::to_string(attachmentIndex) + " index " + std::to_string(i) + ")";
                    VK_DEBUG_NAME(m_Device->getDevice(), IMAGE_VIEW, attachment.Views[i], workingDebugName.c_str());
                }

                break;
            }
            default:
            {
                attachment.Format = Utils::ConvertFormat(format);
                attachment.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                attachment.Images.resize(m_SwapchainImageCount);
                attachment.Views.resize(m_SwapchainImageCount);
                attachment.Memory.resize(m_SwapchainImageCount);

                for (size_t i = 0; i < m_SwapchainImageCount; i++)
                {
                    VkImageCreateInfo imageInfo{};
                    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    imageInfo.imageType = VK_IMAGE_TYPE_2D;
                    imageInfo.extent.width = extent.width;
                    imageInfo.extent.height = extent.height;
                    imageInfo.extent.depth = 1;
                    imageInfo.mipLevels = 1;
                    imageInfo.arrayLayers = 1;
                    imageInfo.format = attachment.Format;
                    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    imageInfo.usage = attachment.Usage;
                    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                    result = vkCreateImage(m_Device->getDevice(), &imageInfo, m_Device->getAllocator(), &attachment.Images[i]);
                    VK_CHECK(result, "Failed to create Vulkan image!");

                    workingDebugName = m_DebugName + " (image attachment " + std::to_string(attachmentIndex) + " index " + std::to_string(i) + ")";
                    VK_DEBUG_NAME(m_Device->getDevice(), IMAGE, attachment.Images[i], workingDebugName.c_str());

                    VkImageViewCreateInfo viewInfo{};
                    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    viewInfo.image = attachment.Images[i];
                    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    viewInfo.format = attachment.Format;
                    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    viewInfo.subresourceRange.baseMipLevel = 0;
                    viewInfo.subresourceRange.levelCount = 1;
                    viewInfo.subresourceRange.baseArrayLayer = 0;
                    viewInfo.subresourceRange.layerCount = 1;

                    VkMemoryRequirements memRequirements;
                    vkGetImageMemoryRequirements(m_Device->getDevice(), attachment.Images[i], &memRequirements);

                    VkMemoryAllocateInfo allocInfo{};
                    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                    allocInfo.allocationSize = memRequirements.size;
                    allocInfo.memoryTypeIndex = Utils::FindMemoryType(m_Device->getPhysicalDevice(), memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                    result = vkAllocateMemory(m_Device->getDevice(), &allocInfo, m_Device->getAllocator(), &attachment.Memory[i]);
                    VK_CHECK(result, "Failed to allocate Vulkan memory!");

                    workingDebugName = m_DebugName + " (memory attachment " + std::to_string(attachmentIndex) + " index " + std::to_string(i) + ")";
                    VK_DEBUG_NAME(m_Device->getDevice(), DEVICE_MEMORY, attachment.Memory[i], workingDebugName.c_str());

                    result = vkBindImageMemory(m_Device->getDevice(), attachment.Images[i], attachment.Memory[i], 0);
                    VK_CHECK(result, "Failed to bind Vulkan image to memory!");

                    result = vkCreateImageView(m_Device->getDevice(), &viewInfo, m_Device->getAllocator(), &attachment.Views[i]);
                    VK_CHECK(result, "Failed to create Vulkan image view!");

                    workingDebugName = m_DebugName + " (view attachment " + std::to_string(attachmentIndex) + " index " + std::to_string(i) + ")";
                    VK_DEBUG_NAME(m_Device->getDevice(), IMAGE_VIEW, attachment.Views[i], workingDebugName.c_str());
                }
                
                break;
            }
            }

            m_Attachments.push_back(attachment);
        }
    }

    const std::vector<VkImageView> VulkanSwapchain::getAttachmentViews(uint32_t imageIndex) const
    {
        std::vector<VkImageView> views;

        for (const auto& attachment : m_Attachments)
        {
            views.push_back(attachment.Views[imageIndex]);
        }

        return views;
    }

    std::vector<VkImageView> VulkanSwapchain::getAttachmentViews(const std::vector<uint32_t>& indices, uint32_t imageIndex) const
    {
        std::vector<VkImageView> result;
        result.reserve(indices.size());
        for (uint32_t idx : indices)
            result.push_back(m_Attachments[idx].Views[imageIndex]);
        return result;
    }

    VkFormat VulkanSwapchain::getDefaultColorAttachmentFormat() const
    {
        SwapchainSupportDetails swapchainSupport = Utils::QuerySwapchainSupport(m_Device->getPhysicalDevice(), m_Device->getSurface());
        return Utils::ChooseSwapSurfaceFormat(swapchainSupport.Formats).format;
    }

    VkFormat VulkanSwapchain::getDefaultDepthAttachmentFormat() const
    {
        return Utils::FindDepthFormat(m_Device->getPhysicalDevice());
    }

    void VulkanSwapchain::disposeSwapchain()
    {
        VkSwapchainKHR oldSwapchain = m_Swapchain;
        std::vector<Attachment> oldAttachments(m_Attachments.size());
        
        for (size_t i = 0; i < m_Attachments.size(); i++)
            oldAttachments[i] = Attachment(m_Attachments[i]);

        m_Device->submitResourceFree([oldSwapchain, oldAttachments](Device* device)
        {
            VulkanDevice* vk = static_cast<VulkanDevice*>(device);

            if (oldSwapchain)
                vkDestroySwapchainKHR(vk->getDevice(), oldSwapchain, vk->getAllocator());
            for (const auto& attachment : oldAttachments)
            {
                for (VkImageView view : attachment.Views)
                {
                    if (view)
                        vkDestroyImageView(vk->getDevice(), view, vk->getAllocator());
                }

                if (!attachment.Memory.empty())
                {
                    for (size_t i = 0; i < attachment.Images.size(); i++)
                    {
                        if (attachment.Images[i])
                            vkDestroyImage(vk->getDevice(), attachment.Images[i], vk->getAllocator());
                        if (attachment.Memory[i])
                            vkFreeMemory(vk->getDevice(), attachment.Memory[i], vk->getAllocator());
                    }
                }
            }
        });
    }

}

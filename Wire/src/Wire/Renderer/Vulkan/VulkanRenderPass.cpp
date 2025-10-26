#include "VulkanRenderPass.h"

#include "VulkanSwapchain.h"

#include <array>

namespace wire {

    namespace Utils {

        VkImageLayout GetImageLayout(AttachmentLayout type)
        {
            switch (type)
            {
            case AttachmentLayout::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
            case AttachmentLayout::General: return VK_IMAGE_LAYOUT_GENERAL;
            case AttachmentLayout::ShaderReadOnly: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            case AttachmentLayout::Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            case AttachmentLayout::Color: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case AttachmentLayout::Depth: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            case AttachmentLayout::TransferSrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            case AttachmentLayout::TransferDst: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            default:
                WR_ASSERT(false, "Unknown attachment usage");
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }
        }

        static std::vector<VkAttachmentReference> GetColorAttachments(const std::vector<VkAttachmentReference>& refs)
        {
            std::vector<VkAttachmentReference> attachments;

            for (const auto& ref : refs)
            {
                if (ref.layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                    attachments.push_back(ref);
            }

            return attachments;
        }

        static VkAttachmentReference* GetDepthAttachment(std::vector<VkAttachmentReference>& refs)
        {
            for (auto& ref : refs)
            {
                if (ref.layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL || ref.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
                    return &ref;
            }

            return nullptr;
        }

        static bool HasDepthAttachment(const std::vector<VkAttachmentReference>& refs)
        {
            for (const auto& ref : refs)
            {
                if (ref.layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL || ref.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
                    return true;
            }

            return false;
        }

    }

    VulkanRenderPass::VulkanRenderPass(VulkanDevice* device, const std::shared_ptr<VulkanSwapchain>& swapchain, const RenderPassDesc& desc, std::string_view debugName)
        : m_Device(device), m_Swapchain(swapchain), m_Desc(desc), m_DebugName(debugName)
    {
        recreate();
    }

    VulkanRenderPass::VulkanRenderPass(VulkanDevice* device, const std::shared_ptr<VulkanFramebuffer>& framebuffer, const RenderPassDesc& desc, std::string_view debugName)
        : m_Device(device), m_Framebuffer(framebuffer), m_Desc(desc), m_DebugName(debugName)
    {
        recreate();
    }

    VulkanRenderPass::~VulkanRenderPass()
    {
        destroy();
    }

    void VulkanRenderPass::recreate()
    {
        if (!m_Valid)
        {
            WR_ASSERT_OR_WARN(false, "RenderPass used after destroyed ({})", m_DebugName);
            return;
        }
        
        dispose();

        std::vector<VkAttachmentDescription> attachments = createAttachmentDescriptions();
        std::vector<VkAttachmentReference> refs = createAttachmentRefs();

        std::vector<VkAttachmentReference> colorRefs = Utils::GetColorAttachments(refs);
        VkAttachmentReference* depthRef = Utils::GetDepthAttachment(refs);

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = (uint32_t)colorRefs.size();
        subpass.pColorAttachments = colorRefs.data();
        subpass.pDepthStencilAttachment = depthRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        if (Utils::HasDepthAttachment(refs))
        {
            dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = (uint32_t)attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VkResult result = vkCreateRenderPass(m_Device->getDevice(), &renderPassInfo, m_Device->getAllocator(), &m_RenderPass);
        VK_CHECK(result, "Failed to create Vulkan render pass!");

        std::string renderPassName = m_DebugName + " (render pass)";
        VK_DEBUG_NAME(m_Device->getDevice(), RENDER_PASS, m_RenderPass, renderPassName.c_str());

        if (m_Swapchain)
        {
            for (auto& rpAttachment : m_Desc.Attachments)
            {
                bool found = false;
                for (uint32_t i = 0; i < m_Swapchain->getInfo().Attachments.size(); i++)
                {
                    if (m_Swapchain->getInfo().Attachments[i] == rpAttachment.Format)
                    {
                        m_AttachmentIndices.push_back(i);
                        found = true;
                        break;
                    }
                }

                WR_ASSERT(found, "Attachment in RenderPass is not present in Swapchain!");
            }
        }

        recreateFramebuffers();
    }

    void VulkanRenderPass::recreateFramebuffers()
    {
        if (!m_Valid)
        {
            WR_ASSERT_OR_WARN(false, "RenderPass used after destroyed ({})", m_DebugName);
            return;
        }
        
        m_Device->submitResourceFree([framebuffers = m_Framebuffers, renderPass = m_RenderPass](Device* device)
        {
            VulkanDevice* vk = (VulkanDevice*)device;

            for (VkFramebuffer framebuffer : framebuffers)
                vkDestroyFramebuffer(vk->getDevice(), framebuffer, vk->getAllocator());
        });

        if (m_Swapchain)
        {
            m_Framebuffers.resize(m_Swapchain->getImageCount());

            for (uint32_t i = 0; i < m_Swapchain->getImageCount(); i++)
            {
                std::vector<VkImageView> attachments = m_Swapchain->getAttachmentViews(m_AttachmentIndices, i);

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = m_RenderPass;
                framebufferInfo.attachmentCount = (uint32_t)attachments.size();
                framebufferInfo.pAttachments = attachments.data();
                framebufferInfo.width = (uint32_t)m_Swapchain->getExtent().x;
                framebufferInfo.height = (uint32_t)m_Swapchain->getExtent().y;
                framebufferInfo.layers = 1;

                VkResult result = vkCreateFramebuffer(m_Device->getDevice(), &framebufferInfo, m_Device->getAllocator(), &m_Framebuffers[i]);
                VK_CHECK(result, "Failed to create Vulkan framebuffer!");

                std::string framebufferName = m_DebugName + " (framebuffer " + std::to_string(i) + ")";
                VK_DEBUG_NAME(m_Device->getDevice(), FRAMEBUFFER, m_Framebuffers[i], framebufferName.c_str());
            }
        }
        else
        {
            m_Framebuffers.resize(1);

            std::array<VkImageView, 2> attachments = { m_Framebuffer->getColorView(), m_Framebuffer->getDepthView() };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_RenderPass;
            framebufferInfo.attachmentCount = (uint32_t)attachments.size();
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = (uint32_t)m_Framebuffer->getExtent().x;
            framebufferInfo.height = (uint32_t)m_Framebuffer->getExtent().y;
            framebufferInfo.layers = 1;

            VkResult result = vkCreateFramebuffer(m_Device->getDevice(), &framebufferInfo, m_Device->getAllocator(), &m_Framebuffers[0]);
            VK_CHECK(result, "failed to create Vulkan framebuffer");

            std::string framebufferName = m_DebugName + " (framebuffer)";
            VK_DEBUG_NAME(m_Device->getDevice(), FRAMEBUFFER, m_Framebuffers[0], framebufferName.c_str());
        }
    }

    void VulkanRenderPass::destroy()
    {
        if (m_Valid && m_Device)
        {
            dispose();
        }
    }

    void VulkanRenderPass::invalidate() noexcept
    {
        m_Valid = false;
        m_Device = nullptr;
    }

    void VulkanRenderPass::dispose()
    {
        m_Device->submitResourceFree([framebuffers = m_Framebuffers, renderPass = m_RenderPass](Device* device)
        {
            VulkanDevice* vk = (VulkanDevice*)device;

            for (VkFramebuffer framebuffer : framebuffers)
                vkDestroyFramebuffer(vk->getDevice(), framebuffer, vk->getAllocator());

            if (renderPass)
                vkDestroyRenderPass(vk->getDevice(), renderPass, vk->getAllocator());
        });
    }

    std::vector<VkAttachmentDescription> VulkanRenderPass::createAttachmentDescriptions()
    {
        std::vector<VkAttachmentDescription> result;

        for (const auto& attachment : m_Desc.Attachments)
        {
            VkAttachmentDescription desc{};

            if (m_Swapchain && attachment.Format == AttachmentFormat::SwapchainColorDefault)
                desc.format = m_Swapchain->getDefaultColorAttachmentFormat();
            else if (m_Swapchain && attachment.Format == AttachmentFormat::SwapchainDepthDefault)
                desc.format = m_Swapchain->getDefaultDepthAttachmentFormat();
            else
                desc.format = Utils::ConvertFormat(attachment.Format);
            desc.samples = (VkSampleCountFlagBits)attachment.Samples;
            desc.loadOp = (VkAttachmentLoadOp)attachment.LoadOp;
            desc.storeOp = (VkAttachmentStoreOp)attachment.StoreOp;
            desc.stencilLoadOp = (VkAttachmentLoadOp)attachment.StencilLoadOp;
            desc.stencilStoreOp = (VkAttachmentStoreOp)attachment.StencilStoreOp;
            desc.initialLayout = Utils::GetImageLayout(attachment.PreviousAttachmentUsage);
            desc.finalLayout = Utils::GetImageLayout(attachment.Usage);
            
            result.push_back(desc);
        }

        return result;
    }

    std::vector<VkAttachmentReference> VulkanRenderPass::createAttachmentRefs()
    {
        std::vector<VkAttachmentReference> refs;

        for (uint32_t i = 0; i < m_Desc.Attachments.size(); i++)
        {
            VkAttachmentReference ref{};
            ref.attachment = i;
            
            if (m_Desc.Attachments[i].Format != AttachmentFormat::D32_SFloat && m_Desc.Attachments[i].Format != AttachmentFormat::SwapchainDepthDefault)
                ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            else
                ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            refs.push_back(ref);
        }

        return refs;
    }

}

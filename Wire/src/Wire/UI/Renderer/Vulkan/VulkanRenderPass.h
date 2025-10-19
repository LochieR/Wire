#pragma once

#include "VulkanRenderer.h"
#include "VulkanSwapchain.h"
#include "VulkanFramebuffer.h"
#include "Wire/UI/Renderer/RenderPass.h"

#include <vulkan/vulkan.h>

namespace wire {

    class VulkanRenderPass : public RenderPass
    {
    public:
        VulkanRenderPass(VulkanRenderer* renderer, VulkanSwapchain* swapchain, const RenderPassDesc& desc, std::string_view debugName);
        VulkanRenderPass(VulkanRenderer* renderer, VulkanFramebuffer* framebuffer, const RenderPassDesc& desc, std::string_view debugName);
        virtual ~VulkanRenderPass();

        virtual void recreate() override;
        virtual void recreateFramebuffers() override;

        VkRenderPass getRenderPass() const { return m_RenderPass; }
        const std::vector<VkFramebuffer>& getFramebuffers() const { return m_Framebuffers; }
        VkFramebuffer getFramebuffer(uint32_t imageIndex) const { return m_Framebuffer ? m_Framebuffers[0] : m_Framebuffers[imageIndex]; }
    private:
        void dispose();

        std::vector<VkAttachmentDescription> createAttachmentDescriptions();
        std::vector<VkAttachmentReference> createAttachmentRefs();
    private:
        VulkanRenderer* m_Renderer = nullptr;
        VulkanSwapchain* m_Swapchain = nullptr;
        VulkanFramebuffer* m_Framebuffer = nullptr;
        RenderPassDesc m_Desc;

        std::string m_DebugName;

        VkRenderPass m_RenderPass = nullptr;
        std::vector<VkFramebuffer> m_Framebuffers;
        std::vector<uint32_t> m_AttachmentIndices;
    };

}

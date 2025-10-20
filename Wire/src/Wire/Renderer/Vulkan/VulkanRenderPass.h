#pragma once

#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanFramebuffer.h"
#include "Wire/Renderer/RenderPass.h"

#include <vulkan/vulkan.h>

namespace wire {

    class VulkanRenderPass : public RenderPass
    {
    public:
        VulkanRenderPass(VulkanDevice* device, VulkanSwapchain* swapchain, const RenderPassDesc& desc, std::string_view debugName);
        VulkanRenderPass(VulkanDevice* device, VulkanFramebuffer* framebuffer, const RenderPassDesc& desc, std::string_view debugName);
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
        VulkanDevice* m_Device = nullptr;
        VulkanSwapchain* m_Swapchain = nullptr;
        VulkanFramebuffer* m_Framebuffer = nullptr;
        RenderPassDesc m_Desc;

        std::string m_DebugName;

        VkRenderPass m_RenderPass = nullptr;
        std::vector<VkFramebuffer> m_Framebuffers;
        std::vector<uint32_t> m_AttachmentIndices;
    };

}

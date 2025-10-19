#include "VulkanRenderer.h"

#include "VulkanFont.h"
#include "VulkanBuffer.h"
#include "VulkanSwapchain.h"
#include "VulkanTexture2D.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanShaderResource.h"
#include "VulkanComputePipeline.h"
#include "VulkanGraphicsPipeline.h"

#include "Wire/Core/Core.h"
#include "Wire/Core/Application.h"

#include <set>
#include <array>
#include <vector>
#include <iostream>
#include <algorithm>

namespace wire {
    
    constexpr bool s_EnableValidationLayers
#ifdef WR_DEBUG
        = true;
#else
        = false;
#endif

    const static std::vector<const char*> s_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef WR_PLATFORM_MAC
        "VK_KHR_portability_subset"
#endif
    };
    const static std::vector<const char*> s_ValidationLayers = { "VK_LAYER_KHRONOS_validation"
    };

    namespace Utils {

        static std::vector<const char*> GetRequiredExtensions()
        {
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

            if (s_EnableValidationLayers)
            {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            return extensions;
        }

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData)
        {
            if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
                return VK_FALSE;

            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
                WR_ERROR("[Validation Layer] {}", pCallbackData->pMessage);
            //else
            //    std::cerr << "\033[38;5;208m[Validation Layer] " << pCallbackData->pMessage << "\u001b[0m" << std::endl;

            return VK_FALSE;
        }

        static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
        {
            auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr)
            {
                return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
            }
            else
            {
                return VK_ERROR_EXTENSION_NOT_PRESENT;
            }
        }

        static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr)
            {
                func(instance, debugMessenger, pAllocator);
            }
        }

        static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
        {
            createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = DebugCallback;
        }

        static bool CheckValidationLayerSupport()
        {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            for (const char* layerName : s_ValidationLayers)
            {
                bool layerFound = false;

                for (const auto& layerProperties : availableLayers)
                {
                    if (strcmp(layerName, layerProperties.layerName) == 0)
                    {
                        layerFound = true;
                        break;
                    }
                }

                if (!layerFound)
                {
                    return false;
                }
            }

            return true;
        }

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
            QueueFamilyIndices indices{};

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            int i = 0;
            for (const auto& queueFamily : queueFamilies)
            {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    indices.GraphicsFamily = i;

                VkBool32 presentSupport = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

                if (presentSupport)
                    indices.PresentFamily = i;

                if (indices.IsComplete())
                    break;

                i++;
            }

            return indices;
        }

        static bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
        {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            std::set<std::string> requiredExtensions;

            for (const char* ext : s_DeviceExtensions)
                requiredExtensions.insert(std::string(ext));

            for (const auto& extension : availableExtensions)
            {
                requiredExtensions.erase(std::string(extension.extensionName));
            }

            return requiredExtensions.empty();
        }

        SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
            SwapchainSupportDetails details;

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

            if (formatCount != 0)
            {
                details.Formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
            }

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

            if (presentModeCount != 0)
            {
                details.PresentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
            }

            return details;
        }

        uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
        {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
            {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                    return i;
            }

            WR_ASSERT(false, "Failed to find suitable memory type!");
            return 0;
        }

        static bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
            QueueFamilyIndices indices = FindQueueFamilies(device, surface);

            bool extensionsSupported = CheckDeviceExtensionSupport(device);

            bool swapchainAdequate = false;
            if (extensionsSupported)
            {
                SwapchainSupportDetails swapChainSupport = QuerySwapchainSupport(device, surface);
                swapchainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
            }

            VkPhysicalDeviceFeatures supportedFeatures;
            vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

            return
                indices.IsComplete() &&
                extensionsSupported &&
                swapchainAdequate &&
                supportedFeatures.sampleRateShading &&
                supportedFeatures.samplerAnisotropy;
        }

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
        {
            for (const auto& availableFormat : availableFormats)
            {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    return availableFormat;
                }
            }

            return availableFormats[0];
        }

        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
        {
            for (const auto& availablePresentMode : availablePresentModes)
            {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    return availablePresentMode;
                }
            }

            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D ChooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities)
        {
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            {
                return capabilities.currentExtent;
            }
            else
            {
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);

                VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                };

                actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                return actualExtent;
            }
        }

        static VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
        {
            for (VkFormat format : candidates)
            {
                VkFormatProperties properties;
                vkGetPhysicalDeviceFormatProperties(device, format, &properties);

                if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
                    return format;
                else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
                    return format;
            }

            WR_ASSERT(false, "Failed to find supported Vulkan format!");
            return candidates[0];
        }

        VkFormat FindDepthFormat(VkPhysicalDevice device)
        {
            return FindSupportedFormat(
                device,
                { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );
        }

        static bool HasStencilComponent(VkFormat format)
        {
            return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
        }
    
    }

    VulkanRenderer::VulkanRenderer(const RendererDesc& desc, const SwapchainDesc& swapchainDesc)
    {
        m_FrameIndex = 0;
        m_ResourceFreeQueue.resize(WR_FRAMES_IN_FLIGHT);

        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createCommandPool();
        createDescriptorPool();

        m_Swapchain = createSwapchain(swapchainDesc);

        createSyncObject();

        m_ShaderCache = ShaderCache::createOrGetShaderCache(desc.ShaderCache);
        m_FontCache = FontCache::createOrGetFontCache(desc.FontCache);
    }

    VulkanRenderer::~VulkanRenderer()
    {
        vkDeviceWaitIdle(m_Device);

        delete m_Swapchain;

        m_FontCache.release();

        for (auto& queue : m_ResourceFreeQueue)
        {
            for (auto& func : queue)
                func(this);
            queue.clear();
        }
        m_ResourceFreeQueue.clear();

        for (VkFence fence : m_InFlightFences)
            vkDestroyFence(m_Device, fence, m_Allocator);

        for (VkSemaphore semaphore : m_ImageAvailableSemaphores)
            vkDestroySemaphore(m_Device, semaphore, m_Allocator);
        m_ImageAvailableSemaphores.clear();

        for (VkSemaphore semaphore : m_RenderFinishedSemaphores)
            vkDestroySemaphore(m_Device, semaphore, m_Allocator);
        m_RenderFinishedSemaphores.clear();

        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, m_Allocator);
        vkDestroyCommandPool(m_Device, m_CommandPool, m_Allocator);
        m_FrameCommandBuffers.clear();

        vkDestroyDevice(m_Device, m_Allocator);
        vkDestroySurfaceKHR(m_Instance, m_Surface, m_Allocator);
        Utils::DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, m_Allocator);
        vkDestroyInstance(m_Instance, m_Allocator);
    }

    void VulkanRenderer::beginFrame()
    {
        VkResult result = vkWaitForFences(m_Device, 1, &m_InFlightFences[m_FrameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());
        VK_CHECK(result, "Failed to wait for Vulkan fence!");

        bool success = m_Swapchain->acquireNextImage(m_ImageIndex);
        if (!success)
        {
            m_SkipFrame = true;
            m_DidSwapchainResize = true;

            return;
        }

        m_DidSwapchainResize = false;

        result = vkResetFences(m_Device, 1, &m_InFlightFences[m_FrameIndex]);
        VK_CHECK(result, "Failed to reset Vulkan fence!");
    }

    void VulkanRenderer::endFrame()
    {
        if (m_SkipFrame)
        {
            m_SkipFrame = false;

            vkDeviceWaitIdle(m_Device);

            for (auto& func : m_ResourceFreeQueue[m_FrameIndex])
            {
                func(this);
            }
            m_ResourceFreeQueue[m_FrameIndex].clear();

            return;
        }

        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkResult result = vkBeginCommandBuffer(m_FrameCommandBuffers[m_FrameIndex], &beginInfo);
        VK_CHECK(result, "Failed to begin Vulkan command buffer!");

        glm::vec2 extent = m_Swapchain->getExtent();

        for (const auto& listInfo : m_SubmittedCommandLists[m_FrameIndex])
        {
            for (size_t i = 0; i < listInfo.Types.size(); i++)
            {
                CommandScope::Type type = listInfo.Types[i];
                VulkanRenderPass* renderPass = (VulkanRenderPass*)listInfo.RenderPasses[i];
                
                if (type == CommandScope::RenderPass)
                {
                    std::array<VkClearValue, 2> clearValues = {};
                    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
                    clearValues[1].depthStencil = { 1.0f, 0 };

                    VkRenderPassBeginInfo renderPassInfo{};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderArea.extent = { (uint32_t)extent.x, (uint32_t)extent.y };
                    renderPassInfo.renderArea.offset = { 0, 0 };
                    renderPassInfo.renderPass = renderPass->getRenderPass();
                    renderPassInfo.framebuffer = renderPass->getFramebuffer(m_ImageIndex);
                    renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
                    renderPassInfo.pClearValues = clearValues.data();

                    vkCmdBeginRenderPass(m_FrameCommandBuffers[m_FrameIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
                }

                vkCmdExecuteCommands(m_FrameCommandBuffers[m_FrameIndex], 1, &listInfo.Buffers[i]);

                if (type == CommandScope::RenderPass)
                {
                    vkCmdEndRenderPass(m_FrameCommandBuffers[m_FrameIndex]);
                }
            }
        }

        result = vkEndCommandBuffer(m_FrameCommandBuffers[m_FrameIndex]);
        VK_CHECK(result, "Failed to end Vulkan command buffer!");

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &m_FrameCommandBuffers[m_FrameIndex];
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &m_ImageAvailableSemaphores[m_FrameIndex];
        submit.pWaitDstStageMask = waitStages;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &m_RenderFinishedSemaphores[m_ImageIndex];

        result = vkQueueSubmit(m_GraphicsQueue, 1, &submit, m_InFlightFences[m_FrameIndex]);
        VK_CHECK(result, "Failed to submit to Vulkan queue!");

        VkSwapchainKHR swapchain = ((VulkanSwapchain*)m_Swapchain)->getSwapchain();

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_ImageIndex];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &m_ImageIndex;

        result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || Application::get().wasWindowResized())
        {
            Application::get().resetWindowResized();

            m_Swapchain->recreateSwapchain();
            m_DidSwapchainResize = true;
        }
        VK_CHECK(result, "Failed to present to Vulkan queue!");

        m_SubmittedCommandLists[m_FrameIndex].clear();
        m_UsedSecondaryCommandBufferCount[m_FrameIndex] = 0;

        for (auto& func : m_ResourceFreeQueue[m_FrameIndex])
        {
            func(this);
        }
        m_ResourceFreeQueue[m_FrameIndex].clear();

        m_FrameIndex = (m_FrameIndex + 1) % WR_FRAMES_IN_FLIGHT;
    }

    uint32_t VulkanRenderer::getNumFramesInFlight() const
    {
        return WR_FRAMES_IN_FLIGHT;
    }

    CommandList VulkanRenderer::beginSingleTimeCommands()
    {
        CommandList list(this, true);
        list.begin();

        return list;
    }

    void VulkanRenderer::endSingleTimeCommands(CommandList& commandList)
    {
        WR_ASSERT(commandList.isSingleTimeCommands(), "command list must be single time commands");

        commandList.end();

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.commandBufferCount = 1;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkCommandBuffer commandBuffer;
        VkResult result = vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);
        VK_CHECK(result, "failed to allocate Vulkan command buffer");

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        VK_CHECK(result, "failed to begin Vulkan command buffer");

        for (const CommandScope& scope : commandList.getScopes())
        {
            executeCommandScope(commandBuffer, scope);
        }

        result = vkEndCommandBuffer(commandBuffer);
        VK_CHECK(result, "Failed to end Vulkan command buffer!");

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, 0);
        VK_CHECK(result, "Failed to submit to Vulkan queue!");

        result = vkQueueWaitIdle(m_GraphicsQueue);
        VK_CHECK(result, "An error occurred while waiting for Vulkan queue!");

        vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
    }

    CommandList VulkanRenderer::createCommandList()
    {
        return CommandList(this);
    }

    void VulkanRenderer::submitCommandList(const CommandList& commandList)
    {
        WR_ASSERT(!commandList.isRecording(), "cannot submit a CommandList that is currently recording!");

        if (m_SkipFrame)
            return;

        CommandListData listData{};

        for (const auto& scope : commandList.getScopes())
        {
            if (scope.Commands.empty())
                continue;

            VkCommandBuffer commandBuffer;

            if (m_UsedSecondaryCommandBufferCount[m_FrameIndex] < m_SecondaryCommandBufferPool[m_FrameIndex].size())
                commandBuffer = m_SecondaryCommandBufferPool[m_FrameIndex][m_UsedSecondaryCommandBufferCount[m_FrameIndex]++];
            else
            {
                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.commandPool = m_CommandPool;
                allocInfo.commandBufferCount = 1;
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

                VkResult result = vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);
                VK_CHECK(result, "Failed to allocate Vulkan command buffer!");

                m_SecondaryCommandBufferPool[m_FrameIndex].push_back(commandBuffer);
                m_UsedSecondaryCommandBufferCount[m_FrameIndex]++;
            }
            
            VkCommandBufferInheritanceInfo inheritanceInfo{};
            inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.pInheritanceInfo = &inheritanceInfo;
            
            if (scope.ScopeType == CommandScope::RenderPass)
            {
                inheritanceInfo.renderPass = ((VulkanRenderPass*)scope.CurrentRenderPass)->getRenderPass();
                inheritanceInfo.framebuffer = ((VulkanRenderPass*)scope.CurrentRenderPass)->getFramebuffer(m_ImageIndex);

                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

                listData.RenderPasses.push_back(scope.CurrentRenderPass);
            }
            else
                listData.RenderPasses.push_back(nullptr);
            
            VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
            VK_CHECK(result, "Failed to begin Vulkan command buffer!");

            executeCommandScope(commandBuffer, scope);

            result = vkEndCommandBuffer(commandBuffer);
            VK_CHECK(result, "Failed to end Vulkan command buffer!");

            listData.Buffers.push_back(commandBuffer);
            listData.Types.push_back(scope.ScopeType);
        }

        m_SubmittedCommandLists[m_FrameIndex].push_back(listData);
    }

    void VulkanRenderer::executeCommandScope(VkCommandBuffer commandBuffer, const CommandScope& commandScope)
    {
        for (const auto& command : commandScope.Commands)
        {
            switch (command.Type)
            {
            case CommandType::BeginRenderPass:
            case CommandType::EndRenderPass:
                break; // don't need to handle this as scopes handle it
            case CommandType::BindPipeline:
            {
                const auto& args = std::get<CommandEntry::BindPipelineArgs>(command.Args);

                VkPipeline pipeline = nullptr;
                VkPipelineBindPoint bindPoint;

                if (args.IsGraphics)
                {
                    const VulkanGraphicsPipeline* vkPipeline = static_cast<const VulkanGraphicsPipeline*>(args.Graphics);
                    pipeline = vkPipeline->getPipeline();
                    bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                }
                else
                {
                    const VulkanComputePipeline* vkPipeline = static_cast<const VulkanComputePipeline*>(args.Compute);
                    pipeline = vkPipeline->getPipeline();
                    bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
                }

                vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
                break;
            }
            case CommandType::PushConstants:
            {
                const auto& args = std::get<CommandEntry::PushConstantsArgs>(command.Args);

                VkPipelineLayout layout = nullptr;

                if (args.IsGraphics)
                {
                    const VulkanGraphicsPipeline* vkPipeline = static_cast<const VulkanGraphicsPipeline*>(args.Graphics);
                    layout = vkPipeline->getPipelineLayout();
                }
                else
                {
                    const VulkanComputePipeline* vkPipeline = static_cast<const VulkanComputePipeline*>(args.Compute);
                    layout = vkPipeline->getPipelineLayout();
                }

                vkCmdPushConstants(commandBuffer, layout, Utils::ConvertShaderType(args.Stage), args.Offset, args.Size, args.Data);
                break;
            }
            case CommandType::BindShaderResource:
            {
                const auto& args = std::get<CommandEntry::BindShaderResourceArgs>(command.Args);

                VkDescriptorSet set = nullptr;
                VkPipelineLayout layout = nullptr;
                VkPipelineBindPoint bindPoint;

                if (args.IsGraphics)
                {
                    const VulkanGraphicsPipeline* vkPipeline = static_cast<const VulkanGraphicsPipeline*>(args.Graphics);
                    set = static_cast<VulkanShaderResource*>(args.Resource)->getSet();
                    layout = vkPipeline->getPipelineLayout();
                    bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                }
                else
                {
                    const VulkanComputePipeline* vkPipeline = static_cast<const VulkanComputePipeline*>(args.Compute);
                    set = static_cast<VulkanShaderResource*>(args.Resource)->getSet();
                    layout = vkPipeline->getPipelineLayout();
                    bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
                }

                vkCmdBindDescriptorSets(
                    commandBuffer,
                    bindPoint,
                    layout,
                    0,
                    1,
                    &set,
                    0,
                    nullptr
                );
                break;
            }
            case CommandType::SetViewport:
            {
                const auto& args = std::get<CommandEntry::SetViewportArgs>(command.Args);

                VkViewport viewport{};
                viewport.x = args.Position.x;
                viewport.y = args.Position.y;
                viewport.width = args.Size.x;
                viewport.height = args.Size.y;
                viewport.minDepth = args.MinDepth;
                viewport.maxDepth = args.MaxDepth;

                vkCmdSetViewport(
                    commandBuffer,
                    0,
                    1,
                    &viewport
                );
                break;
            }
            case CommandType::SetScissor:
            {
                const auto& args = std::get<CommandEntry::SetScissorArgs>(command.Args);

                VkRect2D rect{};
                rect.extent = { (uint32_t)(args.Max.x - args.Min.x), (uint32_t)(args.Max.y - args.Min.y) };
                rect.offset = { (int)args.Min.x, (int)args.Min.y };

                vkCmdSetScissor(
                    commandBuffer,
                    0,
                    1,
                    &rect
                );
                break;
            }
            case CommandType::SetLineWidth:
            {
                const auto& args = std::get<CommandEntry::SetLineWidthArgs>(command.Args);

                vkCmdSetLineWidth(commandBuffer, args.LineWidth);
                break;
            }
            case CommandType::BindVertexBuffers:
            {
                const auto& args = std::get<CommandEntry::BindVertexBuffersArgs>(command.Args);

                std::vector<VkBuffer> buffers(args.Buffers.size());
                std::vector<VkDeviceSize> offsets(args.Buffers.size());
                for (size_t i = 0; i < args.Buffers.size(); i++)
                {
                    buffers[i] = static_cast<const VulkanBufferBase*>(args.Buffers[i])->getBuffer();
                    offsets[i] = 0;
                }

                vkCmdBindVertexBuffers(
                    commandBuffer,
                    0,
                    static_cast<uint32_t>(args.Buffers.size()),
                    buffers.data(),
                    offsets.data()
                );
                break;
            }
            case CommandType::BindIndexBuffer:
            {
                const auto& args = std::get<CommandEntry::BindIndexBufferArgs>(command.Args);

                vkCmdBindIndexBuffer(commandBuffer, static_cast<const VulkanBufferBase*>(args.Buffer)->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
                break;
            }
            case CommandType::ClearImage:
            {
                const auto& args = std::get<CommandEntry::ClearImageArgs>(command.Args);
                
                VkImageSubresourceRange range{};
                range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                range.baseMipLevel = args.BaseMipLevel;
                range.levelCount = args.MipCount;
                range.baseArrayLayer = 0;
                range.layerCount = 1;

                VkClearColorValue clearColor = { args.Color.x, args.Color.y, args.Color.z, args.Color.w };
                vkCmdClearColorImage(
                    commandBuffer,
                    ((VulkanFramebuffer*)args.Framebuffer)->getColorImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    &clearColor,
                    1, &range
                );

                break;
            }
            case CommandType::Draw:
            {
                const auto& args = std::get<CommandEntry::DrawArgs>(command.Args);
                
                vkCmdDraw(commandBuffer, args.VertexCount, 1, args.VertexOffset, 0);
                break;
            }
            case CommandType::DrawIndexed:
            {
                const auto& args = std::get<CommandEntry::DrawIndexedArgs>(command.Args);

                vkCmdDrawIndexed(commandBuffer, args.IndexCount, 1, args.IndexOffset, args.VertexOffset, 0);
                break;
            }
            case CommandType::Dispatch:
            {
                const auto& args = std::get<CommandEntry::DispatchArgs>(command.Args);

                vkCmdDispatch(commandBuffer, args.GroupCountX, args.GroupCountY, args.GroupCountZ);
                break;
            }
            case CommandType::CopyBuffer:
            {
                const auto& args = std::get<CommandEntry::CopyBufferArgs>(command.Args);

                VkBufferCopy copy{};
                copy.srcOffset = args.SrcOffset;
                copy.dstOffset = args.DstOffset;
                copy.size = args.Size;

                VulkanBufferBase* vkSrc = (VulkanBufferBase*)args.SrcBuffer;
                VulkanBufferBase* vkDst = (VulkanBufferBase*)args.DstBuffer;

                vkCmdCopyBuffer(commandBuffer, vkSrc->getBuffer(), vkDst->getBuffer(), 1, &copy);

                break;
            }
            case CommandType::BufferMemoryBarrier:
            {
                const auto& args = std::get<CommandEntry::BufferMemoryBarrierArgs>(command.Args);

                VkBufferMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barrier.srcAccessMask = (VkAccessFlags)args.WaitFor;
                barrier.dstAccessMask = (VkAccessFlags)args.Access;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.buffer = ((VulkanBufferBase*)args.Buffer)->getBuffer();
                barrier.offset = 0;
                barrier.size = VK_WHOLE_SIZE;

                vkCmdPipelineBarrier(
                    commandBuffer,
                    (VkPipelineStageFlags)args.WaitStage,
                    (VkPipelineStageFlags)args.UntilStage,
                    0,
                    0, nullptr,
                    1, &barrier,
                    0, nullptr
                );

                break;
            }
            case CommandType::ImageMemoryBarrier:
            {
                const auto& args = std::get<CommandEntry::ImageMemoryBarrierArgs>(command.Args);

                VkImageLayout oldLayout = Utils::GetImageLayout(args.OldUsage);
                VkImageLayout newLayout = Utils::GetImageLayout(args.NewUsage);

                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = oldLayout;
                barrier.newLayout = newLayout;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = ((VulkanFramebuffer*)args.Framebuffer)->getColorImage();
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = args.BaseMip;
                barrier.subresourceRange.levelCount = args.NumMips;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;

                VkPipelineStageFlags sourceStage;
                VkPipelineStageFlags destinationStage;

                if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                {
                    barrier.srcAccessMask = 0;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                {
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                {
                    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                {
                    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                    sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
                {
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

                    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
                {
                    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

                    sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                {
                    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                    sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                {
                    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
                {
                    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

                    sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
                {
                    barrier.srcAccessMask = 0;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

                    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
                {
                    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

                    sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                    destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                }
                else
                {
                    WR_ASSERT(false, "unsupported layout transition");
                    break;
                }

                vkCmdPipelineBarrier(
                    commandBuffer,
                    sourceStage,
                    destinationStage,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );

                break;
            }
            case CommandType::NativeCommand:
            {
                const auto& args = std::get<CommandEntry::NativeCommandArgs>(command.Args);

                if (args.CommandType == typeid(VulkanCopyBufferNativeCommand))
                {
                    std::shared_ptr<VulkanCopyBufferNativeCommand> nativeCommand = std::static_pointer_cast<VulkanCopyBufferNativeCommand>(args.NativeCommand);

                    VkBufferCopy copy{};
                    copy.srcOffset = nativeCommand->SrcOffset;
                    copy.dstOffset = nativeCommand->DstOffset;
                    copy.size = nativeCommand->Size;

                    vkCmdCopyBuffer(commandBuffer, nativeCommand->SrcBuffer, nativeCommand->DstBuffer, 1, &copy);
                }
                else if (args.CommandType == typeid(VulkanPipelineBarrierNativeCommand))
                {
                    std::shared_ptr<VulkanPipelineBarrierNativeCommand> nativeCommand = std::static_pointer_cast<VulkanPipelineBarrierNativeCommand>(args.NativeCommand);

                    vkCmdPipelineBarrier(
                        commandBuffer,
                        nativeCommand->SrcStage,
                        nativeCommand->DstStage,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &nativeCommand->Barrier
                    );
                }
                else if (args.CommandType == typeid(VulkanCopyBufferToImageNativeCommand))
                {
                    std::shared_ptr<VulkanCopyBufferToImageNativeCommand> nativeCommand = std::static_pointer_cast<VulkanCopyBufferToImageNativeCommand>(args.NativeCommand);

                    vkCmdCopyBufferToImage(
                        commandBuffer,
                        nativeCommand->SrcBuffer,
                        nativeCommand->DstImage,
                        nativeCommand->DstImageLayout,
                        1, &nativeCommand->Region
                    );
                }

                break;
            }
            default:
                WR_ASSERT(false, "Unknown command in CommandList!");
                break;
            }
        }
    }

    void VulkanRenderer::submitResourceFree(std::function<void(Renderer*)>&& func)
    {
        m_ResourceFreeQueue[m_FrameIndex].emplace_back(func);
    }

    Swapchain* VulkanRenderer::createSwapchain(const SwapchainDesc& desc, std::string_view debugName)
    {
        return new VulkanSwapchain(this, desc, debugName);
    }

    Framebuffer* VulkanRenderer::createFramebuffer(const FramebufferDesc& desc, std::string_view debugName)
    {
        return new VulkanFramebuffer(this, desc, debugName);
    }

    RenderPass* VulkanRenderer::createRenderPass(const RenderPassDesc& desc, Swapchain* swapchain, std::string_view debugName)
    {
        return new VulkanRenderPass(this, (VulkanSwapchain*)swapchain, desc, debugName);
    }

    RenderPass* VulkanRenderer::createRenderPass(const RenderPassDesc& desc, Framebuffer* framebuffer, std::string_view debugName)
    {
        return new VulkanRenderPass(this, (VulkanFramebuffer*)framebuffer, desc, debugName);
    }

    ShaderResourceLayout* VulkanRenderer::createShaderResourceLayout(const ShaderResourceLayoutInfo& layoutInfo)
    {
        return new VulkanShaderResourceLayout(this, layoutInfo);
    }

    ShaderResource* VulkanRenderer::createShaderResource(uint32_t set, ShaderResourceLayout* layout)
    {
        return new VulkanShaderResource(this, set, layout);
    }

    GraphicsPipeline* VulkanRenderer::createGraphicsPipeline(const GraphicsPipelineDesc& desc, std::string_view debugName)
    {
        return new VulkanGraphicsPipeline(this, desc, debugName);
    }

    ComputePipeline* VulkanRenderer::createComputePipeline(const ComputePipelineDesc& desc, std::string_view debugName)
    {
        return new VulkanComputePipeline(this, desc, debugName);
    }

    Texture2D* VulkanRenderer::createTexture2D(const std::filesystem::path& path, std::string_view debugName)
    {
        return new VulkanTexture2D(this, path, debugName);
    }

    Texture2D* VulkanRenderer::createTexture2D(uint32_t* data, uint32_t width, uint32_t height, std::string_view debugName)
    {
        return new VulkanTexture2D(this, data, width, height, debugName);
    }

    Sampler* VulkanRenderer::createSampler(const SamplerDesc& desc, std::string_view debugName)
    {
        return new VulkanSampler(this, desc, debugName);
    }

    Font* VulkanRenderer::createFont(const std::filesystem::path& path, std::string_view debugName, uint32_t minChar, uint32_t maxChar)
    {
        return new VulkanFont(this, path, debugName, minChar, maxChar);
    }

    Font* VulkanRenderer::getFontFromCache(const std::filesystem::path& path)
    {
        std::string pathStr = path.string();

        WR_ASSERT(pathStr.contains("fontcache://"), "Invalid font cache path! (must begin with fontcache://)");

        std::string name = pathStr.substr(12);

        for (const auto& font : m_FontCache)
        {
            if (font.Name == name)
                return new VulkanFont(this, font);
        }

        WR_ASSERT(false, "Font not found in cache!");
        return nullptr;
    }

    float VulkanRenderer::getMaxAnisotropy() const
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

        return properties.limits.maxSamplerAnisotropy;
    }

    VkCommandBuffer VulkanRenderer::beginCommandListOverride(RenderPass* renderPass)
    {
        VkCommandBuffer commandBuffer;

        if (m_UsedSecondaryCommandBufferCount[m_FrameIndex] < m_SecondaryCommandBufferPool[m_FrameIndex].size())
            commandBuffer = m_SecondaryCommandBufferPool[m_FrameIndex][m_UsedSecondaryCommandBufferCount[m_FrameIndex]++];
        else
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = m_CommandPool;
            allocInfo.commandBufferCount = 1;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

            VkResult result = vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);
            VK_CHECK(result, "Failed to allocate Vulkan command buffer!");

            m_SecondaryCommandBufferPool[m_FrameIndex].push_back(commandBuffer);
            m_UsedSecondaryCommandBufferCount[m_FrameIndex]++;
        }

        m_CurrentOverrideCommandList = new CommandListData();
        m_CurrentOverrideCommandList->Types.push_back(renderPass ? CommandScope::RenderPass : CommandScope::General);
        m_CurrentOverrideCommandList->Buffers.push_back(commandBuffer);
        m_CurrentOverrideCommandList->RenderPasses.push_back(renderPass);

        VkCommandBufferInheritanceInfo inheritanceInfo{};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pInheritanceInfo = &inheritanceInfo;

        if (renderPass)
        {
            inheritanceInfo.renderPass = ((VulkanRenderPass*)renderPass)->getRenderPass();
            inheritanceInfo.framebuffer = ((VulkanRenderPass*)renderPass)->getFramebuffers()[m_ImageIndex];

            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        }

        VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        VK_CHECK(result, "Failed to begin Vulkan command buffer!");

        return commandBuffer;
    }

    void VulkanRenderer::endCommandListOverride()
    {
        WR_ASSERT(m_CurrentOverrideCommandList, "cannot end command list override without beginning one");

        VkResult result = vkEndCommandBuffer(m_CurrentOverrideCommandList->Buffers[0]);
        VK_CHECK(result, "Failed to end Vulkan command buffer!");

        m_SubmittedCommandLists[m_FrameIndex].push_back(*m_CurrentOverrideCommandList);

        delete m_CurrentOverrideCommandList;
        m_CurrentOverrideCommandList = nullptr;
    }

    uint32_t VulkanRenderer::getGraphicsQueueFamily() const
    {
        QueueFamilyIndices indices = Utils::FindQueueFamilies(m_PhysicalDevice, m_Surface);
        return indices.GraphicsFamily;
    }

    std::shared_ptr<CommandListNativeCommand> VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, size_t size, size_t srcOffset, size_t dstOffset, std::type_index& outType)
    {
        std::shared_ptr<VulkanCopyBufferNativeCommand> result = std::make_shared<VulkanCopyBufferNativeCommand>();
        result->SrcBuffer = srcBuffer;
        result->DstBuffer = dstBuffer;
        result->Size = size;
        result->SrcOffset = srcOffset;
        result->DstOffset = dstOffset;

        outType = typeid(VulkanCopyBufferNativeCommand);

        return result;
    }

    std::shared_ptr<CommandListNativeCommand> VulkanRenderer::pipelineBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, const VkImageMemoryBarrier& barrier, std::type_index& outType)
    {
        std::shared_ptr<VulkanPipelineBarrierNativeCommand> result = std::make_shared<VulkanPipelineBarrierNativeCommand>();
        result->SrcStage = srcStage;
        result->DstStage = dstStage;
        result->Barrier = barrier;

        outType = typeid(VulkanPipelineBarrierNativeCommand);

        return result;
    }

    std::shared_ptr<CommandListNativeCommand> VulkanRenderer::copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, const VkBufferImageCopy& copy, std::type_index& outType)
    {
        std::shared_ptr<VulkanCopyBufferToImageNativeCommand> result = std::make_shared<VulkanCopyBufferToImageNativeCommand>();
        result->SrcBuffer = srcBuffer;
        result->DstImage = dstImage;
        result->DstImageLayout = dstImageLayout;
        result->Region = copy;

        outType = typeid(VulkanCopyBufferToImageNativeCommand);

        return result;
    }

    BufferBase* VulkanRenderer::createBufferBase(BufferType type, size_t size, const void* data, std::string_view debugName)
    {
        return new VulkanBufferBase(this, type, size, data, debugName);
    }

    void VulkanRenderer::createInstance()
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "cNom";
        appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo instanceInfo{};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo = &appInfo;

        std::vector<const char*> extensions = Utils::GetRequiredExtensions();
        
#ifdef WR_PLATFORM_MAC
        instanceInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
        
        instanceInfo.enabledExtensionCount = (uint32_t)extensions.size();
        instanceInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if constexpr (s_EnableValidationLayers)
        {
            WR_ASSERT(Utils::CheckValidationLayerSupport(), "Validation layers enables but are not supported!");

            instanceInfo.enabledLayerCount = (uint32_t)s_ValidationLayers.size();
            instanceInfo.ppEnabledLayerNames = s_ValidationLayers.data();

            Utils::PopulateDebugMessengerCreateInfo(debugCreateInfo);
            instanceInfo.pNext = &debugCreateInfo;
        }
        else
        {
            instanceInfo.enabledLayerCount = 0;
            instanceInfo.ppEnabledLayerNames = nullptr;
        }

        VkResult result = vkCreateInstance(&instanceInfo, m_Allocator, &m_Instance);
        VK_CHECK(result, "Failed to create Vulkan instance!");

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> instanceExtensions(extensionCount);

        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instanceExtensions.data());

        WR_INFO("Available extensions:");
        for (const auto& extension : instanceExtensions)
        {
            WR_INFO("\t{}", extension.extensionName);
        }

        if constexpr (s_EnableValidationLayers)
        {
            result = Utils::CreateDebugUtilsMessengerEXT(m_Instance, &debugCreateInfo, m_Allocator, &m_DebugMessenger);
            VK_CHECK(result, "Failed to create Vulkan debug messenger!");
        }
    }

    void VulkanRenderer::loadExtensions()
    {
        exts::vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(m_Device, "vkSetDebugUtilsObjectNameEXT");
    }

    void VulkanRenderer::createSurface()
    {
        VkResult result = glfwCreateWindowSurface(m_Instance, Application::get().getWindow(), m_Allocator, &m_Surface);
        VK_CHECK(result, "Failed to create Vulkan window surface!");
    }

    void VulkanRenderer::pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

        for (const auto& device : devices)
        {
            if (Utils::IsDeviceSuitable(device, m_Surface))
            {
                m_PhysicalDevice = device;
                break;
            }
        }

        WR_ASSERT(m_PhysicalDevice, "Failed to find a suitable GPU!");
    }

    void VulkanRenderer::createLogicalDevice()
    {
        QueueFamilyIndices indices = Utils::FindQueueFamilies(m_PhysicalDevice, m_Surface);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.GraphicsFamily, indices.PresentFamily };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo& queueCreateInfo = queueCreateInfos.emplace_back();
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.sampleRateShading = VK_TRUE;
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = (uint32_t)s_DeviceExtensions.size();
        createInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();

        if (s_EnableValidationLayers)
        {
            createInfo.enabledLayerCount = (uint32_t)s_ValidationLayers.size();
            createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        VkResult result = vkCreateDevice(m_PhysicalDevice, &createInfo, m_Allocator, &m_Device);
        VK_CHECK(result, "Failed to create Vulkan device!");

        loadExtensions();

        VK_DEBUG_NAME(m_Device, DEVICE, m_Device, "VulkanRenderer::m_Device");
        VK_DEBUG_NAME(m_Device, SURFACE_KHR, m_Surface, "VulkanRenderer::m_Surface");

        vkGetDeviceQueue(m_Device, indices.GraphicsFamily, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, indices.PresentFamily, 0, &m_PresentQueue);

        VK_DEBUG_NAME(m_Device, QUEUE, m_GraphicsQueue, "VulkanRenderer::m_GraphicsQueue");
        VK_DEBUG_NAME(m_Device, QUEUE, m_PresentQueue, "VulkanRenderer::m_PresentQueue");
    }

    void VulkanRenderer::createCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkResult result = vkCreateCommandPool(m_Device, &poolInfo, m_Allocator, &m_CommandPool);
        VK_CHECK(result, "Failed to create Vulkan command pool!");

        VK_DEBUG_NAME(m_Device, COMMAND_POOL, m_CommandPool, "VulkanRenderer::m_CommandPool");

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandBufferCount = WR_FRAMES_IN_FLIGHT;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        m_FrameCommandBuffers.resize(WR_FRAMES_IN_FLIGHT);

        result = vkAllocateCommandBuffers(m_Device, &allocInfo, m_FrameCommandBuffers.data());
        VK_CHECK(result, "Failed to allocate Vulkan command buffers!");

        for (size_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
        {
            std::string debugName = "VulkanRenderer::m_FrameCommandBuffers[" + std::to_string(i) + "]";
            VK_DEBUG_NAME(m_Device, COMMAND_BUFFER, m_FrameCommandBuffers[i], debugName.c_str());
        }

        constexpr uint32_t secondaryCommandBufferCount = 10;

        allocInfo.commandBufferCount = secondaryCommandBufferCount;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

        m_SecondaryCommandBufferPool.resize(WR_FRAMES_IN_FLIGHT);
        m_UsedSecondaryCommandBufferCount.resize(WR_FRAMES_IN_FLIGHT);
        m_SubmittedCommandLists.resize(WR_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
        {
            m_SecondaryCommandBufferPool[i].resize(secondaryCommandBufferCount);
            m_UsedSecondaryCommandBufferCount[i] = 0;

            result = vkAllocateCommandBuffers(m_Device, &allocInfo, m_SecondaryCommandBufferPool[i].data());
            VK_CHECK(result, "Failed to allocate Vulkan command buffers!");
        }

        for (size_t frame = 0; frame < WR_FRAMES_IN_FLIGHT; frame++)
        {
            for (size_t i = 0; i < secondaryCommandBufferCount; i++)
            {
                std::string debugName = "VulkanRenderer::m_SecondaryCommandBuffers[" + std::to_string(frame) + "][" + std::to_string(i) + "]";
                VK_DEBUG_NAME(m_Device, COMMAND_BUFFER, m_SecondaryCommandBufferPool[frame][i], debugName.c_str());
            }
        }
    }

    void VulkanRenderer::createDescriptorPool()
    {
        VkDescriptorPoolSize combinedSamplers{};
        combinedSamplers.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        combinedSamplers.descriptorCount = 1000;

        VkDescriptorPoolSize uniforms{};
        uniforms.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniforms.descriptorCount = 1000;

        VkDescriptorPoolSize sampledImages{};
        sampledImages.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        sampledImages.descriptorCount = 1000;

        VkDescriptorPoolSize samplers{};
        samplers.type = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplers.descriptorCount = 1000;

        VkDescriptorPoolSize storage{};
        storage.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        storage.descriptorCount = 1000;

        std::array poolSizes = { combinedSamplers, uniforms, sampledImages, samplers, storage };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1000 * (uint32_t)poolSizes.size();
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VkResult result = vkCreateDescriptorPool(m_Device, &poolInfo, m_Allocator, &m_DescriptorPool);
        VK_CHECK(result, "Failed to create Vulkan descriptor pool!");

        VK_DEBUG_NAME(m_Device, DESCRIPTOR_POOL, m_DescriptorPool, "VulkanRenderer::m_DescriptorPool");
    }

    void VulkanRenderer::createSyncObject()
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        m_ImageAvailableSemaphores.resize(WR_FRAMES_IN_FLIGHT);
        m_RenderFinishedSemaphores.resize((size_t)m_Swapchain->getImageCount());
        m_InFlightFences.resize(WR_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < m_Swapchain->getImageCount(); i++)
        {
            VkResult result = vkCreateSemaphore(m_Device, &semaphoreInfo, m_Allocator, &m_RenderFinishedSemaphores[i]);
            VK_CHECK(result, "Failed to create Vulkan semaphore!");

            std::string semaphoreName = "VulkanRenderer::m_RenderFinishedSemaphores[" + std::to_string(i) + "]";
            VK_DEBUG_NAME(m_Device, SEMAPHORE, m_RenderFinishedSemaphores[i], semaphoreName.c_str());
        }

        for (size_t i = 0; i < WR_FRAMES_IN_FLIGHT; i++)
        {
            VkResult result = vkCreateSemaphore(m_Device, &semaphoreInfo, m_Allocator, &m_ImageAvailableSemaphores[i]);
            VK_CHECK(result, "Failed to create Vulkan semaphore!");

            std::string semaphoreName = "VulkanRenderer::m_ImageAvailableSemaphores[" + std::to_string(i) + "]";
            VK_DEBUG_NAME(m_Device, SEMAPHORE, m_ImageAvailableSemaphores[i], semaphoreName.c_str());

            result = vkCreateFence(m_Device, &fenceInfo, m_Allocator, &m_InFlightFences[i]);
            VK_CHECK(result, "Failed to create Vulkan fence!");

            std::string fenceName = "VulkanRenderer::m_InFlightFences[" + std::to_string(i) + "]";
            VK_DEBUG_NAME(m_Device, FENCE, m_InFlightFences[i], fenceName.c_str());
        }
    }

}

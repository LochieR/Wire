#include "VulkanInstance.h"

#include "VulkanDevice.h"
#include "Wire/Core/Application.h"

#include <glfw/glfw3.h>

namespace wire {

    constexpr static bool s_EnableValidationLayers
#ifdef WR_DEBUG
        = true;
#else
        = false;
#endif

    const static std::vector<const char*> s_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
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

    }

    VulkanInstance::VulkanInstance(const InstanceInfo& instanceInfo)
    {
        createInstance();
        createSurface();
    }
    
    VulkanInstance::~VulkanInstance()
    {
        vkDestroySurfaceKHR(m_Instance, m_Surface, m_Allocator);
        if constexpr (s_EnableValidationLayers)
            Utils::DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, m_Allocator);
        vkDestroyInstance(m_Instance, m_Allocator);
    }

    Device* VulkanInstance::createDevice(const DeviceInfo& deviceInfo, const SwapchainInfo& swapchainInfo)
    {
        return new VulkanDevice(this, deviceInfo, swapchainInfo);
    }

    void VulkanInstance::createInstance()
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

    void VulkanInstance::createSurface()
    {
        VkResult result = glfwCreateWindowSurface(m_Instance, Application::get().getWindow(), m_Allocator, &m_Surface);
        VK_CHECK(result, "Failed to create Vulkan window surface!");
    }

}
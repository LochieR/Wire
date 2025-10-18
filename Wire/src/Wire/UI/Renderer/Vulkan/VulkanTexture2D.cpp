#include "VulkanTexture2D.h"

#include "VulkanRenderer.h"
#include "Wire/Core/Assert.h"

#include "stb_image.h"
#include <vulkan/vulkan.h>

#include <string>

namespace wire {

    namespace Utils {

        static void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const VkAllocationCallbacks* allocator, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory, std::string_view debugName, bool staging)
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkResult result = vkCreateBuffer(device, &bufferInfo, allocator, &buffer);
            VK_CHECK(result, "Failed to create Vulkan buffer!");

            std::string bufferName(debugName);
            if (staging)
                bufferName += " (staging buffer)";
            else
                bufferName += " (buffer)";

            VK_DEBUG_NAME(device, BUFFER, buffer, bufferName.c_str());

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = Utils::FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

            result = vkAllocateMemory(device, &allocInfo, allocator, &memory);
            VK_CHECK(result, "Failed to allocate Vulkan memory!");

            std::string memoryName(debugName);
            if (staging)
                memoryName += " (staging memory)";
            else
                memoryName += " (memory)";

            VK_DEBUG_NAME(device, DEVICE_MEMORY, memory, memoryName.c_str());

            result = vkBindBufferMemory(device, buffer, memory, 0);
            VK_CHECK(result, "Failed to bind Vulkan buffer memory!");
        }

        static void CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, const VkAllocationCallbacks* allocator, uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, std::string_view debugName)
        {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = numSamples;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkResult result = vkCreateImage(device, &imageInfo, allocator, &image);
            VK_CHECK(result, "Failed to create Vulkan image!");

            std::string imageName(debugName);
            imageName += " (image)";
            VK_DEBUG_NAME(device, IMAGE, image, imageName.c_str());

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device, image, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

            result = vkAllocateMemory(device, &allocInfo, allocator, &imageMemory);
            VK_CHECK(result, "Failed to allocate Vulkan memory!");

            std::string memoryName(debugName);
            memoryName += " (memory)";
            VK_DEBUG_NAME(device, DEVICE_MEMORY, imageMemory, memoryName.c_str());

            vkBindImageMemory(device, image, imageMemory, 0);
        }

        static bool HasStencilComponent(VkFormat format)
        {
            return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
        }

        static void TransitionImageLayout(CommandList& commandList, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

                if (HasStencilComponent(format))
                {
                    barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
            }
            else
            {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
            }

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
            else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            }
            else
            {
                WR_ASSERT(false, "Unsupported layout transition!");
            }

            VulkanRenderer* vk = (VulkanRenderer*)commandList.getRenderer();
            std::type_index id = typeid(void);

            std::shared_ptr<CommandListNativeCommand> command = vk->pipelineBarrier(sourceStage, destinationStage, barrier, id);
            commandList.submitNativeCommand(command, id);
        }

        static void CopyBufferToImage(CommandList& commandList, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
        {
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = {
                width,
                height,
                1
            };

            VulkanRenderer* vk = (VulkanRenderer*)commandList.getRenderer();
            std::type_index id = typeid(void);

            std::shared_ptr<CommandListNativeCommand> command = vk->copyBufferToImage(
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                region,
                id
            );

            commandList.submitNativeCommand(command, id);
        }

        static VkImageView CreateImageView(VkDevice device, const VkAllocationCallbacks* allocator, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, std::string_view debugName)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;
            VkResult result = vkCreateImageView(device, &viewInfo, allocator, &imageView);
            VK_CHECK(result, "Failed to create Vulkan image view!");

            std::string viewName(debugName);
            viewName += " (view)";
            VK_DEBUG_NAME(device, IMAGE_VIEW, imageView, viewName.c_str());

            return imageView;
        }

    }

    VulkanTexture2D::VulkanTexture2D(Renderer* renderer, const std::filesystem::path& path, std::string_view debugName)
        : m_Renderer(renderer), m_DebugName(debugName), m_UUID()
    {
        VulkanRenderer* vk = (VulkanRenderer*)renderer;

        std::string pathStr = path.string();

        int width, height, channels;
        stbi_uc* data = stbi_load(pathStr.c_str(), &width, &height, &channels, 4);

        WR_ASSERT(data, "Failed to load image with stbi!");

        VkDeviceSize imageSize = (uint64_t)(width * height * 4);

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        Utils::CreateBuffer(
            vk->getDevice(),
            vk->getPhysicalDevice(),
            vk->getAllocator(),
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory,
            debugName,
            true
        );
        
        void* bufferData;
        vkMapMemory(vk->getDevice(), stagingBufferMemory, 0, imageSize, 0, &bufferData);
        memcpy(bufferData, data, imageSize);
        vkUnmapMemory(vk->getDevice(), stagingBufferMemory);

        stbi_image_free(data);

        m_Width = (uint32_t)width;
        m_Height = (uint32_t)height;

        Utils::CreateImage(
            vk->getDevice(),
            vk->getPhysicalDevice(),
            vk->getAllocator(),
            m_Width,
            m_Height,
            VK_SAMPLE_COUNT_1_BIT,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_Image,
            m_Memory,
            debugName
        );

        CommandList commandList = vk->beginSingleTimeCommands();

        Utils::TransitionImageLayout(
            commandList,
            m_Image,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        Utils::CopyBufferToImage(
            commandList,
            stagingBuffer,
            m_Image,
            m_Width,
            m_Height
        );

        Utils::TransitionImageLayout(
            commandList,
            m_Image,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        vk->endSingleTimeCommands(commandList);

        vkDestroyBuffer(vk->getDevice(), stagingBuffer, vk->getAllocator());
        vkFreeMemory(vk->getDevice(), stagingBufferMemory, vk->getAllocator());

        m_ImageView = Utils::CreateImageView(vk->getDevice(), vk->getAllocator(), m_Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, debugName);
    }

    VulkanTexture2D::VulkanTexture2D(Renderer* renderer, uint32_t* data, uint32_t width, uint32_t height, std::string_view debugName)
        : m_Renderer(renderer), m_DebugName(debugName), m_UUID()
    {
        VulkanRenderer* vk = (VulkanRenderer*)renderer;

        VkDeviceSize imageSize = (uint64_t)(width * height * 4);

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        Utils::CreateBuffer(
            vk->getDevice(),
            vk->getPhysicalDevice(),
            vk->getAllocator(),
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory,
            debugName,
            true
        );

        void* bufferData;
        vkMapMemory(vk->getDevice(), stagingBufferMemory, 0, imageSize, 0, &bufferData);
        memcpy(bufferData, data, imageSize);
        vkUnmapMemory(vk->getDevice(), stagingBufferMemory);

        m_Width = (uint32_t)width;
        m_Height = (uint32_t)height;

        Utils::CreateImage(
            vk->getDevice(),
            vk->getPhysicalDevice(),
            vk->getAllocator(),
            m_Width,
            m_Height,
            VK_SAMPLE_COUNT_1_BIT,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_Image,
            m_Memory,
            debugName
        );

        CommandList commandList = vk->beginSingleTimeCommands();

        Utils::TransitionImageLayout(
            commandList,
            m_Image,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        Utils::CopyBufferToImage(
            commandList,
            stagingBuffer,
            m_Image,
            m_Width,
            m_Height
        );

        Utils::TransitionImageLayout(
            commandList,
            m_Image,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        vk->endSingleTimeCommands(commandList);

        vkDestroyBuffer(vk->getDevice(), stagingBuffer, vk->getAllocator());
        vkFreeMemory(vk->getDevice(), stagingBufferMemory, vk->getAllocator());

        m_ImageView = Utils::CreateImageView(vk->getDevice(), vk->getAllocator(), m_Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, debugName);
    }

    VulkanTexture2D::VulkanTexture2D(VkImage image, VkDeviceMemory memory, VkImageView view, const std::vector<VkImageView>& mips, uint32_t width, uint32_t height)
        : m_Image(image), m_Memory(memory), m_ImageView(view), m_Mips(mips), m_Width(width), m_Height(height)
    {
        m_NoFree = true;
    }

    VulkanTexture2D::~VulkanTexture2D()
    {
        if (!m_NoFree)
        {
            m_Renderer->submitResourceFree([image = m_Image, memory = m_Memory, view = m_ImageView](Renderer* renderer)
            {
                VulkanRenderer* vk = (VulkanRenderer*)renderer;

                vkDestroyImageView(vk->getDevice(), view, vk->getAllocator());
                vkDestroyImage(vk->getDevice(), image, vk->getAllocator());
                vkFreeMemory(vk->getDevice(), memory, vk->getAllocator());
            });
        }
    }

    VulkanSampler::VulkanSampler(Renderer* renderer, const SamplerDesc& desc, std::string_view debugName)
        : m_Renderer(renderer), m_Desc(desc), m_DebugName(debugName)
    {
        VulkanRenderer* vk = (VulkanRenderer*)renderer;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(vk->getPhysicalDevice(), &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = (VkFilter)desc.MagFilter;
        samplerInfo.minFilter = (VkFilter)desc.MinFilter;
        samplerInfo.addressModeU = (VkSamplerAddressMode)desc.AddressModeU;
        samplerInfo.addressModeV = (VkSamplerAddressMode)desc.AddressModeV;
        samplerInfo.addressModeW = (VkSamplerAddressMode)desc.AddressModeW;
        samplerInfo.anisotropyEnable = desc.EnableAnisotropy ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy = desc.MaxAnisotropy;
        samplerInfo.borderColor = (VkBorderColor)desc.BorderColor;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = desc.CompareEnable ? VK_TRUE : VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = (VkSamplerMipmapMode)desc.MipmapMode;
        samplerInfo.mipLodBias = desc.MipLodBias;
        samplerInfo.minLod = desc.MinLod;
        samplerInfo.maxLod = desc.NoMaxLodClamp ? VK_LOD_CLAMP_NONE : desc.MaxLod;
        
        VkResult result = vkCreateSampler(vk->getDevice(), &samplerInfo, vk->getAllocator(), &m_Sampler);
        VK_CHECK(result, "Failed to create Vulkan sampler!");

        VK_DEBUG_NAME(vk->getDevice(), SAMPLER, m_Sampler, m_DebugName.c_str());
    }

    VulkanSampler::~VulkanSampler()
    {
        m_Renderer->submitResourceFree([sampler = m_Sampler](Renderer* renderer)
        {
            VulkanRenderer* vk = (VulkanRenderer*)renderer;

            vkDestroySampler(vk->getDevice(), sampler, vk->getAllocator());
        });
    }

}

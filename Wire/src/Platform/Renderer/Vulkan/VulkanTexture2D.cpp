#include "wrpch.h"

#include <vulkan/vulkan.h>
#include "VulkanTexture2D.h"

#include <stb_image.h>

namespace Wire {

	VulkanTexture2D::VulkanTexture2D(VulkanRenderer* renderer, std::string_view path)
		: m_Renderer(renderer)
	{
		auto& vkd = m_Renderer->GetVulkanData();

		int width, height, channels;
		stbi_uc* data = stbi_load(path.data(), &width, &height, &channels, 4);

		WR_ASSERT(data);

		VkDeviceSize imageSize = (uint64_t)width * height * 4;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		renderer->CreateBuffer(
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		void* bufferData;
		vkMapMemory(vkd.Device, stagingBufferMemory, 0, imageSize, 0, &bufferData);
		memcpy(bufferData, data, imageSize);
		vkUnmapMemory(vkd.Device, stagingBufferMemory);

		stbi_image_free(data);

		m_Width = (uint32_t)width;
		m_Height = (uint32_t)height;

		renderer->CreateImage(
			m_Width,
			m_Height,
			VK_SAMPLE_COUNT_1_BIT,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_Image,
			m_Memory
		);

		renderer->TransitionImageLayout(
			m_Image,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		renderer->CopyBufferToImage(
			stagingBuffer,
			m_Image,
			m_Width,
			m_Height
		);

		renderer->TransitionImageLayout(
			m_Image,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		vkDestroyBuffer(vkd.Device, stagingBuffer, vkd.Allocator);
		vkFreeMemory(vkd.Device, stagingBufferMemory, vkd.Allocator);

		m_ImageView = renderer->CreateImageView(m_Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(vkd.PhysicalDevice, &properties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		VkResult result = vkCreateSampler(vkd.Device, &samplerInfo, vkd.Allocator, &m_Sampler);
		VK_CHECK(result, "Failed to create Vulkan sampler!");
	}

	VulkanTexture2D::VulkanTexture2D(VulkanRenderer* renderer, uint32_t* data, uint32_t width, uint32_t height)
		: m_Renderer(renderer)
	{
		auto& vkd = renderer->GetVulkanData();

		VkDeviceSize imageSize = (uint64_t)width * height * 4;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		renderer->CreateBuffer(
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		void* bufferData;
		vkMapMemory(vkd.Device, stagingBufferMemory, 0, imageSize, 0, &bufferData);
		memcpy(bufferData, data, imageSize);
		vkUnmapMemory(vkd.Device, stagingBufferMemory);

		m_Width = width;
		m_Height = height;

		renderer->CreateImage(
			m_Width,
			m_Height,
			VK_SAMPLE_COUNT_1_BIT,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_Image,
			m_Memory
		);

		renderer->TransitionImageLayout(
			m_Image,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		renderer->CopyBufferToImage(
			stagingBuffer,
			m_Image,
			m_Width,
			m_Height
		);

		renderer->TransitionImageLayout(
			m_Image,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		vkDestroyBuffer(vkd.Device, stagingBuffer, vkd.Allocator);
		vkFreeMemory(vkd.Device, stagingBufferMemory, vkd.Allocator);

		m_ImageView = renderer->CreateImageView(m_Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(vkd.PhysicalDevice, &properties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		VkResult result = vkCreateSampler(vkd.Device, &samplerInfo, vkd.Allocator, &m_Sampler);
		VK_CHECK(result, "Failed to create Vulkan sampler!");
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		m_Renderer->SubmitResourceFree([image = m_Image, memory = m_Memory, view = m_ImageView, sampler = m_Sampler](VulkanRenderer* renderer)
		{
			auto& vkd = renderer->GetVulkanData();

			vkDestroySampler(vkd.Device, sampler, vkd.Allocator);
			vkDestroyImageView(vkd.Device, view, vkd.Allocator);
			vkDestroyImage(vkd.Device, image, vkd.Allocator);
			vkFreeMemory(vkd.Device, memory, vkd.Allocator);
		});
	}

}

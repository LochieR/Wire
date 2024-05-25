#pragma once

#include "VulkanRenderer.h"

#include "Wire/Renderer/Texture2D.h"

struct VkImage_T; typedef VkImage_T* VkImage;
struct VkDeviceMemory_T; typedef VkDeviceMemory_T* VkDeviceMemory;
struct VkImageView_T; typedef VkImageView_T* VkImageView;
struct VkSampler_T; typedef VkSampler_T* VkSampler;

namespace Wire {

	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(VulkanRenderer* renderer, std::string_view path);
		VulkanTexture2D(VulkanRenderer* renderer, uint32_t* data, uint32_t width, uint32_t height);
		virtual ~VulkanTexture2D();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
	private:
		VulkanRenderer* m_Renderer = nullptr;

		VkImage m_Image = nullptr;
		VkDeviceMemory m_Memory = nullptr;
		VkImageView m_ImageView = nullptr;
		VkSampler m_Sampler = nullptr;

		uint32_t m_Width = 1, m_Height = 1;

		friend class VulkanGraphicsPipeline;
	};

}

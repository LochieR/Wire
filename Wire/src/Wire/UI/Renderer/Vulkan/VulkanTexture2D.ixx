module;

#include <vulkan/vulkan.h>

#include <filesystem>

export module wire.ui.renderer.vk:texture2D;

import wire.ui.renderer;
import wire.core;
import :renderer;

namespace wire {

	export class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(Renderer* renderer, const std::filesystem::path& path);
		VulkanTexture2D(Renderer* renderer, uint32_t* data, uint32_t width, uint32_t height);
		virtual ~VulkanTexture2D();

		virtual uint32_t getWidth() const override { return m_Width; }
		virtual uint32_t getHeight() const override { return m_Height; }

		virtual UUID getUUID() const { return m_UUID; }

		VkImageView getImageView() const { return m_ImageView; }
	private:
		Renderer* m_Renderer = nullptr;

		UUID m_UUID;

		VkImage m_Image = nullptr;
		VkDeviceMemory m_Memory = nullptr;
		VkImageView m_ImageView = nullptr;

		uint32_t m_Width = 1, m_Height = 1;
	};

	export class VulkanSampler : public Sampler
	{
	public:
		VulkanSampler(Renderer* renderer, const SamplerDesc& desc);
		virtual ~VulkanSampler();

		VkSampler getSampler() const { return m_Sampler; }
	private:
		Renderer* m_Renderer = nullptr;
		SamplerDesc m_Desc;

		VkSampler m_Sampler = nullptr;
	};

}

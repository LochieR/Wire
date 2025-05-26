module;

#include <vulkan/vulkan.h>

export module wire.ui.renderer.vk:buffer;

import wire.ui.renderer;

namespace wire {

	export class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(Renderer* renderer, size_t size, const void* data = nullptr);
		virtual ~VulkanVertexBuffer();

		virtual void bind(CommandBuffer commandBuffer) const override;

		virtual void setData(const void* data, size_t size) override;
		virtual void setData(int data, size_t size) override;

		virtual void* map(size_t size) override;
		virtual void unmap() override;

		virtual size_t getSize() const override;
	private:
		Renderer* m_Renderer;
		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;

		size_t m_Size;
	};

	export class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(Renderer* renderer, size_t size, const void* data = nullptr);
		virtual ~VulkanIndexBuffer();

		virtual void bind(CommandBuffer commandBuffer) const override;

		virtual void setData(const void* data, size_t size) override;
		virtual void setData(int data, size_t size) override;

		virtual void* map(size_t size) override;
		virtual void unmap() override;

		virtual size_t getSize() const override;
	private:
		Renderer* m_Renderer;
		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;

		size_t m_Size;

		VkBuffer m_StagingBuffer;
		VkDeviceMemory m_StagingMemory;
	};

	export class VulkanStagingBuffer : public StagingBuffer
	{
	public:
		VulkanStagingBuffer(Renderer* renderer, size_t size, const void* data = nullptr);
		virtual ~VulkanStagingBuffer();

		virtual void setData(const void* data, size_t size) override;
		virtual void setData(int data, size_t size) override;

		virtual void* map(size_t size) override;
		virtual void unmap() override;

		virtual size_t getSize() const override { return m_Size; }
	private:
		Renderer* m_Renderer;
		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;

		size_t m_Size;
	};

}

#pragma once

#include "Wire/UI/Renderer/Renderer.h"

#include <vulkan/vulkan.h>

namespace wire {

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(Renderer* renderer, size_t size, const void* data = nullptr);
		virtual ~VulkanVertexBuffer();

		virtual void setData(const void* data, size_t size, size_t offset = 0) override;
		virtual void setData(int data, size_t size) override;

		virtual void* map(size_t size) override;
		virtual void unmap() override;

		virtual size_t getSize() const override;

		VkBuffer getBuffer() const { return m_Buffer; }
	private:
		Renderer* m_Renderer;
		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;

		size_t m_Size;
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(Renderer* renderer, size_t size, const void* data = nullptr);
		virtual ~VulkanIndexBuffer();

		virtual void setData(const void* data, size_t size) override;
		virtual void setData(int data, size_t size) override;

		virtual void* map(size_t size) override;
		virtual void unmap() override;

		virtual size_t getSize() const override;

		VkBuffer getBuffer() const { return m_Buffer; }
	private:
		Renderer* m_Renderer;
		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;

		size_t m_Size;

		VkBuffer m_StagingBuffer;
		VkDeviceMemory m_StagingMemory;
	};

	class VulkanStagingBuffer : public StagingBuffer
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

	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer(Renderer* renderer, size_t size, const void* data = nullptr);
		virtual ~VulkanUniformBuffer();

		virtual void setData(const void* data, size_t size) override;
		virtual void setData(int data, size_t size) override;

		virtual void* map(size_t size) override;
		virtual void unmap() override;

		virtual size_t getSize() const override { return m_Size; }

		VkBuffer getBuffer() const { return m_Buffer; }
	private:
		Renderer* m_Renderer;
		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;

		size_t m_Size;
	};

}

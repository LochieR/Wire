#pragma once

#include "VulkanRenderer.h"

#include "Wire/Renderer/Buffer.h"

struct VkBuffer_T; typedef VkBuffer_T* VkBuffer;
struct VkDeviceMemory_T; typedef VkDeviceMemory_T* VkDeviceMemory;

namespace Wire {

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(VulkanRenderer* renderer, size_t size);
		virtual ~VulkanVertexBuffer();

		virtual void Bind(rbRef<CommandBuffer> commandBuffer) const override;

		virtual void SetData(const void* vertexData, uint32_t size) override;

		virtual const InputLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const InputLayout& layout) override { m_Layout = layout; }
	private:
		VulkanRenderer* m_Renderer = nullptr;

		VkBuffer m_VertexBuffer = nullptr;
		VkDeviceMemory m_VertexBufferMemory = nullptr;

		InputLayout m_Layout;
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(VulkanRenderer* renderer, uint32_t* indices, uint32_t indexCount);
		virtual ~VulkanIndexBuffer();

		virtual void Bind(rbRef<CommandBuffer> commandBuffer) const override;
	private:
		VulkanRenderer* m_Renderer = nullptr;

		VkBuffer m_IndexBuffer = nullptr;
		VkDeviceMemory m_IndexBufferMemory = nullptr;
	};

	class VulkanStorageBuffer : public StorageBuffer
	{
	public:
		VulkanStorageBuffer(VulkanRenderer* renderer, size_t size);
		virtual ~VulkanStorageBuffer();

		virtual void SetData(const void* data, size_t size) override;
		virtual void SetData(int data, size_t size) override;

		virtual void* Map(uint32_t size, uint32_t offset) override;
		virtual void Unmap() override;

		virtual size_t GetSize() const override { return m_Size; }
	private:
		VulkanRenderer* m_Renderer = nullptr;

		size_t m_Size = 0;

		VkBuffer m_StorageBuffer = nullptr;
		VkDeviceMemory m_StorageBufferMemory = nullptr;

		friend class VulkanGraphicsPipeline;
		friend class VulkanStagingBuffer;
		friend class VulkanRenderer;
	};

	class VulkanStagingBuffer : public StagingBuffer
	{
	public:
		VulkanStagingBuffer(VulkanRenderer* renderer, size_t size);
		virtual ~VulkanStagingBuffer();

		virtual void SetData(const void* data, size_t size) override;
		virtual void SetData(int data, size_t size) override;

		virtual void* Map(uint32_t size) override;
		virtual void Unmap() override;

		virtual void CopyTo(rbRef<StorageBuffer> storageBuffer) override;
	private:
		VulkanRenderer* m_Renderer = nullptr;

		size_t m_Size = 0;

		VkBuffer m_Buffer = nullptr;
		VkDeviceMemory m_BufferMemory = nullptr;

		VkBuffer m_StagingBuffer = nullptr;
		VkDeviceMemory m_StagingBufferMemory = nullptr;

		friend class VulkanRenderer;
		friend class VulkanFramebuffer;
	};

}

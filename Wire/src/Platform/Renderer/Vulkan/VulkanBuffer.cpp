#include "wrpch.h"

#include <vulkan/vulkan.h>
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"

namespace Wire {

	namespace Utils {

		static VkFormat WireFormatToVkFormat(ShaderDataType type)
		{
			switch (type)
			{
				case ShaderDataType::Float: return VK_FORMAT_R32_SFLOAT;
				case ShaderDataType::Float2: return VK_FORMAT_R32G32_SFLOAT;
				case ShaderDataType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
				case ShaderDataType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
				case ShaderDataType::Int: return VK_FORMAT_R32_SINT;
				case ShaderDataType::Int2: return VK_FORMAT_R32G32_SINT;
				case ShaderDataType::Int3: return VK_FORMAT_R32G32B32_SINT;
				case ShaderDataType::Int4: return VK_FORMAT_R32G32B32A32_SINT;
				case ShaderDataType::Mat3: return VK_FORMAT_R32G32B32_SFLOAT;
				case ShaderDataType::Mat4: return VK_FORMAT_R32G32B32A32_SFLOAT;
			}

			WR_ASSERT(false);
			return (VkFormat)0;
		}

		VkVertexInputBindingDescription GetBindingDescription(const BufferLayout& layout)
		{
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = layout.GetStride();
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions(const BufferLayout& layout)
		{
			const std::vector<BufferElement>& elements = layout.GetElements();

			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(elements.size());

			for (uint32_t i = 0; i < elements.size(); i++)
			{
				attributeDescriptions[i].binding = 0;
				attributeDescriptions[i].location = i;
				attributeDescriptions[i].format = WireFormatToVkFormat(elements[i].Type);
				attributeDescriptions[i].offset = (uint32_t)elements[i].Offset;
			}

			return attributeDescriptions;
		}

	}

	VulkanVertexBuffer::VulkanVertexBuffer(VulkanRenderer* renderer, size_t size)
		: m_Renderer(renderer)
	{
		renderer->CreateBuffer(
			size,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_VertexBuffer,
			m_VertexBufferMemory
		);
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		m_Renderer->SubmitResourceFree([buffer = m_VertexBuffer, memory = m_VertexBufferMemory](VulkanRenderer* renderer)
		{
			auto& vkd = renderer->GetVulkanData();

			vkDestroyBuffer(vkd.Device, buffer, vkd.Allocator);
			vkFreeMemory(vkd.Device, memory, vkd.Allocator);
		});
	}

	void VulkanVertexBuffer::Bind(rbRef<CommandBuffer> commandBuffer) const
	{
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer, 0, 1, &m_VertexBuffer, &offset);
	}

	void VulkanVertexBuffer::SetData(const void* vertexData, uint32_t size)
	{
		auto& vkd = m_Renderer->GetVulkanData();

		void* data;
		vkMapMemory(vkd.Device, m_VertexBufferMemory, 0, size, 0, &data);
		memcpy(data, vertexData, size);
		vkUnmapMemory(vkd.Device, m_VertexBufferMemory);
	}

	VulkanIndexBuffer::VulkanIndexBuffer(VulkanRenderer* renderer, uint32_t* indices, uint32_t indexCount)
		: m_Renderer(renderer)
	{
		auto& vkd = renderer->GetVulkanData();

		VkDeviceSize bufferSize = sizeof(uint32_t) * indexCount;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		renderer->CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		void* data;
		vkMapMemory(vkd.Device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices, bufferSize);
		vkUnmapMemory(vkd.Device, stagingBufferMemory);

		renderer->CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_IndexBuffer,
			m_IndexBufferMemory
		);

		renderer->CopyBuffer(
			stagingBuffer,
			m_IndexBuffer,
			bufferSize
		);

		vkDestroyBuffer(vkd.Device, stagingBuffer, vkd.Allocator);
		vkFreeMemory(vkd.Device, stagingBufferMemory, vkd.Allocator);
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		m_Renderer->SubmitResourceFree([buffer = m_IndexBuffer, memory = m_IndexBufferMemory](VulkanRenderer* renderer)
		{
			auto& vkd = renderer->GetVulkanData();

			vkDestroyBuffer(vkd.Device, buffer, vkd.Allocator);
			vkFreeMemory(vkd.Device, memory, vkd.Allocator);
		});
	}

	void VulkanIndexBuffer::Bind(rbRef<CommandBuffer> commandBuffer) const
	{
		vkCmdBindIndexBuffer(((VulkanCommandBuffer*)commandBuffer.Get())->m_CommandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
	}

	VulkanStorageBuffer::VulkanStorageBuffer(VulkanRenderer* renderer, size_t size)
		: m_Renderer(renderer), m_Size(size)
	{
		renderer->CreateBuffer(
			size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_StorageBuffer,
			m_StorageBufferMemory
		);
	}

	VulkanStorageBuffer::~VulkanStorageBuffer()
	{
		m_Renderer->SubmitResourceFree([buffer = m_StorageBuffer, memory = m_StorageBufferMemory](VulkanRenderer* renderer)
		{
			auto& vkd = renderer->GetVulkanData();

			vkDestroyBuffer(vkd.Device, buffer, vkd.Allocator);
			vkFreeMemory(vkd.Device, memory, vkd.Allocator);
		});
	}

	void VulkanStorageBuffer::SetData(const void* data, size_t size)
	{
		auto& vkd = m_Renderer->GetVulkanData();

		void* memory;
		vkMapMemory(vkd.Device, m_StorageBufferMemory, 0, size, 0, &memory);
		memcpy(memory, data, size);
		vkUnmapMemory(vkd.Device, m_StorageBufferMemory);
	}

	void VulkanStorageBuffer::SetData(int data, size_t size)
	{
		auto& vkd = m_Renderer->GetVulkanData();

		void* memory;
		vkMapMemory(vkd.Device, m_StorageBufferMemory, 0, size, 0, &memory);
		memset(memory, data, size);
		vkUnmapMemory(vkd.Device, m_StorageBufferMemory);
	}

	void* VulkanStorageBuffer::Map(uint32_t size, uint32_t offset)
	{
		auto& vkd = m_Renderer->GetVulkanData();

		void* data;
		vkMapMemory(vkd.Device, m_StorageBufferMemory, offset, size, 0, &data);
		return data;
	}

	void VulkanStorageBuffer::Unmap()
	{
		auto& vkd = m_Renderer->GetVulkanData();
		
		vkUnmapMemory(vkd.Device, m_StorageBufferMemory);
	}

	VulkanStagingBuffer::VulkanStagingBuffer(VulkanRenderer* renderer, size_t size)
		: m_Renderer(renderer), m_Size(size)
	{
		auto& vkd = m_Renderer->GetVulkanData();

		renderer->CreateBuffer(
			size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_StagingBuffer,
			m_StagingBufferMemory
		);

		// Initialize to zero
		void* data;
		vkMapMemory(vkd.Device, m_StagingBufferMemory, 0, size, 0, &data);
		memset(data, 0, size);
		vkUnmapMemory(vkd.Device, m_StagingBufferMemory);

		renderer->CreateBuffer(
			size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_Buffer,
			m_BufferMemory
		);

		renderer->CopyBuffer(m_StagingBuffer, m_Buffer, size);
	}

	VulkanStagingBuffer::~VulkanStagingBuffer()
	{
		m_Renderer->SubmitResourceFree([stagingBuffer = m_StagingBuffer, stagingBufferMemory = m_StagingBufferMemory,
			buffer = m_Buffer, bufferMemory = m_BufferMemory](VulkanRenderer* renderer)
		{
			auto& vkd = renderer->GetVulkanData();

			vkDestroyBuffer(vkd.Device, buffer, vkd.Allocator);
			vkFreeMemory(vkd.Device, bufferMemory, vkd.Allocator);
			vkDestroyBuffer(vkd.Device, stagingBuffer, vkd.Allocator);
			vkFreeMemory(vkd.Device, stagingBufferMemory, vkd.Allocator);
		});
	}

	void VulkanStagingBuffer::SetData(const void* data, size_t size)
	{
		auto& vkd = m_Renderer->GetVulkanData();

		void* memory;
		vkMapMemory(vkd.Device, m_StagingBufferMemory, 0, size, 0, &memory);
		memcpy(memory, data, size);
		vkUnmapMemory(vkd.Device, m_StagingBufferMemory);

		m_Renderer->CopyBuffer(m_StagingBuffer, m_Buffer, m_Size);
	}

	void VulkanStagingBuffer::SetData(int data, size_t size)
	{
		auto& vkd = m_Renderer->GetVulkanData();

		void* memory;
		vkMapMemory(vkd.Device, m_StagingBufferMemory, 0, size, 0, &memory);
		memset(memory, data, size);
		vkUnmapMemory(vkd.Device, m_StagingBufferMemory);

		m_Renderer->CopyBuffer(m_StagingBuffer, m_Buffer, m_Size);
	}

	void* VulkanStagingBuffer::Map(uint32_t size)
	{
		auto& vkd = m_Renderer->GetVulkanData();

		void* memory;
		vkMapMemory(vkd.Device, m_StagingBufferMemory, 0, size, 0, &memory);
		return memory;
	}

	void VulkanStagingBuffer::Unmap()
	{
		auto& vkd = m_Renderer->GetVulkanData();

		vkUnmapMemory(vkd.Device, m_StagingBufferMemory);
	}

	void VulkanStagingBuffer::CopyTo(rbRef<StorageBuffer> storageBuffer)
	{
		WR_ASSERT(storageBuffer->GetSize() == m_Size);

		m_Renderer->CopyBuffer(m_Buffer, ((VulkanStorageBuffer*)storageBuffer.Get())->m_StorageBuffer, m_Size);
	}

}

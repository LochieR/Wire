#pragma once

#include <cstdint>
#include <type_traits>

using size_t = std::size_t;

namespace wire {

	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual void setData(const void* data, size_t size, size_t offset = 0) = 0;
		virtual void setData(int data, size_t size) = 0;

		virtual void* map(size_t size) = 0;
		virtual void unmap() = 0;

		virtual size_t getSize() const = 0;
	};

	class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() = default;

		virtual void setData(const void* data, size_t size) = 0;
		virtual void setData(int data, size_t size) = 0;

		virtual void* map(size_t size) = 0;
		virtual void unmap() = 0;

		virtual size_t getSize() const = 0;
	};

	class StagingBuffer
	{
	public:
		virtual ~StagingBuffer() = default;

		virtual void setData(const void* data, size_t size) = 0;
		virtual void setData(int data, size_t size) = 0;

		virtual void* map(size_t size) = 0;
		virtual void unmap() = 0;

		virtual size_t getSize() const = 0;
	};

	class UniformBuffer
	{
	public:
		virtual ~UniformBuffer() = default;

		virtual void setData(const void* data, size_t size) = 0;
		virtual void setData(int data, size_t size) = 0;

		virtual void* map(size_t size) = 0;
		virtual void unmap() = 0;

		virtual size_t getSize() const = 0;
	};

}

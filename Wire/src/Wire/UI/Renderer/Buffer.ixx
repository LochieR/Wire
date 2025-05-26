module;

#include <type_traits>

export module wire.ui.renderer:buffer;

import :cmdBuffer;

namespace wire {

	export class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual void bind(CommandBuffer commandBuffer) const = 0;

		virtual void setData(const void* data, size_t size) = 0;
		virtual void setData(int data, size_t size) = 0;

		virtual void* map(size_t size) = 0;
		virtual void unmap() = 0;

		virtual size_t getSize() const = 0;
	};

	export class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() = default;

		virtual void bind(CommandBuffer commandBuffer) const = 0;

		virtual void setData(const void* data, size_t size) = 0;
		virtual void setData(int data, size_t size) = 0;

		virtual void* map(size_t size) = 0;
		virtual void unmap() = 0;

		virtual size_t getSize() const = 0;
	};

	export class StagingBuffer
	{
	public:
		virtual ~StagingBuffer() = default;

		virtual void setData(const void* data, size_t size) = 0;
		virtual void setData(int data, size_t size) = 0;

		virtual void* map(size_t size) = 0;
		virtual void unmap() = 0;

		virtual size_t getSize() const = 0;
	};

}

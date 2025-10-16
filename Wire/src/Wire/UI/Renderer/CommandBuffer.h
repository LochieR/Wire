#pragma once

#include "Buffer.h"
#include "ShaderCache.h"
#include "GraphicsPipeline.h"

#include <glm/glm.hpp>

#include <type_traits>

namespace wire {

	class CommandBuffer
	{
	public:
		virtual ~CommandBuffer() = default;

		virtual void begin(bool renderPassStarted = true) = 0;
		virtual void end() = 0;
		virtual void draw(uint32_t vertexCount, uint32_t vertexOffset = 0) = 0;
		virtual void drawIndexed(uint32_t indexCount, uint32_t vertexOffset = 0, uint32_t indexOffset = 0) = 0;

		virtual void bindPipeline(GraphicsPipeline* pipeline) = 0;
		virtual void pushConstants(ShaderType stage, size_t size, const void* data, size_t offset = 0) = 0;
		virtual void setLineWidth(float lineWidth) = 0;
		virtual void bindDescriptorSet() = 0;
		virtual void setScissor(const glm::vec2& min, const glm::vec2& max) = 0;

		virtual void bindBuffer(VertexBuffer* vertexBuffer) = 0;
		virtual void bindBuffer(IndexBuffer* indexBuffer) = 0;

		virtual void* get() const = 0;

		template<typename T>
		std::enable_if_t<std::is_pointer<T>::value, T> as() const
		{
			return reinterpret_cast<T>(get());
		}

		template<typename T>
		void pushConstants(ShaderType stage, const T& value, size_t offset = 0)
		{
			pushConstants(stage, sizeof(T), &value, offset);
		}
	};

}

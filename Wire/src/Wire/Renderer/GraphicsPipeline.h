#pragma once

#include "Buffer.h"
#include "Texture2D.h"
#include "CommandBuffer.h"
#include "IResource.h"

namespace Wire {

	enum class PrimitiveTopology
	{
		TriangleList = 0,
		LineList,
		LineStrip
	};

	class GraphicsPipeline : public IResource
	{
	public:
		virtual ~GraphicsPipeline() = default;

		virtual void Bind(rbRef<CommandBuffer> commandBuffer) const = 0;
		virtual void PushConstants(rbRef<CommandBuffer> commandBuffer, ShaderStage shaderStage, size_t size, const void* data, size_t offset = 0) = 0;
		virtual void SetLineWidth(rbRef<CommandBuffer> commandBuffer, float lineWidth) = 0;

		virtual void UpdateDescriptor(rbRef<Texture2D> texture2D, uint32_t binding, uint32_t index) = 0;
		virtual void UpdateDescriptor(rbRef<StorageBuffer> buffer, uint32_t binding, uint32_t index) = 0;
		virtual void BindDescriptor(rbRef<CommandBuffer> commandBuffer) const = 0;
	};

}

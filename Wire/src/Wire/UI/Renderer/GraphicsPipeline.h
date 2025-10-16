#pragma once

#include <string>
#include <vector>

#include "ShaderCache.h"
#include "ShaderCompiler.h"
#include "Texture2D.h"

namespace wire {

	enum class PrimitiveTopology
	{
		TriangleList = 0,
		LineList,
		LineStrip
	};

	enum class ShaderDataType
	{
		None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, UInt, UInt2, UInt3, UInt4, Bool
	};

	struct InputElement
	{
		std::string Name;
		ShaderDataType Type;
		size_t Size;
		size_t Offset;
	};

	struct PushConstantInfo
	{
		size_t Size;
		size_t Offset;
		ShaderType Shader;
	};

	enum class ShaderResourceType
	{
		UniformBuffer = 0,
		CombinedImageSampler,
		SampledImage,
		Sampler,
		StorageBuffer
	};

	struct ShaderResourceInfo
	{
		ShaderResourceType ResourceType;
		uint32_t Binding;
		ShaderType Shader;
		uint32_t ResourceCount = 1;
	};

	struct InputLayout
	{
		std::vector<InputElement> VertexBufferLayout;
		size_t Stride;

		std::vector<PushConstantInfo> PushConstantInfos;
		std::vector<ShaderResourceInfo> ShaderResources;
	};

	struct GraphicsPipelineDesc
	{
		std::string ShaderPath;
		InputLayout Layout;
		PrimitiveTopology Topology;
	};

	class GraphicsPipeline
	{
	public:
		virtual ~GraphicsPipeline() = default;

		virtual void updateDescriptor(Texture2D* texture, uint32_t binding, uint32_t index) = 0;
		virtual void updateDescriptor(Sampler* sampler, uint32_t binding, uint32_t index) = 0;
		virtual void updateDescriptor(Texture2D* texture, Sampler* sampler, uint32_t binding, uint32_t index) = 0;
		virtual void updateDescriptor(UniformBuffer* uniformBuffer, uint32_t binding, uint32_t index) = 0;

		virtual void updateFrameDescriptor(Texture2D* texture, uint32_t frameIndex, uint32_t binding, uint32_t index) = 0;
		virtual void updateFrameDescriptor(Sampler* sampler, uint32_t frameIndex, uint32_t binding, uint32_t index) = 0;
		virtual void updateFrameDescriptor(Texture2D* texture, Sampler* sampler, uint32_t frameIndex, uint32_t binding, uint32_t index) = 0;
		virtual void updateFrameDescriptor(UniformBuffer* uniformBuffer, uint32_t frameIndex, uint32_t binding, uint32_t index) = 0;

		virtual void updateAllDescriptors(Texture2D* texture, uint32_t binding, uint32_t index) = 0;
		virtual void updateAllDescriptors(Sampler* sampler, uint32_t binding, uint32_t index) = 0;
		virtual void updateAllDescriptors(Texture2D* texture, Sampler* sampler, uint32_t binding, uint32_t index) = 0;
		virtual void updateAllDescriptors(UniformBuffer* uniformBuffer, uint32_t binding, uint32_t index) = 0;
	};

}

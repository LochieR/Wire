module;

#include <string>
#include <vector>

export module wire.ui.renderer:graphicsPipeline;

import :shaderCache;
import :shaderCompiler;
import :cmdBuffer;
import :texture2D;

namespace wire {

	export enum class PrimitiveTopology
	{
		TriangleList = 0,
		LineList,
		LineStrip
	};

	export enum class ShaderDataType
	{
		None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
	};

	export struct InputElement
	{
		std::string Name;
		ShaderDataType Type;
		size_t Size;
		size_t Offset;
	};

	export struct PushConstantInfo
	{
		size_t Size;
		size_t Offset;
		ShaderType Shader;
	};

	export enum class ShaderResourceType
	{
		UniformBuffer = 0,
		CombinedImageSampler,
		SampledImage,
		Sampler,
		StorageBuffer
	};

	export struct ShaderResourceInfo
	{
		ShaderResourceType ResourceType;
		uint32_t Binding;
		ShaderType Shader;
		uint32_t ResourceCount = 1;
	};

	export struct InputLayout
	{
		std::vector<InputElement> VertexBufferLayout;
		size_t Stride;

		std::vector<PushConstantInfo> PushConstantInfos;
		std::vector<ShaderResourceInfo> ShaderResources;
	};

	export struct GraphicsPipelineDesc
	{
		std::string ShaderPath;
		InputLayout Layout;
		PrimitiveTopology Topology;
	};

	export class GraphicsPipeline
	{
	public:
		virtual ~GraphicsPipeline() = default;

		virtual void bind(CommandBuffer commandBuffer) = 0;
		virtual void pushConstants(CommandBuffer commandBuffer, ShaderType stage, size_t size, const void* data, size_t offset = 0) = 0;
		virtual void setLineWidth(CommandBuffer commandBuffer, float lineWidth) = 0;

		virtual void updateDescriptor(Texture2D* texture, uint32_t binding, uint32_t index) = 0;
		virtual void updateDescriptor(Sampler* sampler, uint32_t binding, uint32_t index) = 0;
		virtual void bindDescriptorSet(CommandBuffer commandBuffer) = 0;

		template<typename T>
		void pushConstants(CommandBuffer commandBuffer, ShaderType stage, const T& value, size_t offset = 0)
		{
			pushConstants(commandBuffer, stage, sizeof(T), &value, offset);
		}
	};

}

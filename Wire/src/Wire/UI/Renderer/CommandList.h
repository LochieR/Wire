#pragma once

#include "Buffer.h"
#include "GraphicsPipeline.h"

#include "Wire/Core/Assert.h"

#include <glm/glm.hpp>

#include <vector>
#include <variant>

namespace wire {

	enum class CommandType
	{
		BeginRenderPass, EndRenderPass,
		BindPipeline, PushConstants, BindDescriptorSet, SetViewport, SetScissor, SetLineWidth,
		BindVertexBuffers, BindIndexBuffer,
		Draw, DrawIndexed, Dispatch
	};

	struct CommandEntry
	{
	private:
		struct BeginRenderPassArgs
		{
		};

		struct EndRenderPassArgs
		{
		};

		struct BindPipelineArgs
		{
			GraphicsPipeline* Pipeline;
		};

		struct PushConstantsArgs
		{
			GraphicsPipeline* Pipeline;
			ShaderType Stage;

			uint32_t Size;
			uint32_t Offset;
			char Data[128];
		};

		struct BindDescriptorSetArgs
		{
			GraphicsPipeline* Pipeline;
		};

		struct SetViewportArgs
		{
			GraphicsPipeline* Pipeline;
			glm::vec2 Position;
			glm::vec2 Size;
			float MinDepth;
			float MaxDepth;
		};

		struct SetScissorArgs
		{
			GraphicsPipeline* Pipeline;
			glm::vec2 Min;
			glm::vec2 Max;
		};

		struct SetLineWidthArgs
		{
			GraphicsPipeline* Pipeline;
			float LineWidth;
		};

		struct BindVertexBuffersArgs
		{
			std::vector<VertexBuffer*> Buffers;
		};

		struct BindIndexBufferArgs
		{
			IndexBuffer* Buffer;
		};

		struct DrawArgs
		{
			uint32_t VertexCount;
			uint32_t VertexOffset;
		};

		struct DrawIndexedArgs
		{
			uint32_t IndexCount;
			uint32_t VertexOffset;
			uint32_t IndexOffset;
		};

		friend class CommandList;
		friend class VulkanRenderer;
	public:
		CommandType Type;
		std::variant<
			BeginRenderPassArgs,
			EndRenderPassArgs,
			BindPipelineArgs,
			PushConstantsArgs,
			BindDescriptorSetArgs,
			SetViewportArgs,
			SetScissorArgs,
			SetLineWidthArgs,
			BindVertexBuffersArgs,
			BindIndexBufferArgs,
			DrawArgs,
			DrawIndexedArgs
		> Args;
	};

	struct CommandScope
	{
		enum Type { General, RenderPass };
		Type ScopeType;

		std::vector<CommandEntry> Commands;
	};

	class Renderer;

	class CommandList
	{
	public:
		CommandList() = default;
		CommandList(Renderer* renderer);
		~CommandList() = default;

		void begin();
		void end();

		void beginRenderPass();
		void endRenderPass();

		void bindPipeline(GraphicsPipeline* pipeline);
		void pushConstants(ShaderType shaderStage, const void* data, size_t size, size_t offset = 0);
		void bindDescriptorSet();

		void setViewport(const glm::vec2& position, const glm::vec2& size, float minDepth, float maxDepth);
		void setScissor(const glm::vec2& min, const glm::vec2& max);
		void setLineWidth(float lineWidth);

		void bindVertexBuffers(const std::vector<VertexBuffer*> vertexBuffers);
		void bindIndexBuffer(IndexBuffer* indexBuffer);

		void draw(uint32_t vertexCount, uint32_t vertexOffset = 0);
		void drawIndexed(uint32_t indexCount, uint32_t vertexOffset = 0, uint32_t indexOffset = 0);

		bool isRecording() const { return m_IsRecording; }
		
		const std::vector<CommandScope>& getScopes() const { return m_Scopes; }

		template<typename T>
		void pushConstants(ShaderType shaderStage, const T& value, size_t offset = 0)
		{
			pushConstants(shaderStage, &value, sizeof(T), offset);
		}
	private:
		Renderer* m_Renderer;

		bool m_IsRecording = false;

		std::vector<CommandScope> m_Scopes;
		CommandScope m_CurrentScope;

		GraphicsPipeline* m_CurrentPipeline = nullptr;
	};

}

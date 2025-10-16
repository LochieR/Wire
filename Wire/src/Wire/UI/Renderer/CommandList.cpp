#include "CommandList.h"

#include "Renderer.h"

namespace wire {

	CommandList::CommandList(Renderer* renderer)
		: m_Renderer(renderer)
	{
	}
	
	void CommandList::begin()
	{
		m_CurrentScope = CommandScope{ .ScopeType = CommandScope::General };
		m_IsRecording = true;

		m_Scopes.clear();
	}

	void CommandList::end()
	{
		m_IsRecording = false;
		m_CurrentPipeline = nullptr;
		m_Scopes.push_back(m_CurrentScope);
		m_CurrentScope = {};
	}

	void CommandList::beginRenderPass()
	{
		m_Scopes.push_back(m_CurrentScope);
		m_CurrentScope = CommandScope{ .ScopeType = CommandScope::RenderPass };
	}

	void CommandList::endRenderPass()
	{
		m_Scopes.push_back(m_CurrentScope);
		m_CurrentScope = CommandScope{ .ScopeType = CommandScope::General };
	}

	void CommandList::bindPipeline(GraphicsPipeline* pipeline)
	{
		CommandEntry entry;
		entry.Type = CommandType::BindPipeline;
		entry.Args = CommandEntry::BindPipelineArgs{ .Pipeline = pipeline };

		m_CurrentPipeline = pipeline;

		m_CurrentScope.Commands.push_back(entry);
	}

	void CommandList::pushConstants(ShaderType shaderStage, const void* data, size_t size, size_t offset)
	{
		WR_ASSERT(m_CurrentPipeline, "cannot push constants without binding a pipeline");
		WR_ASSERT(size <= 128, "push constant size must be <= 128");

		CommandEntry::PushConstantsArgs args{};
		args.Pipeline = m_CurrentPipeline;
		args.Stage = shaderStage;
		args.Size = static_cast<uint32_t>(size);
		args.Offset = static_cast<uint32_t>(offset);
		std::memcpy(args.Data, data, size);

		CommandEntry entry;
		entry.Type = CommandType::PushConstants;
		entry.Args = args;

		m_CurrentScope.Commands.push_back(entry);
	}

	void CommandList::bindDescriptorSet()
	{
		WR_ASSERT(m_CurrentPipeline, "cannot bind descriptor set without binding a pipeline");

		CommandEntry entry;
		entry.Type = CommandType::BindDescriptorSet;
		entry.Args = CommandEntry::BindDescriptorSetArgs{ .Pipeline = m_CurrentPipeline };

		m_CurrentScope.Commands.push_back(entry);
	}

	void CommandList::setViewport(const glm::vec2& position, const glm::vec2& size, float minDepth, float maxDepth)
	{
		WR_ASSERT(m_CurrentPipeline, "cannot set viewport without binding a pipeline");

		CommandEntry entry;
		entry.Type = CommandType::SetViewport;
		entry.Args = CommandEntry::SetViewportArgs{ .Pipeline = m_CurrentPipeline, .Position = position, .Size = size, .MinDepth = minDepth, .MaxDepth = maxDepth };

		m_CurrentScope.Commands.push_back(entry);
	}

	void CommandList::setScissor(const glm::vec2& min, const glm::vec2& max)
	{
		WR_ASSERT(m_CurrentPipeline, "cannot set scissor without binding a pipeline");

		CommandEntry entry;
		entry.Type = CommandType::SetScissor;
		entry.Args = CommandEntry::SetScissorArgs{ .Pipeline = m_CurrentPipeline, .Min = min, .Max = max };

		m_CurrentScope.Commands.push_back(entry);
	}

	void CommandList::setLineWidth(float lineWidth)
	{
		WR_ASSERT(m_CurrentPipeline, "cannot set line width without binding a pipeline");

		CommandEntry entry;
		entry.Type = CommandType::SetLineWidth;
		entry.Args = CommandEntry::SetLineWidthArgs{ .Pipeline = m_CurrentPipeline, .LineWidth = lineWidth };

		m_CurrentScope.Commands.push_back(entry);
	}

	void CommandList::bindVertexBuffers(const std::vector<VertexBuffer*> vertexBuffers)
	{
		CommandEntry entry;
		entry.Type = CommandType::BindVertexBuffers;
		entry.Args = CommandEntry::BindVertexBuffersArgs{ .Buffers = vertexBuffers };

		m_CurrentScope.Commands.push_back(entry);
	}

	void CommandList::bindIndexBuffer(IndexBuffer* indexBuffer)
	{
		CommandEntry entry;
		entry.Type = CommandType::BindIndexBuffer;
		entry.Args = CommandEntry::BindIndexBufferArgs{ .Buffer = indexBuffer };

		m_CurrentScope.Commands.push_back(entry);
	}

	void CommandList::draw(uint32_t vertexCount, uint32_t vertexOffset)
	{
		WR_ASSERT(m_CurrentPipeline, "cannot draw without binding graphics pipeline");

		CommandEntry entry;
		entry.Type = CommandType::Draw;
		entry.Args = CommandEntry::DrawArgs{ .VertexCount = vertexCount, .VertexOffset = vertexOffset };

		m_CurrentScope.Commands.push_back(entry);
	}

	void CommandList::drawIndexed(uint32_t indexCount, uint32_t vertexOffset, uint32_t indexOffset)
	{
		WR_ASSERT(m_CurrentPipeline, "cannot draw without binding graphics pipeline");

		CommandEntry entry;
		entry.Type = CommandType::DrawIndexed;
		entry.Args = CommandEntry::DrawIndexedArgs{ .IndexCount = indexCount, .VertexOffset = vertexOffset, .IndexOffset = indexOffset };

		m_CurrentScope.Commands.push_back(entry);
	}

}

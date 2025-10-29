#include "CommandList.h"

#include "Device.h"

namespace wire {

    CommandList::CommandList(Device* device, bool singleTimeCommands)
        : m_Device(device), m_SingleTimeCommands(singleTimeCommands)
    {
    }
    
    void CommandList::begin()
    {
        m_CurrentScope = CommandScope{ .ScopeType = CommandScope::General, .CurrentRenderPass = nullptr };
        m_IsRecording = true;

        m_Scopes.clear();
    }

    void CommandList::end()
    {
        m_IsRecording = false;
        m_CurrentGraphicsPipeline = nullptr;
        m_Scopes.push_back(m_CurrentScope);
        m_CurrentScope = {};
    }

    void CommandList::beginRenderPass(const std::shared_ptr<RenderPass>& renderPass)
    {
        WR_ASSERT(!m_SingleTimeCommands, "cannot begin render pass during single time commands");

        m_Scopes.push_back(m_CurrentScope);
        m_CurrentScope = CommandScope{ .ScopeType = CommandScope::RenderPass, .CurrentRenderPass = renderPass };
    }

    void CommandList::endRenderPass()
    {
        m_Scopes.push_back(m_CurrentScope);
        m_CurrentScope = CommandScope{ .ScopeType = CommandScope::General, .CurrentRenderPass = nullptr };
    }

    void CommandList::bindPipeline(const std::shared_ptr<GraphicsPipeline>& pipeline)
    {
        CommandEntry entry;
        entry.Type = CommandType::BindPipeline;
        entry.Args = CommandEntry::BindPipelineArgs{ .IsGraphics = true, .Pipeline = pipeline };

        m_CurrentGraphicsPipeline = pipeline;
        m_CurrentComputePipeline = nullptr;

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::bindPipeline(const std::shared_ptr<ComputePipeline>& pipeline)
    {
        CommandEntry entry;
        entry.Type = CommandType::BindPipeline;
        entry.Args = CommandEntry::BindPipelineArgs{ .IsGraphics = false, .Pipeline = pipeline };

        m_CurrentComputePipeline = pipeline;
        m_CurrentGraphicsPipeline = nullptr;

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::pushConstants(ShaderType shaderStage, const void* data, size_t size, size_t offset)
    {
        WR_ASSERT(m_CurrentGraphicsPipeline || m_CurrentComputePipeline, "cannot push constants without binding a pipeline");
        WR_ASSERT(size <= 128, "push constant size must be <= 128");

        CommandEntry::PushConstantsArgs args{};
        
        if (m_CurrentComputePipeline)
        {
            args.IsGraphics = false;
            args.Pipeline = m_CurrentComputePipeline;
        }
        else
        {
            args.IsGraphics = true;
            args.Pipeline = m_CurrentGraphicsPipeline;
        }

        args.Stage = shaderStage;
        args.Size = static_cast<uint32_t>(size);
        args.Offset = static_cast<uint32_t>(offset);
        std::memcpy(args.Data, data, size);

        CommandEntry entry;
        entry.Type = CommandType::PushConstants;
        entry.Args = args;

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::bindShaderResource(uint32_t set, const std::shared_ptr<ShaderResource>& resource)
    {
        WR_ASSERT(m_CurrentGraphicsPipeline || m_CurrentComputePipeline, "cannot bind descriptor set without binding a pipeline");

        CommandEntry entry;
        entry.Type = CommandType::BindShaderResource;

        if (m_CurrentGraphicsPipeline)
            entry.Args = CommandEntry::BindShaderResourceArgs{ .IsGraphics = true, .Pipeline = m_CurrentGraphicsPipeline, .Set = set, .Resource = resource };
        else if (m_CurrentComputePipeline)
            entry.Args = CommandEntry::BindShaderResourceArgs{ .IsGraphics = false, .Pipeline = m_CurrentComputePipeline, .Set = set, .Resource = resource };

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::setViewport(const glm::vec2& position, const glm::vec2& size, float minDepth, float maxDepth)
    {
        WR_ASSERT(m_CurrentGraphicsPipeline, "cannot set viewport without binding a pipeline");

        CommandEntry entry;
        entry.Type = CommandType::SetViewport;
        entry.Args = CommandEntry::SetViewportArgs{ .Pipeline = m_CurrentGraphicsPipeline, .Position = position, .Size = size, .MinDepth = minDepth, .MaxDepth = maxDepth };

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::setScissor(const glm::vec2& min, const glm::vec2& max)
    {
        WR_ASSERT(m_CurrentGraphicsPipeline, "cannot set scissor without binding a pipeline");

        CommandEntry entry;
        entry.Type = CommandType::SetScissor;
        entry.Args = CommandEntry::SetScissorArgs{ .Pipeline = m_CurrentGraphicsPipeline, .Min = min, .Max = max };

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::setLineWidth(float lineWidth)
    {
        WR_ASSERT(m_CurrentGraphicsPipeline, "cannot set line width without binding a pipeline");

        CommandEntry entry;
        entry.Type = CommandType::SetLineWidth;
        entry.Args = CommandEntry::SetLineWidthArgs{ .Pipeline = m_CurrentGraphicsPipeline, .LineWidth = lineWidth };

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::bindVertexBuffers(const std::vector<std::shared_ptr<Buffer>>& vertexBuffers)
    {
        CommandEntry entry;
        entry.Type = CommandType::BindVertexBuffers;
        entry.Args = CommandEntry::BindVertexBuffersArgs{ .Buffers = vertexBuffers };

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::bindIndexBuffer(const std::shared_ptr<Buffer>& indexBuffer)
    {
        CommandEntry entry;
        entry.Type = CommandType::BindIndexBuffer;
        entry.Args = CommandEntry::BindIndexBufferArgs{ .Buffer = indexBuffer };

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::draw(uint32_t vertexCount, uint32_t vertexOffset)
    {
        WR_ASSERT(m_CurrentGraphicsPipeline, "cannot draw without binding graphics pipeline");

        CommandEntry entry;
        entry.Type = CommandType::Draw;
        entry.Args = CommandEntry::DrawArgs{ .VertexCount = vertexCount, .VertexOffset = vertexOffset };

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::drawIndexed(uint32_t indexCount, uint32_t vertexOffset, uint32_t indexOffset)
    {
        WR_ASSERT(m_CurrentGraphicsPipeline, "cannot draw without binding graphics pipeline");

        CommandEntry entry;
        entry.Type = CommandType::DrawIndexed;
        entry.Args = CommandEntry::DrawIndexedArgs{ .IndexCount = indexCount, .VertexOffset = vertexOffset, .IndexOffset = indexOffset };

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        WR_ASSERT(m_CurrentComputePipeline, "cannot dispatch without binding compute pipeline");

        CommandEntry entry;
        entry.Type = CommandType::Dispatch;
        entry.Args = CommandEntry::DispatchArgs{ .GroupCountX = groupCountX, .GroupCountY = groupCountY, .GroupCountZ = groupCountZ };

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::clearImage(const std::shared_ptr<Framebuffer>& framebuffer, const glm::vec4& color, AttachmentLayout currentLayout, uint32_t baseMip, uint32_t numMips)
    {
        WR_ASSERT(m_CurrentScope.ScopeType == CommandScope::General, "cannot clear image inside a render pass");

        if (currentLayout != AttachmentLayout::TransferDst)
        {
            imageMemoryBarrier(framebuffer, currentLayout, AttachmentLayout::TransferDst, baseMip, numMips);
        }

        CommandEntry entry;
        entry.Type = CommandType::ClearImage;
        entry.Args = CommandEntry::ClearImageArgs{ .Framebuffer = framebuffer, .Color = color, .BaseMipLevel = baseMip, .MipCount = numMips };

        m_CurrentScope.Commands.push_back(entry);

        if (currentLayout != AttachmentLayout::TransferDst)
        {
            imageMemoryBarrier(framebuffer, AttachmentLayout::TransferDst, currentLayout, baseMip, numMips);
        }
    }

    void CommandList::copyBuffer(const std::shared_ptr<Buffer>& srcBuffer, const std::shared_ptr<Buffer>& dstBuffer, size_t size, size_t srcOffset, size_t dstOffset)
    {
        CommandEntry entry;
        entry.Type = CommandType::CopyBuffer;
        entry.Args = CommandEntry::CopyBufferArgs{ .SrcBuffer = srcBuffer, .DstBuffer = dstBuffer, .Size = size, .SrcOffset = srcOffset, .DstOffset = dstOffset };

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::bufferMemoryBarrier(const std::shared_ptr<Buffer>& buffer, BarrierMask waitFor, BarrierMask access, PipelineStage waitStage, PipelineStage untilStage)
    {
        CommandEntry entry;
        entry.Type = CommandType::BufferMemoryBarrier;
        entry.Args = CommandEntry::BufferMemoryBarrierArgs{ .WaitFor = waitFor, .Access = access, .WaitStage = waitStage, .UntilStage = untilStage, .Buffer = buffer };

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::imageMemoryBarrier(const std::shared_ptr<Framebuffer>& framebuffer, AttachmentLayout oldLayout, AttachmentLayout newLayout, uint32_t baseMip, uint32_t numMips)
    {
        CommandEntry entry;
        entry.Type = CommandType::ImageMemoryBarrier;
        entry.Args = CommandEntry::ImageMemoryBarrierArgs{ .Framebuffer = framebuffer, .OldUsage = oldLayout, .NewUsage = newLayout, .BaseMip = baseMip, .NumMips = numMips };

        m_CurrentScope.Commands.push_back(entry);
    }

    void CommandList::submitNativeCommand(std::shared_ptr<CommandListNativeCommand> nativeCommand, std::type_index typeIndex)
    {
        CommandEntry entry;
        entry.Type = CommandType::NativeCommand;
        entry.Args = CommandEntry::NativeCommandArgs{ .CommandType = typeIndex, .NativeCommand = nativeCommand };

        m_CurrentScope.Commands.push_back(entry);
    }

}

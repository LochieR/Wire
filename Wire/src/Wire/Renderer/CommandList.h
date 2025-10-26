#pragma once

#include "Buffer.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "ShaderResource.h"
#include "ComputePipeline.h"
#include "GraphicsPipeline.h"

#include "Wire/Core/Assert.h"

#include <glm/glm.hpp>

#include <vector>
#include <variant>
#include <typeindex>
#include <type_traits>

namespace wire {

    struct CommandListNativeCommand
    {
        virtual ~CommandListNativeCommand() = default;
    };

    enum class CommandType
    {
        BeginRenderPass, EndRenderPass,
        BindPipeline, PushConstants, BindShaderResource, SetViewport, SetScissor, SetLineWidth,
        BindVertexBuffers, BindIndexBuffer,
        ClearImage,
        Draw, DrawIndexed, Dispatch,
        CopyBuffer, BufferMemoryBarrier, ImageMemoryBarrier,
        NativeCommand
    };

    enum class BarrierMask
    {
        ShaderRead = 1 << 5,
        ShaderWrite = 1 << 6,
        ColorAttachmentRead = 1 << 7,
        ColorAttachmentWrite = 1 << 8,
        DepthStencilAttachmentRead = 1 << 9,
        DepthStencilAttachmentWrite = 1 << 10,
        TransferRead = 1 << 11,
        TransferWrite = 1 << 12
    };

    enum class PipelineStage
    {
        TopOfPipe = 1 << 0,
        DrawIndirect = 1 << 1,
        VertexInput = 1 << 2,
        VertexShader = 1 << 3,
        FragmentShader = 1 << 7,
        EarlyFragmentTests = 1 << 8,
        LateFragmentTests = 1 << 9,
        ColorAttachmentOutput = 1 << 10,
        ComputeShader = 1 << 11,
        Transfer = 1 << 12
    };

    struct CommandEntry
    {
    private:
        struct BindPipelineArgs
        {
            bool IsGraphics;
            std::variant<
                std::shared_ptr<GraphicsPipeline>,
                std::shared_ptr<ComputePipeline>
            > Pipeline;
        };

        struct PushConstantsArgs
        {
            bool IsGraphics;
            std::variant<
                std::shared_ptr<GraphicsPipeline>,
                std::shared_ptr<ComputePipeline>
            > Pipeline;
            ShaderType Stage;

            uint32_t Size;
            uint32_t Offset;
            char Data[128];
        };

        struct BindShaderResourceArgs
        {
            bool IsGraphics;
            std::variant<
                std::shared_ptr<GraphicsPipeline>,
                std::shared_ptr<ComputePipeline>
            > Pipeline;

            uint32_t Set;
            std::shared_ptr<ShaderResource> Resource;
        };

        struct SetViewportArgs
        {
            std::shared_ptr<GraphicsPipeline> Pipeline;
            glm::vec2 Position;
            glm::vec2 Size;
            float MinDepth;
            float MaxDepth;
        };

        struct SetScissorArgs
        {
            std::shared_ptr<GraphicsPipeline> Pipeline;
            glm::vec2 Min;
            glm::vec2 Max;
        };

        struct SetLineWidthArgs
        {
            std::shared_ptr<GraphicsPipeline> Pipeline;
            float LineWidth;
        };

        struct BindVertexBuffersArgs
        {
            std::vector<std::shared_ptr<Buffer>> Buffers;
        };

        struct BindIndexBufferArgs
        {
            std::shared_ptr<Buffer> Buffer;
        };

        struct ClearImageArgs
        {
            std::shared_ptr<Framebuffer> Framebuffer;
            glm::vec4 Color;
            uint32_t BaseMipLevel;
            uint32_t MipCount;
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

        struct DispatchArgs
        {
            uint32_t GroupCountX;
            uint32_t GroupCountY;
            uint32_t GroupCountZ;
        };

        struct CopyBufferArgs
        {
            std::shared_ptr<Buffer> SrcBuffer;
            std::shared_ptr<Buffer> DstBuffer;
            size_t Size;
            size_t SrcOffset;
            size_t DstOffset;
        };

        struct BufferMemoryBarrierArgs
        {
            BarrierMask WaitFor;
            BarrierMask Access;
            PipelineStage WaitStage;
            PipelineStage UntilStage;
            std::shared_ptr<Buffer> Buffer;
        };

        struct ImageMemoryBarrierArgs
        {
            std::shared_ptr<Framebuffer> Framebuffer;
            AttachmentLayout OldUsage, NewUsage;
            uint32_t BaseMip;
            uint32_t NumMips;
        };

        struct NativeCommandArgs
        {
            std::type_index CommandType;
            std::shared_ptr<CommandListNativeCommand> NativeCommand;
        };

        friend class CommandList;
        friend class VulkanDevice;
    public:
        CommandType Type;
        std::variant<
            BindPipelineArgs,
            PushConstantsArgs,
            BindShaderResourceArgs,
            SetViewportArgs,
            SetScissorArgs,
            SetLineWidthArgs,
            BindVertexBuffersArgs,
            BindIndexBufferArgs,
            ClearImageArgs,
            DrawArgs,
            DrawIndexedArgs,
            DispatchArgs,
            CopyBufferArgs,
            BufferMemoryBarrierArgs,
            ImageMemoryBarrierArgs,
            NativeCommandArgs
        > Args;
    };

    struct CommandScope
    {
        enum Type { General, RenderPass };
        Type ScopeType;

        std::shared_ptr<::wire::RenderPass> CurrentRenderPass = nullptr;
        std::vector<CommandEntry> Commands;
    };

    class Device;

    class CommandList
    {
    public:
        CommandList() = default;
        CommandList(Device* device, bool singleTimeCommands = false);
        ~CommandList() = default;

        void begin();
        void end();

        void beginRenderPass(const std::shared_ptr<RenderPass>& renderPass);
        void endRenderPass();

        void bindPipeline(const std::shared_ptr<GraphicsPipeline>& pipeline);
        void bindPipeline(const std::shared_ptr<ComputePipeline>& pipeline);
        void pushConstants(ShaderType shaderStage, const void* data, size_t size, size_t offset = 0);
        void bindShaderResource(uint32_t set, const std::shared_ptr<ShaderResource>& resource);

        void setViewport(const glm::vec2& position, const glm::vec2& size, float minDepth, float maxDepth);
        void setScissor(const glm::vec2& min, const glm::vec2& max);
        void setLineWidth(float lineWidth);

        void bindVertexBuffers(const std::vector<std::shared_ptr<Buffer>>& vertexBuffers);
        void bindIndexBuffer(const std::shared_ptr<Buffer>& indexBuffer);

        void draw(uint32_t vertexCount, uint32_t vertexOffset = 0);
        void drawIndexed(uint32_t indexCount, uint32_t vertexOffset = 0, uint32_t indexOffset = 0);

        void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

        void clearImage(const std::shared_ptr<Framebuffer>& framebuffer, const glm::vec4& color, AttachmentLayout currentLayout, uint32_t baseMip = 0, uint32_t numMips = 1);

        void copyBuffer(const std::shared_ptr<Buffer>& srcBuffer, const std::shared_ptr<Buffer>& dstBuffer, size_t size, size_t srcOffset = 0, size_t dstOffset = 0);
        void bufferMemoryBarrier(const std::shared_ptr<Buffer>& buffer, BarrierMask waitFor, BarrierMask access, PipelineStage waitStage, PipelineStage untilStage);

        void imageMemoryBarrier(const std::shared_ptr<Framebuffer>& framebuffer, AttachmentLayout oldLayout, AttachmentLayout newLayout, uint32_t baseMip = 0, uint32_t numMips = 1);

        void submitNativeCommand(std::shared_ptr<CommandListNativeCommand> nativeCommand, std::type_index typeIndex);

        bool isRecording() const { return m_IsRecording; }
        bool isSingleTimeCommands() const { return m_SingleTimeCommands; }
        
        const std::vector<CommandScope>& getScopes() const { return m_Scopes; }

        Device* getDevice() const { return m_Device; }

        template<typename T>
        void pushConstants(ShaderType shaderStage, const T& value, size_t offset = 0)
        {
            pushConstants(shaderStage, &value, sizeof(T), offset);
        }
    private:
        Device* m_Device;

        bool m_SingleTimeCommands;
        bool m_IsRecording = false;

        std::vector<CommandScope> m_Scopes;
        CommandScope m_CurrentScope;

        std::shared_ptr<GraphicsPipeline> m_CurrentGraphicsPipeline = nullptr;
        std::shared_ptr<ComputePipeline> m_CurrentComputePipeline = nullptr;
    };

}

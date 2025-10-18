#pragma once

#include "Buffer.h"
#include "RenderPass.h"
#include "Framebuffer.h"
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
        BindPipeline, PushConstants, BindDescriptorSet, SetViewport, SetScissor, SetLineWidth,
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
            union
            {
                GraphicsPipeline* Graphics;
                ComputePipeline* Compute;
            };
        };

        struct PushConstantsArgs
        {
            bool IsGraphics;
            union
            {
                GraphicsPipeline* Graphics;
                ComputePipeline* Compute;
            };
            ShaderType Stage;

            uint32_t Size;
            uint32_t Offset;
            char Data[128];
        };

        struct BindDescriptorSetArgs
        {
            bool IsGraphics;
            union
            {
                GraphicsPipeline* Graphics;
                ComputePipeline* Compute;
            };

            uint32_t Set;
            uint32_t SetIndex;
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
            std::vector<BufferBase*> Buffers;
        };

        struct BindIndexBufferArgs
        {
            BufferBase* Buffer;
        };

        struct ClearImageArgs
        {
            Framebuffer* Framebuffer;
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
            BufferBase* SrcBuffer;
            BufferBase* DstBuffer;
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
            BufferBase* Buffer;
        };

        struct ImageMemoryBarrierArgs
        {
            Framebuffer* Framebuffer;
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
        friend class VulkanRenderer;
    public:
        CommandType Type;
        std::variant<
            BindPipelineArgs,
            PushConstantsArgs,
            BindDescriptorSetArgs,
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

        ::wire::RenderPass* CurrentRenderPass = nullptr;
        std::vector<CommandEntry> Commands;
    };

    class Renderer;

    class CommandList
    {
    public:
        CommandList() = default;
        CommandList(Renderer* renderer, bool singleTimeCommands = false);
        ~CommandList() = default;

        void begin();
        void end();

        void beginRenderPass(RenderPass* renderPass);
        void endRenderPass();

        void bindPipeline(GraphicsPipeline* pipeline);
        void bindPipeline(ComputePipeline* pipeline);
        void pushConstants(ShaderType shaderStage, const void* data, size_t size, size_t offset = 0);
        void bindDescriptorSet(uint32_t set, uint32_t setIndex);

        void setViewport(const glm::vec2& position, const glm::vec2& size, float minDepth, float maxDepth);
        void setScissor(const glm::vec2& min, const glm::vec2& max);
        void setLineWidth(float lineWidth);

        void bindVertexBuffers(const std::vector<BufferBase*> vertexBuffers);
        void bindIndexBuffer(BufferBase* indexBuffer);

        void draw(uint32_t vertexCount, uint32_t vertexOffset = 0);
        void drawIndexed(uint32_t indexCount, uint32_t vertexOffset = 0, uint32_t indexOffset = 0);

        void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

        void clearImage(Framebuffer* framebuffer, const glm::vec4& color, AttachmentLayout currentLayout, uint32_t baseMip = 0, uint32_t numMips = 1);

        void copyBuffer(BufferBase* srcBuffer, BufferBase* dstBuffer, size_t size, size_t srcOffset = 0, size_t dstOffset = 0);
        void bufferMemoryBarrier(BufferBase* buffer, BarrierMask waitFor, BarrierMask access, PipelineStage waitStage, PipelineStage untilStage);

        void imageMemoryBarrier(Framebuffer* framebuffer, AttachmentLayout oldLayout, AttachmentLayout newLayout, uint32_t baseMip = 0, uint32_t numMips = 1);

        void submitNativeCommand(std::shared_ptr<CommandListNativeCommand> nativeCommand, std::type_index typeIndex);

        bool isRecording() const { return m_IsRecording; }
        bool isSingleTimeCommands() const { return m_SingleTimeCommands; }
        
        const std::vector<CommandScope>& getScopes() const { return m_Scopes; }

        Renderer* getRenderer() const { return m_Renderer; }

        template<typename T>
        void pushConstants(ShaderType shaderStage, const T& value, size_t offset = 0)
        {
            pushConstants(shaderStage, &value, sizeof(T), offset);
        }

        template<BufferType... Type>
        std::enable_if_t<std::conjunction_v<std::bool_constant<is_vertex_buffer<Type>()>...>> bindVertexBuffers(Buffer<Type>*... buffers)
        {
            std::vector<BufferBase*> bases;
            (bases.push_back(buffers->getBase()), ...);

            bindVertexBuffers(bases);
        }

        template<BufferType Type>
        std::enable_if_t<Type & IndexBuffer, void> bindIndexBuffer(Buffer<Type>* indexBuffer)
        {
            bindIndexBuffer(indexBuffer->getBase());
        }

        template<BufferType Type1, BufferType Type2>
        void copyBuffer(Buffer<Type1>* srcBuffer, Buffer<Type2>* dstBuffer, size_t size, size_t srcOffset = 0, size_t dstOffset = 0)
        {
            copyBuffer(srcBuffer->getBase(), dstBuffer->getBase(), size, srcOffset, dstOffset);
        }

        template<BufferType Type>
        void bufferMemoryBarrier(Buffer<Type>* buffer, BarrierMask waitFor, BarrierMask access, PipelineStage waitStage, PipelineStage untilStage)
        {
            bufferMemoryBarrier(buffer->getBase(), waitFor, access, waitStage, untilStage);
        }
    private:
        Renderer* m_Renderer;

        bool m_SingleTimeCommands;
        bool m_IsRecording = false;

        std::vector<CommandScope> m_Scopes;
        CommandScope m_CurrentScope;

        GraphicsPipeline* m_CurrentGraphicsPipeline = nullptr;
        ComputePipeline* m_CurrentComputePipeline = nullptr;
    };

}

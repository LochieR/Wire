#pragma once

#include <string>
#include <vector>

#include "Texture2D.h"
#include "RenderPass.h"
#include "ShaderCache.h"
#include "ShaderCompiler.h"
#include "ShaderResource.h"

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

    struct InputLayout
    {
        std::vector<InputElement> VertexBufferLayout;
        size_t Stride = 0;

        std::vector<PushConstantInfo> PushConstantInfos;
        ShaderResourceLayout* ResourceLayout;
    };

    struct GraphicsPipelineDesc
    {
        std::string ShaderPath;
        InputLayout Layout;
        PrimitiveTopology Topology;
        RenderPass* RenderPass;
    };

    class GraphicsPipeline
    {
    public:
        virtual ~GraphicsPipeline() = default;
    };

}

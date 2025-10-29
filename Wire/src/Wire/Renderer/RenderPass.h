#pragma once

#include "IResource.h"
#include "Wire/Core/Assert.h"

#include <vector>

namespace wire {

    enum class AttachmentFormat;

    enum class LoadOperation : uint8_t
    {
        Load = 0,
        Clear = 1,
        DontCare = 2,
    };

    enum class StoreOperation : uint8_t
    {
        Store = 0,
        DontCare = 1,
    };

    enum class AttachmentLayout : uint8_t
    {
        Undefined = 0,
        General,
        ShaderReadOnly,
        Present,
        Color,
        Depth,
        TransferSrc,
        TransferDst,
    };

    enum class BlendFactor
    {
        Zero = 0,
        One = 1,
        SrcColor = 2,
        OneMinusSrcColor = 3,
        DstColor = 4,
        OneMinusDstColor = 5,
        SrcAlpha = 6,
        OneMinusSrcAlpha = 7,
        DstAlpha = 8,
        OneMinusDstAlpha = 9,
        ConstantColor = 10,
        OneMinusConstantColor = 11,
        ConstantAlpha = 12,
        OneMinusConstantAlpha = 13,
        SrcAlphaSaturate  = 14,
        Src1Color = 15,
        OneMinusSrc1Color = 16,
        Src1Alpha = 17,
        OneMinusSrc1Alpha = 18
    };

    enum class BlendOperation
    {
        Add = 0,
        Subtract = 1,
        ReverseSubtract = 2,
        Min = 3,
        Max = 4
    };

    enum ColorComponentFlags : uint32_t
    {
        ColorComponentR = 1 << 0,
        ColorComponentG = 1 << 1,
        ColorComponentB = 1 << 2,
        ColorComponentA = 1 << 3,
    };
    using ColorComponentFlagBits = uint32_t;

    struct BlendState
    {
        bool BlendEnable = true;
        BlendFactor SrcColorBlendFactor = BlendFactor::SrcAlpha;
        BlendFactor DstColorBlendFactor = BlendFactor::OneMinusSrcAlpha;
        BlendOperation ColorBlendOp = BlendOperation::Add;
        BlendFactor SrcAlphaBlendFactor = BlendFactor::One;
        BlendFactor DstAlphaBlendFactor = BlendFactor::Zero;
        BlendOperation AlphaBlendOp = BlendOperation::Add;
        ColorComponentFlagBits ColorWriteMask = ColorComponentR | ColorComponentG | ColorComponentB | ColorComponentA;
    };

    struct AttachmentDesc
    {
        enum SampleCount
        {
            Count1Bit = 0x00000001,
            Count2Bit = 0x00000002,
            Count4Bit = 0x00000004,
            Count8Bit = 0x00000008,
            Count16Bit = 0x00000010,
            Count32Bit = 0x00000020,
            Count64Bit = 0x00000040,
        };

        AttachmentFormat Format;
        AttachmentLayout Usage;
        AttachmentLayout PreviousAttachmentUsage;
        SampleCount Samples;
        LoadOperation LoadOp;
        StoreOperation StoreOp;
        LoadOperation StencilLoadOp;
        StoreOperation StencilStoreOp;
        BlendState BlendState;
    };

    struct RenderPassDesc
    {
        std::vector<AttachmentDesc> Attachments;
    };

    class RenderPass : public IResource
    {
    public:
        virtual ~RenderPass() = default;

        virtual void recreate() = 0;
        virtual void recreateFramebuffers() = 0;
    };

}

#pragma once

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
    };

    struct RenderPassDesc
    {
        std::vector<AttachmentDesc> Attachments;
    };

    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        virtual void recreate() = 0;
        virtual void recreateFramebuffers() = 0;
    };

}

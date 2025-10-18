#pragma once

#include "Swapchain.h"
#include "Texture2D.h"

namespace wire {

    enum class AttachmentUsage : uint16_t
    {
        TransferSrc = 1 << 0,
        TransferDst = 1 << 1,
        Sampled = 1 << 2,
        Storage = 1 << 3,
        ColorAttachment = 1 << 4,
        DepthStencilAttachment = 1 << 5,
        TransientAttachment = 1 << 6,
        InputAttachment = 1 << 7,
        HostTransfer = 1 << 8
    };

    constexpr inline AttachmentUsage operator|(AttachmentUsage lhs, AttachmentUsage rhs)
    {
        return static_cast<AttachmentUsage>(
            static_cast<std::underlying_type<AttachmentUsage>::type>(lhs) |
            static_cast<std::underlying_type<AttachmentUsage>::type>(rhs)
        );
    }

    constexpr inline AttachmentUsage operator&(AttachmentUsage lhs, AttachmentUsage rhs)
    {
        return static_cast<AttachmentUsage>(
            static_cast<std::underlying_type<AttachmentUsage>::type>(lhs) &
            static_cast<std::underlying_type<AttachmentUsage>::type>(rhs)
        );
    }

    constexpr inline AttachmentUsage operator^(AttachmentUsage lhs, AttachmentUsage rhs)
    {
        return static_cast<AttachmentUsage>(
            static_cast<std::underlying_type<AttachmentUsage>::type>(lhs) ^
            static_cast<std::underlying_type<AttachmentUsage>::type>(rhs)
        );
    }

    constexpr inline AttachmentUsage operator~(AttachmentUsage type)
    {
        return static_cast<AttachmentUsage>(~static_cast<std::underlying_type<AttachmentUsage>::type>(type));
    }

    constexpr inline AttachmentUsage& operator|=(AttachmentUsage& lhs, AttachmentUsage rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    constexpr inline AttachmentUsage& operator&=(AttachmentUsage& lhs, AttachmentUsage rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }

    constexpr inline AttachmentUsage& operator^=(AttachmentUsage& lhs, AttachmentUsage rhs)
    {
        lhs = lhs ^ rhs;
        return lhs;
    }

    struct FramebufferDesc
    {
        AttachmentFormat Format;
        AttachmentUsage Usage;
        AttachmentLayout Layout;
        glm::vec2 Extent;

        uint32_t MipCount;

        bool HasDepth;
        AttachmentFormat DepthFormat;
    };

    class Framebuffer
    {
    public:
        virtual ~Framebuffer() = default;

        virtual void resize(const glm::vec2& extent) = 0;
        virtual glm::vec2 getExtent() const = 0;
        virtual uint32_t getNumMips() const = 0;

        virtual Texture2D* asTexture2D() const = 0;
    };

}

#pragma once

#include "RenderPass.h"

#include <glm/glm.hpp>

#include <vector>

namespace wire {

    enum class AttachmentFormat
    {
        SwapchainColorDefault = 0,
        SwapchainDepthDefault = 1,

        R8_UInt,
        R16_UInt,
        R32_UInt,
        R64_UInt,
        R8_SInt,
        R16_SInt,
        R32_SInt,
        R64_SInt,
        R8_UNorm,
        R16_UNorm,
        R32_SFloat,

        BGRA8_UNorm,
        RGBA8_UNorm,
        RGBA16_SFloat,
        RGBA32_SFloat,

        D32_SFloat,
        // TBC
    };

    enum class PresentMode
    {
        SwapchainDefault = 0,
        Mailbox,
        Fifo,
        MailboxOrFifo,
        Immediate
    };

    struct SwapchainDesc
    {
        std::vector<AttachmentFormat> Attachments;
        PresentMode PresentMode;
    };

    class Swapchain
    {
    public:
        virtual ~Swapchain() = default;

        virtual bool acquireNextImage(uint32_t& imageIndex) = 0;

        virtual void recreateSwapchain() = 0;

        virtual glm::vec2 getExtent() const = 0;
        virtual uint32_t getImageCount() const = 0;

        virtual const SwapchainDesc& getDesc() const = 0;
    };

}

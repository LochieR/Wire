#pragma once

#include <cstdint>

#include "Wire/Core/UUID.h"

namespace wire {

    class Texture2D
    {
    public:
        virtual ~Texture2D() = default;

        virtual uint32_t getWidth() const = 0;
        virtual uint32_t getHeight() const = 0;

        virtual UUID getUUID() const = 0;
    };

    enum class SamplerFilter
    {
        Nearest = 0,
        Linear = 1
    };

    enum class AddressMode
    {
        Repeat = 0,
        MirroredRepeat = 1,
        ClampToEdge = 2,
        ClampToBorder = 3,
        MirrorClampToEdge = 4
    };
    
    enum class BorderColor
    {
        FloatTransparentBlack = 0,
        IntTransparentBlack = 1,
        FloatOpaqueBlack = 2,
        IntOpaqueBlack = 3,
        FloatOpaqueWhite = 4,
        IntOpaqueWhite = 5
    };

    enum class MipmapMode
    {
        Nearest = 0,
        Linear
    };

    struct SamplerDesc
    {
        SamplerFilter MinFilter;
        SamplerFilter MagFilter;
        AddressMode AddressModeU;
        AddressMode AddressModeV;
        AddressMode AddressModeW;
        bool EnableAnisotropy;
        float MaxAnisotropy;
        BorderColor BorderColor;
        MipmapMode MipmapMode;
        bool CompareEnable;
        float MinLod;
        float MaxLod;
        bool NoMaxLodClamp;
        float MipLodBias;
    };

    class Sampler
    {
    public:
        virtual ~Sampler() = default;
    };

}

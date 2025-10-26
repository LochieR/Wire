#pragma once

#include "IResource.h"

#include <cstdint>
#include <type_traits>

using size_t = std::size_t;

namespace wire {

    enum BufferType : uint16_t
    {
        VertexBuffer  = 1 << 0,
        IndexBuffer   = 1 << 1,
        StagingBuffer = 1 << 2,
        UniformBuffer = 1 << 3,
        StorageBuffer = 1 << 4
    };

    constexpr inline BufferType operator|(BufferType lhs, BufferType rhs)
    {
        return static_cast<BufferType>(
            static_cast<std::underlying_type<BufferType>::type>(lhs) |
            static_cast<std::underlying_type<BufferType>::type>(rhs)
        );
    }

    constexpr inline BufferType operator&(BufferType lhs, BufferType rhs)
    {
        return static_cast<BufferType>(
            static_cast<std::underlying_type<BufferType>::type>(lhs) &
            static_cast<std::underlying_type<BufferType>::type>(rhs)
        );
    }

    constexpr inline BufferType operator^(BufferType lhs, BufferType rhs)
    {
        return static_cast<BufferType>(
            static_cast<std::underlying_type<BufferType>::type>(lhs) ^
            static_cast<std::underlying_type<BufferType>::type>(rhs)
        );
    }

    constexpr inline BufferType operator~(BufferType type)
    {
        return static_cast<BufferType>(~static_cast<std::underlying_type<BufferType>::type>(type));
    }

    constexpr inline BufferType& operator|=(BufferType& lhs, BufferType rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    constexpr inline BufferType& operator&=(BufferType& lhs, BufferType rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }

    constexpr inline BufferType& operator^=(BufferType& lhs, BufferType rhs)
    {
        lhs = lhs ^ rhs;
        return lhs;
    }

    class Buffer : public IResource
    {
    public:
        virtual ~Buffer() = default;

        virtual void setData(const void* data, size_t size, size_t offset = 0) = 0;
        virtual void setData(int data, size_t size) = 0;

        virtual void* map(size_t size) = 0;
        virtual void unmap() = 0;

        virtual size_t getSize() const = 0;
    };

}

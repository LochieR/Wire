#pragma once

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

    template<BufferType Type>
    constexpr bool is_vertex_buffer()
    {
        return Type & VertexBuffer;
    }

    class BufferBase
    {
    public:
        virtual ~BufferBase() = default;

        virtual void setData(const void* data, size_t size, size_t offset = 0) = 0;
        virtual void setData(int data, size_t size) = 0;

        virtual void* map(size_t size) = 0;
        virtual void unmap() = 0;

        virtual size_t getSize() const = 0;
    };

    template<BufferType Type>
    class Buffer
    {
    public:
        Buffer(BufferBase* base)
            : m_Base(base)
        {
        }
        ~Buffer() { delete m_Base; }

        void setData(const void* data, size_t size, size_t offset = 0) { m_Base->setData(data, size, offset); }
        void setData(int data, size_t size) { m_Base->setData(data, size); }

        void* map(size_t size) { return m_Base->map(size); }
        void unmap() { m_Base->unmap(); }

        size_t getSize() const { return m_Base->getSize(); }

        BufferBase* getBase() const { return m_Base; }
    private:
        BufferBase* m_Base;
    };

}

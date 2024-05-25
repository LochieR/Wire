#pragma once

#include <stdint.h>
#include <cstring>

#include <glm/glm.hpp>

namespace Wire {

	struct Buffer
	{
		uint8_t* Data = nullptr;
		uint64_t Size = 0;

		Buffer() = default;

		Buffer(uint64_t size)
		{
			Allocate(size);
		}

		Buffer(const Buffer&) = default;

		Buffer(std::nullptr_t)
			: Buffer()
		{
		}

		static Buffer Copy(Buffer other)
		{
			Buffer result(other.Size);
			memcpy(result.Data, other.Data, other.Size);
			return result;
		}

		void Allocate(uint64_t size)
		{
			Release();

			Data = new uint8_t[size];
			Size = size;
		}

		void Release()
		{
			delete[] Data;
			Data = nullptr;
			Size = 0;
		}

		template<typename T>
		T* As()
		{
			return (T*)Data;
		}

		operator bool() const
		{
			return Data;
		}
	};

	struct ScopedBuffer
	{
		ScopedBuffer(Buffer buffer)
			: m_Buffer(buffer)
		{
		}

		ScopedBuffer(uint64_t size)
			: m_Buffer(size)
		{
		}

		~ScopedBuffer()
		{
			m_Buffer.Release();
		}

		uint8_t* Data() { return m_Buffer.Data; }
		uint64_t Size() { return m_Buffer.Size; }

		template<typename T>
		T* As()
		{
			return m_Buffer.As<T>();
		}

		operator bool() const { return m_Buffer; }
	private:
		Buffer m_Buffer;
	};

	struct ICircularBuffer
	{
		virtual ~ICircularBuffer() = default;

		virtual void AddPoint(float x, float y) = 0;
		virtual void Reset() = 0;

		virtual size_t GetCurrentPosition() const = 0;
	};

	template<size_t capacity>
	struct CircularBuffer : public ICircularBuffer
	{
		CircularBuffer()
		{
			m_XBuffer.fill(0);
			m_YBuffer.fill(0);
		}

		virtual ~CircularBuffer() = default;

		/*CircularBuffer(std::initializer_list<glm::vec2> init)
			: m_Buffer(init)
		{
		}*/

		void AddPoint(float x, float y) override
		{
			AddPoint({ x, y });
		}

		void AddPoint(const glm::vec2& pos)
		{
			m_XBuffer[m_CurrentPosition] = pos.x;
			m_YBuffer[m_CurrentPosition] = pos.y;
			m_CurrentPosition++;

			if (m_CurrentPosition == capacity)
			{
				m_CurrentPosition = 0;
			}
		}

		void Reset() override
		{
			m_XBuffer.fill(0);
			m_YBuffer.fill(0);
		}

		static constexpr size_t Capacity() { return capacity; }

		float* XData()
		{
			return m_XBuffer.data();
		}

		float* YData()
		{
			return m_YBuffer.data();
		}

		size_t GetCurrentPosition() const override { return m_CurrentPosition; }

		std::array<float, capacity>& GetXBuffer() { return m_XBuffer; }
		const std::array<float, capacity>& GetXBuffer() const { return m_XBuffer; }
		std::array<float, capacity>& GetYBuffer() { return m_YBuffer; }
		const std::array<float, capacity>& GetYBuffer() const { return m_YBuffer; }

		/*float& operator[](uint8_t xy, int index)
		{
			if (xy == 0)
				return m_XBuffer[index];
			else if (xy == 1)
				return m_YBuffer[index];
			else
				WR_ASSERT(false);

			return m_XBuffer[0];
		}*/
	private:
		std::array<float, capacity> m_XBuffer;
		std::array<float, capacity> m_YBuffer;

		size_t m_CurrentPosition = 0;
	};

}

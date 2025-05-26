module;

#include <string>

export module wire.core.buffer;

namespace wire {

	export struct MemoryBuffer
	{
		void* Data;
		size_t Size;

		MemoryBuffer()
			: Data(nullptr), Size(0)
		{
		}

		MemoryBuffer(const void* data, size_t size)
			: Data(const_cast<void*>(data)), Size(size)
		{
		}

		MemoryBuffer(const MemoryBuffer& other, size_t size)
			: Data(other.Data), Size(size)
		{
		}

		static MemoryBuffer copy(const MemoryBuffer& other)
		{
			MemoryBuffer buffer;
			buffer.allocate(other.Size);
			memcpy(buffer.Data, other.Data, other.Size);
			return buffer;
		}

		static MemoryBuffer copy(const void* data, size_t size)
		{
			MemoryBuffer buffer;
			buffer.allocate(size);
			memcpy(buffer.Data, data, size);
			return buffer;
		}

		void allocate(size_t size)
		{
			delete[] reinterpret_cast<uint8_t*>(Data);
			Data = nullptr;

			if (size == 0)
				return;

			Data = new uint8_t[size];
			Size = size;
		}

		void release()
		{
			delete[] reinterpret_cast<uint8_t*>(Data);
			Data = nullptr;
			Size = 0;
		}

		operator bool() const
		{
			return Data;
		}

		uint8_t& operator[](int index)
		{
			return ((uint8_t*)Data)[index];
		}

		uint8_t operator[](int index) const
		{
			return ((uint8_t*)Data)[index];
		}

		template<typename T>
		T* as() const
		{
			return (T*)Data;
		}
	};

}

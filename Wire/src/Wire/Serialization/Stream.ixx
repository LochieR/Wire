module;

#include "Wire/Core/Assert.h"

#include <map>
#include <vector>
#include <ostream>
#include <istream>
#include <concepts>
#include <functional>
#include <unordered_map>

export module wire.serialization:stream;

import wire.core.buffer;

namespace wire {

	class StreamWriter;
	class StreamReader;

	template<typename C>
	concept Serializable = requires {
		{ C::Serialize(std::declval<StreamWriter&>(), std::declval<const C&>()) } -> std::same_as<void>;
	};

	template<typename C>
	concept Deserializable = requires {
		{ C::Deserialize(std::declval<StreamReader&>(), std::declval<C&>()) } -> std::same_as<void>;
	};

	export class StreamWriter
	{
	public:
		StreamWriter(std::ostream& stream);

		void writeString(const std::string& string);
		void writeBuffer(MemoryBuffer buffer, bool writeSize = true);

		template<typename T>
		void writeRaw(const T& value)
		{
			m_Stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
		}

		template<typename T>
		void writeObject(const T& obj)
		{
			T::serialize(this, obj);
		}

		template<typename T>
		void writeObject(const T& obj, std::function<void(StreamWriter&, const T&)>&& serializeT)
		{
			serializeT(*this, obj);
		}

		template<typename Key, typename Value>
		void writeMap(const std::map<Key, Value>& map, bool writeSize = true)
		{
			if (writeSize)
				writeRaw<size_t>(map.size());

			for (const auto& [key, value] : map)
			{
				if constexpr (Serializable<Key>)
					writeObject<Key>(key);
				else
					writeRaw<Key>(key);

				if constexpr (Serializable<Value>)
					writeObject<Value>(value);
				else
					writeRaw<Value>(value);
			}
		}

		template<typename Key, typename Value>
		void writeMap(const std::unordered_map<Key, Value>& map, bool writeSize = true)
		{
			if (writeSize)
				writeRaw<size_t>(map.size());

			for (const auto& [key, value] : map)
			{
				if constexpr (Serializable<Key>)
					writeObject<Key>(key);
				else
					writeRaw<Key>(key);

				if constexpr (Serializable<Value>)
					writeObject<Value>(value);
				else
					writeRaw<Value>(value);
			}
		}

		template<typename Value>
		void writeMap(const std::unordered_map<std::string, Value>& map, bool writeSize = true)
		{
			if (writeSize)
				writeRaw<size_t>(map.size());

			for (const auto& [key, value] : map)
			{
				writeString(key);

				if constexpr (Serializable<Value>)
					writeObject<Value>(value);
				else
					writeRaw<Value>(value);
			}
		}

		template<typename T>
		void writeArray(const std::vector<T>& array, bool writeSize = true)
		{
			if (writeSize)
				writeRaw<size_t>(array.size());

			for (const auto& element : array)
			{
				if constexpr (std::is_trivial<T>())
					writeRaw<T>(element);
				else
					writeObject<T>(element);
			}
		}

		template<typename T>
		void writeArray(const std::vector<T>& array, std::function<void(StreamWriter&, const T&)> serializeT, bool writeSize = true)
		{
			if (writeSize)
				writeRaw<size_t>(array.size());

			for (const auto& element : array)
			{
				serializeT(*this, element);
			}
		}
	private:
		std::ostream& m_Stream;
	};

	export class StreamReader
	{
	public:
		StreamReader(std::istream& stream);

		void readString(std::string& string);
		void readBuffer(MemoryBuffer& buffer, size_t size = 0);

		template<typename T>
		void readRaw(T& value)
		{
			m_Stream.read(reinterpret_cast<char*>(&value), sizeof(T));
		}

		template<typename T>
		void readObject(T& obj)
		{
			T::deserialize(this, obj);
		}

		template<typename T>
		void readObject(T& obj, std::function<void(StreamReader&, T&)>&& deserializeT)
		{
			deserializeT(*this, obj);
		}

		template<typename Key, typename Value>
		void readMap(std::map<Key, Value>& map, size_t size = 0)
		{
			if (size == 0)
				readRaw<size_t>(size);

			for (size_t i = 0; i < size; i++)
			{
				Key key;
				if constexpr (Deserializable<Key>)
					readObject<Key>(key);
				else
					readRaw<Key>(key);

				if constexpr (Deserializable<Value>)
					readObject<Value>(map[key]);
				else
					readRaw<Value>(map[key]);
			}
		}

		template<typename Key, typename Value>
		void ReadMap(std::unordered_map<Key, Value>& map, size_t size = 0)
		{
			if (size == 0)
				readRaw<size_t>(size);

			for (size_t i = 0; i < size; i++)
			{
				Key key;
				if constexpr (Deserializable<Key>)
					readObject<Key>(key);
				else
					readRaw<Key>(key);

				if constexpr (Deserializable<Value>)
					readObject<Value>(map[key]);
				else
					readRaw<Value>(map[key]);
			}
		}

		template<typename Value>
		void ReadMap(std::unordered_map<std::string, Value>& map, size_t size = 0)
		{
			if (size == 0)
				readRaw<size_t>(size);

			for (size_t i = 0; i < size; i++)
			{
				std::string key;
				readString(key);

				if constexpr (Deserializable<Value>)
					readObject<Value>(map[key]);
				else
					readRaw<Value>(map[key]);
			}
		}

		template<typename T>
		void readArray(std::vector<T>& array, size_t size = 0)
		{
			if (size == 0)
				readRaw(size);

			array.resize(size);

			for (uint32_t i = 0; i < size; i++)
			{
				if constexpr (std::is_trivial<T>())
					readRaw<T>(array[i]);
				else
					readObject<T>(array[i]);
			}
		}

		template<typename T>
		void readArray(std::vector<T>& array, std::function<void(StreamReader&, T&)> deserializeT, size_t size = 0)
		{
			if (size == 0)
				readRaw(size);

			array.resize(size);

			for (uint32_t i = 0; i < size; i++)
			{
				deserializeT(*this, array[i]);
			}
		}

		std::istream& getStream() { return m_Stream; }
	private:
		std::istream& m_Stream;
	};

}

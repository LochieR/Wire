module;

#include <vector>
#include <ostream>
#include <istream>
#include <iostream>
#include <functional>

module wire.serialization:stream;

import wire.core.buffer;

namespace wire {

	StreamWriter::StreamWriter(std::ostream& stream)
		: m_Stream(stream)
	{
	}

	void StreamWriter::writeString(const std::string& string)
	{
		writeRaw<size_t>(string.length());
		m_Stream.write(string.c_str(), string.length());
	}

	void StreamWriter::writeBuffer(MemoryBuffer buffer, bool writeSize)
	{
		if (writeSize)
			writeRaw<size_t>(buffer.Size);

		m_Stream.write(reinterpret_cast<char*>(buffer.Data), buffer.Size);
	}

	StreamReader::StreamReader(std::istream& stream)
		: m_Stream(stream)
	{
	}

	void StreamReader::readString(std::string& string)
	{
		size_t size;
		readRaw(size);

		string.resize(size);
		m_Stream.read(reinterpret_cast<char*>(string.data()), size);
	}

	void StreamReader::readBuffer(MemoryBuffer& buffer, size_t size)
	{
		buffer.Size = size;
		if (size == 0)
		{
			readRaw<size_t>(buffer.Size);
		}

		buffer.allocate(buffer.Size);
		m_Stream.read(reinterpret_cast<char*>(buffer.Data), buffer.Size);
	}

}

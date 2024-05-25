#include "wrpch.h"
#include "FileSystem.h"

#include <fstream>

namespace Wire {

	Buffer FileSystem::ReadFileBytes(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

		if (!stream)
			return nullptr;

		std::streampos end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		uint64_t size = end - stream.tellg();

		if (size == 0)
			return nullptr;

		Buffer buffer(size);
		stream.read(buffer.As<char>(), buffer.Size);
		stream.close();

		return buffer;
	}

}

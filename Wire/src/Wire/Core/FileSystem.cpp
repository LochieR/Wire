#include "wrpch.h"
#include "FileSystem.h"

namespace Wire {

	Buffer FileSystem::ReadFileBinary(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

		if (!stream)
		{
			// Failed to open the file
			return nullptr;
		}

		std::streampos end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		uint64_t size = end - stream.tellg();

		if (size == 0)
		{
			// File is empty
			return nullptr;
		}

		Buffer buffer(size);
		stream.read(buffer.As<char>(), buffer.Size);
		stream.close();

		return buffer;
	}

}

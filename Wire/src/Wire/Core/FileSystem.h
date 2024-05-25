#pragma once

#include "Buffer.h"

#include <filesystem>

namespace Wire {

	class FileSystem
	{
	public:
		static Buffer ReadFileBytes(const std::filesystem::path& filepath);
	};

}

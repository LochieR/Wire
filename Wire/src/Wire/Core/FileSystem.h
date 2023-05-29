#pragma once

#include "Buffer.h"

namespace Wire {

	class FileSystem
	{
	public:
		static Buffer ReadFileBinary(const std::filesystem::path& filepath);
	};

}

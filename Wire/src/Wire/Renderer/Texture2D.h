#pragma once

#include "IResource.h"

#include <cstdint>

namespace Wire {

	class Texture2D : public IResource
	{
	public:
		virtual ~Texture2D() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
	};

}

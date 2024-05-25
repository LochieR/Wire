#pragma once

#include "Wire/Core/Base.h"
#include "Texture2D.h"

#include "IResource.h"

#include <string>
#include <filesystem>

namespace Wire {

	struct MSDFData;
	class Renderer;

	class Font : public IResource
	{
	public:
		Font() = default;
		~Font();

		const MSDFData* GetMSDFData() const { return m_Data; }
		rbRef<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }
	private:
		Font(Renderer* renderer, std::string_view path, uint32_t minChar = 0x0020, uint32_t maxChar = 0x00FF);
	private:
		Renderer* m_Renderer = nullptr;

		MSDFData* m_Data = nullptr;
		rbRef<Texture2D> m_AtlasTexture = nullptr;

		friend class VulkanRenderer;
	};

}

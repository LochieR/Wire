#pragma once

#include <filesystem>
#include "Wire/Core/Core.h"
#include "Wire/Renderer/Texture.h"

namespace Wire {

	struct MSDFData;

	class Font
	{
	public: 
		Font(const std::filesystem::path& filepath);
		~Font();

		const MSDFData* GetMSDFData() const { return m_Data; }
		Ref<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }

		static Ref<Font> GetDefault();
	private:
		MSDFData* m_Data;
		Ref<Texture2D> m_AtlasTexture;
	};

}

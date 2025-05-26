module;

#undef INFINITE
#include <msdf-atlas-gen.h>

#include <string>
#include <vector>
#include <filesystem>

export module wire.ui.renderer:font;

import :texture2D;

namespace wire {

	export class Font
	{
	public:
		virtual ~Font() = default;

		virtual Texture2D* getAtlasTexture() const = 0;
		virtual const msdf_atlas::FontGeometry& getFontGeometry() const = 0;
		virtual const std::vector<msdf_atlas::GlyphGeometry>& getGlyphs() const = 0;
	};

	export struct NaiveFont
	{
		std::string Name;
		uint32_t MinChar = 0x0020, MaxChar = 0x00FF;
		size_t FontFileSize;
		uint8_t* FontFileData;
		size_t AtlasSize;
		uint32_t* AtlasData;
		uint32_t Width, Height;

		static NaiveFont create(const std::filesystem::path& path, uint32_t minChar = 0x0020, uint32_t maxChar = 0x00FF);
		void release();
	};

}

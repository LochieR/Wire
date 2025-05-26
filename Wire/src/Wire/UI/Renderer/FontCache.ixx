module;

#include <msdf-atlas-gen.h>

#include <filesystem>

export module wire.ui.renderer:fontCache;

import :font;

namespace wire {

	export struct FontInfo
	{
		std::filesystem::path FontTTFPath;
	};

	export struct FontCacheDesc
	{
		std::filesystem::path CachePath;
		std::vector<FontInfo> FontInfos;
	};

	export class FontCache
	{
	public:
		FontCache() = default;
		~FontCache();

		void release();

		void outputToFile(const std::filesystem::path& path);

		std::vector<NaiveFont>::iterator begin() { return m_Fonts.begin(); }
		std::vector<NaiveFont>::iterator end() { return m_Fonts.end(); }
		std::vector<NaiveFont>::const_iterator begin() const { return m_Fonts.begin(); }
		std::vector<NaiveFont>::const_iterator end() const { return m_Fonts.end(); }

		static FontCache createFromFile(const std::filesystem::path& path);
		static FontCache createFontCache(const FontCacheDesc& desc);
		static FontCache createOrGetFontCache(const FontCacheDesc& desc);
	private:
		std::vector<NaiveFont> m_Fonts;
	};

}

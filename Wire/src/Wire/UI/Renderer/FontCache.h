#pragma once

#include "Font.h"

#include <filesystem>

namespace wire {

	struct FontInfo
	{
		std::filesystem::path FontTTFPath;
	};

	struct FontCacheDesc
	{
		std::filesystem::path CachePath;
		std::vector<FontInfo> FontInfos;
	};

	class FontCache
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

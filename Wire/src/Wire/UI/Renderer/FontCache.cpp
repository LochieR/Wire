module;

#include "Wire/Core/Assert.h"

#undef INFINITE
#include <msdf-atlas-gen.h>

#include <string>
#include <fstream>
#include <filesystem>

module wire.ui.renderer:fontCache;

import :buffer;
import :font;
import wire.serialization;

namespace wire::utils {

	struct IntPair
	{
		int _1, _2;

		bool operator<(const IntPair& other) const
		{
			std::pair<int, int> thisPair = std::make_pair(_1, _2);
			std::pair<int, int> otherPair = std::make_pair(other._1, other._2);

			return  thisPair < otherPair;
		}
	};

}

namespace std {

	template<>
	struct hash<::wire::utils::IntPair>
	{
		std::size_t operator()(const ::wire::utils::IntPair& pair) const
		{
			auto hash1 = std::hash<int>{}(pair._1);
			auto hash2 = std::hash<int>{}(pair._2);

			return hash1 ^ (hash2 << 1);
		}
	};

}

namespace wire {

#define HEADER_VER(major, minor, patch, build) (static_cast<uint32_t>(major) << 24) \
    | (static_cast<uint32_t>(minor) << 16) \
    | (static_cast<uint32_t>(patch) << 8)  \
    | (static_cast<uint32_t>(build))

	struct FontCacheHeader
	{
		// ID data
		const char AppID[4] = { 'W', 'I', 'R', 'E' };
		const char TypeID[4] = { 'F', 'C', 'C', 'H' };
		const uint32_t Version = HEADER_VER(1, 0, 0, 0);

		// cache data
		size_t FontCount;
	};

	static void WriteNaiveFont(StreamWriter& stream, const NaiveFont& naiveFont);

	static void ReadNaiveFont(StreamReader& stream, NaiveFont& naiveFont);

	FontCache::~FontCache()
	{
	}

	void FontCache::release()
	{
		for (auto& font : m_Fonts)
		{
			font.release();
		}
	}

	void FontCache::outputToFile(const std::filesystem::path& path)
	{
		FontCacheHeader header{};
		header.FontCount = m_Fonts.size();

		if (path.has_parent_path())
			std::filesystem::create_directories(path.parent_path());
		std::ofstream file(path, std::ios::binary);

		WR_ASSERT(file.good(), "Failed to open file!");

		StreamWriter stream(file);

		stream.writeRaw(header);
		stream.writeArray<NaiveFont>(m_Fonts, WriteNaiveFont, false);

		file.close();
	}

	FontCache FontCache::createFromFile(const std::filesystem::path& path)
	{
		FontCache cache;

		std::ifstream file(path, std::ios::binary);
		StreamReader stream(file);

		FontCacheHeader header;
		stream.readRaw(header);

		std::vector<NaiveFont>& naiveFonts = cache.m_Fonts;

		stream.readArray<NaiveFont>(naiveFonts, ReadNaiveFont, header.FontCount);

		return cache;
	}

	FontCache FontCache::createFontCache(const FontCacheDesc& desc)
	{
		FontCache fontCache;

		for (const auto& info : desc.FontInfos)
		{
			NaiveFont font = NaiveFont::create(info.FontTTFPath);

			fontCache.m_Fonts.push_back(font);
		}

		return fontCache;
	}

	FontCache FontCache::createOrGetFontCache(const FontCacheDesc& desc)
	{
		if (!std::filesystem::exists(desc.CachePath))
		{
			FontCache cache = createFontCache(desc);
			cache.outputToFile(desc.CachePath);

			return cache;
		}

		FontCache oldCache = createFromFile(desc.CachePath);

		if (oldCache.m_Fonts.size() != desc.FontInfos.size())
		{
			FontCache cache = createFontCache(desc);
			cache.outputToFile(desc.CachePath);

			return cache;
		}

		return oldCache;
	}

	void WriteNaiveFont(StreamWriter& stream, const NaiveFont& naiveFont)
	{
		MemoryBuffer fontMemory{ reinterpret_cast<void*>(naiveFont.AtlasData), naiveFont.AtlasSize };
		MemoryBuffer fontFileMemory{ reinterpret_cast<void*>(naiveFont.FontFileData), naiveFont.FontFileSize };

		stream.writeString(naiveFont.Name);
		stream.writeBuffer(fontFileMemory);
		stream.writeRaw<size_t>(naiveFont.AtlasSize);
		stream.writeRaw<uint32_t>(naiveFont.Width);
		stream.writeRaw<uint32_t>(naiveFont.Height);
		stream.writeBuffer(fontMemory);
	}

	void ReadNaiveFont(StreamReader& stream, NaiveFont& naiveFont)
	{
		MemoryBuffer fontMemory;
		MemoryBuffer fontFileMemory;

		stream.readString(naiveFont.Name);
		stream.readBuffer(fontFileMemory);
		stream.readRaw<size_t>(naiveFont.AtlasSize);
		stream.readRaw<uint32_t>(naiveFont.Width);
		stream.readRaw<uint32_t>(naiveFont.Height);
		stream.readBuffer(fontMemory);

		naiveFont.AtlasData = new uint32_t[fontMemory.Size / sizeof(uint32_t)];
		std::memcpy(naiveFont.AtlasData, fontMemory.Data, fontMemory.Size);

		naiveFont.FontFileData = new uint8_t[fontFileMemory.Size];
		std::memcpy(naiveFont.FontFileData, fontFileMemory.Data, fontFileMemory.Size);
		naiveFont.FontFileSize = fontFileMemory.Size;
	}

}

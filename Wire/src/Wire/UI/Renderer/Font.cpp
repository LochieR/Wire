#include "Font.h"

#include "MSDFData.h"
#include "Wire/Core/Assert.h"

#include <fstream>

namespace wire {

	template<typename T, typename S, int N, msdf_atlas::GeneratorFunction<S, N> GenFunc>
	static uint32_t* CreateAndCacheAtlas(const std::string& fontName, float fontSize, const std::vector<msdf_atlas::GlyphGeometry>& glyphs, 
		const msdf_atlas::FontGeometry& fontGeometry, uint32_t width, uint32_t height, size_t& outSize)
	{
		msdf_atlas::GeneratorAttributes attributes;
		attributes.config.overlapSupport = true;
		attributes.scanlinePass = true;

		msdf_atlas::ImmediateAtlasGenerator<S, N, GenFunc, msdf_atlas::BitmapAtlasStorage<T, N>> generator(width, height);
		generator.setAttributes(attributes);
		generator.setThreadCount(8);
		generator.generate(glyphs.data(), (int)glyphs.size());

		msdfgen::BitmapConstRef<T, N> bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();

		uint32_t* atlasData = new uint32_t[width * height * N];
		std::memcpy(atlasData, reinterpret_cast<const void*>(bitmap.pixels), width * height * N);
		outSize = (size_t)(width * height * N);

		return atlasData;
	}

	NaiveFont NaiveFont::create(const std::filesystem::path& path, uint32_t minChar, uint32_t maxChar)
	{
		NaiveFont naiveFont;

		msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
		WR_ASSERT(ft, "Failed to initialize freetype!");

		std::string fileString = path.string();

		msdfgen::FontHandle* font = msdfgen::loadFont(ft, fileString.c_str());
		if (!font)
		{
			std::cerr << "Failed to load font " << path << std::endl;
			return {};
		}

		struct CharsetRange
		{
			uint32_t Begin, End;
		};

		CharsetRange charsetRanges[] = {
			{ minChar, maxChar }
		};

		msdf_atlas::Charset charset;
		for (CharsetRange range : charsetRanges)
		{
			for (uint32_t c = range.Begin; c <= range.End; c++)
				charset.add(c);
		}

		double fontScale = 1.0;
		std::vector<msdf_atlas::GlyphGeometry> glyphs;
		msdf_atlas::FontGeometry geometry(&glyphs);
		int glyphsLoaded = geometry.loadCharset(font, fontScale, charset);

		double emSize = 40.0;

		msdf_atlas::TightAtlasPacker atlasPacker;
		atlasPacker.setPixelRange(2.0);
		atlasPacker.setMiterLimit(1.0);
		atlasPacker.setPadding(0);
		atlasPacker.setScale(emSize);
		int remaining = atlasPacker.pack(glyphs.data(), (int)glyphs.size());

		int width, height;
		atlasPacker.getDimensions(width, height);
		emSize = atlasPacker.getScale();

#define DEFAULT_ANGLE_THRESHOLD 3.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define THREAD_COUNT 8

		uint64_t coloringSeed = 0;
		bool expensizeColoring = false;
		if (expensizeColoring)
		{
			msdf_atlas::Workload([&glyphs = glyphs, &coloringSeed](int i, int threadNo) -> bool
			{
				uint64_t glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
				glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
				return true;
			}, (int)glyphs.size()).finish(THREAD_COUNT);
		}
		else
		{
			uint64_t glyphSeed = coloringSeed;
			for (msdf_atlas::GlyphGeometry& glyph : glyphs)
			{
				glyphSeed *= LCG_MULTIPLIER;
				glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
			}
		}

		naiveFont.AtlasData = CreateAndCacheAtlas<uint8_t, float, 4, msdf_atlas::mtsdfGenerator>(
			"Font",
			(float)emSize,
			glyphs,
			geometry,
			(uint32_t)width,
			(uint32_t)height,
			naiveFont.AtlasSize
		);

		naiveFont.Width = (uint32_t)width;
		naiveFont.Height = (uint32_t)height;

		naiveFont.Name = path.filename().string();

		msdfgen::destroyFont(font);
		msdfgen::deinitializeFreetype(ft);

		std::ifstream file(path, std::ios::binary | std::ios::ate);

		size_t size = file.tellg();
		file.seekg(0, std::ios::beg);

		naiveFont.FontFileSize = size;
		naiveFont.FontFileData = new uint8_t[size];
		file.read(reinterpret_cast<char*>(naiveFont.FontFileData), size);

		return naiveFont;
	}

	void NaiveFont::release()
	{
		delete[] FontFileData;
		delete[] AtlasData;
	}

}

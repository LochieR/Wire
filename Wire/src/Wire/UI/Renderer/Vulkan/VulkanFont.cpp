module;

#include "Wire/Core/Assert.h"

#undef INFINITE
#include <msdf-atlas-gen.h>
#include <FontGeometry.h>
#include <GlyphGeometry.h>

module wire.ui.renderer.vk:font;

namespace wire {

	VulkanFont::VulkanFont(Renderer* renderer, const std::filesystem::path& path, uint32_t minChar, uint32_t maxChar)
		: m_Renderer(renderer)
	{
		NaiveFont naive = NaiveFont::create(path, minChar, maxChar);


		m_AtlasTexture = renderer->createTexture2D(naive.AtlasData, naive.Width, naive.Height);

		naive.release();
	}

	VulkanFont::VulkanFont(Renderer* renderer, const NaiveFont& naive)
		: m_Renderer(renderer)
	{
		msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
		WR_ASSERT(ft, "Failed to initialize freetype!");

		msdfgen::FontHandle* font = msdfgen::loadFontData(ft, naive.FontFileData, (int)naive.FontFileSize);
		if (!font)
		{
			std::cerr << "Failed to load font " << naive.Name << std::endl;
			return;
		}

		struct CharsetRange
		{
			uint32_t Begin, End;
		};

		CharsetRange charsetRanges[] = {
			{ naive.MinChar, naive.MaxChar }
		};

		msdf_atlas::Charset charset;
		for (CharsetRange range : charsetRanges)
		{
			for (uint32_t c = range.Begin; c <= range.End; c++)
				charset.add(c);
		}

		double fontScale = 1.0;
		m_FontGeometry = msdf_atlas::FontGeometry(&m_Glyphs);
		int glyphsLoaded = m_FontGeometry.loadCharset(font, fontScale, charset);

		std::cout << "Loaded " << glyphsLoaded << " glyphs from font (out of " << charset.size() << ")" << std::endl;

		double emSize = 40.0;

		msdf_atlas::TightAtlasPacker atlasPacker;
		atlasPacker.setPixelRange(2.0);
		atlasPacker.setMiterLimit(1.0);
		atlasPacker.setPadding(0);
		atlasPacker.setScale(emSize);
		int remaining = atlasPacker.pack(m_Glyphs.data(), (int)m_Glyphs.size());

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
			msdf_atlas::Workload([&glyphs = m_Glyphs, &coloringSeed](int i, int threadNo) -> bool
			{
				uint64_t glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
				glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
				return true;
			}, (int)m_Glyphs.size()).finish(THREAD_COUNT);
		}
		else
		{
			uint64_t glyphSeed = coloringSeed;
			for (msdf_atlas::GlyphGeometry& glyph : m_Glyphs)
			{
				glyphSeed *= LCG_MULTIPLIER;
				glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
			}
		}

		m_AtlasTexture = renderer->createTexture2D(naive.AtlasData, naive.Width, naive.Height);
		
		msdfgen::destroyFont(font);
		msdfgen::deinitializeFreetype(ft);
	}

	VulkanFont::~VulkanFont()
	{
		delete m_AtlasTexture;
	}

}

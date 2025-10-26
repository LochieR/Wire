#include "VulkanFont.h"

#include "Wire/Renderer/MSDFData.h"
#include "Wire/Core/Assert.h"

#include <FontGeometry.h>
#include <GlyphGeometry.h>

namespace wire {

    VulkanFont::VulkanFont(Device* device, const std::filesystem::path& path, std::string_view debugName, uint32_t minChar, uint32_t maxChar)
        : m_Device(device), m_DebugName(debugName)
    {
        NaiveFont naive = NaiveFont::create(path, minChar, maxChar);

        createFontData(naive);

        naive.release();
    }

    VulkanFont::VulkanFont(Device* device, const NaiveFont& naive)
        : m_Device(device)
    {
        createFontData(naive);
    }

    VulkanFont::~VulkanFont()
    {
        destroy();
    }

    void VulkanFont::destroy()
    {
        if (m_Valid)
        {
            delete m_Data;
            m_Device->drop(m_AtlasTexture);
            m_AtlasTexture = nullptr;
        }
    }

    void VulkanFont::invalidate() noexcept
    {
        m_Valid = false;
        m_Device = nullptr;
    }

    void VulkanFont::createFontData(const NaiveFont& naive)
    {
        msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
        WR_ASSERT(ft, "Failed to initialize freetype!");

        m_Data = new MSDFData();

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
        m_Data->FontGeometry = msdf_atlas::FontGeometry(&m_Data->Glyphs);
        int glyphsLoaded = m_Data->FontGeometry.loadCharset(font, fontScale, charset);

        WR_INFO("Loaded {} glyphs from font (out of {})", glyphsLoaded, charset.size());

        double emSize = 40.0;

        msdf_atlas::TightAtlasPacker atlasPacker;
        atlasPacker.setPixelRange(2.0);
        atlasPacker.setMiterLimit(1.0);
        atlasPacker.setPadding(0);
        atlasPacker.setScale(emSize);
        int remaining = atlasPacker.pack(m_Data->Glyphs.data(), (int)m_Data->Glyphs.size());

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
            msdf_atlas::Workload([&glyphs = m_Data->Glyphs, &coloringSeed](int i, int threadNo) -> bool
            {
                uint64_t glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
                glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
                return true;
            }, (int)m_Data->Glyphs.size()).finish(THREAD_COUNT);
        }
        else
        {
            uint64_t glyphSeed = coloringSeed;
            for (msdf_atlas::GlyphGeometry& glyph : m_Data->Glyphs)
            {
                glyphSeed *= LCG_MULTIPLIER;
                glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
            }
        }

        m_AtlasTexture = m_Device->createTexture2D(naive.AtlasData, naive.Width, naive.Height, m_DebugName);

        msdfgen::destroyFont(font);
        msdfgen::deinitializeFreetype(ft);
    }

}

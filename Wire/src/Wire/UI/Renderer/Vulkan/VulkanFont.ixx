module;

#undef INFINITE
#include <msdf-atlas-gen.h>

#include <vector>
#include <filesystem>

export module wire.ui.renderer.vk:font;

import wire.ui.renderer;

namespace wire {

	export class VulkanFont : public Font
	{
	public:
		VulkanFont() = default;
		VulkanFont(Renderer* renderer, const std::filesystem::path& path, uint32_t minChar = 0x0020, uint32_t maxChar = 0x00FF);
		VulkanFont(Renderer* renderer, const NaiveFont& naive);
		virtual ~VulkanFont();

		virtual Texture2D* getAtlasTexture() const override { return m_AtlasTexture; }
		virtual const msdf_atlas::FontGeometry& getFontGeometry() const override { return m_FontGeometry; }
		virtual const std::vector<msdf_atlas::GlyphGeometry>& getGlyphs() const override { return m_Glyphs; }
	private:
		Renderer* m_Renderer = nullptr;

		std::vector<msdf_atlas::GlyphGeometry> m_Glyphs;
		msdf_atlas::FontGeometry m_FontGeometry;

		Texture2D* m_AtlasTexture = nullptr;
	};

}

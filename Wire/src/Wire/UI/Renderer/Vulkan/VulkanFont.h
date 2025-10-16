#pragma once

#include "Wire/UI/Renderer/Renderer.h"

#undef INFINITE
#include <msdf-atlas-gen.h>

#include <vector>
#include <filesystem>

namespace wire {

	class VulkanFont : public Font
	{
	public:
		VulkanFont() = default;
		VulkanFont(Renderer* renderer, const std::filesystem::path& path, uint32_t minChar = 0x0020, uint32_t maxChar = 0x00FF);
		VulkanFont(Renderer* renderer, const NaiveFont& naive);
		virtual ~VulkanFont();

		virtual Texture2D* getAtlasTexture() const override { return m_AtlasTexture; }
		virtual const MSDFData& getMSDFData() const override { return *m_Data; }
	private:
		void createFontData(const NaiveFont& naive);
	private:
		Renderer* m_Renderer = nullptr;

		MSDFData* m_Data = nullptr;
		Texture2D* m_AtlasTexture = nullptr;
	};

}

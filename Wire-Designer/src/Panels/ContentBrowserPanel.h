#pragma once

#include <filesystem>

#include "Wire/Renderer/Texture.h"
#include "Wire/Core/Timestep.h"

namespace Wire {

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender(Timestep ts);
		void ReloadPathEntries();
	private:
		std::filesystem::path m_CurrentDirectory;
		std::vector<std::filesystem::directory_entry> m_DirectoryEntries;

		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;

		uint32_t m_Ticks = 0;
	};

}

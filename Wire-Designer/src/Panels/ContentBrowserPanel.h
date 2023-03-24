#pragma once

#include "Wire/Renderer/Texture.h"
#include "Wire/Core/Timestep.h"
#include "Wire/Projects/Project.h"

#include <filesystem>

namespace Wire {

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender(bool* open, Timestep ts);
		void ReloadDirectoryEntries();

		void OnOpenProject(const Ref<Project>& project);
		std::filesystem::path GetAssetPath() const { return m_AssetPath; }

		float GetAverageFrameRate() { return m_AverageFrameRate; }
	private:
		std::filesystem::path m_AssetPath;
		std::filesystem::path m_CurrentDirectory;
		std::vector<std::filesystem::directory_entry> m_DirectoryEntries;

		Ref<Project> m_Project = CreateRef<Project>(Project::CreateNullProject());

		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;

		uint32_t m_Ticks = 0;

		double m_TotalFrameRates = 0.0f;
		float m_AverageFrameRate = 0.0f;
	};

}

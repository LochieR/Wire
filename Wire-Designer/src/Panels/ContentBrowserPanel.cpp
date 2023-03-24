#include "ContentBrowserPanel.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#ifdef WR_PLATFORM_WINDOWS
#include <direct.h>
#else
#include <unistd.h>
#endif

namespace Wire {

	ContentBrowserPanel::ContentBrowserPanel()
		: m_AssetPath("assets"), m_CurrentDirectory(m_AssetPath), m_Project(nullptr)
	{
		m_DirectoryIcon = Texture2D::Create("Resources/Icons/ContentBrowser/DirectoryIcon.png");
		m_FileIcon = Texture2D::Create("Resources/Icons/ContentBrowser/FileIcon.png");

		ReloadDirectoryEntries();
	}

	void ContentBrowserPanel::OnImGuiRender(bool* open, Timestep ts)
	{
		if (*open)
		{
			ImGui::Begin("Content Browser", open);

			static float width = ImGui::GetWindowWidth();
			static float height = 30.0f;
			if (m_CurrentDirectory == "assets" && m_Project != nullptr)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f });
				if (ImGui::Button(m_CurrentDirectory.string().c_str()))
				{
					m_CurrentDirectory = "assets";
					ReloadDirectoryEntries();
				}
				ImGui::PopStyleColor();
				ImGui::SameLine();
				ImGui::Text("/");
			}
			else if (m_Project != nullptr)
			{
				std::vector<std::string> pathSplit;
				std::string delimiter = "/";
				std::string str = m_CurrentDirectory.string();
				std::replace(str.begin(), str.end(), '\\', '/');

				size_t pos = 0;
				std::string token;
				while ((pos = str.find(delimiter)) != std::string::npos)
				{
					token = str.substr(0, pos);
					pathSplit.push_back(token);
					str.erase(0, pos + delimiter.length());
				}
				pathSplit.push_back(str);

				int i = 0;
				for (std::string str : pathSplit)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f });
					if (ImGui::Button(str.c_str()))
					{
						std::string gotoPath;
						for (int x = 0; x <= i; x++)
							gotoPath += pathSplit[x] + (x == i ? "" : "/");
						m_CurrentDirectory = gotoPath;
						ReloadDirectoryEntries();
					}
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::Text("/");
					if (i != pathSplit.size() - 1)
						ImGui::SameLine();
					i++;
				}
			}

			if (m_Project != nullptr)
			{
				static float padding = 12.0f;
				static float thumbnailSize = 80.0f;
				float cellSize = thumbnailSize + padding;

				float panelWidth = ImGui::GetContentRegionAvail().x;
				int columnCount = (int)(panelWidth / cellSize);
				if (columnCount < 1)
					columnCount = 1;

				ImGui::Columns(columnCount, 0, false);

				for (auto& directoryEntry : m_DirectoryEntries)
				{
					const auto& path = directoryEntry.path();
					auto relativePath = std::filesystem::relative(path, m_AssetPath);
					std::string filenameString = relativePath.filename().string();

					ImGui::PushID(filenameString.c_str());
					Ref<Texture2D> icon = directoryEntry.is_directory() ? m_DirectoryIcon : m_FileIcon;
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0, 0, 0, 0 });
					ImGui::ImageButton((ImTextureID)(uint64_t)icon->GetRendererID(), { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });

					if (ImGui::BeginDragDropSource())
					{
						ImGui::Image((ImTextureID)(uint64_t)icon->GetRendererID(), { thumbnailSize / 1.5f, thumbnailSize / 1.5f }, { 0, 1 }, { 1, 0 });
						ImGui::Text(filenameString.c_str());
						auto absolutePath = std::filesystem::absolute("assets" / relativePath);
						const wchar_t* itemPath = absolutePath.c_str();
						ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t), ImGuiCond_Once);
						ImGui::EndDragDropSource();
					}

					ImGui::PopStyleColor();
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						if (directoryEntry.is_directory())
						{
							m_CurrentDirectory /= path.filename();
							ReloadDirectoryEntries();
						}
					}
					ImGui::TextWrapped(filenameString.c_str());

					ImGui::NextColumn();

					ImGui::PopID();
				}

				ImGui::Columns(1);
			}
			else
			{
				auto windowSize = ImGui::GetWindowSize();
				auto textSize = ImGui::CalcTextSize("No Open Project");

				ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
				ImGui::SetCursorPosY((windowSize.y - textSize.y) * 0.5f);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0.388f, 0.388f, 0.388f, 1.0f });
				ImGui::Text("No Open Project");
				ImGui::PopStyleColor();
			}

			ImGui::End();

			m_Ticks++;

			m_TotalFrameRates += (int)(1000.0f / ts.GetMilliseconds());
			m_AverageFrameRate = (float)m_TotalFrameRates / m_Ticks;

			int twoSeconds = 2 * (int)m_AverageFrameRate;

			if (m_Ticks != 0 && twoSeconds != 0)
				if (m_Ticks % twoSeconds == 0 && m_Project != nullptr)
					ReloadDirectoryEntries();
		}
	}

	void ContentBrowserPanel::ReloadDirectoryEntries()
	{
		m_DirectoryEntries.clear();

		for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
		{
			m_DirectoryEntries.push_back(directoryEntry);
		}
	}

	void ContentBrowserPanel::OnOpenProject(const Ref<Project>& project)
	{
		m_Project = project; 
		std::filesystem::current_path(project->GetDir());
		m_AssetPath = "assets";
		m_CurrentDirectory = m_AssetPath;
		ReloadDirectoryEntries();
	}

}

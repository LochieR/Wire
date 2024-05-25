#include "wrpch.h"
#include "FontLibrary.h"

#include "Wire/Core/Base.h"
#include "Wire/Core/Application.h"
#include "Wire/Renderer/Renderer.h"

#include <imgui.h>

namespace Wire {

	FontLibrary::FontLibrary(const std::filesystem::path& defaultFontPath, float defaultFontSize)
		: m_DefaultFontSize(defaultFontSize)
	{
		ImGuiIO& io = ImGui::GetIO();
		m_FontMap["Default"] = ImGui::GetIO().FontDefault = io.Fonts->AddFontFromFileTTF(defaultFontPath.string().c_str(), defaultFontSize);
	}

	void FontLibrary::UploadFonts()
	{
	}

	void FontLibrary::AddFont(const std::string& name, const std::filesystem::path& fontPath)
	{
		m_FontMap[name] = ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.string().c_str(), m_DefaultFontSize);
	}

	void FontLibrary::AddFont(const std::string& name, const std::filesystem::path& fontPath, float fontSize)
	{
		m_FontMap[name] = ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.string().c_str(), fontSize);
	}

	void FontLibrary::PushDefaultFont()
	{
		PushFont("Default");
	}

	void FontLibrary::PushFont(const std::string& name)
	{
		WR_ASSERT(m_FontMap.find(name) != m_FontMap.end());

		ImGui::PushFont(m_FontMap.at(name));
	}

	void FontLibrary::PopFont()
	{
		ImGui::PopFont();
	}

}

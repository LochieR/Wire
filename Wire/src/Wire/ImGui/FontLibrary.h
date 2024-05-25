#pragma once

#include "Wire/Core/Window.h"

#include <string>
#include <filesystem>
#include <unordered_map>

struct ImFont;

namespace Wire {

	class FontLibrary
	{
	public:
		FontLibrary(const std::filesystem::path& defaultFontPath, float defaultFontSize);

		void UploadFonts();

		void AddFont(const std::string& name, const std::filesystem::path& fontPath);
		void AddFont(const std::string& name, const std::filesystem::path& fontPath, float fontSize);

		void PushDefaultFont();
		void PushFont(const std::string& name);

		void PopFont();
	private:
		float m_DefaultFontSize;

		std::unordered_map<std::string, ImFont*> m_FontMap;
	};

}

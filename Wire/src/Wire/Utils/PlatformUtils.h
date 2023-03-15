#pragma once

#include <string>

#include "Wire/Core/Window.h"

namespace Wire {

	class FileDialogs
	{
	public:
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter);
	};

	class Popups
	{
	public:
		static void OpenSettingsPopup(const std::string& title, int width, int height);
		
		static std::vector<Ref<Window>> GetPopupWindows() { return m_PopupWindows; }
	private:
		static std::vector<Ref<Window>> m_PopupWindows;
	};

}

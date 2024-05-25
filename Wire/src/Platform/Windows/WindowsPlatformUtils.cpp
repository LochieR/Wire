#include "wrpch.h"
#include "Wire/Utils/PlatformUtils.h"

#include "Wire/Core/Application.h"

#include <glfw/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>

#include <commdlg.h>
#include <dwmapi.h>
#include <winuser.h>

namespace Wire {

#ifdef WR_PLATFORM_WINDOWS

#define WR_REFRESH_WINDOW(window) SetWindowPos(window, NULL, NULL, NULL, NULL, NULL, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE)

	namespace Utils::Win32 {

		void SetWindowAttribute(Window& window, Attribute attribute, uint32_t value)
		{
			GLFWwindow* glfwWin = static_cast<GLFWwindow*>(window.GetNativeHandle());
			SetWindowAttribute(glfwWin, attribute, value);
		}

		void SetWindowAttribute(Window& window, Attribute attribute, bool value)
		{
			GLFWwindow* glfwWin = static_cast<GLFWwindow*>(window.GetNativeHandle());
			SetWindowAttribute(glfwWin, attribute, value);
		}

		void SetWindowAttribute(GLFWwindow* window, Attribute attribute, uint32_t value)
		{
			if (!window)
				return;
			HWND handle = glfwGetWin32Window(window);

			switch (attribute)
			{
				case None:
					WR_ASSERT(false && "Cannot set attribute None!");
					break;
				case BorderColor:
					DwmSetWindowAttribute(handle, DWMWA_BORDER_COLOR, &value, sizeof(value));
					break;
				case DarkMode:
					WR_ASSERT(false && "The DarkMode attribute is a bool value!");
					break;
				case TitleBarColor:
					break;
			}

			WR_REFRESH_WINDOW(handle);
		}

		void SetWindowAttribute(GLFWwindow* window, Attribute attribute, bool value)
		{
			if (!window)
				return;
			HWND handle = glfwGetWin32Window(window);

			switch (attribute)
			{
				case None:
					WR_ASSERT(false && "Cannot set attribute None!");
					break;
				case BorderColor:
					WR_ASSERT(false && "The BorderColor attribute is a uint32_t value!");
					break;
				case DarkMode:
					BOOL dark;
					if (value)
						dark = TRUE;
					else
						dark = FALSE;
					DwmSetWindowAttribute(handle, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
					break;
			}

			WR_REFRESH_WINDOW(handle);
		}

	}

#endif

	std::string FileDialogs::OpenFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeHandle());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}
		return std::string();
	}

	std::string FileDialogs::SaveFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeHandle());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetSaveFileNameA(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}
		return std::string();
	}

	float Time::GetTime()
	{
		return static_cast<float>(glfwGetTime());
	}

}

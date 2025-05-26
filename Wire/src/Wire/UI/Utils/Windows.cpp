module;

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>

#include <dwmapi.h>

module wire.ui.utils.windows;

namespace wire::windows {
	
	static constexpr COLORREF ConvertToColorRef(const glm::vec3& color)
	{
		BYTE r = static_cast<BYTE>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
		BYTE g = static_cast<BYTE>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
		BYTE b = static_cast<BYTE>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);

		return RGB(r, g, b);
	}

	void SetWindowBorderColor(GLFWwindow* window, const glm::vec3& color)
	{
		HWND hwnd = glfwGetWin32Window(window);
		COLORREF colorRef = ConvertToColorRef(color);

		DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, (void*)&colorRef, sizeof(COLORREF));
	}

	void SetWindowTitlebarColor(GLFWwindow* window, const glm::vec3& color)
	{
		HWND hwnd = glfwGetWin32Window(window);
		COLORREF colorRef = ConvertToColorRef(color);

		DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, (void*)&colorRef, sizeof(COLORREF));
	}

	void SetWindowShowIcon(GLFWwindow* window, bool show)
	{
		HWND hwnd = glfwGetWin32Window(window);

		if (show)
		{
			HICON hIcon = (HICON)LoadIcon(NULL, IDI_APPLICATION);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

			LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
			exStyle &= ~WS_EX_DLGMODALFRAME;
			SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
		}
		else
		{
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)NULL);
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)NULL);

			LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
			exStyle |= WS_EX_DLGMODALFRAME;
			SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
		}

		SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}

}

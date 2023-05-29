#include "wrpch.h"
#include "Wire/Utils/PlatformUtils.h"

#include "Wire/Core/Application.h"

#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <dwmapi.h>
#include <winuser.h>

namespace Wire {

#define WR_REFRESH_WINDOW(window) SetWindowPos(window, NULL, NULL, NULL, NULL, NULL, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE)

	std::string FileDialogs::OpenFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
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
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
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
		return glfwGetTime();
	}

	void WindowUtils::SetWindowAttributes(void* window)
	{
		HWND hwnd = glfwGetWin32Window((GLFWwindow*)window);

		BOOL dark;
		winrt::Windows::UI::ViewManagement::UISettings settings;
		auto background = settings.GetColorValue(winrt::Windows::UI::ViewManagement::UIColorType::Background);
		if (background.R < 0.5f)
			dark = TRUE;
		else
			dark = FALSE;

		COLORREF colour = RGB(250, 36, 12);

		DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
		WR_REFRESH_WINDOW(hwnd);
	}

	void WindowUtils::SetWindowBorderColour(void* window, float r, float g, float b)
	{
		HWND hwnd = glfwGetWin32Window((GLFWwindow*)window);

		COLORREF colour = RGB(r, g, b);

		DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &colour, sizeof(colour));
		WR_REFRESH_WINDOW(hwnd);
	}

#define WR_SET_CURSOR(icon, id) case MouseIcon::icon: SetCursor(LoadCursor(0, IDC_##id)); return;

	void Mouse::SetMouseIcon(MouseIcon icon)
	{
		switch (icon)
		{
			WR_SET_CURSOR(Arrow, ARROW);
			WR_SET_CURSOR(Loading, WAIT)
			WR_SET_CURSOR(ArrowLoading, APPSTARTING);
			WR_SET_CURSOR(Crosshair, CROSS);
			WR_SET_CURSOR(Hand, HAND);
			WR_SET_CURSOR(Help, HELP);
			WR_SET_CURSOR(TextCursor, IBEAM);
			WR_SET_CURSOR(Unavailable, NO);
			WR_SET_CURSOR(Move_A, SIZEALL);
			WR_SET_CURSOR(Move_BL_TR, SIZENESW);
			WR_SET_CURSOR(Move_T_B, SIZENS);
			WR_SET_CURSOR(Move_TL_BR, SIZENWSE);
			WR_SET_CURSOR(Move_L_R, SIZEWE);
			WR_SET_CURSOR(UpArrow, UPARROW);
		}
		WR_CORE_ASSERT(false);
	}

}

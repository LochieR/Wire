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

	class Time
	{
	public:
		static float GetTime();
	};

	class WindowUtils
	{
	public:
		static void SetWindowAttributes(void* window);
		static void SetWindowBorderColour(void* window, float r, float g, float b);
	};

	enum class MouseIcon
	{
		None = 0,
		Arrow,
		Loading
	};

	class Mouse
	{
	public:
		static void SetMouseIcon(MouseIcon icon);
	};

}

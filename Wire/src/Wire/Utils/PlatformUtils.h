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
		Arrow,			// Normal arrow
		Loading,		// Loading icon
		ArrowLoading,	// Arrow and loading at the same time
		Crosshair,		// Crosshair
		Hand,			// Used when clicking on button
		Help,			// Normal arrow with question mark
		TextCursor,		// Used when hovering over text/editing text
		Unavailable,	// Used when something is unavailable/cannot be used
		Move_A,			// Four arrows pointing up/right/left/down
		Move_BL_TR,		// Two arrows pointing bottom left/top right
		Move_T_B,		// Two arrows pointing top/bottom
		Move_TL_BR,		// Two arrows pointing top left/bottom right
		Move_L_R,		// Two arrows pointing left/right
		UpArrow			// One arrow pointing up
	};

	class Mouse
	{
	public:
		static void SetMouseIcon(MouseIcon icon);
	};

}

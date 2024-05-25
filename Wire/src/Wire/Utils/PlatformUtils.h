#pragma once

#include "Wire/Core/Base.h"
#include "Wire/Core/Window.h"

#define WR_RGB(r, g, b) (((uint32_t)(((uint8_t)(r) | ((uint16_t)((uint8_t)(g)) << 8)) | (((uint32_t)(uint8_t)(b)) << 16))))

struct GLFWwindow;

namespace Wire {

#ifdef WR_PLATFORM_WINDOWS

	namespace Utils::Win32 {

		enum Attribute
		{
			None = 0,
			BorderColor,	// Set with a uint32_t value (use WR_RGB macro)
			DarkMode,		// Set with a bool value
			TitleBarColor,	// Set with a uint32_t
		};

		void SetWindowAttribute(Window& window, Attribute attribute, uint32_t value);
		void SetWindowAttribute(Window& window, Attribute attribute, bool value);

		void SetWindowAttribute(GLFWwindow* window, Attribute attribute, uint32_t value);
		void SetWindowAttribute(GLFWwindow* window, Attribute attribute, bool value);
	}

#endif

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

}

#pragma once

#include "Event.h"

#include <string>
#include <memory>

#include <glm/glm.hpp>

struct GLFWwindow;

namespace Wire {

	struct WindowSpecification
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;
		bool VSync;
		bool MainWindow;

		WindowSpecification(const std::string& title = "Wire Window",
			uint32_t width = 1600,
			uint32_t height = 900,
			bool vsync = true,
			bool mainWindow = false)
			: Title(title), Width(width), Height(height), VSync(vsync), MainWindow(mainWindow)
		{
		}
	};

	class Window
	{
	public:
		typedef std::function<void(Event&)> EventCallbackFn;

		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;
			bool VSync;
			bool FramebufferResized = false;

			bool* TitlebarHovered = nullptr;

			EventCallbackFn EventCallback;
		};
	public:
		Window(const WindowSpecification& spec = WindowSpecification());
		~Window();

		void OnUpdate();

		void Show() const;
		void Hide() const;

		uint32_t GetWidth() const { return m_Data.Width; }
		uint32_t GetHeight() const { return m_Data.Height; }
		glm::vec2 GetSize() const { return { m_Data.Width, m_Data.Height }; };

		void SetVSync(bool enabled);
		bool IsVSync() const { return m_Data.VSync; }

		bool IsMaximized() const;

		void Minimize();
		void Restore();
		void Maximize();

		void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }

		void* GetNativeHandle() const { return m_Window; }

		bool WasWindowResized() const { return m_Data.FramebufferResized; }
		void ResetResizedFlag() { m_Data.FramebufferResized = false; }
		void SetResizedFlag(bool value) { m_Data.FramebufferResized = value; }

		const std::string& GetTitle() const { return m_Data.Title; }

		void SetTitlebarHoveredPointer(bool* ptr);

		WindowData& GetWindowData() { return m_Data; }
	private:
		void Init(const WindowSpecification& spec);
		void Shutdown();
	private:
		GLFWwindow* m_Window;

		WindowData m_Data;
	};

}

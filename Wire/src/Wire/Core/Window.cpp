#include "wrpch.h"
#include "Window.h"

#include "Base.h"

#include "Wire/Renderer/Renderer.h"

#include <glfw/glfw3.h>
#include <stb_image.h>

#include <format>

namespace Wire {

	static uint8_t s_GLFWWindowCount = 0;

	static void GLFWErrorCallback(int error, const char* description)
	{
		WR_TEMP_TAG("GLFW");
		WR_ERROR("GLFW Error (", error, "): ", description);
	}

	Window::Window(const WindowSpecification& spec)
	{
		Init(spec);
	}

	Window::~Window()
	{
		Shutdown();
	}

	void Window::Init(const WindowSpecification& spec)
	{
		WR_TAG("Window");

		m_Data.Title = spec.Title;
		m_Data.Width = spec.Width;
		m_Data.Height = spec.Height;
		m_Data.VSync = spec.VSync;

		WR_INFO("Creating window \"", spec.Title, "\" (", spec.Width, ", ", spec.Height, ")");

		if (s_GLFWWindowCount == 0)
		{
			int success = glfwInit();
			WR_ASSERT(success && "Failed to initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, false);
		glfwWindowHint(GLFW_TITLEBAR, true);

		m_Window = glfwCreateWindow(static_cast<int>(spec.Width), static_cast<int>(spec.Height), spec.Title.c_str(), nullptr, nullptr);
		s_GLFWWindowCount++;

		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(spec.VSync);

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);
		});

		glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			data.FramebufferResized = true;
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event((KeyCode)key, false);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event((KeyCode)key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event((KeyCode)key, true);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, uint32_t keycode)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(static_cast<KeyCode>(keycode));
			data.EventCallback(event);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(static_cast<MouseButton>(button));
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(static_cast<MouseButton>(button));
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPos, (float)yPos);
			data.EventCallback(event);
		});

		{
			const uint32_t iconCount = 9;

			GLFWimage images[iconCount] = { 0 };

			for (uint32_t i = 0; i < iconCount; i++)
			{
				std::string path = std::format("Resources/Icons/App/Wire-{}.png", i + 1);
				images[i].pixels = stbi_load(path.c_str(), &images[i].width, &images[i].height, nullptr, 4);
			}

			glfwSetWindowIcon(m_Window, iconCount, images);

			for (uint32_t i = 0; i < iconCount; i++)
				stbi_image_free(images[i].pixels);
		}
	}

	void Window::Shutdown()
	{
		glfwDestroyWindow(m_Window);
		s_GLFWWindowCount--;

		if (s_GLFWWindowCount == 0)
		{
			glfwTerminate();
		}
	}

	void Window::OnUpdate()
	{
		glfwPollEvents();
	}

	void Window::Show() const
	{
		glfwShowWindow(m_Window);
	}

	void Window::Hide() const
	{
		glfwHideWindow(m_Window);
	}

	void Window::Minimize()
	{
		glfwIconifyWindow(m_Window);
	}

	void Window::Maximize()
	{
		glfwMaximizeWindow(m_Window);
	}

	void Window::Restore()
	{
		glfwRestoreWindow(m_Window);
	}

	void Window::SetVSync(bool enabled)
	{
		m_Data.VSync = enabled;
	}

	bool Window::IsMaximized() const
	{
		return (bool)glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED);
	}

	void Window::SetTitlebarHoveredPointer(bool* ptr)
	{
		m_Data.TitlebarHovered = ptr;

		glfwSetTitlebarHitTestCallback(m_Window, [](GLFWwindow* window, int x, int y, int* hit)
		{
			WindowData* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));
			*hit = (int)*data->TitlebarHovered;
		});
	}

}

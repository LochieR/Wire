module;

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <vector>
#include <filesystem>

module wire.core:app;

import wire.ui.renderer;
import wire.ui.core;

import wire.ui.utils.windows;

namespace wire {

	Application::Application(const ApplicationDesc& desc)
		: m_Desc(desc)
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_Window = glfwCreateWindow((int)desc.WindowWidth, (int)desc.WindowHeight, desc.WindowTitle.c_str(), nullptr, nullptr);
	
		glfwSetWindowUserPointer(m_Window, &m_Desc);

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			ApplicationDesc& desc = *reinterpret_cast<ApplicationDesc*>(glfwGetWindowUserPointer(window));

			desc.WindowWidth = (uint32_t)width;
			desc.WindowHeight = (uint32_t)height;
			desc.m_WasWindowResized = true;
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			ApplicationDesc& desc = *reinterpret_cast<ApplicationDesc*>(glfwGetWindowUserPointer(window));

			desc.m_Running = false;
		});

		s_App = this;

		windows::SetWindowShowIcon(m_Window, false);
		windows::SetWindowBorderColor(m_Window, { 0.39f, 0.07f, 0.54f });
		windows::SetWindowTitlebarColor(m_Window, { 0.39f, 0.07f, 0.54f });

		RendererDesc rendererDesc{};
		rendererDesc.API = RendererAPI::Vulkan;
		rendererDesc.ShaderCache.CachePath = "wire.shadercache";
		rendererDesc.FontCache.CachePath = "wire.fontcache";
		
		for (const auto& dir : std::filesystem::directory_iterator("shaders/"))
		{
			if (dir.path().has_extension() && dir.path().extension() == ".hlsl")
			{
				rendererDesc.ShaderCache.ShaderInfos.push_back(ShaderInfo{
					.Path = dir.path(),
					.VertexEntryPoint = "VShader",
					.PixelEntryPoint = "PShader"
				});
			}
		}

		for (const auto& dir : std::filesystem::directory_iterator("fonts/"))
		{
			if (dir.path().has_extension() && dir.path().extension() == ".ttf")
			{
				rendererDesc.FontCache.FontInfos.push_back(FontInfo{
					.FontTTFPath = dir.path()
				});
			}
		}

		m_Renderer = createRenderer(rendererDesc);

		UIRenderer::init(m_Renderer);
	}

	Application::~Application()
	{
		UIRenderer::shutdown();

		delete m_Renderer;

		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	void Application::run()
	{
		glm::mat4 textTransform = glm::translate(glm::mat4(1.0f), { 500.0f, 100.0f, 0.0f })
			* glm::scale(glm::mat4(1.0f), { 25.0f, 25.0f, 1.0f });

		UIRenderer::TextParams params{};

		while (m_Desc.m_Running)
		{
			m_Renderer->beginFrame();
			uint32_t frameIndex = m_Renderer->getFrameIndex();

			UIRenderer::beginFrame();
			UIRenderer::drawRect({ { 0.0f, 695.0f }, { 1280.0f, 720.0f } }, { 0.39f, 0.07f, 0.54f, 1.0f });

			// button
			UIRenderer::drawText("hello", textTransform, params);
			UIRenderer::drawRect({ { 500.0f, 80.0f }, { 550.0f, 120.0f } }, { 0.21f, 0.21f, 0.21f, 1.0f });
			UIRenderer::endFrame();

			m_Renderer->endFrame();
			glfwPollEvents();
		}
	}

}

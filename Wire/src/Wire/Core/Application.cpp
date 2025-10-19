#include "Application.h"

#include "Wire/Audio/AudioEngine.h"

#include "Wire/UI/Core/Canvas.h"
#include "Wire/UI/Renderer/Renderer.h"
#include "Wire/UI/Components/ComponentLibrary.h"

#include "Wire/UI/Utils/Windows.h"
#include "Wire/UI/Utils/macOS.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <vector>
#include <numbers>
#include <iostream>
#include <filesystem>

namespace wire {

	Application::Application(const ApplicationDesc& desc)
		: m_Desc(desc)
	{
		//AudioEngine::init();
		//AudioEngine::shutdown();

		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		m_Window = glfwCreateWindow((int)desc.WindowWidth, (int)desc.WindowHeight, desc.WindowTitle.c_str(), nullptr, nullptr);

		Input::setWindow(m_Window);

		m_Desc.m_EventCallback = [this](auto&&... args) { this->onEvent(std::forward<decltype(args)>(args)...); };

		submitPostFrameTask([](Application& app)
		{
			app.showWindow();
		});
	
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

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, uint32_t codepoint)
		{
			ApplicationDesc& desc = *reinterpret_cast<ApplicationDesc*>(glfwGetWindowUserPointer(window));
			KeyTypedEvent event(static_cast<KeyCode>(codepoint));

			desc.m_EventCallback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			ApplicationDesc& desc = *reinterpret_cast<ApplicationDesc*>(glfwGetWindowUserPointer(window));
			KeyPressedEvent event(static_cast<KeyCode>(key), action == GLFW_REPEAT);

			if (action == GLFW_PRESS || action == GLFW_REPEAT)
			{
				desc.m_EventCallback(event);
			}
		});

		s_App = this;

		windows::SetWindowShowIcon(m_Window, false);
		windows::SetWindowBorderColor(m_Window, { 0.39f, 0.07f, 0.54f });
		windows::SetWindowTitlebarColor(m_Window, { 0.39f, 0.07f, 0.54f });
        
        std::vector<macOS::MenuItem> testMenuItems = {
            macOS::MenuItem{
                "Test 1",
                []() -> void { WR_INFO("Test 1"); }
            },
            macOS::MenuItem{
                "Test 2",
                []() -> void { WR_INFO("Test 2"); }
            }
        };
        
        std::vector<macOS::Menu> menus = {
            macOS::Menu{
                "Test",
                testMenuItems.size(),
                testMenuItems.data()
            }
        };
        
        macOS::CreateMenuBar(menus.size(), menus.data());

		RendererDesc rendererDesc{};
		rendererDesc.API = RendererAPI::Vulkan;
		rendererDesc.ShaderCache.CachePath = "wire.shadercache";
		rendererDesc.FontCache.CachePath = "wire.fontcache";
        
        if (!std::filesystem::exists("shaders/"))
            std::filesystem::create_directory("shaders/");

        for (const auto& dir : std::filesystem::directory_iterator("shaders/"))
        {
            if (dir.path().has_extension() && dir.path().extension() == ".hlsl")
            {
                if (dir.path().string().ends_with(".compute.hlsl"))
                {
                    rendererDesc.ShaderCache.ShaderInfos.push_back(ShaderInfo{
                        .Path = dir.path(),
                        .IsGraphics = false,
                        .VertexOrComputeEntryPoint = "CShader"
                    });

                    continue;
                }

                rendererDesc.ShaderCache.ShaderInfos.push_back(ShaderInfo{
                    .Path = dir.path(),
                    .IsGraphics = true,
                    .VertexOrComputeEntryPoint = "VShader",
                    .PixelEntryPoint = "PShader"
                });
            }
        }

        if (!std::filesystem::exists("fonts/"))
            std::filesystem::create_directory("fonts/");

        for (const auto& dir : std::filesystem::directory_iterator("fonts/"))
        {
            if (dir.path().has_extension() && dir.path().extension() == ".ttf")
            {
                rendererDesc.FontCache.FontInfos.push_back(FontInfo{
                    .FontTTFPath = dir.path()
                });
            }
        }
        
        SwapchainDesc scDesc = {};
        scDesc.Attachments = {
            AttachmentFormat::SwapchainColorDefault,
            AttachmentFormat::SwapchainDepthDefault
        };

		m_Renderer = createRenderer(rendererDesc, scDesc);

		m_LayerStack = new LayerStack();
	}

	Application::~Application()
	{
		delete m_LayerStack;
		delete m_Renderer;

		glfwDestroyWindow(m_Window);
		glfwTerminate();

		Input::setWindow(nullptr);
	}

	static void testTrigger(Layout& layout)
	{
		layout.clear();

		static std::wstring text = L"";
		static std::wstring otherText = L"";
		
		layout.addButton("hi there");
		layout.addInputText(text, { 200.0f, 20.0f });
		layout.addInputText(otherText, { 150.0f, 20.0f });
	}

	void Application::run()
	{
		while (m_Desc.m_Running)
		{
			float time = (float)glfwGetTime();
			float timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			m_Renderer->beginFrame();

			for (Layer* layer : *m_LayerStack)
				layer->onUpdate(timestep);

			m_Renderer->endFrame();
			glfwPollEvents();

			for (auto& func : m_PostFrameTasks)
				func(*this);
			m_PostFrameTasks.clear();
		}
	}

	void Application::showWindow()
	{
		glfwShowWindow(m_Window);
	}

	void Application::hideWindow()
	{
		glfwHideWindow(m_Window);
	}

	void Application::onEvent(Event& event)
	{
		for (auto it = m_LayerStack->rbegin(); it != m_LayerStack->rend(); ++it)
		{
			if (event.handled)
				break;
			(*it)->onEvent(event);
		}
	}

	void Application::pushLayer(Layer* layer)
	{
		m_LayerStack->pushLayer(layer);
		layer->onAttach();
	}

	void Application::pushOverlay(Layer* layer)
	{
		m_LayerStack->pushOverlay(layer);
		layer->onAttach();
	}

	void Application::submitPostFrameTask(std::function<void(Application&)>&& func)
	{
		m_PostFrameTasks.emplace_back(func);
	}

}

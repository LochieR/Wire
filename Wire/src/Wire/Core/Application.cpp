#include "wrpch.h"
#include "Application.h"

#include "Log.h"
#include "Input.h"

#include "Wire/Renderer/Renderer.h"
#include "Wire/Audio/Audio.h"

#include "Wire/Utils/PlatformUtils.h"

#include <glfw/glfw3.h>
#include <imgui.h>

namespace Wire {

	Application* Application::s_Instance = nullptr;

	Application::Application(const std::string& name, ApplicationCommandLineArgs args)
		: m_CommandLineArgs(args)
	{
		WR_PROFILE_FUNCTION();

		WR_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;
		m_Window = Window::Create(WindowProps(name));
		m_Window->SetEventCallback(WR_BIND_EVENT_FN(Application::OnEvent));

		Renderer::Init();
		AudioPlayer::Init();

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	Application::~Application()
	{
		WR_PROFILE_FUNCTION();

		Renderer::Shutdown();
		AudioPlayer::Shutdown();
	}

	void Application::PushLayer(Layer* layer)
	{
		WR_PROFILE_FUNCTION();

		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		WR_PROFILE_FUNCTION();

		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnEvent(Event& e)
	{
		WR_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(WR_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(WR_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::Run()
	{
		WR_PROFILE_FUNCTION();

		while (m_Running)
		{
			WR_PROFILE_SCOPE("RunLoop");

			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				{
					WR_PROFILE_SCOPE("LayerStack OnUpdate");

					for (Layer* layer : m_LayerStack)
						layer->OnUpdate(timestep);
				}
			}

			m_ImGuiLayer->Begin();
			{
				WR_PROFILE_SCOPE("LayerStack OnImGuiRender");

				ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 11.25f);

				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();

				ImGui::PopStyleVar();
			}
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
			for (auto window : Popups::GetPopupWindows())
				window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		WR_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

		return false;
	}

}

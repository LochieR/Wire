#include "wrpch.h"
#include "Application.h"

#include "Wire/Audio/AudioEngine.h"
#include "Wire/Scripting/ScriptEngine.h"
#include "Wire/Renderer/Renderer.h"

#include "Wire/Utils/PlatformUtils.h"
#include "Wire/ImGui/ImGuiLayer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <iostream>

namespace Wire {

	Application* Application::s_Instance;

	Application::Application(const ApplicationSpecification& spec)
		: m_Specification(spec)
	{
		WR_ASSERT(!s_Instance);
		s_Instance = this;

		m_Window = std::make_shared<Window>(WindowSpecification{ spec.WindowSpec.Title, spec.WindowSpec.Width, spec.WindowSpec.Height, spec.WindowSpec.VSync, true });
		m_Window->SetEventCallback(WR_BIND_EVENT_FN(OnEvent));

		//ScriptEngine::Init();
		//AudioEngine::Init();
		
		m_Renderer = Renderer::Create(*m_Window);

		Input::SetActiveWindow(m_Window->GetNativeHandle());

		if (spec.EnableImGui)
		{
			m_ImGuiLayer = m_Renderer->CreateImGuiLayer();
			PushOverlay(m_ImGuiLayer);
		}
	}

	Application::~Application()
	{
		m_LayerStack.reset();

		m_Renderer->Release();

		//AudioEngine::Shutdown();
		//ScriptEngine::Shutdown();
	}

	void Application::Run()
	{
		m_Window->Show();

		while (m_Running)
		{
			float time = Time::GetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			ExecuteMainThreadQueue();

			if (m_Renderer->BeginFrame())
			{
				if (!m_Minimized) 
				{
					for (Layer* layer : *m_LayerStack)
						layer->OnUpdate(timestep);
				}

				if (m_Specification.EnableImGui)
				{
					m_ImGuiLayer->Begin();

					for (Layer* layer : *m_LayerStack)
						layer->OnImGuiRender();

					m_ImGuiLayer->End();
				}

				m_Renderer->EndFrame();

				for (Layer* layer : *m_LayerStack)
					layer->PostRender();
			}

			if (m_Specification.EnableImGui)
				m_ImGuiLayer->UpdateViewports();

			m_Window->OnUpdate();
		}

		m_Window->Hide();
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(WR_BIND_EVENT_FN(OnWindowClose));
		dispatcher.Dispatch<WindowEndResizeEvent>(WR_BIND_EVENT_FN(OnWindowResize));

		for (auto it = m_LayerStack->end(); it != m_LayerStack->begin(); )
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack->PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack->PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::SubmitToMainThread(std::function<void()>&& function)
	{
		std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

		m_MainThreadQueue.emplace_back(function);
	}

	void Application::ExecuteMainThreadQueue()
	{
		std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

		for (auto& func : m_MainThreadQueue)
			func();

		m_MainThreadQueue.clear();
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowEndResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;

		return false;
	}

}

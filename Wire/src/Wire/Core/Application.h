#pragma once

#include "Core.h"

#include "Window.h"
#include "LayerStack.h"
#include "Wire/Events/Event.h"
#include "Wire/Events/ApplicationEvent.h"

#include "Wire/Audio/Audio.h"

#include "Wire/Plugins/PluginManager.h"

#include "Timestep.h"

#include "Wire/ImGui/ImGuiLayer.h"

namespace Wire {

	class Application
	{
	public:
		Application(const std::string& name = "Wire Designer");
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }

		void Close();

		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		inline static Application& Get() { return *s_Instance; }
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
	private:
		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;

		bool m_Running = true;
		bool m_Minimized = false;

		LayerStack m_LayerStack;

		float m_LastFrameTime = 0.0f;
	private:
		static Application* s_Instance;
	};

	Application* CreateApplication();

}
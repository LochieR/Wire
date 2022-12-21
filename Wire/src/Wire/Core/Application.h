#pragma once

#include "Wire/Core/Core.h"

#include "Wire/Core/Window.h"
#include "Wire/Core/LayerStack.h"
#include "Wire/Events/Event.h"
#include "Wire/Events/ApplicationEvent.h"

#include "Wire/Core/Timestep.h"

#include "Wire/ImGui/ImGuiLayer.h"

namespace Wire {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }

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
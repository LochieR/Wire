#pragma once

#include "Core.h"

#include "Window.h"
#include "LayerStack.h"
#include "Wire/Events/Event.h"
#include "Wire/Events/ApplicationEvent.h"

#include "Wire/Audio/AudioEngine.h"

#include "Timestep.h"

#include "Wire/ImGui/ImGuiLayer.h"

namespace Wire {

	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		std::vector<char*> Args;

		const char* operator[](int index) const
		{
			WR_CORE_ASSERT(index < Count);
			return Args[index];
		}

		std::vector<char*>::iterator begin()
		{
			return Args.begin();
		}

		std::vector<char*>::iterator end()
		{
			return Args.end();
		}

		ApplicationCommandLineArgs(int count, char** args)
			: Count(count)
		{
			Args = std::vector<char*>(args, args + count);
		}

		ApplicationCommandLineArgs()
			: Count(0), Args()
		{
		}
	};

	class Application
	{
	public:
		Application(const std::string& name = "Wire Designer", ApplicationCommandLineArgs args = ApplicationCommandLineArgs());
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }

		void Close();

		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		static Application& Get() { return *s_Instance; }

		ApplicationCommandLineArgs GetCommandLineArgs() const { return m_CommandLineArgs; }
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
	private:
		ApplicationCommandLineArgs m_CommandLineArgs;
		Ref<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;

		bool m_Running = true;
		bool m_Minimized = false;

		LayerStack m_LayerStack;

		float m_LastFrameTime = 0.0f;
	private:
		static Application* s_Instance;
	};

	Application* CreateApplication(ApplicationCommandLineArgs args);

}

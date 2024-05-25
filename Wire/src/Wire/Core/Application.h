#pragma once

#include "Base.h"
#include "Window.h"
#include "Event.h"
#include "LayerStack.h"
#include "Timestep.h"

#include <vector>
#include <string>
#include <mutex>

namespace Wire {

	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		std::vector<char*> Args;

		std::vector<char*>::iterator begin()
		{
			return Args.begin();
		}

		std::vector<char*>::iterator end()
		{
			return Args.end();
		}

		std::vector<char*>::const_iterator begin() const
		{
			return Args.begin();
		}

		std::vector<char*>::const_iterator end() const
		{
			return Args.end();
		}

		std::string operator[](int index) const
		{
			return Args[index];
		}

		ApplicationCommandLineArgs() = default;

		ApplicationCommandLineArgs(int count, char** args)
			: Count(count)
		{
			Args = std::vector<char*>(args, args + count);
		}
	};

	struct ApplicationSpecification
	{
		std::string Name = "Wire Application";
		ApplicationCommandLineArgs CommandLineArgs = ApplicationCommandLineArgs();

		WindowSpecification WindowSpec = WindowSpecification();

		bool EnableImGui = true;
	};

	class Renderer;
	class ImGuiLayer;

	class Application
	{
	public:
		Application(const ApplicationSpecification& spec);
		~Application();

		void Run();
		void Close();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		Window& GetWindow() { return *m_Window; }

		ImGuiLayer* GetImGuiLayer() const { return m_ImGuiLayer; }
		bool GetImGuiPaused() const { return m_ImGuiPaused; }
		void SetImGuiPaused(bool paused) { m_ImGuiPaused = paused; }

		void SubmitToMainThread(std::function<void()>&& function);

		Renderer* GetRenderer() { return m_Renderer; }

		static Application& Get() { return *s_Instance; }
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowEndResizeEvent& e);

		void ExecuteMainThreadQueue();
	private:
		ApplicationSpecification m_Specification;
		std::shared_ptr<Window> m_Window;

		Renderer* m_Renderer = nullptr;

		ImGuiLayer* m_ImGuiLayer = nullptr;
		bool m_ImGuiPaused = false;

		bool m_Minimized = false;
		bool m_Running = true;

		std::unique_ptr<LayerStack> m_LayerStack = std::make_unique<LayerStack>();
		float m_LastFrameTime = 0.0f;

		std::vector<std::function<void()>> m_MainThreadQueue;
		std::mutex m_MainThreadQueueMutex;
	private:
		static Application* s_Instance;
	};
	Application* CreateApplication(const ApplicationCommandLineArgs& args);

}

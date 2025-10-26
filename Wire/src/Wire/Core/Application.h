#pragma once

#include "Event.h"
#include "Input.h"
#include "LayerStack.h"
#include "Wire/UI/ImGuiLayer.h"

#include "Wire/Renderer/Instance.h"

#include <string>
#include <cstdint>
#include <functional>

struct GLFWwindow;

namespace wire {

	struct ApplicationDesc
	{
		std::string WindowTitle;
		uint32_t WindowWidth, WindowHeight;

	private:
		bool m_Running = true;
		bool m_WasWindowResized = false;

		std::function<void(Event&)> m_EventCallback;

		friend class Application;
	};

	class Application
	{
	public:
		Application(const ApplicationDesc& desc);
		~Application();

		void run();

		GLFWwindow* getWindow() const { return m_Window; }
		void showWindow();
		void hideWindow();

		void onEvent(Event& event);

		void pushLayer(Layer* layer);
		void pushOverlay(Layer* layer);

		bool wasWindowResized() const { return m_Desc.m_WasWindowResized; };
		void resetWindowResized() { m_Desc.m_WasWindowResized = false; }

		void submitPostFrameTask(std::function<void(Application&)>&& func);

		Instance& getInstance() const { return *m_Instance; }
		const std::shared_ptr<Device>& getDevice() const { return m_Device; }
		const ApplicationDesc& getDesc() const { return m_Desc; }

		static Application& get() { return *s_App; }
	private:
		ApplicationDesc m_Desc;

		GLFWwindow* m_Window;
		std::unique_ptr<Instance> m_Instance;
		std::shared_ptr<Device> m_Device;

		LayerStack* m_LayerStack = nullptr;
		ImGuiLayer* m_ImGuiLayer = nullptr;

		float m_LastFrameTime = 0.0f;

		std::vector<std::function<void(Application&)>> m_PostFrameTasks;
	private:
		inline static Application* s_App = nullptr;
	};

}

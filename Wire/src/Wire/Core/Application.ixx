module;

#include <string>
#include <cstdint>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 5202)
#elif defined(__GNUC__) || defined(__clang__)
#endif

struct GLFWwindow;

#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
#endif

export module wire.core:app;

import wire.ui.renderer;

namespace wire {

	export struct ApplicationDesc
	{
		std::string WindowTitle;
		uint32_t WindowWidth, WindowHeight;

	private:
		bool m_Running = true;
		bool m_WasWindowResized = false;

		friend class Application;
	};

	export class Application
	{
	public:
		Application(const ApplicationDesc& desc);
		~Application();

		void run();

		GLFWwindow* getWindow() const { return m_Window; }
		bool wasWindowResized() const { return m_Desc.m_WasWindowResized; };
		void resetWindowResized() { m_Desc.m_WasWindowResized = false; }

		const ApplicationDesc& getDesc() const { return m_Desc; }

		static Application& get() { return *s_App; }
	private:
		ApplicationDesc m_Desc;

		GLFWwindow* m_Window;
		Renderer* m_Renderer;

		inline static Application* s_App = nullptr;
	};

}

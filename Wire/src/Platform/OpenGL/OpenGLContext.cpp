#include "wrpch.h"
#include "Platform/OpenGL/OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Wire {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		WR_CORE_ASSERT(windowHandle, "Window handle is null!")
	}

	void OpenGLContext::Init()
	{
		WR_PROFILE_FUNCTION();

		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		WR_CORE_ASSERT(status, "Failed to initialize Glad!");

		WR_CORE_INFO("OpenGL Info:");
		WR_CORE_INFO("  Vendor: {0}", glGetString(GL_VENDOR));
		WR_CORE_INFO("  Renderer: {0}", glGetString(GL_RENDERER));
		WR_CORE_INFO("  Version: {0}", glGetString(GL_VERSION));

		WR_CORE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Wire requires at least OpenGL version 4.5!");
	}

	void OpenGLContext::SwapBuffers()
	{
		WR_PROFILE_FUNCTION();

		glfwSwapBuffers(m_WindowHandle);
	}

}

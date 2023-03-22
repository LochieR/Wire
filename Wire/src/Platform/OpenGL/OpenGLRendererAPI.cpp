#include "wrpch.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

#include <glad/glad.h>

namespace Wire {
	
	void OpenGLMessageCallback(
		uint32_t source,
		uint32_t type,
		uint32_t id,
		uint32_t severity,
		int length,
		const char* message,
		const void* userParam)
	{
		// Disable warning for lines with a width of over 1.00
		if (id == 7)
			return;

		switch (severity)
		{
			case GL_DEBUG_SEVERITY_HIGH:         WR_CORE_CRITICAL(message); return;
			case GL_DEBUG_SEVERITY_MEDIUM:       WR_CORE_ERROR(message); return;
			case GL_DEBUG_SEVERITY_LOW:          WR_CORE_WARN(message); return;
			case GL_DEBUG_SEVERITY_NOTIFICATION: WR_CORE_TRACE(message); return;
		}
		
		WR_CORE_ASSERT(false, "Unknown severity level!");
	}

	void OpenGLRendererAPI::Init()
	{
		WR_PROFILE_FUNCTION();

	#ifdef WR_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLMessageCallback, nullptr);
		
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
	#endif

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);
	}

	void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::SetClearColour(const glm::vec4& colour)
	{
		glClearColor(colour.r, colour.g, colour.b, colour.a);
	}

	void OpenGLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
		vertexArray->Bind();
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
	{
		vertexArray->Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

	void OpenGLRendererAPI::SetLineWidth(float width)
	{
		glLineWidth(width);
	}

}

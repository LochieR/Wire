#include "wrpch.h"
#include "Wire/Core/Input.h"

#include <glfw/glfw3.h>

namespace Wire {

	bool Input::IsKeyDown(KeyCode key)
	{
		return glfwGetKey(static_cast<GLFWwindow*>(s_ActiveWindow), static_cast<int>(key)) == GLFW_PRESS;
	}

	bool Input::IsMouseButtonDown(MouseButton button)
	{
		return glfwGetMouseButton(static_cast<GLFWwindow*>(s_ActiveWindow), static_cast<int>(button)) == GLFW_PRESS;
	}

	glm::vec2 Input::GetMousePosition()
	{
		double x, y;
		glfwGetCursorPos(static_cast<GLFWwindow*>(s_ActiveWindow), &x, &y);
		return { static_cast<float>(x), static_cast<float>(y) };
	}

}

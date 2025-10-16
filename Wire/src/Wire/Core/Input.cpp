#include "Input.h"

#include <glfw/glfw3.h>

namespace wire {

	bool Input::isKeyDown(KeyCode key)
	{
		return glfwGetKey(s_Window, static_cast<int>(key)) == GLFW_PRESS;
	}

	bool Input::isMouseButtonDown(MouseButton mouseButton)
	{
		return glfwGetMouseButton(s_Window, static_cast<int>(mouseButton)) == GLFW_PRESS;
	}

	glm::vec2 Input::getMousePosition()
	{
		double x, y;
		glfwGetCursorPos(s_Window, &x, &y);
		return { (float)x, (float)y };
	}

}

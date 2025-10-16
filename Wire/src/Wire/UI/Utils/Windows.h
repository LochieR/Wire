#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace wire::windows {

	void SetWindowBorderColor(GLFWwindow* window, const glm::vec3& color);
	void SetWindowTitlebarColor(GLFWwindow* window, const glm::vec3& color);
	void SetWindowShowIcon(GLFWwindow* window, bool show);

}

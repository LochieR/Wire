module;

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

export module wire.ui.utils.windows;


namespace wire::windows {

	export void SetWindowBorderColor(GLFWwindow* window, const glm::vec3& color);
	export void SetWindowTitlebarColor(GLFWwindow* window, const glm::vec3& color);
	export void SetWindowShowIcon(GLFWwindow* window, bool show);

}

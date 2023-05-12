#pragma once

#include "Core.h"
#include "KeyCodes.h"
#include "MouseCodes.h"

namespace Wire {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode keycode);

		static bool IsMouseButtonPressed(MouseButton button);
		static std::pair<float, float> GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};
}
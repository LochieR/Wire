#pragma once

#include <memory>

namespace Wire {

	enum class MouseButton : uint16_t
	{
		// From glfw3.h
		Button0 = 0,
		Button1 = 1,
		Button2 = 2,
		Button3 = 3,
		Button4 = 4,
		Button5 = 5,
		Button6 = 6,
		Button7 = 7,

		ButtonLast = Button7,
		ButtonLeft = Button0,
		ButtonRight = Button1,
		ButtonMiddle = Button2
	};

	inline std::ostream& operator<<(std::ostream& os, MouseButton mouseCode)
	{
		os << static_cast<int>(mouseCode);
		return os;
	}

}

#define WR_MOUSE_BUTTON_0      ::Wire::MouseButton::Button0
#define WR_MOUSE_BUTTON_1      ::Wire::MouseButton::Button1
#define WR_MOUSE_BUTTON_2      ::Wire::MouseButton::Button2
#define WR_MOUSE_BUTTON_3      ::Wire::MouseButton::Button3
#define WR_MOUSE_BUTTON_4      ::Wire::MouseButton::Button4
#define WR_MOUSE_BUTTON_5      ::Wire::MouseButton::Button5
#define WR_MOUSE_BUTTON_6      ::Wire::MouseButton::Button6
#define WR_MOUSE_BUTTON_7      ::Wire::MouseButton::Button7
#define WR_MOUSE_BUTTON_LAST   ::Wire::MouseButton::ButtonLast
#define WR_MOUSE_BUTTON_LEFT   ::Wire::MouseButton::ButtonLeft
#define WR_MOUSE_BUTTON_RIGHT  ::Wire::MouseButton::ButtonRight
#define WR_MOUSE_BUTTON_MIDDLE ::Wire::MouseButton::ButtonMiddle
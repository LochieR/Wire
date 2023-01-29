#pragma once

#include <memory>

namespace Wire {

	typedef enum class MouseCode : uint16_t
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
	} Mouse;

	inline std::ostream& operator<<(std::ostream& os, MouseCode mouseCode)
	{
		os << static_cast<int>(mouseCode);
		return os;
	}

}

#define WR_MOUSE_BUTTON_0      ::Wire::Mouse::Button0
#define WR_MOUSE_BUTTON_1      ::Wire::Mouse::Button1
#define WR_MOUSE_BUTTON_2      ::Wire::Mouse::Button2
#define WR_MOUSE_BUTTON_3      ::Wire::Mouse::Button3
#define WR_MOUSE_BUTTON_4      ::Wire::Mouse::Button4
#define WR_MOUSE_BUTTON_5      ::Wire::Mouse::Button5
#define WR_MOUSE_BUTTON_6      ::Wire::Mouse::Button6
#define WR_MOUSE_BUTTON_7      ::Wire::Mouse::Button7
#define WR_MOUSE_BUTTON_LAST   ::Wire::Mouse::ButtonLast
#define WR_MOUSE_BUTTON_LEFT   ::Wire::Mouse::ButtonLeft
#define WR_MOUSE_BUTTON_RIGHT  ::Wire::Mouse::ButtonRight
#define WR_MOUSE_BUTTON_MIDDLE ::Wire::Mouse::ButtonMiddle
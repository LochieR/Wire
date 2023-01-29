#pragma once

namespace Wire {

	typedef enum class KeyCode : uint16_t
	{
		// From glfw3.h
		Space = 32,
		Apostrophe = 39, /* ' */
		Comma = 44, /* , */
		Minus = 45, /* - */
		Period = 46, /* . */
		Slash = 47, /* / */

		D0 = 48, /* 0 */
		D1 = 49, /* 1 */
		D2 = 50, /* 2 */
		D3 = 51, /* 3 */
		D4 = 52, /* 4 */
		D5 = 53, /* 5 */
		D6 = 54, /* 6 */
		D7 = 55, /* 7 */
		D8 = 56, /* 8 */
		D9 = 57, /* 9 */

		Semicolon = 59, /* ; */
		Equal = 61, /* = */

		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 84,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,

		LeftBracket = 91,  /* [ */
		Backslash = 92,  /* \ */
		RightBracket = 93,  /* ] */
		GraveAccent = 96,  /* ` */

		World1 = 161, /* non-US #1 */
		World2 = 162, /* non-US #2 */

		/* Function keys */
		Escape = 256,
		Enter = 257,
		Tab = 258,
		Backspace = 259,
		Insert = 260,
		Delete = 261,
		Right = 262,
		Left = 263,
		Down = 264,
		Up = 265,
		PageUp = 266,
		PageDown = 267,
		Home = 268,
		End = 269,
		CapsLock = 280,
		ScrollLock = 281,
		NumLock = 282,
		PrintScreen = 283,
		Pause = 284,
		F1 = 290,
		F2 = 291,
		F3 = 292,
		F4 = 293,
		F5 = 294,
		F6 = 295,
		F7 = 296,
		F8 = 297,
		F9 = 298,
		F10 = 299,
		F11 = 300,
		F12 = 301,
		F13 = 302,
		F14 = 303,
		F15 = 304,
		F16 = 305,
		F17 = 306,
		F18 = 307,
		F19 = 308,
		F20 = 309,
		F21 = 310,
		F22 = 311,
		F23 = 312,
		F24 = 313,
		F25 = 314,

		/* Keypad */
		KP0 = 320,
		KP1 = 321,
		KP2 = 322,
		KP3 = 323,
		KP4 = 324,
		KP5 = 325,
		KP6 = 326,
		KP7 = 327,
		KP8 = 328,
		KP9 = 329,
		KPDecimal = 330,
		KPDivide = 331,
		KPMultiply = 332,
		KPSubtract = 333,
		KPAdd = 334,
		KPEnter = 335,
		KPEqual = 336,

		LeftShift = 340,
		LeftControl = 341,
		LeftAlt = 342,
		LeftSuper = 343,
		RightShift = 344,
		RightControl = 345,
		RightAlt = 346,
		RightSuper = 347,
		Menu = 348
	} Key;

	inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
	{
		os << static_cast<int>(keyCode);
		return os;
	}

}

// From glfw3.h
#define WR_KEY_SPACE           ::Wire::Key::Space
#define WR_KEY_APOSTROPHE      ::Wire::Key::Apostrophe    /* ' */
#define WR_KEY_COMMA           ::Wire::Key::Comma         /* , */
#define WR_KEY_MINUS           ::Wire::Key::Minus         /* - */
#define WR_KEY_PERIOD          ::Wire::Key::Period        /* . */
#define WR_KEY_SLASH           ::Wire::Key::Slash         /* / */
#define WR_KEY_0               ::Wire::Key::D0
#define WR_KEY_1               ::Wire::Key::D1
#define WR_KEY_2               ::Wire::Key::D2
#define WR_KEY_3               ::Wire::Key::D3
#define WR_KEY_4               ::Wire::Key::D4
#define WR_KEY_5               ::Wire::Key::D5
#define WR_KEY_6               ::Wire::Key::D6
#define WR_KEY_7               ::Wire::Key::D7
#define WR_KEY_8               ::Wire::Key::D8
#define WR_KEY_9               ::Wire::Key::D9
#define WR_KEY_SEMICOLON       ::Wire::Key::Semicolon     /* ; */
#define WR_KEY_EQUAL           ::Wire::Key::Equal         /* = */
#define WR_KEY_A               ::Wire::Key::A
#define WR_KEY_B               ::Wire::Key::B
#define WR_KEY_C               ::Wire::Key::C
#define WR_KEY_D               ::Wire::Key::D
#define WR_KEY_E               ::Wire::Key::E
#define WR_KEY_F               ::Wire::Key::F
#define WR_KEY_G               ::Wire::Key::G
#define WR_KEY_H               ::Wire::Key::H
#define WR_KEY_I               ::Wire::Key::I
#define WR_KEY_J               ::Wire::Key::J
#define WR_KEY_K               ::Wire::Key::K
#define WR_KEY_L               ::Wire::Key::L
#define WR_KEY_M               ::Wire::Key::M
#define WR_KEY_N               ::Wire::Key::N
#define WR_KEY_O               ::Wire::Key::O
#define WR_KEY_P               ::Wire::Key::P
#define WR_KEY_Q               ::Wire::Key::Q
#define WR_KEY_R               ::Wire::Key::R
#define WR_KEY_S               ::Wire::Key::S
#define WR_KEY_T               ::Wire::Key::T
#define WR_KEY_U               ::Wire::Key::U
#define WR_KEY_V               ::Wire::Key::V
#define WR_KEY_W               ::Wire::Key::W
#define WR_KEY_X               ::Wire::Key::X
#define WR_KEY_Y               ::Wire::Key::Y
#define WR_KEY_Z               ::Wire::Key::Z
#define WR_KEY_LEFT_BRACKET    ::Wire::Key::LeftBracket   /* [ */
#define WR_KEY_BACKSLASH       ::Wire::Key::Backslash     /* \ */
#define WR_KEY_RIGHT_BRACKET   ::Wire::Key::RightBracket  /* ] */
#define WR_KEY_GRAVE_ACCENT    ::Wire::Key::GraveAccent   /* ` */
#define WR_KEY_WORLD_1         ::Wire::Key::World1        /* non-US #1 */
#define WR_KEY_WORLD_2         ::Wire::Key::World2        /* non-US #2 */

/* Function keys */
#define WR_KEY_ESCAPE          ::Wire::Key::Escape
#define WR_KEY_ENTER           ::Wire::Key::Enter
#define WR_KEY_TAB             ::Wire::Key::Tab
#define WR_KEY_BACKSPACE       ::Wire::Key::Backspace
#define WR_KEY_INSERT          ::Wire::Key::Insert
#define WR_KEY_DELETE          ::Wire::Key::Delete
#define WR_KEY_RIGHT           ::Wire::Key::Right
#define WR_KEY_LEFT            ::Wire::Key::Left
#define WR_KEY_DOWN            ::Wire::Key::Down
#define WR_KEY_UP              ::Wire::Key::Up
#define WR_KEY_PAGE_UP         ::Wire::Key::PageUp
#define WR_KEY_PAGE_DOWN       ::Wire::Key::PageDown
#define WR_KEY_HOME            ::Wire::Key::Home
#define WR_KEY_END             ::Wire::Key::End
#define WR_KEY_CAPS_LOCK       ::Wire::Key::CapsLock
#define WR_KEY_SCROLL_LOCK     ::Wire::Key::ScrollLock
#define WR_KEY_NUM_LOCK        ::Wire::Key::NumLock
#define WR_KEY_PRINT_SCREEN    ::Wire::Key::PrintScreen
#define WR_KEY_PAUSE           ::Wire::Key::Pause
#define WR_KEY_F1              ::Wire::Key::F1
#define WR_KEY_F2              ::Wire::Key::F2
#define WR_KEY_F3              ::Wire::Key::F3
#define WR_KEY_F4              ::Wire::Key::F4
#define WR_KEY_F5              ::Wire::Key::F5
#define WR_KEY_F6              ::Wire::Key::F6
#define WR_KEY_F7              ::Wire::Key::F7
#define WR_KEY_F8              ::Wire::Key::F8
#define WR_KEY_F9              ::Wire::Key::F9
#define WR_KEY_F10             ::Wire::Key::F10
#define WR_KEY_F11             ::Wire::Key::F11
#define WR_KEY_F12             ::Wire::Key::F12
#define WR_KEY_F13             ::Wire::Key::F13
#define WR_KEY_F14             ::Wire::Key::F14
#define WR_KEY_F15             ::Wire::Key::F15
#define WR_KEY_F16             ::Wire::Key::F16
#define WR_KEY_F17             ::Wire::Key::F17
#define WR_KEY_F18             ::Wire::Key::F18
#define WR_KEY_F19             ::Wire::Key::F19
#define WR_KEY_F20             ::Wire::Key::F20
#define WR_KEY_F21             ::Wire::Key::F21
#define WR_KEY_F22             ::Wire::Key::F22
#define WR_KEY_F23             ::Wire::Key::F23
#define WR_KEY_F24             ::Wire::Key::F24
#define WR_KEY_F25             ::Wire::Key::F25

/* Keypad */
#define WR_KEY_KP_0            ::Wire::Key::KP0
#define WR_KEY_KP_1            ::Wire::Key::KP1
#define WR_KEY_KP_2            ::Wire::Key::KP2
#define WR_KEY_KP_3            ::Wire::Key::KP3
#define WR_KEY_KP_4            ::Wire::Key::KP4
#define WR_KEY_KP_5            ::Wire::Key::KP5
#define WR_KEY_KP_6            ::Wire::Key::KP6
#define WR_KEY_KP_7            ::Wire::Key::KP7
#define WR_KEY_KP_8            ::Wire::Key::KP8
#define WR_KEY_KP_9            ::Wire::Key::KP9
#define WR_KEY_KP_DECIMAL      ::Wire::Key::KPDecimal
#define WR_KEY_KP_DIVIDE       ::Wire::Key::KPDivide
#define WR_KEY_KP_MULTIPLY     ::Wire::Key::KPMultiply
#define WR_KEY_KP_SUBTRACT     ::Wire::Key::KPSubtract
#define WR_KEY_KP_ADD          ::Wire::Key::KPAdd
#define WR_KEY_KP_ENTER        ::Wire::Key::KPEnter
#define WR_KEY_KP_EQUAL        ::Wire::Key::KPEqual

#define WR_KEY_LEFT_SHIFT      ::Wire::Key::LeftShift
#define WR_KEY_LEFT_CONTROL    ::Wire::Key::LeftControl
#define WR_KEY_LEFT_ALT        ::Wire::Key::LeftAlt
#define WR_KEY_LEFT_SUPER      ::Wire::Key::LeftSuper
#define WR_KEY_RIGHT_SHIFT     ::Wire::Key::RightShift
#define WR_KEY_RIGHT_CONTROL   ::Wire::Key::RightControl
#define WR_KEY_RIGHT_ALT       ::Wire::Key::RightAlt
#define WR_KEY_RIGHT_SUPER     ::Wire::Key::RightSuper
#define WR_KEY_MENU            ::Wire::Key::Menu
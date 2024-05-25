#pragma once

#include "wrpch.h"

#include "Input.h"

namespace Wire {

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowEndResize,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	class Event
	{
	public:
		virtual ~Event() = default;

		bool Handled = false;

		virtual EventType GetEventType() const = 0;
		virtual std::string GetName() const = 0;
		virtual std::string ToString() const { return GetName(); }
	};

	class EventDispatcher
	{
	public:
		EventDispatcher(Event& event)
			: m_Event(event)
		{
		}

		template<typename T, typename Func>
		bool Dispatch(const Func& func)
		{
			if (m_Event.GetEventType() == T::GetEventTypeStatic())
			{
				m_Event.Handled |= func(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}
	private:
		Event& m_Event;
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}

	class KeyEvent : public Event
	{
	public:
		KeyCode GetKeyCode() const { return m_KeyCode; }
	protected:
		KeyEvent(KeyCode keycode)
			: m_KeyCode(keycode) {}

		KeyCode m_KeyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(const KeyCode keycode, bool isRepeat = false)
			: KeyEvent(keycode), m_IsRepeat(isRepeat)
		{
		}

		bool IsRepeat() const { return m_IsRepeat; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyPressedEvent: " << (int)m_KeyCode << " (is repeat: " << m_IsRepeat << ")";
			return ss.str();
		}

		virtual EventType GetEventType() const override { return GetEventTypeStatic(); }
		virtual std::string GetName() const override { return "KeyPressed"; }

		static EventType GetEventTypeStatic() { return EventType::KeyPressed; }
	private:
		bool m_IsRepeat;
	};
	
	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(KeyCode keycode)
			: KeyEvent(keycode) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << (int)m_KeyCode;
			return ss.str();
		}

		virtual EventType GetEventType() const override { return GetEventTypeStatic(); }
		virtual std::string GetName() const override { return "KeyReleased"; }

		static EventType GetEventTypeStatic() { return EventType::KeyReleased; }
	};

	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(KeyCode keycode)
			: KeyEvent(keycode)
		{
		}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyTypedEvent: " << (int)m_KeyCode;
			return ss.str();
		}

		virtual EventType GetEventType() const override { return GetEventTypeStatic(); }
		virtual std::string GetName() const override { return "KeyTyped"; }

		static EventType GetEventTypeStatic() { return EventType::KeyTyped; }
	};

	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(uint32_t width, uint32_t height)
			: m_Width(width), m_Height(height)
		{
		}

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		virtual EventType GetEventType() const override { return GetEventTypeStatic(); }
		virtual std::string GetName() const override { return "WindowResize"; }

		static EventType GetEventTypeStatic() { return EventType::WindowResize; }
	private:
		uint32_t m_Width, m_Height;
	};

	class WindowEndResizeEvent : public Event
	{
	public:
		WindowEndResizeEvent(uint32_t width, uint32_t height)
			: m_Width(width), m_Height(height)
		{
		}

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowEndResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		virtual EventType GetEventType() const override { return GetEventTypeStatic(); }
		virtual std::string GetName() const override { return "WindowEndResize"; }

		static EventType GetEventTypeStatic() { return EventType::WindowEndResize; }
	private:
		uint32_t m_Width, m_Height;
	};

	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

		virtual EventType GetEventType() const override { return GetEventTypeStatic(); }
		virtual std::string GetName() const override { return "WindowClose"; }

		static EventType GetEventTypeStatic() { return EventType::WindowClose; }
	};

	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(float x, float y)
			: m_X(x), m_Y(y) {}

		float GetX() const { return m_X; }
		float GetY() const { return m_Y; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_X << ", " << m_Y;
			return ss.str();
		}

		virtual EventType GetEventType() const override { return GetEventTypeStatic(); }
		virtual std::string GetName() const override { return "MouseMoved"; }

		static EventType GetEventTypeStatic() { return EventType::MouseMoved; }
	private:
		float m_X, m_Y;
	};

	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(float xOffset, float yOffset)
			: m_XOffset(xOffset), m_YOffset(yOffset)
		{
		}

		float GetXOffset() const { return m_XOffset; }
		float GetYOffset() const { return m_YOffset; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << GetXOffset() << ", " << GetYOffset();
			return ss.str();
		}

		virtual EventType GetEventType() const override { return GetEventTypeStatic(); }
		virtual std::string GetName() const override { return "MouseScrolled"; }

		static EventType GetEventTypeStatic() { return EventType::MouseScrolled; }
	private:
		float m_XOffset, m_YOffset;
	};

	class MouseButtonEvent : public Event
	{
	public:
		MouseButton GetMouseButton() const { return m_Button; }
	protected:
		MouseButtonEvent(MouseButton button)
			: m_Button(button)
		{
		}

		MouseButton m_Button;
	};

	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(MouseButton button)
			: MouseButtonEvent(button)
		{
		}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << (int)m_Button;
			return ss.str();
		}

		virtual EventType GetEventType() const override { return GetEventTypeStatic(); }
		virtual std::string GetName() const override { return "MouseButtonPressed"; }

		static EventType GetEventTypeStatic() { return EventType::MouseButtonPressed; }
	};

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(MouseButton button)
			: MouseButtonEvent(button)
		{
		}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << (int)m_Button;
			return ss.str();
		}

		virtual EventType GetEventType() const override { return GetEventTypeStatic(); }
		virtual std::string GetName() const override { return "MouseButtonReleased"; }

		static EventType GetEventTypeStatic() { return EventType::MouseButtonReleased; }
	};

}

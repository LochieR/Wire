#pragma once

#include "Input.h"

#include <string>

namespace wire {

	enum class EventType
	{
		None = 0,
		KeyTyped, KeyPressed
	};

	class Event
	{
	public:
		virtual ~Event() = default;

		bool handled = false;

		virtual EventType getEventType() const = 0;
	};

	class EventDispatcher
	{
	public:
		EventDispatcher(Event& event)
			: m_Event(event)
		{
		}

		template<typename T, typename F>
		bool Dispatch(const F& func)
		{
			if (m_Event.getEventType() == T::getStaticType())
			{
				m_Event.handled |= func(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}
	private:
		Event& m_Event;
	};

	class KeyPressedEvent : public Event
	{
	public:
		KeyPressedEvent(KeyCode keycode, bool isRepeat = false)
			: m_KeyCode(keycode), m_IsRepeat(isRepeat)
		{
		}

		KeyCode getKeyCode() const { return m_KeyCode; }

		virtual EventType getEventType() const override { return getStaticType(); }
		static EventType getStaticType() { return EventType::KeyPressed; }
	private:
		KeyCode m_KeyCode;
		bool m_IsRepeat;
	};

	class KeyTypedEvent : public Event
	{
	public:
		KeyTypedEvent(KeyCode keycode)
			: m_KeyCode(keycode)
		{
		}

		KeyCode getKeyCode() const { return m_KeyCode; }

		virtual EventType getEventType() const override { return getStaticType(); }
		static EventType getStaticType() { return EventType::KeyTyped; }
	private:
		KeyCode m_KeyCode;
	};

}

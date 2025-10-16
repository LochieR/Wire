#include "Component.h"

#include "Button.h"
#include "InputText.h"

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace wire {

	Layout::Layout(Canvas& canvas, const glm::vec2& layoutPosition)
		: m_Canvas(&canvas), m_LayoutPosition(layoutPosition)
	{
	}

	void Layout::draw(Canvas& canvas)
	{
		for (auto& component : m_Components)
			component->draw(canvas);

		for (auto& trigger : m_CurrentTriggers)
			trigger(*this);
		m_CurrentTriggers.clear();

		m_InputBuffer.clear();
	}

	Component& Layout::addButton(const std::string& label, const glm::vec2& size)
	{
		m_Components.push_back(std::make_shared<Button>(*this, label, m_LayoutPosition + m_CursorPos, size));

		Component& component = *m_Components.back();

		m_CursorPos.y += component.getSize().y + m_Padding.y * 2.0f + m_Spacing;
		m_CursorPos.x = 0.0f;

		return component;
	}

	Component& Layout::addInputText(std::string& text, const glm::vec2& size)
	{
		m_Components.push_back(std::make_shared<InputText>(*this, text, m_LayoutPosition + m_CursorPos, size));

		Component& component = *m_Components.back();

		m_CursorPos.y += component.getSize().y + m_Padding.y * 2.0f + m_Spacing;
		m_CursorPos.x = 0.0f;

		return *m_Components.back();
	}

	Component& Layout::addInputText(std::wstring& text, const glm::vec2& size)
	{
		m_Components.push_back(std::make_shared<InputText>(*this, text, m_LayoutPosition + m_CursorPos, size));

		Component& component = *m_Components.back();

		m_CursorPos.y += component.getSize().y + m_Padding.y * 2.0f + m_Spacing;
		m_CursorPos.x = 0.0f;

		return *m_Components.back();
	}

	void Layout::onEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyTypedEvent>([this](auto&&... args) { return onKeyTypedEvent(std::forward<decltype(args)>(args)...); });
		dispatcher.Dispatch<KeyPressedEvent>([this](auto&&... args) { return onKeyPressedEvent(std::forward<decltype(args)>(args)...); });
	}

	void Layout::sameLine()
	{
		Component& back = *m_Components.back();
		glm::vec2 lastPosition = back.getPosition() - m_LayoutPosition;
		glm::vec2 lastSize = back.getSize();
		m_CursorPos.x = lastPosition.x + lastSize.x + 2.0f * m_Padding.x + m_Spacing;
		m_CursorPos.y = lastPosition.y;
	}

	void Layout::clear()
	{
		m_Components.clear();
		m_CursorPos = { 0.0f, 0.0f };
	}

	void Layout::trigger(const std::function<void(Layout&)>& func)
	{
		m_CurrentTriggers.push_back(func);
	}
	
	bool Layout::onKeyTypedEvent(KeyTypedEvent& event)
	{
		m_InputBuffer += static_cast<wchar_t>(event.getKeyCode());

		return false;
	}

	bool Layout::onKeyPressedEvent(KeyPressedEvent& event)
	{
		if (event.getKeyCode() == KeyCode::Backspace)
			m_InputBuffer += CustomUnicodeEvents::Backspace;
		else if (event.getKeyCode() == KeyCode::Delete)
			m_InputBuffer += CustomUnicodeEvents::Delete;
		else if (event.getKeyCode() == KeyCode::Left)
			m_InputBuffer += CustomUnicodeEvents::LeftArrow;
		else if (event.getKeyCode() == KeyCode::Right)
			m_InputBuffer += CustomUnicodeEvents::RightArrow;

		return false;
	}

}

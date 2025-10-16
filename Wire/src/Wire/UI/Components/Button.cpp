#include "Button.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>

namespace wire {

	Button::Button(Layout& layout, const std::string& label, const glm::vec2& position, const glm::vec2& size)
		: m_Layout(layout), m_Label(label), m_Position(position), m_Size(size)
	{
		if (m_Size.x == 0.0f || m_Size.y == 0.0f)
		{
			glm::vec2 textSize = layout.getCanvas().calculateTextSize(label, glm::scale(glm::mat4(1.0f), { 25.0f, 25.0f, 1.0f }), {});

			if (m_Size.x == 0.0f)
				m_Size.x = textSize.x;
			if (m_Size.y == 0.0f)
				m_Size.y = 20.0f;
		}
	}

	Button::~Button()
	{
	}

	void Button::draw(Canvas& canvas)
	{
		bool press = false;

		// TODO: style
		glm::vec2 padding = m_Layout.getPadding();
		
		glm::vec2 textSize = canvas.calculateTextSize(m_Label, glm::scale(glm::mat4(1.0f), { 25.0f, 25.0f, 1.0f }), {});

		glm::vec2 min = m_Position;
		glm::vec2 max = { min.x + padding.x + m_Size.x + padding.x, min.y + padding.y + m_Size.y + padding.y };

		glm::vec2 textPosition = { min.x + padding.x, min.y + padding.y + textSize.y + 2.0f };
		glm::mat4 textTransform = glm::translate(glm::mat4(1.0f), { textPosition.x, textPosition.y, 0.0f })
			* glm::scale(glm::mat4(1.0f), { 25.0f, 25.0f, 1.0f });

		glm::vec4 color = { 0.11f, 0.11f, 0.11f, 1.0f };
		glm::vec2 mousePos = Input::getMousePosition();

		if (mousePos.x >= min.x && mousePos.y >= min.y && mousePos.x <= max.x && mousePos.y <= max.y)
		{
			color = { 0.16f, 0.16f, 0.16f, 1.0f };

			if (Input::isMouseButtonDown(MouseButton::ButtonLeft))
			{
				press = true;
				color = { 0.21f, 0.21f, 0.21f, 1.0f };
			}
		}

		if (press && m_HasBeenPressed)
			press = false;
		else if (!press)
			m_HasBeenPressed = false;
		else if (press && !m_HasBeenPressed)
			m_HasBeenPressed = true;

		canvas.drawRoundedRect({ min, max }, color, 10.0f, RoundedRectFlags::AllCorners, 1.0f);
		canvas.drawText(m_Label, textTransform, {});

		if (press)
		{
			for (auto& trigger : m_Triggers)
				m_Layout.trigger(trigger);
		}
	}

	void Button::addTrigger(std::function<void(Layout&)>&& trigger)
	{
		m_Triggers.emplace_back(trigger);
	}

}

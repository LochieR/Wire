#include "Panel.h"

namespace wire {

	Panel::Panel(Canvas& canvas, const std::string& title, const glm::vec2& position, const glm::vec2& size)
		: m_Canvas(&canvas), m_Layout(canvas, position + m_Padding + glm::vec2{ 0.0f, m_TitlebarHeight }), m_Position(position), m_Size(size)
	{
	}

	void Panel::draw()
	{
		Canvas& canvas = *m_Canvas;

		glm::vec2 min = m_Position;
		glm::vec2 max = m_Position + m_Size;

		glm::vec2 titlebarMin = min;
		glm::vec2 titlebarMax = { max.x, min.y + m_TitlebarHeight };

		canvas.drawRoundedRect({ titlebarMin, titlebarMax }, { 0.15f, 0.15f, 0.15f, 1.0f }, 10.0f, RoundedRectFlags::TopLeft | RoundedRectFlags::TopRight, 1.0f);
		canvas.drawRoundedRect({ min, max }, { 0.22f, 0.22f, 0.22f, 1.0f }, 10.0f, RoundedRectFlags::AllCorners, 1.0f);

		m_Layout.draw(canvas);
	}

}

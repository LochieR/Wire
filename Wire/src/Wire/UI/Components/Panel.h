#pragma once

#include "Component.h"

#include <glm/glm.hpp>

#include <string>

namespace wire {

	class Panel
	{
	public:
		Panel() = default;
		Panel(Canvas& canvas, const std::string& title, const glm::vec2& position, const glm::vec2& size);

		void draw();

		Layout& setupLayout() { return m_Layout; }
	private:
		Canvas* m_Canvas = nullptr;

		glm::vec2 m_Padding = { 10.0f, 10.0f };
		float m_TitlebarHeight = 35.0f;
		Layout m_Layout;

		glm::vec2 m_Position;
		glm::vec2 m_Size = { 200, 200 };
	};

}

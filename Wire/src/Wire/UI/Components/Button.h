#pragma once

#include "Component.h"
#include "Wire/UI/Core/Canvas.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace wire {

	class Button : public Component
	{
	public:
		Button(Layout& layout, const std::string& label, const glm::vec2& position, const glm::vec2& size = { 0.0f, 0.0f });
		virtual ~Button();

		virtual void draw(Canvas& canvas) override;
		virtual void addTrigger(std::function<void(Layout&)>&& trigger) override;

		virtual glm::vec2 getPosition() const override { return m_Position; }
		virtual glm::vec2 getSize() const override { return m_Size; }
	private:
		Layout& m_Layout;

		std::string m_Label;

		glm::vec2 m_Position;
		glm::vec2 m_Size;

		std::vector<std::function<void(Layout&)>> m_Triggers;

		bool m_HasBeenPressed = false;
	};

}

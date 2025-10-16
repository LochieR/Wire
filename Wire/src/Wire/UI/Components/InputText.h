#pragma once

#include "Component.h"
#include "Wire/UI/Core/Canvas.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace wire {

	class InputText : public Component
	{
	public:
		InputText(Layout& layout, std::string& text, const glm::vec2& position, const glm::vec2& size);
		InputText(Layout& layout, std::wstring& text, const glm::vec2& position, const glm::vec2& size);
		virtual ~InputText();

		virtual void draw(Canvas& canvas) override;
		virtual void addTrigger(std::function<void(Layout&)>&& trigger) override;

		virtual glm::vec2 getPosition() const override { return m_Position; }
		virtual glm::vec2 getSize() const override { return m_Size; }
	private:
		Layout& m_Layout;

		std::string* m_Text = nullptr;
		std::wstring* m_WideText = nullptr;

		glm::vec2 m_Position;
		glm::vec2 m_Size;

		std::vector<std::function<void(Layout&)>> m_Triggers;

		bool m_HasBeenPressed = false;
		bool m_Selected = false;
		size_t m_CursorIndex = 0;
		bool m_CursorVisible = true;
	};

}

#include "InputText.h"

#include "Wire/Core/Application.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glfw/glfw3.h>

#include <string>
#include <vector>
#include <cwctype>

namespace wire {

	static double s_LastBlinkTime = 0.0;
	static const double s_BlinkInterval = 0.5;

	InputText::InputText(Layout& layout, std::string& text, const glm::vec2& position, const glm::vec2& size)
		: m_Layout(layout), m_Text(&text), m_WideText(nullptr), m_Position(position), m_Size(size)
	{
	}

	InputText::InputText(Layout& layout, std::wstring& text, const glm::vec2& position, const glm::vec2& size)
		: m_Layout(layout), m_Text(nullptr), m_WideText(&text), m_Position(position), m_Size(size)
	{
	}

	InputText::~InputText()
	{
	}

	void InputText::draw(Canvas& canvas)
	{
		size_t textLength = m_Text ? m_Text->size() : m_WideText ? m_WideText->size() : 0;

		bool press = false;
		bool textChanged = false;

		if (m_Selected)
		{
			if (m_CursorIndex > textLength)
				m_CursorIndex = textLength;

			double currentTime = glfwGetTime();
			double elapsed = currentTime - s_LastBlinkTime;
			double cycleTime = s_BlinkInterval * 2.0;

			bool ctrlDown = Input::isKeyDown(KeyCode::LeftControl) || Input::isKeyDown(KeyCode::RightControl);

			double cyclePos = std::fmod(elapsed, cycleTime);
			m_CursorVisible = (cyclePos < s_BlinkInterval);

			for (wchar_t c : m_Layout.getInputBuffer())
			{
				if (c == CustomUnicodeEvents::Backspace)
				{
					if (ctrlDown)
					{
						if (m_Text && !m_Text->empty() && m_CursorIndex > 0)
						{
							size_t start = m_CursorIndex;
							while (start > 0 && std::isspace((*m_Text)[start - 1]))
								--start;
							while (start > 0 && !std::isspace((*m_Text)[start - 1]))
								--start;
							m_Text->erase(start, m_CursorIndex - start);
							m_CursorIndex = start;
							textChanged = true;
						}
						else if (m_WideText && !m_WideText->empty() && m_CursorIndex > 0)
						{
							size_t start = m_CursorIndex;
							while (start > 0 && std::iswspace((*m_WideText)[start - 1]))
								--start;
							while (start > 0 && !std::iswspace((*m_WideText)[start - 1]))
								--start;
							m_WideText->erase(start, m_CursorIndex - start);
							m_CursorIndex = start;
							textChanged = true;
						}
					}
					else
					{
						if (m_Text && !m_Text->empty() && m_CursorIndex > 0)
						{
							m_Text->erase(m_CursorIndex - 1, 1);
							m_CursorIndex--;
							textChanged = true;
						}
						else if (m_WideText && !m_WideText->empty() && m_CursorIndex > 0)
						{
							m_WideText->erase(m_CursorIndex - 1, 1);
							m_CursorIndex--;
							textChanged = true;
						}
					}
					s_LastBlinkTime = glfwGetTime();
					m_CursorVisible = true;
				}
				else if (c == CustomUnicodeEvents::Delete)
				{
					if (ctrlDown)
					{
						if (m_Text && m_CursorIndex < m_Text->size())
						{
							size_t end = m_CursorIndex;
							while (end < m_Text->size() && std::isspace((*m_Text)[end]))
								++end;
							while (end < m_Text->size() && !std::isspace((*m_Text)[end]))
								++end;
							m_Text->erase(m_CursorIndex, end - m_CursorIndex);
							textChanged = true;
						}
						else if (m_WideText && m_CursorIndex < m_WideText->size())
						{
							size_t end = m_CursorIndex;
							while (end < m_WideText->size() && std::iswspace((*m_WideText)[end]))
								++end;
							while (end < m_WideText->size() && !std::iswspace((*m_WideText)[end]))
								++end;
							m_WideText->erase(m_CursorIndex, end - m_CursorIndex);
							textChanged = true;
						}
					}
					else
					{
						if (m_Text && m_CursorIndex < m_Text->size())
						{
							m_Text->erase(m_CursorIndex, 1);
							textChanged = true;
						}
						else if (m_WideText && m_CursorIndex < m_WideText->size())
						{
							m_WideText->erase(m_CursorIndex, 1);
							textChanged = true;
						}
					}
					s_LastBlinkTime = glfwGetTime();
					m_CursorVisible = true;
				}
				else if (c == CustomUnicodeEvents::LeftArrow)
				{
					if (ctrlDown)
					{
						if (m_Text)
						{
							while (m_CursorIndex > 0 && std::isspace((*m_Text)[m_CursorIndex - 1]))
								--m_CursorIndex;
							while (m_CursorIndex > 0 && !std::isspace((*m_Text)[m_CursorIndex - 1]))
								--m_CursorIndex;
						}
						else if (m_WideText)
						{
							while (m_CursorIndex > 0 && std::iswspace((*m_WideText)[m_CursorIndex - 1]))
								--m_CursorIndex;
							while (m_CursorIndex > 0 && !std::iswspace((*m_WideText)[m_CursorIndex - 1]))
								--m_CursorIndex;
						}
					}
					else
					{
						if (m_CursorIndex > 0)
							m_CursorIndex--;
					}
					s_LastBlinkTime = glfwGetTime();
					m_CursorVisible = true;
				}
				else if (c == CustomUnicodeEvents::RightArrow)
				{
					if (ctrlDown)
					{
						if (m_Text)
						{
							while (m_CursorIndex < m_Text->size() && std::isspace((*m_Text)[m_CursorIndex]))
								++m_CursorIndex;
							while (m_CursorIndex < m_Text->size() && !std::isspace((*m_Text)[m_CursorIndex]))
								++m_CursorIndex;
						}
						else if (m_WideText)
						{
							while (m_CursorIndex < m_WideText->size() && std::iswspace((*m_WideText)[m_CursorIndex]))
								++m_CursorIndex;
							while (m_CursorIndex < m_WideText->size() && !std::iswspace((*m_WideText)[m_CursorIndex]))
								++m_CursorIndex;
						}
					}
					else
					{
						if (m_Text && m_CursorIndex < m_Text->size())
							m_CursorIndex++;
						else if (m_WideText && m_CursorIndex < m_WideText->size())
							m_CursorIndex++;
					}
					s_LastBlinkTime = glfwGetTime();
					m_CursorVisible = true;
				}
				else
				{
					if (m_Text)
					{
						if (c <= std::numeric_limits<char>::max())
						{
							m_Text->insert(m_CursorIndex++, 1, static_cast<char>(c));
							textChanged = true;
						}
					}
					else if (m_WideText)
					{
						m_WideText->insert(m_CursorIndex++, 1, c);
						textChanged = true;
					}
				}
			}
		}

		glm::vec2 padding = { 20.0f, 10.0f };
		glm::vec2 textSize;
		if (m_Text)
			textSize = canvas.calculateTextSize(*m_Text, glm::scale(glm::mat4(1.0f), { 25.0f, 25.0f, 1.0f }), {});
		else if (m_WideText)
			textSize = canvas.calculateTextSize(*m_WideText, glm::scale(glm::mat4(1.0f), { 25.0f, 25.0f, 1.0f }), {});
		else
			return;

		glm::vec2 useSize;
		if (m_Size.x == 0)
			useSize.x = m_Size.x;
		else
			useSize.x = m_Size.x;
		if (m_Size.y == 0)
			useSize.y = textSize.y;
		else
			useSize.y = m_Size.y;

		useSize.y = glm::max(useSize.y, 20.0f);

		glm::vec2 min = m_Position;
		glm::vec2 max = { min.x + padding.x + useSize.x + padding.x, min.y + padding.y + useSize.y + padding.y };
		glm::vec2 textPosition = { min.x + padding.x, min.y + padding.y + useSize.y - 2.0f };

		glm::mat4 textTransform = glm::translate(glm::mat4(1.0f), { textPosition.x, textPosition.y, 0.0f })
			* glm::scale(glm::mat4(1.0f), { 25.0f, 25.0f, 1.0f });

		glm::vec4 color = { 0.11f, 0.11f, 0.11f, 1.0f };
		glm::vec2 mousePos = Input::getMousePosition();

		bool mouseButtonDown = Input::isMouseButtonDown(MouseButton::ButtonLeft);

		if (mousePos.x >= min.x && mousePos.y >= min.y && mousePos.x <= max.x && mousePos.y <= max.y)
		{
			color = { 0.16f, 0.16f, 0.16f, 1.0f };

			if (mouseButtonDown)
			{
				press = true;
				color = { 0.21f, 0.21f, 0.21f, 1.0f };
			}
		}
		else if (mouseButtonDown)
			m_Selected = false;

		if (press && m_HasBeenPressed)
			press = false;
		else if (!press)
			m_HasBeenPressed = false;
		else if (press && !m_HasBeenPressed)
			m_HasBeenPressed = true;

		glm::vec2 beforeCursorSize;
		if (m_WideText)
		{
			std::wstring_view beforeCursor{ m_WideText->begin(), m_WideText->begin() + m_CursorIndex };
			beforeCursorSize = canvas.calculateTextSize(std::wstring(beforeCursor), glm::scale(glm::mat4(1.0f), { 25.0f, 25.0f, 1.0f }), {});
		}
		else
		{
			std::string_view beforeCursor{ m_Text->begin(), m_Text->begin() + m_CursorIndex };
			beforeCursorSize = canvas.calculateTextSize(std::string(beforeCursor), glm::scale(glm::mat4(1.0f), { 25.0f, 25.0f, 1.0f }), {});
		}

		if (m_CursorVisible && m_Selected)
			canvas.drawLine({ textPosition.x + beforeCursorSize.x + 2.0f, textPosition.y + 5.0f, 0.0f }, { textPosition.x + beforeCursorSize.x + 2.0f, textPosition.y - 15.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f });

		canvas.drawRoundedRect({ min, max }, color, 10.0f, RoundedRectFlags::AllCorners, 1.0f);
		if (m_Text)
			canvas.drawText(*m_Text, textTransform, {});
		else if (m_WideText)
			canvas.drawText(*m_WideText, textTransform, {});

		if (press)
			m_Selected = true;

		if (textChanged)
		{
			for (auto& trigger : m_Triggers)
				m_Layout.trigger(trigger);
		}
	}

	void InputText::addTrigger(std::function<void(Layout&)>&& trigger)
	{
		m_Triggers.emplace_back(trigger);
	}

}

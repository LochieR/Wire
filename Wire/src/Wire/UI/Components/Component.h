#pragma once

#include "Wire/UI/Core/Canvas.h"
#include "Wire/Core/Application.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace wire {

	class Layout;

	struct CustomUnicodeEvents
	{
		// private use area in the basic multilingual plane

		const static wchar_t Backspace = 0xE000;
		const static wchar_t Delete = 0xE001;
		const static wchar_t LeftArrow = 0xE002;
		const static wchar_t RightArrow = 0xE003;
	};

	class Component
	{
	public:
		virtual ~Component() = default;

		virtual void draw(Canvas& canvas) = 0;
		virtual void addTrigger(std::function<void(Layout&)>&& trigger) = 0;

		virtual glm::vec2 getPosition() const = 0;
		virtual glm::vec2 getSize() const = 0;
	};

	class Layout
	{
	public:
		Layout() = default;
		Layout(Canvas& canvas, const glm::vec2& layoutPosition);
		Layout(const Layout&) = default;

		void draw(Canvas& canvas);

		Component& addButton(const std::string& label, const glm::vec2& size = { 0.0f, 0.0f });
		Component& addInputText(std::string& text, const glm::vec2& size);
		Component& addInputText(std::wstring& text, const glm::vec2& size);

		void onEvent(Event& event);

		const std::wstring& getInputBuffer() const { return m_InputBuffer; }

		void sameLine();
		void clear();

		void trigger(const std::function<void(Layout&)>& func);

		Canvas& getCanvas() { return *m_Canvas; }
		const Canvas& getCanvas() const { return *m_Canvas; }

		const glm::vec2& getPadding() const { return m_Padding; }
	private:
		bool onKeyTypedEvent(KeyTypedEvent& event);
		bool onKeyPressedEvent(KeyPressedEvent& event);
	private:
		Canvas* m_Canvas = nullptr;

		std::vector<std::shared_ptr<Component>> m_Components;

		glm::vec2 m_LayoutPosition;

		glm::vec2 m_CursorPos = { 0.0f, 0.0f };
		float m_Spacing = 4.0f;

		std::wstring m_InputBuffer = L"";

		glm::vec2 m_Padding = { 20.0f, 10.0f };

		std::vector<std::function<void(Layout&)>> m_CurrentTriggers;
	};

}

#include "UIExampleLayer.h"

namespace bloom {

	static void testTrigger(wire::Layout& layout);

	void UIExampleLayer::onAttach()
	{
		wire::Renderer* renderer = wire::Application::get().getRenderer();

		m_Canvas = wire::Canvas(renderer);
		m_Panel = wire::Panel(m_Canvas, "Main", { 500.0f, 100.0f }, { 300.0f, 500.0f });
		m_Layout = &m_Panel.setupLayout();

		m_Layout->addButton("button 1")
			.addTrigger(testTrigger);
		m_Layout->addButton("button 2");
		m_Layout->sameLine();
		m_Layout->addButton("button 3");
		m_Layout->addButton("button 4");
	}

	void testTrigger(wire::Layout& layout)
	{
		layout.clear();

		static std::wstring text = L"";
		static std::wstring otherText = L"";

		layout.addButton("hi there");
		layout.addInputText(text, { 200.0f, 20.0f });
		layout.addInputText(otherText, { 150.0f, 20.0f });
	}

	void UIExampleLayer::onDetach()
	{
		m_Canvas.release();
	}

	void UIExampleLayer::onUpdate(float timestep)
	{
		const auto& desc = wire::Application::get().getDesc();

		m_Canvas.beginFrame();
		m_Canvas.drawRect({ { 0.0f, (float)desc.WindowHeight - 25.0f }, { (float)desc.WindowWidth, (float)desc.WindowHeight } }, { 0.39f, 0.07f, 0.54f, 1.0f });

		m_Panel.draw();

		m_Canvas.endFrame();
	}

	void UIExampleLayer::onEvent(wire::Event& event)
	{
		m_Layout->onEvent(event);
	}

}

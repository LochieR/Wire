#pragma once

#include "Wire/Core/Layer.h"

#include "Wire/Events/ApplicationEvent.h"
#include "Wire/Events/KeyEvent.h"
#include "Wire/Events/MouseEvent.h"

namespace Wire {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		void Begin();
		void End();

		void BlockEvents(bool block) { m_BlockEvents = block; }

		void SetDarkThemeColours();
	private:
		bool m_BlockEvents = true;
		float m_Time = 0.0f;
	};

}
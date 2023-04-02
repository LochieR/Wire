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

		std::filesystem::path m_ImGuiIniPath;
	};

}

#define ACCENT_COLOUR_0  0   / 255.0f, 3   / 255.0f, 181 / 255.0f
#define ACCENT_COLOUR_1  181 / 255.0f, 0   / 255.0f, 0	 / 255.0f
#define ACCENT_COLOUR_2  181 / 255.0f, 46  / 255.0f, 0	 / 255.0f
#define ACCENT_COLOUR_3  181 / 255.0f, 129 / 255.0f, 0	 / 255.0f
#define ACCENT_COLOUR_4  172 / 255.0f, 181 / 255.0f, 0	 / 255.0f
#define ACCENT_COLOUR_5  95  / 255.0f, 181 / 255.0f, 0	 / 255.0f
#define ACCENT_COLOUR_6  0   / 255.0f, 181 / 255.0f, 55	 / 255.0f
#define ACCENT_COLOUR_7  0   / 255.0f, 181 / 255.0f, 117 / 255.0f
#define ACCENT_COLOUR_8  0   / 255.0f, 181 / 255.0f, 168 / 255.0f
#define ACCENT_COLOUR_9  0   / 255.0f, 112 / 255.0f, 181 / 255.0f
#define ACCENT_COLOUR_10 0   / 255.0f, 86  / 255.0f, 181 / 255.0f
#define ACCENT_COLOUR_11 0   / 255.0f, 19  / 255.0f, 181 / 255.0f
#define ACCENT_COLOUR_12 69  / 255.0f, 0   / 255.0f, 181 / 255.0f
#define ACCENT_COLOUR_13 110 / 255.0f, 0   / 255.0f, 181 / 255.0f
#define ACCENT_COLOUR_14 167 / 255.0f, 0   / 255.0f, 181 / 255.0f
#define ACCENT_COLOUR_15 181 / 255.0f, 0   / 255.0f, 134 / 255.0f
#define ACCENT_COLOUR_16 181 / 255.0f, 0   / 255.0f, 82	 / 255.0f
#define ACCENT_COLOUR_17 181 / 255.0f, 0   / 255.0f, 41	 / 255.0f

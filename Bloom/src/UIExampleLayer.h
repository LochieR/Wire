#pragma once

#include "Wire/Core/Application.h"

#include "Wire/UI/Core/Canvas.h"
#include "Wire/UI/Components/ComponentLibrary.h"

namespace bloom {

	class UIExampleLayer : public wire::Layer
	{
	public:
		UIExampleLayer() = default;
		~UIExampleLayer() = default;

		virtual void onAttach() override;
		virtual void onDetach() override;
		virtual void onUpdate(float timestep) override;
		virtual void onEvent(wire::Event& event) override;
	private:
		wire::Canvas m_Canvas;
		wire::Panel m_Panel;
		wire::Layout* m_Layout = nullptr;
	};

}

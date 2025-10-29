#pragma once

#include "Event.h"

namespace wire {

	class Layer
	{
	public:
		Layer() = default;
		virtual ~Layer() = default;

		virtual void onAttach() {}
		virtual void onDetach() {}
		virtual void onImGuiRender() {}
		virtual void onUpdate(float timestep) {}
		virtual void onEvent(Event& event) {}
	};

}

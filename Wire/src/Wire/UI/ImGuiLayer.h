#pragma once

#include "Wire/Core/Layer.h"
#include "Wire/Renderer/Device.h"

namespace wire {

    class ImGuiLayer : public Layer
    {
	public:
		ImGuiLayer() = default;
		virtual ~ImGuiLayer() = default;

		void begin();
		void end();

		virtual void onAttach() override;
		virtual void onDetach() override;
		virtual void onUpdate(float timestep) override;
		virtual void onEvent(Event& event) override;
	private:
		Device* m_Device = nullptr;
		std::shared_ptr<RenderPass> m_RenderPass = nullptr;
	};

}

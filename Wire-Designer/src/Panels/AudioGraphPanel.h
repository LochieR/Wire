#pragma once

#include "Panel.h"

#include "GraphEditor.h"

namespace Wire {

	class AudioGraphPanel : public Panel
	{
	public:
		AudioGraphPanel();
		~AudioGraphPanel();

		virtual void OnImGuiRender() override;
		virtual void SetContext(const Ref<Scene>& context) override;

		virtual bool* GetOpen() override;
	private:
		bool m_Open;
	};

}

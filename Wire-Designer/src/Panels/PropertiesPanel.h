#pragma once

#include "Panel.h"

#include "Wire/Scene/Entity.h"

namespace Wire {

	class PropertiesPanel : public Panel
	{
	public:
		struct Vec3ControlData
		{
			std::string Label;
			glm::vec3& Values;
			float ResetValue = 0.0f;
		};
	public:
		PropertiesPanel();
		PropertiesPanel(const Ref<Scene>& context);
		~PropertiesPanel();

		virtual void OnImGuiRender() override;
		virtual void SetContext(const Ref<Scene>& context) override;

		virtual bool* GetOpen() override;

		void DrawComponents(Entity entity);
	private:
		template<typename T>
		void DisplayAddComponentEntry(const std::string& displayName);
	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;

		bool m_Open;
	};

}

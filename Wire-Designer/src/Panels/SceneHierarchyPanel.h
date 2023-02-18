#pragma once

#include "Wire/Core/Core.h"
#include "Wire/Core/Log.h"
#include "Wire/Scene/Scene.h"
#include "Wire/Scene/Entity.h"

#include <glm/gtc/type_ptr.hpp>

namespace Wire {

	class SceneHierarchyPanel
	{
	public:
		struct Vec3ControlData
		{
			std::string Label;
			glm::vec3& Values;
			float ResetValue = 0.0f;
		};
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& context);

		void SetContext(const Ref<Scene>& context);

		void OnImGuiRender();

		Entity GetSelectedEntity() const { return m_SelectionContext; }
		void SetSelectedEntity(Entity entity);
	private:
		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);
	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;
	};

}

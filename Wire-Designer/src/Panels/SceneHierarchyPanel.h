#pragma once

#include "Wire/Core/Core.h"
#include "Wire/Core/Log.h"
#include "Wire/Scene/Scene.h"
#include "Wire/Scene/Entity.h"
#include "Wire/Projects/Project.h"

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

		void OnImGuiRender(bool* sceneHierarchyOpen, bool* propertiesPanelOpen);

		Entity GetSelectedEntity() const { return m_SelectionContext; }
		void SetSelectedEntity(Entity entity);

		void OnOpenProject(const Ref<Project>& project) { m_Project = project; }
	private:
		template<typename T>
		void DisplayAddComponentEntry(const std::string& displayName);

		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);
	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;

		Ref<Project> m_Project = CreateRef<Project>(Project::CreateNullProject());
	};

}

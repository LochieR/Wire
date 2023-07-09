#pragma once

#include "Panel.h"

#include "Wire/Core/Core.h"
#include "Wire/Core/Log.h"
#include "Wire/Scene/Scene.h"
#include "Wire/Scene/Entity.h"
#include "Wire/Projects/Project.h"

#include <glm/gtc/type_ptr.hpp>

namespace Wire {

	class SceneHierarchyPanel : public Panel
	{
	public:
		SceneHierarchyPanel();
		SceneHierarchyPanel(const Ref<Scene>& context);

		virtual void OnImGuiRender() override;
		virtual void SetContext(const Ref<Scene>& context) override;

		virtual bool* GetOpen() override;

		Entity GetSelectedEntity() const;
		void SetSelectedEntity(Entity entity);

		void OnOpenProject(const Ref<Project>& project) { m_Project = project; }
	private:
		void DrawEntityNode(Entity entity);
	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;

		Ref<Project> m_Project = Project::CreateNullProject();

		bool m_Open;
	};

}

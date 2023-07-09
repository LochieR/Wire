#include "SceneHierarchyPanel.h"

#include "Wire/Scene/Components.h"
#include "Wire/Scripting/ScriptEngine.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

namespace Wire {

	SceneHierarchyPanel::SceneHierarchyPanel()
	{
		m_Open = true;
		SetContext(nullptr);
	}

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
		m_SelectionContext = {};
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		if (m_Open)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 8));
			ImGui::Begin("Scene Hierarchy", &m_Open);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

			m_Context->m_Registry.each([&](auto entityId)
			{
				Entity entity{ entityId, m_Context.get() };
				DrawEntityNode(entity);
			});

			ImGui::PopStyleVar();

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				m_SelectionContext = {};

			ImGuiPopupFlags popupFlags = ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
			if (ImGui::BeginPopupContextWindow("SceneHierarchyPopupMenu", popupFlags))
			{
				if (ImGui::MenuItem("Create Empty Entity"))
				{
					auto entity = m_Context->CreateEntity("Empty Entity");
					if (!m_SelectionContext)
						SetSelectedEntity(entity);
				}

				ImGui::EndPopup();
			}
			ImGui::PopStyleVar();

			ImGui::End();
			ImGui::PopStyleVar();
		}
	}

	bool* SceneHierarchyPanel::GetOpen()
	{
		return &m_Open;
	}

	Entity SceneHierarchyPanel::GetSelectedEntity() const
	{
		if (m_Context)
			return m_SelectionContext;
		return Entity{};
	}

	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		m_SelectionContext = entity;
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		
		bool isSelectedEntity = entity == m_SelectionContext;

		ImGuiTreeNodeFlags flags = (isSelectedEntity ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_FramePadding;
		if (isSelectedEntity)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f });
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f });
		}
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)entity.GetUUID(), flags, tag.c_str());
		if (isSelectedEntity)
			ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();
		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (opened) // For children
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(entity.GetUUID() + 1 /* TEMP */), flags, tag.c_str());
			ImGui::PopStyleVar();
			if (opened)
				ImGui::TreePop();
			ImGui::TreePop();
		}

		if (entityDeleted)
		{
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
			m_Context->DestroyEntity(entity);
		}
	}

}

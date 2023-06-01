#pragma once

#include "Wire/Core/Timestep.h"
#include "Wire/Core/UUID.h"
#include "Wire/Renderer/EditorCamera.h"

#include "entt.hpp"

namespace Wire {

	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		static Ref<Scene> Copy(Ref<Scene> other);

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		void OnSceneStart();
		void OnSceneStop();

		void OnUpdateEditor(Timestep ts, EditorCamera& camera);
		void OnUpdateRuntime(Timestep ts);
		void OnViewportResize(uint32_t width, uint32_t height);

		Entity DuplicateEntity(Entity entity);

		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);

		Entity GetPrimaryCameraEntity();

		bool IsRunning() const { return m_Running; }
		bool IsPaused() const { return m_Paused; }

		void SetPaused(bool paused) { m_Paused = paused; }

		void Step(int frames = 1);
	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);
	private:
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_Running = false;
		bool m_Paused = false;
		int m_StepFrames = 0;

		std::unordered_map<UUID, entt::entity> m_EntityMap;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

}

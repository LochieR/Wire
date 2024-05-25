#pragma once

#include "Wire/Core/Base.h"
#include "Wire/Core/UUID.h"

#include "Scene.h"
#include "Components.h"

#include <entt.hpp>

namespace Wire {

	class Module
	{
	public:
		Module() = default;
		Module(entt::entity handle, Scene* scene);
		Module(const Module&) = default;

		template<typename T, typename... TArgs>
		T& AddComponent(TArgs&&... args)
		{
			WR_ASSERT(!HasComponent<T>() && "Module already has component!");
			return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<TArgs>(args)...);
		}

		template<typename T, typename... TArgs>
		T& AddOrReplaceComponent(TArgs&&... args)
		{
			return m_Scene->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<TArgs>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			WR_ASSERT(HasComponent<T>() && "Module does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Scene->m_Registry.has<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			WR_ASSERT(HasComponent<T>() && "Entity does not have component!");
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		operator bool() const { return m_EntityHandle != entt::null; }
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return static_cast<uint32_t>(m_EntityHandle); }

		UUID GetUUID() { return GetComponent<IDComponent>().ID; }
		std::string GetName() { return GetComponent<IDComponent>().Name; }

		bool operator==(const Module& other) const
		{
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		}

		bool operator!=(const Module& other) const
		{
			return !(*this == other);
		}
	private:
		entt::entity m_EntityHandle{ entt::null };
		Scene* m_Scene;
	};

}

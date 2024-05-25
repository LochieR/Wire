#pragma once

#include "SceneCamera.h"

#include "Wire/Scripting/ScriptEngine.h"
#include "Wire/Core/UUID.h"

#include <entt.hpp>

#include <string>

namespace Coral {

	class Type;

}

namespace Wire {

	class Module;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Module CreateModule(const std::string& name = std::string());
		Module CreateModuleWithUUID(UUID id, const std::string& name = std::string());

		Module CreateModuleFromManagedClass(Coral::Type* type);

		void DestroyModule(Module module);

		std::unordered_map<UUID, entt::entity>::iterator begin() { return m_ModuleMap.begin(); }
		std::unordered_map<UUID, entt::entity>::iterator end() { return m_ModuleMap.end(); }
		std::unordered_map<UUID, entt::entity>::const_iterator begin() const { return m_ModuleMap.cbegin(); }
		std::unordered_map<UUID, entt::entity>::const_iterator end() const { return m_ModuleMap.cend(); }
	private:
		entt::registry m_Registry;

		std::unordered_map<UUID, entt::entity> m_ModuleMap;

		SceneCamera m_SceneCamera;
		glm::mat4 m_SceneCameraTransform{ 1.0f };

		friend class Module;
		friend class SceneRenderer;
	};

}

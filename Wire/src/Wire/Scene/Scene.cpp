#include "wrpch.h"
#include "Scene.h"

#include "Module.h"
#include "Components.h"

#include <Coral/Type.hpp>

namespace Wire {

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
	}

	Module Scene::CreateModule(const std::string& name)
	{
		return CreateModuleWithUUID(UUID{}, name);
	}

	Module Scene::CreateModuleWithUUID(UUID id, const std::string& name)
	{
		Module module = { m_Registry.create(), this };
		module.AddComponent<IDComponent>(id, name.empty() ? "Module" : name);
		module.AddComponent<TransformComponent>();

		m_ModuleMap[id] = module;
		return module;
	}

	Module Scene::CreateModuleFromManagedClass(Coral::Type* type)
	{
		Module module = CreateModule(type->GetFullName());

		return module;
	}

	void Scene::DestroyModule(Module module)
	{
		m_ModuleMap.erase(module.GetUUID());
		m_Registry.destroy(module);
	}

}

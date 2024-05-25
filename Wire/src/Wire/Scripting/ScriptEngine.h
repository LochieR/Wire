#pragma once

#include "Wire/Core/UUID.h"

#include <filesystem>
#include <unordered_map>

namespace Coral {

	class Type;
	class MethodInfo;
	class ManagedObject;

}

namespace Wire {

	class Module;
	class Scene;

	class ScriptEngine
	{
	public:
		using UpdateMethod = void(*)();
	public:
		static void Init();
		static void Shutdown();

		static void LoadModules();
		static void InstantiateModules(Scene* scene);
		static const std::unordered_map<UUID, Coral::ManagedObject>& GetModuleInstances();
		static UpdateMethod GetUpdateMethod(UUID uuid);

		static Coral::Type* GetType(const std::string& typeName);
	private:
		static void InitCoral();
		static void ShutdownCoral();
	};

}

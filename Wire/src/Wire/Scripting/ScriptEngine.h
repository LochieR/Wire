#pragma once

#include "Wire/Scene/Scene.h"
#include "Wire/Scene/Entity.h"
#include "Wire/Projects/Project.h"

#include <string>

extern "C" {
	typedef struct _MonoClass MonoClass;
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoMethod MonoMethod;
	typedef struct _MonoAssembly MonoAssembly;
	typedef struct _MonoImage MonoImage;
	typedef struct _MonoDomain MonoDomain;
}

namespace Wire {

	class ScriptClass
	{
	public:
		ScriptClass() = default;

		ScriptClass(const std::string& namespaceName, const std::string& className, bool isCore = false);
		ScriptClass(MonoClass* monoClass);

		MonoObject* Instantiate();
		MonoMethod* GetMethod(const std::string& methodName, int parameterCount);
		MonoObject* InvokeMethod(MonoObject* instance, MonoMethod* method, void** params = nullptr);
	private:
		std::string m_Namespace;
		std::string m_Class;
		MonoClass* m_MonoClass = nullptr;
	};

	class ScriptInstance
	{
	public:
		ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity);

		void InvokeOnCreate();
		void InvokeOnUpdate(float ts);
	private:
		Ref<ScriptClass> m_ScriptClass;

		MonoObject* m_Instance = nullptr;
		MonoMethod* m_Constructor = nullptr;
		MonoMethod* m_OnCreateMethod = nullptr;
		MonoMethod* m_OnUpdateMethod = nullptr;
	};

	class ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static void LoadAssembly(const std::filesystem::path& filepath);
		static void LoadAppAssembly(const std::filesystem::path& filepath);

		static void OnSceneStart(Scene* scene);
		static void OnSceneStop();

		static bool EntityClassExists(const std::string& fullClassName);
		static void OnCreateEntity(Entity entity);
		static void OnUpdateEntity(Entity entity, float ts);

		static Scene* GetSceneContext();
		static std::unordered_map<std::string, Ref<ScriptClass>> GetEntityClasses();

		static MonoImage* GetCoreAssemblyImage();
		static MonoDomain* GetAppDomain();

		static void OnOpenProject(const Ref<Project>& project);
	private:
		static void InitMono();
		static void ShutdownMono();

		static MonoObject* InstantiateClass(MonoClass* monoClass);
		static void LoadAssemblyClasses();

		static Ref<Project> m_Project;

		friend class ScriptClass;
		friend class ScriptGlue;
	};

}

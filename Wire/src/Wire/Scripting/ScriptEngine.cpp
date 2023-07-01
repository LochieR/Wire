#include "wrpch.h"
#include "ScriptEngine.h"
#include "ScriptGlue.h"

#include "Wire/Core/Application.h"
#include "Wire/Utils/PlatformUtils.h"
#include "Wire/Scene/Entity.h"
#include "Wire/Core/Buffer.h"
#include "Wire/Core/FileSystem.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/metadata/tabledefs.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/threads.h>

#include "FileWatch.h"

namespace Wire {
	
	static std::unordered_map<std::string, ScriptFieldType> s_ScriptFieldTypeMap =
	{
		{ "System.Single",	ScriptFieldType::Float   },
		{ "System.Double",	ScriptFieldType::Double  },
		{ "System.Boolean", ScriptFieldType::Bool    },
		{ "System.Char",	ScriptFieldType::Char    },
		{ "System.Int16",	ScriptFieldType::Short   },
		{ "System.Int32",	ScriptFieldType::Int     },
		{ "System.Int64",	ScriptFieldType::Long    },
		{ "System.Byte",	ScriptFieldType::Byte    },
		{ "System.UInt16",	ScriptFieldType::UShort  },
		{ "System.UInt32",	ScriptFieldType::UInt    },
		{ "System.UInt64",	ScriptFieldType::ULong   },

		{ "Wire.Vector2",	ScriptFieldType::Vector2 },
		{ "Wire.Vector3",	ScriptFieldType::Vector3 },
		{ "Wire.Vector4",	ScriptFieldType::Vector4 },

		{ "Wire.Entity",	ScriptFieldType::Entity  },
	};
	
	namespace Utils {

		static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& assemblyPath, bool loadPDB = false)
		{
			ScopedBuffer fileData = FileSystem::ReadFileBinary(assemblyPath);

			// NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
			MonoImageOpenStatus status;
			MonoImage* image = mono_image_open_from_data_full(fileData.As<char>(), fileData.Size(), 1, &status, 0);

			if (status != MONO_IMAGE_OK)
			{
				const char* errorMessage = mono_image_strerror(status);
				// Log some error message using the errorMessage data
				Application::Get().GetApplicationLogFunction()(2, errorMessage);
				return nullptr;
			}

			if (loadPDB)
			{
				std::filesystem::path pdbPath = assemblyPath;
				pdbPath.replace_extension(".pdb");

				if (std::filesystem::exists(pdbPath))
				{
					ScopedBuffer pdbFileData = FileSystem::ReadFileBinary(pdbPath);
					mono_debug_open_image_from_memory(image, pdbFileData.As<const mono_byte>(), pdbFileData.Size());
					std::string pdbPathString = pdbPath.string();
					std::replace(pdbPathString.begin(), pdbPathString.end(), '\\', '/');
					WR_CORE_INFO("Loaded PDB {}", pdbPathString);
				}
			}

			MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyPath.string().c_str(), &status, 0);
			mono_image_close(image);

			return assembly;
		}

		ScriptFieldType MonoTypeToScriptFieldType(MonoType* monoType)
		{
			std::string typeName = mono_type_get_name(monoType);

			auto it = s_ScriptFieldTypeMap.find(typeName);
			if (it == s_ScriptFieldTypeMap.end())
			{
				WR_CORE_ERROR("Unknown type: {}", typeName);
				return ScriptFieldType::None;
			}

			return it->second;
		}

	}

	struct ScriptEngineData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* AppDomain = nullptr;

		MonoAssembly* CoreAssembly = nullptr;
		MonoImage* CoreAssemblyImage = nullptr;

		MonoAssembly* AppAssembly = nullptr;
		MonoImage* AppAssemblyImage = nullptr;
		
		std::filesystem::path CoreAssemblyFilePath;
		std::filesystem::path AppAssemblyFilePath;

		ScriptClass EntityClass;

		std::unordered_map<std::string, Ref<ScriptClass>> EntityClasses;
		std::unordered_map<UUID, Ref<ScriptInstance>> EntityInstances;
		std::unordered_map<UUID, ScriptFieldMap> EntityScriptFields;

		Scope<filewatch::FileWatch<std::string>> AppAssemblyFileWatcher;
		bool AssemblyReloadPending = false;

		bool EnableDebugging = true;

		Scene* SceneContext = nullptr;
	};

	static ScriptEngineData* s_Data;

	Ref<Project> ScriptEngine::m_Project;

	void ScriptEngine::Init()
	{
		s_Data = new ScriptEngineData();

		InitMono();
		ScriptGlue::RegisterFunctions();

		bool status = LoadAssembly("Resources/Scripts/Wire-ScriptCore.dll");
		if (!status)
		{
			WR_CORE_ERROR("[ScriptEngine] Could not load Wire-ScriptCore assembly!");
			// Must submit to main thread to make sure that this runs after this is set
			Application::Get().SubmitToMainThread([]() { Application::Get().GetApplicationLogFunction()(2, "[ScriptEngine] Could not load Wire-ScriptCore assembly!"); });
			return;
		}
		if (m_Project != nullptr)
		{
			bool status = LoadAppAssembly(m_Project->GetDir() / "Binaries" / "Wire-ScriptRuntime.dll");
			if (!status)
			{
				WR_CORE_ERROR("[ScriptEngine] Could not load app assembly!");
				// Must submit to main thread to make sure that this runs after this is set
				Application::Get().SubmitToMainThread([]() { Application::Get().GetApplicationLogFunction()(2, "[ScriptEngine] Could not load app assembly!"); });
				return;
			}
			LoadAssemblyClasses();
		}
		
		ScriptGlue::RegisterComponents();

		s_Data->EntityClass = ScriptClass("Wire", "Entity", true);
	}

	void ScriptEngine::Shutdown()
	{
		ShutdownMono();

		delete s_Data;
	}

	void ScriptEngine::InitMono()
	{
		mono_set_assemblies_path("mono/lib");

		if (s_Data->EnableDebugging)
		{
			const char* argv[2] = {
				"--debugger-agent=transport=dt_socket,address=127.0.0.1:2550,server=y,suspend=n,loglevel=3,logfile=MonoDebugger.log",
				"--soft-breakpoints"
			};

			mono_jit_parse_options(2, (char**)argv);
			mono_debug_init(MONO_DEBUG_FORMAT_MONO);
		}
		MonoDomain* rootDomain = mono_jit_init("WireJITRuntime");
		WR_CORE_ASSERT(rootDomain);

		// Store the root domain pointer
		s_Data->RootDomain = rootDomain;

		if (s_Data->EnableDebugging)
			mono_debug_domain_create(s_Data->RootDomain);

		mono_thread_set_main(mono_thread_current());
	}

	void ScriptEngine::ShutdownMono()
	{
		mono_domain_set(mono_get_root_domain(), false);

		mono_domain_unload(s_Data->AppDomain);
		s_Data->AppDomain = nullptr;

		mono_jit_cleanup(s_Data->RootDomain);
		s_Data->RootDomain = nullptr;
	}

	bool ScriptEngine::LoadAssembly(const std::filesystem::path& filepath)
	{
		// Create an App Domain
		s_Data->AppDomain = mono_domain_create_appdomain("WireScriptRuntime", nullptr);
		mono_domain_set(s_Data->AppDomain, true);

		s_Data->CoreAssemblyFilePath = filepath;
		s_Data->CoreAssembly = Utils::LoadMonoAssembly(filepath, s_Data->EnableDebugging);
		if (s_Data->CoreAssembly == nullptr)
			return false;

		s_Data->CoreAssemblyImage = mono_assembly_get_image(s_Data->CoreAssembly);

		return true;
	}

	static void OnAppAssemblyFileSystemEvent(const std::string& path, const filewatch::Event event)
	{
		if (!s_Data->AssemblyReloadPending && event == filewatch::Event::modified)
		{
			s_Data->AssemblyReloadPending = true;

			Application::Get().SubmitToMainThread([]() 
			{
				Mouse::SetMouseIcon(MouseIcon::Loading);
				s_Data->AppAssemblyFileWatcher.reset();
				ScriptEngine::ReloadAssembly(); 
				Mouse::SetMouseIcon(MouseIcon::Arrow);
			});
		}
	}

	bool ScriptEngine::LoadAppAssembly(const std::filesystem::path& filepath)
	{
		s_Data->AppAssemblyFilePath = filepath;
		s_Data->AppAssembly = Utils::LoadMonoAssembly(filepath, s_Data->EnableDebugging);
		if (s_Data->AppAssembly == nullptr)
			return false;
		
		s_Data->AppAssemblyImage = mono_assembly_get_image(s_Data->AppAssembly);

		s_Data->AppAssemblyFileWatcher = CreateScope<filewatch::FileWatch<std::string>>(filepath.string(), OnAppAssemblyFileSystemEvent);
		s_Data->AssemblyReloadPending = false;

		return true;
	}

	void ScriptEngine::ReloadAssembly()
	{
		mono_domain_set(mono_get_root_domain(), false);
		
		mono_domain_unload(s_Data->AppDomain);

		bool status = LoadAssembly(s_Data->CoreAssemblyFilePath);
		if (!status)
		{
			WR_CORE_ERROR("[ScriptEngine] Could not load Wire-ScriptCore assembly!");
			// Must submit to main thread to make sure that this runs after this is set
			Application::Get().SubmitToMainThread([]() { Application::Get().GetApplicationLogFunction()(2, "[ScriptEngine] Could not load Wire-ScriptCore assembly!"); });
			return;
		}
		status = LoadAppAssembly(s_Data->AppAssemblyFilePath);
		if (!status)
		{
			WR_CORE_ERROR("[ScriptEngine] Could not load app assembly!");
			// Must submit to main thread to make sure that this runs after this is set
			Application::Get().SubmitToMainThread([]() { Application::Get().GetApplicationLogFunction()(2, "[ScriptEngine] Could not load app assembly!"); });
			return;
		}
		LoadAssemblyClasses();

		ScriptGlue::RegisterComponents();

		s_Data->EntityClass = ScriptClass("Wire", "Entity", true);
	}

	void ScriptEngine::OnSceneStart(Scene* scene)
	{
		s_Data->SceneContext = scene;
	}

	void ScriptEngine::OnSceneStop()
	{
		s_Data->SceneContext = nullptr;

		s_Data->EntityInstances.clear();
	}

	bool ScriptEngine::EntityClassExists(const std::string& fullClassName)
	{
		return s_Data->EntityClasses.find(fullClassName) != s_Data->EntityClasses.end();
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		const auto& sc = entity.GetComponent<ScriptComponent>();
		if (ScriptEngine::EntityClassExists(sc.ClassName))
		{
			UUID entityID = entity.GetUUID();

			Ref<ScriptInstance> instance = CreateRef<ScriptInstance>(s_Data->EntityClasses[sc.ClassName], entity);
			s_Data->EntityInstances[entityID] = instance;

			// Copy field values
			if (s_Data->EntityScriptFields.find(entityID) != s_Data->EntityScriptFields.end())
			{
				const ScriptFieldMap& fieldMap = s_Data->EntityScriptFields.at(entityID);
				for (const auto& [name, fieldInstance] : fieldMap)
					instance->SetFieldValueInternal(name, fieldInstance.m_Buffer);
			}

			instance->InvokeOnCreate();
		}
	}

	void ScriptEngine::OnUpdateEntity(Entity entity, Timestep ts)
	{
		UUID entityUUID = entity.GetUUID();
		if (s_Data->EntityInstances.find(entityUUID) != s_Data->EntityInstances.end())
		{
			Ref<ScriptInstance> instance = s_Data->EntityInstances[entityUUID];
			instance->InvokeOnUpdate(ts);
		}
		else
		{
			WR_CORE_ERROR("Could not find ScriptInstance for entity {}", entityUUID);
			Application::Get().SubmitToMainThread([entityUUID]() { Application::Get().GetApplicationLogFunction()(2, fmt::format("Could not find ScriptInstance for entity {}", entityUUID)); });
		}
	}

	Scene* ScriptEngine::GetSceneContext()
	{
		return s_Data->SceneContext;
	}

	Ref<ScriptInstance> ScriptEngine::GetEntityScriptInstance(UUID entityID)
	{
		auto it = s_Data->EntityInstances.find(entityID);
		if (it == s_Data->EntityInstances.end())
			return nullptr;

		return it->second;
	}

	Ref<ScriptClass> ScriptEngine::GetEntityClass(const std::string& name)
	{
		if (s_Data->EntityClasses.find(name) == s_Data->EntityClasses.end())
			return nullptr;

		return s_Data->EntityClasses.at(name);
	}

	std::unordered_map<std::string, Ref<ScriptClass>> ScriptEngine::GetEntityClasses()
	{
		return s_Data->EntityClasses;
	}

	ScriptFieldMap& ScriptEngine::GetScriptFieldMap(Entity entity)
	{
		WR_CORE_ASSERT(entity);

		UUID entityID = entity.GetUUID();
		return s_Data->EntityScriptFields[entityID];
	}

	void ScriptEngine::AddToScriptFieldMap(Entity entity, const std::string& name, const ScriptFieldInstance& fieldInstance)
	{
		WR_CORE_ASSERT(entity);

		UUID entityID = entity.GetUUID();
		s_Data->EntityScriptFields[entityID][name] = fieldInstance;
	}

	void ScriptEngine::LoadAssemblyClasses()
	{
		s_Data->EntityClasses.clear();

		const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(s_Data->AppAssemblyImage, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

		MonoClass* entityClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Wire", "Entity");

		for (int i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* nameSpace = mono_metadata_string_heap(s_Data->AppAssemblyImage, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* className = mono_metadata_string_heap(s_Data->AppAssemblyImage, cols[MONO_TYPEDEF_NAME]);

			MonoClass* monoClass = mono_class_from_name(s_Data->AppAssemblyImage, nameSpace, className);

			if (!monoClass)
				continue;

			if (monoClass == entityClass)
				continue;
			
			std::string fullName;
			if (strlen(nameSpace) != 0)
				fullName = fmt::format("{}.{}", nameSpace, className);
			else
				fullName = className;

			bool isEntity = mono_class_is_subclass_of(monoClass, entityClass, false);
			if (!isEntity)
				continue;

			Ref<ScriptClass> scriptClass = CreateRef<ScriptClass>(nameSpace, className);
			s_Data->EntityClasses[fullName] = scriptClass;

			int fieldCount = mono_class_num_fields(monoClass);

			WR_CORE_WARN("{0} has {1} fields:", className, fieldCount);
			void* iterator = nullptr;
			while (MonoClassField* field = mono_class_get_fields(monoClass, &iterator))
			{
				const char* fieldName = mono_field_get_name(field);
				uint32_t flags = mono_field_get_flags(field);
				if (flags & FIELD_ATTRIBUTE_PUBLIC)
				{
					MonoType* type = mono_field_get_type(field);
					ScriptFieldType fieldType = Utils::MonoTypeToScriptFieldType(type);
					WR_CORE_WARN("  {0} ({1})", fieldName, Utils::ScriptFieldTypeToString(fieldType));

					scriptClass->m_Fields[fieldName] = { fieldType, fieldName, field };
				}
			}
		}
	}

	MonoImage* ScriptEngine::GetCoreAssemblyImage()
	{
		return s_Data->CoreAssemblyImage;
	}

	MonoDomain* ScriptEngine::GetAppDomain()
	{
		return s_Data->AppDomain;
	}

	MonoObject* ScriptEngine::GetManagedInstance(UUID uuid)
	{
		WR_CORE_ASSERT(s_Data->EntityInstances.find(uuid) != s_Data->EntityInstances.end());
		return s_Data->EntityInstances.at(uuid)->GetManagedObject();
	}

	MonoString* ScriptEngine::CreateString(const char* string)
	{
		return mono_string_new(s_Data->AppDomain, string);
	}

	MonoObject* ScriptEngine::InstantiateClass(MonoClass* monoClass)
	{
		MonoObject* instance = mono_object_new(s_Data->AppDomain, monoClass);
		mono_runtime_object_init(instance);
		return instance;
	}

	void ScriptEngine::OnOpenProject(const Ref<Project>& project)
	{
		m_Project = project;
		bool status = LoadAppAssembly(m_Project->GetDir() / "Binaries" / "Wire-ScriptRuntime.dll");
		if (!status)
		{
			WR_CORE_ERROR("[ScriptEngine] Could not load app assembly!");
			// Must submit to main thread to make sure that this runs after this is set
			Application::Get().SubmitToMainThread([]() { Application::Get().GetApplicationLogFunction()(2, "[ScriptEngine] Could not load app assembly!"); });
			return;
		}
		LoadAssemblyClasses();
	}

	ScriptClass::ScriptClass(const std::string& namespaceName, const std::string& className, bool isCore)
		: m_Namespace(namespaceName), m_ClassName(className)
	{
		m_MonoClass = mono_class_from_name(isCore ? s_Data->CoreAssemblyImage : s_Data->AppAssemblyImage, m_Namespace.c_str(), m_ClassName.c_str());
	}

	ScriptClass::ScriptClass(MonoClass* monoClass)
		: m_MonoClass(monoClass), m_Namespace(mono_class_get_namespace(monoClass), mono_class_get_name(monoClass))
	{
	}
	
	MonoObject* ScriptClass::Instantiate()
	{
		return ScriptEngine::InstantiateClass(m_MonoClass);
	}

	MonoMethod* ScriptClass::GetMethod(const std::string& methodName, int parameterCount)
	{
		return mono_class_get_method_from_name(m_MonoClass, methodName.c_str(), parameterCount);
	}

	MonoObject* ScriptClass::InvokeMethodInternal(MonoObject* instance, MonoMethod* method, void** params)
	{
		MonoObject* exception = nullptr;
		return mono_runtime_invoke(method, instance, params, &exception);
	}

	ScriptInstance::ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity)
		: m_ScriptClass(scriptClass)
	{
		m_Instance = m_ScriptClass->Instantiate();

		m_Constructor = s_Data->EntityClass.GetMethod(".ctor", 1);
		m_OnCreateMethod = m_ScriptClass->GetMethod("OnCreate", 0);
		m_OnUpdateMethod = m_ScriptClass->GetMethod("OnUpdate", 1);

		m_ScriptClass->InvokeMethod(m_Instance, m_Constructor, entity.GetUUID());
	}

	void ScriptInstance::InvokeOnCreate()
	{
		if (m_OnCreateMethod)
			m_ScriptClass->InvokeMethod(m_Instance, m_OnCreateMethod);
	}

	void ScriptInstance::InvokeOnUpdate(float ts)
	{
		if (m_OnUpdateMethod)
		{
			m_ScriptClass->InvokeMethod(m_Instance, m_OnUpdateMethod, ts);
		}
	}

	bool ScriptInstance::GetFieldValueInternal(const std::string& name, void* buffer)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto it = fields.find(name);
		if (it == fields.end())
			return false;

		const ScriptField& field = it->second;
		mono_field_get_value(m_Instance, field.ClassField, buffer);
		return true;
	}

	bool ScriptInstance::SetFieldValueInternal(const std::string& name, const void* value)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto it = fields.find(name);
		if (it == fields.end())
			return false;

		const ScriptField& field = it->second;
		mono_field_set_value(m_Instance, field.ClassField, (void*)value);
		return true;
	}

}

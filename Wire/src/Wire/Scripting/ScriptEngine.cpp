#include "wrpch.h"
#include "ScriptEngine.h"

#include "ScriptGlue.h"

#include "Wire/Core/Base.h"
#include "Wire/Core/FileSystem.h"
#include "Wire/Scene/Scene.h"
#include "Wire/Scene/Module.h"

#include <Coral/HostInstance.hpp>
#include <Coral/Attribute.hpp>
#include <Coral/Array.hpp>

namespace Wire {

	namespace Callbacks {

		static void ExceptionCallback(std::string_view message)
		{
			WR_TAG("ScriptEngine:Coral:Exception");

			WR_ERROR("Unhandled native exception: ", message);
			WR_ASSERT(false);
		}

		static void MessageCallback(std::string_view message, Coral::MessageLevel level)
		{
			WR_TAG("ScriptEngine:Coral");

			switch (level)
			{
				case Coral::MessageLevel::Info:
					WR_INFO(message);
					break;
				case Coral::MessageLevel::Warning:
					WR_WARNING(message);
					break;
				case Coral::MessageLevel::Error:
					WR_ERROR(message); WR_ASSERT(false && "A Coral error occurred!");
					break;
			}
		}

	}

	struct ScriptEngineData
	{
		Coral::HostInstance HostInstance;
		Coral::AssemblyLoadContext LoadContext;

		Coral::ManagedAssembly* Assembly;

		std::string AssemblyPath;

		Coral::Type* ModuleType;
		std::unordered_map<std::string, Coral::Type*> ModuleTypes;
		std::unordered_map<UUID, Coral::ManagedObject> Instances;
		std::unordered_map<UUID, void(*)()> UpdateMethods;
	};

	static ScriptEngineData s_Data;

	void ScriptEngine::Init()
	{
		s_Data = ScriptEngineData();

		InitCoral();
		LoadModules();
	}

	void ScriptEngine::Shutdown()
	{
	}

	void ScriptEngine::LoadModules()
	{
		s_Data.ModuleTypes.clear();

		s_Data.ModuleType = &s_Data.Assembly->GetType("Wire.Modules.Module");
		const auto& types = s_Data.Assembly->GetTypes();

		for (auto type : types)
		{
			if (*type == *s_Data.ModuleType)
				continue;

			bool isModule = type->IsSubclassOf(*s_Data.ModuleType);
			if (!isModule)
				continue;

			s_Data.ModuleTypes[type->GetFullName()] = type;
		}
	}

	void ScriptEngine::InstantiateModules(Scene* scene)
	{
		s_Data.Instances.clear();

		Coral::Type& controlAttribute = s_Data.Assembly->GetType("Wire.ControlAttribute");

		for (const auto& [uuid, moduleID] : *scene)
		{
			Module module{ moduleID, scene };

			if (s_Data.ModuleTypes.find(module.GetName()) == s_Data.ModuleTypes.end())
				continue;

			Coral::Type* type = s_Data.ModuleTypes.at(module.GetName()); // temp

			Coral::ManagedObject object = type->CreateInstance(static_cast<uint64_t>(uuid));
			s_Data.Instances[uuid] = object;

			s_Data.UpdateMethods[uuid] = s_Data.ModuleType->InvokeStaticMethod<void(*)()>("GetUpdateMethod", object);

			std::vector<Coral::FieldInfo> fields = type->GetFields();
			auto& component = module.AddOrReplaceComponent<NumberControlsComponent>();
			for (auto& fieldInfo : fields)
			{
				bool hasAttribute = false;

				Coral::String name = fieldInfo.GetName();
				std::string nameStr = std::string(name);

				std::vector<Coral::Attribute> fieldAttributes = fieldInfo.GetAttributes();
				for (auto& attrib : fieldAttributes)
				{
					if (attrib.GetType() == controlAttribute)
					{
						hasAttribute = true;
					}
				}

				if (hasAttribute)
				{
					component.Controls.push_back(new Controls::SliderControl<float>([object = &s_Data.Instances[uuid], nameStr]()
					{
						float out;
						object->GetFieldValueRaw(nameStr, &out);

						return out;
					}, [object = &s_Data.Instances[uuid], nameStr](float value)
					{
						object->SetFieldValueRaw(nameStr, &value);
					}));
				}
			}
		}
	}

	const std::unordered_map<UUID, Coral::ManagedObject>& ScriptEngine::GetModuleInstances()
	{
		return s_Data.Instances;
	}

	ScriptEngine::UpdateMethod ScriptEngine::GetUpdateMethod(UUID uuid)
	{
		WR_ASSERT(s_Data.UpdateMethods.find(uuid) != s_Data.UpdateMethods.end());

		return s_Data.UpdateMethods.at(uuid);
	}

	Coral::Type* ScriptEngine::GetType(const std::string& typeName)
	{
		return &s_Data.Assembly->GetType(typeName);
	}

	void ScriptEngine::InitCoral()
	{
		std::filesystem::path coralDir = std::filesystem::current_path() / "Resources" / "Scripts";
		std::string coralDirStr = coralDir.string();
		Coral::HostSettings settings
		{
			.CoralDirectory = coralDirStr,
			.MessageCallback = Callbacks::MessageCallback,
			.ExceptionCallback = Callbacks::ExceptionCallback
		};
		s_Data.HostInstance.Initialize(settings);

		s_Data.LoadContext = s_Data.HostInstance.CreateAssemblyLoadContext("WireContext");

		s_Data.AssemblyPath = (coralDir / "Wire-ScriptCore.dll").string();
		auto& assembly = s_Data.LoadContext.LoadAssembly(s_Data.AssemblyPath);
		s_Data.Assembly = &assembly;

		ScriptGlue::RegisterFunctions(s_Data.Assembly);
	}

	void ScriptEngine::ShutdownCoral()
	{
		s_Data.HostInstance.Shutdown();
	}

}

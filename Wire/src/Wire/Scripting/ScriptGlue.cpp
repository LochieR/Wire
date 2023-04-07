#include "wrpch.h"
#include "ScriptGlue.h"

#include "ScriptEngine.h"
#include "Wire/Core/UUID.h"
#include "Wire/Scene/Scene.h"

#include <mono/metadata/object.h>
#include <mono/metadata/reflection.h>

namespace Wire {

	static std::unordered_map<MonoType*, std::function<bool(Entity)>> s_EntityHasComponentFuncs;

#define WR_ADD_INTERNAL_CALL(name) mono_add_internal_call("Wire.InternalCalls::" #name, name)

	#pragma region Debug
	static void Debug_Log(int logLevel, MonoString* message)
	{
		std::string msg = mono_string_to_utf8(message);

		ScriptGlue::GetUILogFunc()(logLevel, msg);
	}
	#pragma endregion

	#pragma region Entity
	static bool Entity_HasComponent(UUID entityID, MonoReflectionType* componentType)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		WR_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		WR_CORE_ASSERT(entity);

		MonoType* managedType = mono_reflection_type_get_type(componentType);
		WR_CORE_ASSERT(s_EntityHasComponentFuncs.find(managedType) != s_EntityHasComponentFuncs.end());
		return s_EntityHasComponentFuncs.at(managedType)(entity);
	}
	#pragma endregion

	#pragma region TransformComponent
	static void TransformComponent_GetTranslation(UUID entityID, glm::vec3* outTranslation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		*outTranslation = entity.GetComponent<TransformComponent>().Translation;
	}

	static void TransformComponent_SetTranslation(UUID entityID, glm::vec3* translation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<TransformComponent>().Translation = *translation;
	}

	static void TransformComponent_GetRotation(UUID entityID, glm::vec3* outRotation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		*outRotation = entity.GetComponent<TransformComponent>().Rotation;
	}

	static void TransformComponent_SetRotation(UUID entityID, glm::vec3* rotation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<TransformComponent>().Rotation = *rotation;
	}

	static void TransformComponent_GetScale(UUID entityID, glm::vec3* outScale)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		*outScale = entity.GetComponent<TransformComponent>().Scale;
	}

	static void TransformComponent_SetScale(UUID entityID, glm::vec3* scale)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<TransformComponent>().Scale = *scale;
	}
	#pragma endregion

	#pragma region SpriteRendererComponent
	static void SpriteRendererComponent_GetColour(UUID entityID, glm::vec4* outColour)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		*outColour = entity.GetComponent<SpriteRendererComponent>().Colour;
	}

	static void SpriteRendererComponent_SetColour(UUID entityID, glm::vec4* colour)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<SpriteRendererComponent>().Colour = *colour;
	}

	static MonoString* SpriteRendererComponent_GetTexturePath(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		std::string path = entity.GetComponent<SpriteRendererComponent>().Texture->GetTexturePath();
		MonoString* monoStr = mono_string_new(ScriptEngine::GetAppDomain(), path.c_str());
		return monoStr;
	}

	static void SpriteRendererComponent_SetTexturePath(UUID entityID, MonoString* monoStr)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		std::string path = mono_string_to_utf8(monoStr);
		entity.GetComponent<SpriteRendererComponent>().Texture = Texture2D::Create(path);
	}

	static float SpriteRendererComponent_GetTilingFactor(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		float tilingFactor = entity.GetComponent<SpriteRendererComponent>().TilingFactor;
		return tilingFactor;
	}

	static void SpriteRendererComponent_SetTilingFactor(UUID entityID, float tilingFactor)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<SpriteRendererComponent>().TilingFactor = tilingFactor;
	}
	#pragma endregion

	#pragma region Input
	static bool Input_IsKeyDown(KeyCode keycode)
	{
		return Input::IsKeyPressed(keycode);
	}
	#pragma endregion

	template<typename... Component>
	static void RegisterComponent()
	{
		([]() {
			std::string_view typeName = typeid(Component).name();
			size_t pos = typeName.find_last_of(":");
			std::string_view structName = typeName.substr(pos + 1);
			std::string managedTypeName = fmt::format("Wire.{}", structName);

			MonoType* managedType = mono_reflection_type_from_name(managedTypeName.data(), ScriptEngine::GetCoreAssemblyImage());
			if (!managedType)
			{
				WR_CORE_ERROR("Could not find component type {}", managedTypeName);
				return;
			}
			s_EntityHasComponentFuncs[managedType] = [](Entity entity) { return entity.HasComponent<Component>(); };
		}(), ...);
	}

	template<typename... Component>
	static void RegisterComponent(ComponentGroup<Component...>)
	{
		RegisterComponent<Component...>();
	}

	ScriptGlue::LogFunc ScriptGlue::m_UILogFunc = nullptr;

	void ScriptGlue::RegisterComponents()
	{
		RegisterComponent(AllComponents{});
	}

	void ScriptGlue::RegisterFunctions()
	{
		#pragma region Log
		WR_ADD_INTERNAL_CALL(Debug_Log);
		#pragma endregion

		#pragma region Entity
		WR_ADD_INTERNAL_CALL(Entity_HasComponent);
		#pragma endregion

		#pragma region TransformComponent
		WR_ADD_INTERNAL_CALL(TransformComponent_GetTranslation);
		WR_ADD_INTERNAL_CALL(TransformComponent_SetTranslation);
		WR_ADD_INTERNAL_CALL(TransformComponent_GetRotation);
		WR_ADD_INTERNAL_CALL(TransformComponent_SetRotation);
		WR_ADD_INTERNAL_CALL(TransformComponent_GetScale);
		WR_ADD_INTERNAL_CALL(TransformComponent_SetScale);
		#pragma endregion

		#pragma region SpriteRendererComponent
		WR_ADD_INTERNAL_CALL(SpriteRendererComponent_GetColour);
		WR_ADD_INTERNAL_CALL(SpriteRendererComponent_SetColour);
		WR_ADD_INTERNAL_CALL(SpriteRendererComponent_GetTexturePath);
		WR_ADD_INTERNAL_CALL(SpriteRendererComponent_SetTexturePath);
		WR_ADD_INTERNAL_CALL(SpriteRendererComponent_GetTilingFactor);
		WR_ADD_INTERNAL_CALL(SpriteRendererComponent_SetTilingFactor);
		#pragma endregion

		#pragma region Input
		WR_ADD_INTERNAL_CALL(Input_IsKeyDown);
		#pragma endregion
	}

	void ScriptGlue::SetUILogFunc(const LogFunc& func)
	{
		m_UILogFunc = func;
	}

	ScriptGlue::LogFunc ScriptGlue::GetUILogFunc()
	{
		return m_UILogFunc;
	}

}

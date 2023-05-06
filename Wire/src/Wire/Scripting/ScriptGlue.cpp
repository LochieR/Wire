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
		char* cStr = mono_string_to_utf8(message);
		std::string msg = std::string(cStr);
		mono_free(cStr);

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
	#pragma region TagComponent
	static MonoString* TagComponent_GetTag(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		WR_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		WR_CORE_ASSERT(entity);

		std::string tag = entity.GetComponent<TagComponent>().Tag;

		return mono_string_new(ScriptEngine::GetAppDomain(), tag.c_str());
	}

	static void TagComponent_SetTag(UUID entityID, MonoString* value)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		WR_CORE_ASSERT(scene);
		Entity entity = scene->GetEntityByUUID(entityID);
		WR_CORE_ASSERT(entity);

		char* cStr = mono_string_to_utf8(value);
		entity.GetComponent<TagComponent>().Tag = std::string(cStr);
		mono_free(cStr);
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

		char* cStr = mono_string_to_utf8(monoStr);
		std::string path = std::string(cStr);
		mono_free(cStr);

		if (path.empty())
		{
			Ref<Texture2D> whiteTexture = Texture2D::Create(1, 1);
			uint32_t whiteTextureData = 0xffffffff;
			whiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));
			entity.GetComponent<SpriteRendererComponent>().Texture = whiteTexture;
		}
		else
		{
			entity.GetComponent<SpriteRendererComponent>().Texture = Texture2D::Create(path);
		}
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

	#pragma region CameraComponent
	static bool CameraComponent_IsPrimary(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		return entity.GetComponent<CameraComponent>().Primary;
	}

	static void CameraComponent_SetPrimary(UUID entityID, bool primary)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<CameraComponent>().Primary = primary;
	}

	static SceneCamera::ProjectionType CameraComponent_GetProjectionType(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		return entity.GetComponent<CameraComponent>().Camera.GetProjectionType();
	}

	static void CameraComponent_SetProjectionType(UUID entityID, SceneCamera::ProjectionType type)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<CameraComponent>().Camera.SetProjectionType(type);
	}

	static float CameraComponent_GetPerspectiveVerticalFOV(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		return entity.GetComponent<CameraComponent>().Camera.GetPerspectiveVerticalFOV();
	}

	static void CameraComponent_SetPerspectiveVerticalFOV(UUID entityID, float value)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<CameraComponent>().Camera.SetPerspectiveVerticalFOV(value);
	}

	static float CameraComponent_GetPerspectiveNear(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		return entity.GetComponent<CameraComponent>().Camera.GetPerspectiveNearClip();
	}

	static void CameraComponent_SetPerspectiveNear(UUID entityID, float value)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<CameraComponent>().Camera.SetPerspectiveNearClip(value);
	}

	static float CameraComponent_GetPerspectiveFar(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		return entity.GetComponent<CameraComponent>().Camera.GetPerspectiveFarClip();
	}

	static void CameraComponent_SetPerspectiveFar(UUID entityID, float value)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<CameraComponent>().Camera.SetPerspectiveFarClip(value);
	}

	static float CameraComponent_GetOrthographicSize(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		return entity.GetComponent<CameraComponent>().Camera.GetOrthographicSize();
	}

	static void CameraComponent_SetOrthographicSize(UUID entityID, float value)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<CameraComponent>().Camera.SetOrthographicSize(value);
	}

	static float CameraComponent_GetOrthographicNear(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		return entity.GetComponent<CameraComponent>().Camera.GetOrthographicNearClip();
	}

	static void CameraComponent_SetOrthographicNear(UUID entityID, float value)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<CameraComponent>().Camera.SetOrthographicNearClip(value);
	}

	static float CameraComponent_GetOrthographicFar(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		return entity.GetComponent<CameraComponent>().Camera.GetOrthographicFarClip();
	}

	static void CameraComponent_SetOrthographicFar(UUID entityID, float value)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<CameraComponent>().Camera.SetOrthographicFarClip(value);
	}

	static bool CameraComponent_IsFixedAspectRatio(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		return entity.GetComponent<CameraComponent>().FixedAspectRatio;
	}

	static void CameraComponent_SetFixedAspectRatio(UUID entityID, bool value)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->GetEntityByUUID(entityID);

		entity.GetComponent<CameraComponent>().FixedAspectRatio = value;
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
		#pragma region Debug
		WR_ADD_INTERNAL_CALL(Debug_Log);
		#pragma endregion

		#pragma region Entity
		WR_ADD_INTERNAL_CALL(Entity_HasComponent);
		#pragma region TagComponent
		WR_ADD_INTERNAL_CALL(TagComponent_GetTag);
		WR_ADD_INTERNAL_CALL(TagComponent_SetTag);
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

		#pragma region CameraComponent
		WR_ADD_INTERNAL_CALL(CameraComponent_IsPrimary);
		WR_ADD_INTERNAL_CALL(CameraComponent_SetPrimary);
		WR_ADD_INTERNAL_CALL(CameraComponent_GetProjectionType);
		WR_ADD_INTERNAL_CALL(CameraComponent_SetProjectionType);
		WR_ADD_INTERNAL_CALL(CameraComponent_GetPerspectiveVerticalFOV);
		WR_ADD_INTERNAL_CALL(CameraComponent_SetPerspectiveVerticalFOV);
		WR_ADD_INTERNAL_CALL(CameraComponent_GetPerspectiveNear);
		WR_ADD_INTERNAL_CALL(CameraComponent_SetPerspectiveNear);
		WR_ADD_INTERNAL_CALL(CameraComponent_GetPerspectiveFar);
		WR_ADD_INTERNAL_CALL(CameraComponent_SetPerspectiveFar);
		WR_ADD_INTERNAL_CALL(CameraComponent_GetOrthographicSize);
		WR_ADD_INTERNAL_CALL(CameraComponent_SetOrthographicSize);
		WR_ADD_INTERNAL_CALL(CameraComponent_GetOrthographicNear);
		WR_ADD_INTERNAL_CALL(CameraComponent_SetOrthographicNear);
		WR_ADD_INTERNAL_CALL(CameraComponent_GetOrthographicFar);
		WR_ADD_INTERNAL_CALL(CameraComponent_SetOrthographicFar);
		WR_ADD_INTERNAL_CALL(CameraComponent_IsFixedAspectRatio);
		WR_ADD_INTERNAL_CALL(CameraComponent_SetFixedAspectRatio);
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
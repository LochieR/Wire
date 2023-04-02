#include "wrpch.h"
#include "ScriptGlue.h"

#include "ScriptEngine.h"
#include "Wire/Core/UUID.h"
#include "Wire/Scene/Scene.h"

#include <mono/metadata/object.h>

namespace Wire {

#define WR_ADD_INTERNAL_CALL(name) mono_add_internal_call("Wire.InternalCalls::" #name, name)

	static void NativeLog(MonoString* string, int value)
	{
		char* cStr = mono_string_to_utf8(string);
		std::string str(cStr);
		mono_free(cStr);
		std::cout << str << value << std::endl;
	}

	static void NativeLog_Vector(glm::vec3* parameter, glm::vec3* outResult)
	{
		WR_CORE_WARN("Value: {0}", *parameter);

		*outResult = glm::normalize(*parameter);
	}

	static void Entity_GetTranslation(UUID entityId, glm::vec3* outTranslation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();

		Entity entity = scene->GetEntityByUUID(entityId);
		*outTranslation = entity.GetComponent<TransformComponent>().Translation;
	}

	static void Entity_SetTranslation(UUID entityId, glm::vec3* translation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();

		Entity entity = scene->GetEntityByUUID(entityId);
		entity.GetComponent<TransformComponent>().Translation = *translation;
	}

	static bool Input_IsKeyDown(KeyCode keycode)
	{
		return Input::IsKeyPressed(keycode);
	}

	void ScriptGlue::RegisterFunctions()
	{
		WR_ADD_INTERNAL_CALL(NativeLog);
		WR_ADD_INTERNAL_CALL(NativeLog_Vector);

		WR_ADD_INTERNAL_CALL(Entity_GetTranslation);
		WR_ADD_INTERNAL_CALL(Entity_SetTranslation);

		WR_ADD_INTERNAL_CALL(Input_IsKeyDown);
	}

}

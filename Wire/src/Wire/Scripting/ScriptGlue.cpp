#include "wrpch.h"
#include "ScriptGlue.h"

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

	void ScriptGlue::RegisterFunctions()
	{
		WR_ADD_INTERNAL_CALL(NativeLog);
		WR_ADD_INTERNAL_CALL(NativeLog_Vector);
	}

}

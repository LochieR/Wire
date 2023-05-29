#pragma once

#include <string>
#include <functional>

namespace Wire {

	class ScriptGlue
	{
	public:
		static void RegisterComponents();
		static void RegisterFunctions();
	};

}

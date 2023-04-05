#pragma once

#include <string>
#include <functional>

namespace Wire {

	class ScriptGlue
	{
	public:
		using LogFunc = std::function<void(int, const std::string&)>;

		static void RegisterComponents();
		static void RegisterFunctions();

		static void SetUILogFunc(const LogFunc& func);
		static LogFunc GetUILogFunc();
	private:
		static LogFunc m_UILogFunc;
	};

}

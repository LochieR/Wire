#pragma once

#include "Wire/Core/Core.h"
#include "Wire/Core/Log.h"
#include <filesystem>

#ifdef WR_ENABLE_ASSERTS
	#define WR_INTERNAL_ASSERT_IMPL(type, check, msg, ...) {if(!(check)) { WR##type##ERROR(msg, __VA_ARGS__); WR_DEBUGBREAK(); } }
	#define WR_INTERNAL_ASSERT_WITH_MSG(type, check, ...) WR_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define WR_INTERNAL_ASSERT_NO_MSG(type, check) WR_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", WR_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define WR_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define WR_INTERNAL_ASSERT_GET_MACRO(...) WR_EXPAND_MACRO( WR_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, WR_INTERNAL_ASSERT_WITH_MSG, WR_INTERNAL_ASSERT_NO_MSG) )

	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define WR_ASSERT(...) WR_EXPAND_MACRO( WR_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define WR_CORE_ASSERT(...) WR_EXPAND_MACRO( WR_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define WR_ASSERT(...)
	#define WR_CORE_ASSERT(...)
#endif

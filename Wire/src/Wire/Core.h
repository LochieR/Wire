#pragma once

#include <memory>

#ifdef WR_PLATFORM_WINDOWS
#if WR_DYNAMIC_LINK
	#ifdef WR_BUILD_DLL
		#define WIRE_API __declspec(dllexport)
	#else
		#define WIRE_API __declspec(dllimport)
	#endif
#else
    #define WIRE_API
#endif
#else
	#error Wire only supports Windows!
#endif

#ifdef WR_DEBUG
	#define WR_ENABLE_ASSERTS
#endif

#ifdef WR_ENABLE_ASSERTS
	#define WR_ASSERT(x, ...) { if(!(x)) { WR_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define WR_CORE_ASSERT(x, ...) { if(!(x)) { WR_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define WR_ASSERT(x, ...)
	#define WR_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define WR_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

namespace Wire {

	template<typename T>
	using Scope = std::unique_ptr<T>;

	template<typename T>
	using Ref = std::shared_ptr<T>;

}
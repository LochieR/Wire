#pragma once

#include <memory>

#include "Wire/Core/PlatformDetection.h"

#ifdef WR_DEBUG
	#if defined(WR_PLATFORM_WINDOWS)
		#define WR_DEBUGBREAK() __debugbreak()
	#elif defined(WR_PLATFORM_LINUX)
		#include <signal.h>
		#define WR_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define WR_ENABLE_ASSERTS
#else
	#define WR_DEBUGBREAK()
#endif

#define WR_EXPAND_MACRO(x) x
#define WR_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define WR_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Wire {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}

#include "Wire/Core/Log.h"
#include "Wire/Core/Assert.h"
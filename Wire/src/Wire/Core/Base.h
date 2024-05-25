#pragma once

// Platform detection using predefined macros
#ifdef _WIN32
	/* Windows x64/x86 */
	#ifdef _WIN64
		/* Windows x64  */
		#define WR_PLATFORM_WINDOWS
	#else
		/* Windows x86 */
		#error "x86 Builds are not supported!"
	#endif
#elif defined(__APPLE__) || defined(__MACH__)
	#include <TargetConditionals.h>
	/* TARGET_OS_MAC exists on all the platforms
	 * so we must check all of them (in this order)
	 * to ensure that we're running on MAC
	 * and not some other Apple platform */
	#if TARGET_IPHONE_SIMULATOR == 1
		#error "IOS simulator is not supported!"
	#elif TARGET_OS_IPHONE == 1
		#define WR_PLATFORM_IOS
		#error "IOS is not supported!"
	#elif TARGET_OS_MAC == 1
		#define WR_PLATFORM_MACOSX
		#error "MacOS is not supported!"
	#else
		#error "Unknown Apple platform!"
	#endif
/* We also have to check __ANDROID__ before __linux__
 * since android is based on the linux kernel
 * it has __linux__ defined */
#elif defined(__ANDROID__)
	#define WR_PLATFORM_ANDROID
	#error "Android is not supported!"
#elif defined(__linux__)
	#define WR_PLATFORM_LINUX
	#error "Linux is not supported!"
#else
	/* Unknown compiler/platform */
	#error "Unknown platform!"
#endif // End of platform detection

#ifdef WR_DEBUG
	#if defined(WR_PLATFORM_WINDOWS)
		#define WR_DEBUGBREAK() __debugbreak()
	#elif defined(WR_PLATFORM_LINUX) || defined(WR_PLATFORM_MACOSX)
		#include <signal.h>
		#define WR_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define WR_ENABLE_ASSERTS
#else
	#define WR_DEBUGBREAK()
#endif

#include "Log.h"

#ifdef WR_ENABLE_ASSERTS
#define WR_ASSERT(x) { if (!(x)) { WR_ERROR("Assertion failed: ", #x); __debugbreak(); } }
#else
#define WR_ASSERT(x)
#endif

#define WR_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

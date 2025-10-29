#pragma once

#include "Log.h"
#include <csignal>

#ifdef _WIN32

#include <windows.h>
#include <iostream>

#ifdef WR_DEBUG
#define WR_ASSERT(x, msg, ...) { if (!(x)) { WR_ERROR("[Assertion failed, {}:{}] " msg, __FILE__, __LINE__, ##__VA_ARGS__); __debugbreak(); } }

#define WR_ASSERT_OR_WARN(x, msg, ...) { if (!(x)) { WR_ERROR("[Assertion failed, {}:{}] " msg, __FILE__, __LINE__, ##__VA_ARGS__); __debugbreak(); } }
#define WR_ASSERT_OR_ERROR(x, msg, ...) { if (!(x)) { WR_ERROR("[Assertion failed, {}:{}] " msg, __FILE__, __LINE__, ##__VA_ARGS__); __debugbreak(); } }
#else
#define WR_ASSERT(...)

#define WR_ASSERT_OR_WARN(x, msg, ...) { if (!(x)) { WR_WARN(msg, ##__VA_ARGS__); } }
#define WR_ASSERT_OR_ERROR(x, msg, ...) { if (!(x)) { WR_ERROR(msg, ##__VA_ARGS__); } }
#endif

#else

#ifdef WR_DEBUG
#define WR_ASSERT(x, msg, ...) { if (!(x)) { WR_ERROR("[Assertion failed, {}:{}] " msg, __FILE__, __LINE__, ##__VA_ARGS__); raise(SIGTRAP); } }

#define WR_ASSERT_OR_WARN(x, msg, ...) { if (!(x)) { WR_ERROR("[Assertion failed, {}:{}] " msg, __FILE__, __LINE__, ##__VA_ARGS__); raise(SIGTRAP); } }
#define WR_ASSERT_OR_ERROR(x, msg, ...) { if (!(x)) { WR_ERROR("[Assertion failed, {}:{}] " msg, __FILE__, __LINE__, ##__VA_ARGS__); raise(SIGTRAP); } }
#else
#define WR_ASSERT(...)

#define WR_ASSERT_OR_WARN(x, msg, ...) { if (!(x)) { WR_WARN(msg, ##__VA_ARGS__); } }
#define WR_ASSERT_OR_ERROR(x, msg, ...) { if (!(x)) { WR_ERROR(msg, ##__VA_ARGS__); } }
#endif

#endif

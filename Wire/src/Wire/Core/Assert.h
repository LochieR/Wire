#pragma once

#include "Log.h"

#ifdef _WIN32

#include <windows.h>
#include <iostream>

#ifdef WR_DEBUG
#define WR_ASSERT(x, msg, ...) { if (!(x)) { WR_ERROR(msg, ##__VA_ARGS__); __debugbreak(); } }

#define WR_ASSERT_OR_WARN(x, msg, ...) { if (!(x)) { WR_ERROR(msg, ##__VA_ARGS__); __debugbreak(); } }
#define WR_ASSERT_OR_ERROR(x, msg, ...) { if (!(x)) { WR_ERROR(msg, ##__VA_ARGS__); __debugbreak(); } }
#else
#define WR_ASSERT(...)

#define WR_ASSERT_OR_WARN(x, msg, ...) { if (!(x)) { WR_WARN(msg, ##__VA_ARGS__); } }
#define WR_ASSERT_OR_ERROR(x, msg, ...) { if (!(x)) { WR_ERROR(msg, ##__VA_ARGS__); } }
#endif

#endif

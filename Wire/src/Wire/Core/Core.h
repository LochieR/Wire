#pragma once

#include "Assert.h"

#ifdef _WIN32
#define WR_PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
#define WR_PLATFORM_MAC
#else
#error "Unknown platform"
#endif

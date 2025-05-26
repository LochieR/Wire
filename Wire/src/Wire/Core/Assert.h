#pragma once

#ifdef _WIN32

#include <windows.h>
#include <iostream>

#define WR_ASSERT(x, msg) { if (!(x)) { std::cout << "Assertion failed: " << msg << std::endl; __debugbreak(); } }

#endif

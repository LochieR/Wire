#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#include <thread>

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include <sstream>
#include <array>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Wire/Core/Log.h"

#include "Wire/Debug/Instrumentor.h"

#ifdef WR_PLATFORM_WINDOWS
	#include <Windows.h>
#endif
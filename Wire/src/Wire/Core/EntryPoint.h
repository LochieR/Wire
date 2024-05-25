#pragma once

#include "Base.h"

#ifdef WR_PLATFORM_WINDOWS

#include <windows.h>

#include "Application.h"

namespace Wire {

	int Main(int argc, char** argv)
	{
		Wire::Log::Init();

		Application* app = CreateApplication({ argc, argv });
		app->Run();

		delete app;

		return 0;
	}

}

#ifdef WR_DIST

int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	return Wire::Main(__argc, __argv);
}

#else

int main(int argc, char** argv)
{
	return Wire::Main(argc, argv);
}

#endif

#endif

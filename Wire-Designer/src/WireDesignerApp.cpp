#include <Wire.h>

#include "EditorLayer.h"

#if !defined(WR_DIST)
int main(int argc, char** argv)
#elif defined(WR_PLATFORM_WINDOWS)
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
#endif
{
	Wire::Log::Init();

#if !defined(WR_DIST) || !defined(WR_PLATFORM_WINDOWS)
	Wire::Application* app = new Wire::Application("Wire Designer", { argc, argv });
#elif defined(WR_PLATFORM_WINDOWS)
	Wire::Application* app = new Wire::Application("Wire Designer", { __argc, __argv });
#endif

	app->PushLayer(new Wire::EditorLayer());
	app->Run();
	
	delete app;
}
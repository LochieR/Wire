#pragma once
#include "Wire/Core/Core.h"
#include "Wire/Core/Application.h"

#ifdef WR_PLATFORM_WINDOWS

extern Wire::Application* Wire::CreateApplication(ApplicationCommandLineArgs args);

int main(int argc, char** argv)
{
	Wire::Log::Init();

	WR_PROFILE_BEGIN_SESSION("Startup", "WireProfile-Startup.json");
	auto app = Wire::CreateApplication({ argc, argv });
	WR_PROFILE_END_SESSION();

	WR_PROFILE_BEGIN_SESSION("Runtime", "WireProfile-Runtime.json");
	app->Run();
	WR_PROFILE_END_SESSION();

	WR_PROFILE_BEGIN_SESSION("Shutdown", "WireProfile-Shutdown.json");
	delete app;
	WR_PROFILE_END_SESSION();
}

#endif

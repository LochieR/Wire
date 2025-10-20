#include "Wire.h"

#include "EngineLayer.h"

#if !defined(WR_DIST) || !defined(WR_PLATFORM_WINDOWS)

int main()
{
	WR_SETUP_LOG({ &std::cout }, { "bloom.log" }, "%c[%H:%M:%S]%c %m");

	wire::ApplicationDesc desc{};
	desc.WindowTitle = "bloom";
	desc.WindowWidth = 1280;
	desc.WindowHeight = 720;

	wire::Application app(desc);
	
	app.pushLayer(new bloom::EngineLayer());
	app.run();

	return 0;
}

#else

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	WR_SETUP_LOG({ &std::cout }, { "bloom.log" }, "%c[%H:%M:%S]%c %m");

	wire::ApplicationDesc desc{};
	desc.WindowTitle = "bloom";
	desc.WindowWidth = 1280;
	desc.WindowHeight = 720;

	wire::Application app(desc);

	app.pushLayer(new bloom::EngineLayer());
	app.run();

	return 0;
}

#endif

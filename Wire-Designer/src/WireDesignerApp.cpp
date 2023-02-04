#include <Wire.h>

#include "EditorLayer.h"

#ifdef WR_DEBUG

int main(int argc, char** argv)
{
	Wire::Log::Init();

	WR_PROFILE_BEGIN_SESSION("Startup", "WireProfile-Startup.json");

	auto app = new Wire::Application("Wire Designer", { argc, argv });

	app->PushLayer(new Wire::EditorLayer());

	WR_PROFILE_END_SESSION();
	WR_PROFILE_BEGIN_SESSION("Runtime", "WireProfile-Runtime.json");

	app->Run();

	WR_PROFILE_END_SESSION();
	WR_PROFILE_BEGIN_SESSION("Shutdown", "WireProfile-Shutdown.json");

	delete app;

	WR_PROFILE_END_SESSION();
}

#elif defined(WR_RELEASE)

int main(int argc, char** argv)
{
	Wire::Log::Init();

	auto app = new Wire::Application("Wire Designer", { argc, argv });

	app->PushLayer(new Wire::EditorLayer());
	app->Run();

	delete app;
}

#elif defined(WR_DIST)

#ifdef WR_PLATFORM_WINDOWS
#include <vector>
#include <locale>
#include <codecvt>

// TODO: Fix command line arguments for Windows:Dist
// Windows has different command line arguments, so a few functions are needed for string splitting

template <typename Out>
void split(const std::string& s, char delim, Out result)
{
	std::istringstream iss(s);
	std::string item;
	while (std::getline(iss, item, delim))
	{
		*result++ = item;
	}
}

std::vector<std::string> split(const std::string& s, char delim)
{
	std::vector<std::string> elems;
	split(s, delim, std::back_inserter(elems));
	return elems;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
#else
int main(int argc, char** argv)
#endif
{
	Wire::Log::Init();
#ifdef WR_PLATFORM_WINDOWS
	int argc = __argc;
	char** argv = (char**)__wargv;
	
	WR_TRACE("{0}", argc);
	
	wchar_t* cmdLine = (wchar_t*)pCmdLine;

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::string args(converter.to_bytes(pCmdLine));

	if (!args.empty())
	{
		auto substrs = split(args, ' ');
		argc = substrs.size() + 1;
		substrs.insert(substrs.begin(), "Wire-Designer.exe");

	}
#endif


	auto app = new Wire::Application("Wire Designer", { argc, argv });

	app->PushLayer(new Wire::EditorLayer());
	app->Run();

	delete app;
}

#endif

#include <Wire.h>

#include "EditorLayer.h"

#if defined(WR_DEBUG) || defined(WR_RELEASE)
int main(int argc, char** argv)
#elif defined(WR_DIST) && defined(WR_PLATFORM_WINDOWS)
#pragma warning(disable: 4996)
#include <windows.h>
#include <string>
#include <locale>
#include <codecvt>

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

char** GetArgs(const std::wstring& original, int argc)
{
	std::string str = std::filesystem::path(original).string();
	auto splitted = split(str, ' ');
	char** argv = new char* [argc];
	argv[0] = "WireDesigner.exe";
	for (int i = 0; i < argc; i++)
	{
		if (i != argc - 1)
		{
			argv[i + 1] = (char*)splitted[i].c_str();
		}
	}
	return argv;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
#endif
{
	Wire::Log::Init();

#if defined(WR_DIST) && defined(WR_PLATFORM_WINDOWS)
	// TODO: Get command line args (argc, argv);
	int argc = __argc;
	char** argv = GetArgs(std::filesystem::path(pCmdLine).wstring(), argc);
#endif

	for (int i = 0; i < argc; i++)
	{
		WR_INFO("{0}", argv[i]);
	}

	Wire::Application* app = new Wire::Application("Wire Designer", { argc, argv });
	app->PushLayer(new Wire::EditorLayer());
	WR_PROFILE_BEGIN_SESSION("Runtime", "Wire_Runtime.json");
	app->Run();
	WR_PROFILE_END_SESSION();
	
#if defined(WR_DIST) && defined(WR_PLATFORM_WINDOWS)
	delete[] argv;
#endif
	delete app;
}


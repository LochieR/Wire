#include <Wire.h>

#include "EditorLayer.h"

#ifdef WR_DEBUG
int main(int argc, char** argv)
#elif defined(WR_DIST) && defined(_WIN32)
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

char** GetArgs(wchar_t* original)
{
	std::wstring wstr(original);
	std::string str = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(wstr);
	WR_INFO("{0}", str);
	std::vector<char*> c_strings{};
	auto splitted = split(str, ' ');
	splitted.insert(splitted.begin(), "Wire-Designer.exe");
	for (auto& string : splitted)
		c_strings.push_back(&string.front());

	char** result = new char* [c_strings.size()];
	for (int i = 0; i < c_strings.size(); i++)
		result[i] = c_strings[i];

	return result;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
#endif
{
	Wire::Log::Init();

#if defined(WR_DIST) && defined(_WIN32)
	// TODO: Get command line args (argc, argv);
	int argc = __argc;
	char** argv = GetArgs((wchar_t*)pCmdLine);
#endif

	Wire::Application* app = new Wire::Application("Wire Designer", { argc, argv });
	app->PushLayer(new Wire::EditorLayer());
	app->Run();

	delete app;
}


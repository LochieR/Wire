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

	Wire::EditorLayer* editor = new Wire::EditorLayer();
	editor->AddPanel<Wire::SceneHierarchyPanel>();
	editor->AddPanel<Wire::PropertiesPanel>();
	editor->AddPanel<Wire::ContentBrowserPanel>();
	editor->AddPanel<Wire::ConsolePanel>();
	editor->AddPanel<Wire::AudioGraphPanel>();

	app->PushLayer(editor);
	app->Run();
	
	delete app;
}
#include <Wire/Wire.h>
#include <Wire/Core/EntryPoint.h>

#include "EditorLayer.h"

#include <string>

Wire::Application* Wire::CreateApplication(const Wire::ApplicationCommandLineArgs& args)
{
	Wire::ApplicationSpecification spec;

	spec.Name = "Wire Editor";
	spec.CommandLineArgs = args;

	Wire::WindowSpecification windowSpec;
	windowSpec.Title = spec.Name;
	windowSpec.Width = 1600;
	windowSpec.Height = 900;
	windowSpec.VSync = true;

	spec.WindowSpec = windowSpec;
	spec.EnableImGui = true;

	Wire::Application* app = new Wire::Application(spec);

	app->PushLayer(new Wire::EditorLayer());

	return app;
}

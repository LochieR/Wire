#include <Wire.h>
#include <Wire/Core/EntryPoint.h>

#include "Sandbox2D.h"
#include "ExampleLayer.h"

class Sandbox : public Wire::Application
{
public:
	Sandbox()
	{
		// PushLayer(new ExampleLayer());
		PushLayer(new Sandbox2D());
	}

	~Sandbox()
	{
	}
};

Wire::Application* Wire::CreateApplication(Wire::ApplicationCommandLineArgs args)
{
	return new Sandbox();
}

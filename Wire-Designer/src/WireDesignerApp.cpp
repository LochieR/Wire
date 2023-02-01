#include <Wire.h>
#include <Wire/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Wire {

	class WireDesigner : public Application
	{
	public:
		WireDesigner(ApplicationCommandLineArgs args)
			: Application("Wire Designer", args)
		{
			PushLayer(new EditorLayer());
		}

		~WireDesigner()
		{
		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		return new WireDesigner(args);
	}

}

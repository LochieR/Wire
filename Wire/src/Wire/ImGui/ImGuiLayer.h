#pragma once

#include "FontLibrary.h"
#include "Wire/Renderer/IResource.h"

#include "Wire/Core/Layer.h"
#include "Wire/Core/Window.h"

#include <imgui.h>

namespace Wire {

	class ImGuiLayer : public Layer, public IResource
	{
	public:
		virtual ~ImGuiLayer() = default;

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void UpdateViewports() = 0;

		virtual FontLibrary& GetFontLibrary() const = 0;
	};

}

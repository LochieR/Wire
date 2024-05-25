#include "wrpch.h"
#include "Renderer.h"

#include "Platform/Renderer/Vulkan/VulkanRenderer.h"

namespace Wire {

	Renderer* Renderer::Create(Window& window)
	{
		return new VulkanRenderer(window);
	}

}

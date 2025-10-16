#include "Renderer.h"

#include "Wire/UI/Renderer/Vulkan/VulkanRenderer.h"

namespace wire {

	Renderer* createRenderer(const RendererDesc& desc)
	{
		return new VulkanRenderer(desc);
	}

}

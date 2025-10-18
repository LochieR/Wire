#include "Renderer.h"

#include "Wire/UI/Renderer/Vulkan/VulkanRenderer.h"

namespace wire {

	Renderer* createRenderer(const RendererDesc& desc, const SwapchainDesc& swapchainDesc)
	{
		return new VulkanRenderer(desc, swapchainDesc);
	}

}

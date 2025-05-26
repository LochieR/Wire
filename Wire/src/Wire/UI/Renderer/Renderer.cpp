module wire.ui.renderer;

import wire.ui.renderer.vk;

namespace wire {

	Renderer* createRenderer(const RendererDesc& desc)
	{
		return new VulkanRenderer(desc);
	}

}

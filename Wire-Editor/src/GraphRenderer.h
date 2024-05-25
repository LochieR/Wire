#pragma once

#include "Wire/Renderer/Renderer.h"

#include <string>

namespace Wire {

	class GraphRenderer
	{
	public:
		GraphRenderer(Renderer* renderer, rbRef<Framebuffer> framebuffer = nullptr);
		~GraphRenderer();

		void Draw(const OrthographicCamera& camera, std::string_view expression);
	private:
		Renderer* m_Renderer = nullptr;

		rbRef<VertexBuffer> m_VertexBuffer = nullptr;
		rbRef<Shader> m_Shader = nullptr;
		rbRef<GraphicsPipeline> m_Pipeline = nullptr;
		rbRef<Framebuffer> m_Framebuffer = nullptr;
		
		std::vector<rbRef<CommandBuffer>> m_CommandBuffers;

		uint32_t m_VertexCount = 0;

		std::string m_Expression;
	};

}

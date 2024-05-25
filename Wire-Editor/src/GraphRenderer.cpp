#include "GraphRenderer.h"

#include <glm/glm.hpp>

namespace Wire {

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	GraphRenderer::GraphRenderer(Renderer* renderer, rbRef<Framebuffer> framebuffer)
		: m_Renderer(renderer), m_Framebuffer(framebuffer)
	{
		InputLayout layout{};
		layout.VertexLayout = {
			{ ShaderDataType::Float3, "Position" },
			{ ShaderDataType::Float4, "Color" }
		};

		PushConstantInfo pushConstant{};
		pushConstant.Size = sizeof(glm::mat4);
		pushConstant.Offset = 0;
		pushConstant.Stage = ShaderStage::Vertex;

		layout.PushConstants.push_back(pushConstant);

		m_VertexBuffer = renderer->CreateVertexBuffer(sizeof(LineVertex) * 10'000ui64);
		m_VertexBuffer->SetLayout(layout);

		m_Shader = renderer->CreateShader("Resources/Shaders/LineShader.glsl");
		if (!framebuffer)
			m_Pipeline = m_Shader->CreatePipeline(layout, PrimitiveTopology::LineStrip);
		else
			m_Pipeline = m_Shader->CreatePipeline(layout, PrimitiveTopology::LineStrip, framebuffer);

		for (uint32_t i = 0; i < renderer->GetMaxFramesInFlight(); i++)
		{
			m_CommandBuffers.push_back(renderer->AllocateCommandBuffer());
		}

		std::vector<LineVertex> vertices(1000);
		float x = -4.0f;
		for (uint32_t i = 0; i < 1000; i++)
		{
			vertices[i].Position.x = x;
			vertices[i].Position.y = x * x - 1.0f;
			vertices[i].Color = { 1.0f, 1.0f, 1.0f, 1.0f };

			x += 8.0f / 1000.0f;
		}

		m_VertexCount = 1000;

		m_VertexBuffer->SetData(vertices.data(), sizeof(LineVertex) * vertices.size());
	}

	GraphRenderer::~GraphRenderer()
	{
	}

	void GraphRenderer::Draw(const OrthographicCamera& camera, std::string_view expression)
	{
		if (std::string(expression) == m_Expression)
		{
			m_Expression = std::string(expression);
		}

		auto commandBuffer = m_CommandBuffers[m_Renderer->GetFrameIndex()];

		commandBuffer->Begin();
		if (m_Framebuffer)
			m_Framebuffer->BeginRenderPass(commandBuffer);
		else
			m_Renderer->BeginRenderPass(commandBuffer);

		m_Pipeline->Bind(commandBuffer);
		m_Pipeline->PushConstants(commandBuffer, ShaderStage::Vertex, sizeof(glm::mat4), &camera.GetViewProjectionMatrix());
		m_Pipeline->SetLineWidth(commandBuffer, 5.0f);

		m_VertexBuffer->Bind(commandBuffer);

		m_Renderer->Draw(commandBuffer, m_VertexCount);

		if (m_Framebuffer)
			m_Framebuffer->EndRenderPass(commandBuffer);
		else
			m_Renderer->EndRenderPass(commandBuffer);
		commandBuffer->End();
		
		m_Renderer->SubmitCommandBuffer(commandBuffer);
	}

}

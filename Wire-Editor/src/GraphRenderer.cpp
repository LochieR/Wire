#include "GraphRenderer.h"

#include <glm/glm.hpp>

namespace Wire {

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	namespace Utils {

		static std::vector<LineVertex> GenerateBezierCurve(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, uint32_t numSegments)
		{
			std::vector<LineVertex> vertices;
			vertices.reserve((size_t)numSegments + 1);

			for (uint32_t i = 0; i <= numSegments; i++)
			{
				float t = (float)i / (float)numSegments;
				float u = 1.0f - t;

				glm::vec2 point = (u * u * p0) + (2 * u * t * p1) + (t * t * p2);
				vertices.push_back({ glm::vec3(point, 0.0f), { 1.0f, 1.0f, 1.0f, 1.0f } });
			}

			return vertices;
		}

	}

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
			m_Pipeline = m_Shader->CreatePipeline(layout, PrimitiveTopology::LineStrip, false);
		else
			m_Pipeline = m_Shader->CreatePipeline(layout, PrimitiveTopology::LineStrip, framebuffer->IsMultiSampled(), framebuffer);

		for (uint32_t i = 0; i < renderer->GetMaxFramesInFlight(); i++)
		{
			m_CommandBuffers.push_back(renderer->AllocateCommandBuffer());
		}

		static glm::vec2 p0 = { -2.0f, -2.0f };
		static glm::vec2 p1 = { -1.8f, 0.8f };
		static glm::vec2 p2 = { 1.0f, 3.0f };

		m_VertexCount = 1000;

		std::vector<LineVertex> vertices = Utils::GenerateBezierCurve(p0, p1, p2, m_VertexCount);

		m_VertexBuffer->SetData(vertices.data(), sizeof(LineVertex) * vertices.size());
	}

	GraphRenderer::~GraphRenderer()
	{
	}

	void GraphRenderer::Draw(const OrthographicCamera& camera, std::string_view expression)
	{
		static glm::vec2 p0 = { -2.0f, -2.0f };
		static glm::vec2 p1 = { -1.8f, 0.8f };
		static glm::vec2 p2 = { 1.0f, 3.0f };

		if (std::string(expression) != m_Expression)
		{
			m_Expression = std::string(expression);
		}

		auto& renderer2D = m_Renderer->GetRenderer2D();
		renderer2D.Begin(camera, m_Framebuffer);

		auto& commandBuffer = renderer2D.GetCurrentCommandBuffer();

		m_Pipeline->Bind(commandBuffer);
		m_Pipeline->PushConstants(commandBuffer, ShaderStage::Vertex, sizeof(glm::mat4), &camera.GetViewProjectionMatrix());
		m_Pipeline->SetLineWidth(commandBuffer, 5.0f);

		m_VertexBuffer->Bind(commandBuffer);

		m_Renderer->Draw(commandBuffer, m_VertexCount);

		renderer2D.DrawCircle(glm::vec3(p0, 0.0f), { 0.1f, 0.1f }, { 1.0f, 1.0f, 1.0f, 1.0f });
		renderer2D.DrawCircle(glm::vec3(p1, 0.0f), { 0.1f, 0.1f }, { 1.0f, 1.0f, 1.0f, 1.0f });
		renderer2D.DrawCircle(glm::vec3(p2, 0.0f), { 0.1f, 0.1f }, { 1.0f, 1.0f, 1.0f, 1.0f });

		renderer2D.End();
	}

}

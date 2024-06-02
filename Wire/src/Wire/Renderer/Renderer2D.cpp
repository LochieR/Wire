#include "wrpch.h"
#include "Renderer2D.h"

#include "Renderer.h"
#include "Wire/Core/Application.h"

#include "MSDFData.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Wire {

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		int TexIndex;
	};

	struct CircleVertex
	{
		glm::vec3 WorldPosition;
		glm::vec3 LocalPosition;
		glm::vec4 Color;
		float Thickness;
		float Fade;
	};

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	struct RoundedQuadVertex
	{
		glm::vec3 WorldPosition;
		glm::vec3 LocalPosition;
		glm::vec4 Color;
		float CornerRadius;
		float Fade;
	};

	struct TextVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		int FontIndex;
	};

	struct Renderer2DData
	{
		static constexpr size_t MaxQuads = 20'000;
		static constexpr size_t MaxVertices = MaxQuads * 4;
		static constexpr size_t MaxIndices = MaxQuads * 6;

		rbRef<VertexBuffer> QuadVertexBuffer = nullptr;
		rbRef<Shader> QuadShader = nullptr;
		std::vector<rbRef<GraphicsPipeline>> QuadPipelines;

		rbRef<VertexBuffer> CircleVertexBuffer = nullptr;
		rbRef<Shader> CircleShader = nullptr;
		std::vector<rbRef<GraphicsPipeline>> CirclePipelines;
		
		rbRef<VertexBuffer> LineVertexBuffer = nullptr;
		rbRef<Shader> LineShader = nullptr;
		std::vector<rbRef<GraphicsPipeline>> LinePipelines;

		rbRef<VertexBuffer> RoundedQuadVertexBuffer = nullptr;
		rbRef<Shader> RoundedQuadShader = nullptr;
		std::vector<rbRef<GraphicsPipeline>> RoundedQuadPipelines;

		rbRef<VertexBuffer> TextVertexBuffer = nullptr;
		rbRef<Shader> TextShader = nullptr;
		std::vector<rbRef<GraphicsPipeline>> TextPipelines;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		uint32_t CircleIndexCount = 0;
		CircleVertex* CircleVertexBufferBase = nullptr;
		CircleVertex* CircleVertexBufferPtr = nullptr;

		uint32_t LineVertexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;

		uint32_t RoundedQuadIndexCount = 0;
		RoundedQuadVertex* RoundedQuadVertexBufferBase = nullptr;
		RoundedQuadVertex* RoundedQuadVertexBufferPtr = nullptr;

		uint32_t TextIndexCount = 0;
		TextVertex* TextVertexBufferBase = nullptr;
		TextVertex* TextVertexBufferPtr = nullptr;

		rbRef<IndexBuffer> IndexBuffer = nullptr;
		rbRef<StagingBuffer> IDBuffer = nullptr;

		std::array<rbRef<Texture2D>, 32> TextureSlots;
		uint32_t TextureSlotIndex = 0;

		std::array<rbRef<Texture2D>, 32> FontSlots;
		uint32_t FontSlotIndex = 0;

		float LineWidth = 1.0f;
		rbRef<Font> DefaultFont = nullptr;

		std::vector<rbRef<CommandBuffer>> CommandBuffers;

		rbRef<Framebuffer> CurrentFramebuffer = nullptr;

		std::vector<rbRef<Framebuffer>> SetupFramebuffers;
		uint32_t FramebufferIndex = 0;

		glm::vec3 QuadVertexPositions[4];

		struct CameraData
		{
			glm::mat4 ViewProjection;
		};
		CameraData CameraPushConstantData{};
	};

	Renderer2DData s_Data;

	Renderer2D::Renderer2D(Renderer* renderer)
		: m_Renderer(renderer)
	{
		s_Data = {};

		uint32_t* indices = new uint32_t[s_Data.MaxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
		{
			indices[i + 0] = offset + 0;
			indices[i + 1] = offset + 1;
			indices[i + 2] = offset + 2;

			indices[i + 3] = offset + 2;
			indices[i + 4] = offset + 3;
			indices[i + 5] = offset + 0;

			offset += 4;
		}

		s_Data.IndexBuffer = renderer->CreateIndexBuffer(indices, s_Data.MaxIndices);

		delete[] indices;

		s_Data.CommandBuffers.resize(renderer->GetMaxFramesInFlight());

		for (uint32_t i = 0; i < renderer->GetMaxFramesInFlight(); i++)
		{
			s_Data.CommandBuffers[i] = renderer->AllocateCommandBuffer();
		}

		InputLayout quadLayout;
		quadLayout.VertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Int,	  "a_TexIndex" }
		};

		PushConstantInfo cameraPushConstantInfo{};
		cameraPushConstantInfo.Size = sizeof(Renderer2DData::CameraData);
		cameraPushConstantInfo.Stage = ShaderStage::Vertex;
		cameraPushConstantInfo.Offset = 0;

		quadLayout.PushConstants.push_back(cameraPushConstantInfo);

		ShaderResourceInfo samplerInfo{};
		samplerInfo.ResourceType = ShaderResourceType::CombinedImageSampler;
		samplerInfo.Binding = 0;
		samplerInfo.Stage = ShaderStage::Fragment;
		samplerInfo.ResourceCount = 32;

		quadLayout.ShaderResources.push_back(samplerInfo);

		s_Data.QuadShader = renderer->CreateShader("Resources/Shaders/QuadShader.glsl");
		s_Data.QuadPipelines.push_back(s_Data.QuadShader->CreatePipeline(quadLayout, PrimitiveTopology::TriangleList, false));
		s_Data.QuadVertexBuffer = renderer->CreateVertexBuffer(sizeof(QuadVertex) * s_Data.MaxVertices);
		s_Data.QuadVertexBuffer->SetLayout(quadLayout);

		s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

		InputLayout circleLayout;
		circleLayout.VertexLayout = {
			{ ShaderDataType::Float3, "a_WorldPosition" },
			{ ShaderDataType::Float3, "a_LocalPosition" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float,  "a_Thickness" },
			{ ShaderDataType::Float,  "a_Fade" }
		};

		circleLayout.PushConstants.push_back(cameraPushConstantInfo);

		s_Data.CircleShader = renderer->CreateShader("Resources/Shaders/CircleShader.glsl");
		s_Data.CirclePipelines.push_back(s_Data.CircleShader->CreatePipeline(circleLayout, PrimitiveTopology::TriangleList, false));
		s_Data.CircleVertexBuffer = renderer->CreateVertexBuffer(sizeof(CircleVertex) * s_Data.MaxVertices);
		s_Data.CircleVertexBuffer->SetLayout(circleLayout);

		s_Data.CircleVertexBufferBase = new CircleVertex[s_Data.MaxVertices];

		InputLayout lineLayout;
		lineLayout.VertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" }
		};

		lineLayout.PushConstants.push_back(cameraPushConstantInfo);

		s_Data.LineShader = renderer->CreateShader("Resources/Shaders/LineShader.glsl");
		s_Data.LinePipelines.push_back(s_Data.LineShader->CreatePipeline(lineLayout, PrimitiveTopology::LineList, false));
		s_Data.LineVertexBuffer = renderer->CreateVertexBuffer(sizeof(LineVertex) * s_Data.MaxVertices);
		s_Data.LineVertexBuffer->SetLayout(lineLayout);

		s_Data.LineVertexBufferBase = new LineVertex[s_Data.MaxVertices];

		InputLayout roundedQuadLayout;
		roundedQuadLayout.VertexLayout = {
			{ ShaderDataType::Float3, "a_WorldPosition" },
			{ ShaderDataType::Float3, "a_LocalPosition" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float,  "a_CornerRadius" },
			{ ShaderDataType::Float,  "a_Fade" }
		};

		roundedQuadLayout.PushConstants.push_back(cameraPushConstantInfo);

		s_Data.RoundedQuadShader = renderer->CreateShader("Resources/Shaders/RoundedQuadShader.glsl");
		s_Data.RoundedQuadPipelines.push_back(s_Data.RoundedQuadShader->CreatePipeline(roundedQuadLayout, PrimitiveTopology::TriangleList, false));
		s_Data.RoundedQuadVertexBuffer = renderer->CreateVertexBuffer(sizeof(RoundedQuadVertex) * s_Data.MaxVertices);
		s_Data.RoundedQuadVertexBuffer->SetLayout(roundedQuadLayout);

		s_Data.RoundedQuadVertexBufferBase = new RoundedQuadVertex[s_Data.MaxVertices];

		InputLayout textLayout;
		textLayout.VertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Int,    "a_FontIndex" }
		};

		textLayout.PushConstants.push_back(cameraPushConstantInfo);

		ShaderResourceInfo textResource{};
		textResource.ResourceType = ShaderResourceType::CombinedImageSampler;
		textResource.Binding = 0;
		textResource.Stage = ShaderStage::Fragment;
		textResource.ResourceCount = 32;

		textLayout.ShaderResources.push_back(textResource);

		s_Data.TextShader = renderer->CreateShader("Resources/Shaders/TextShader.glsl");
		s_Data.TextPipelines.push_back(s_Data.TextShader->CreatePipeline(textLayout, PrimitiveTopology::TriangleList, false));
		s_Data.TextVertexBuffer = renderer->CreateVertexBuffer(sizeof(TextVertex) * s_Data.MaxVertices);
		s_Data.TextVertexBuffer->SetLayout(textLayout);

		s_Data.TextVertexBufferBase = new TextVertex[s_Data.MaxVertices];

		s_Data.QuadVertexPositions[0] = {  0.5f, -0.5f, 0.0f };
		s_Data.QuadVertexPositions[1] = { -0.5f, -0.5f, 0.0f };
		s_Data.QuadVertexPositions[2] = { -0.5f,  0.5f, 0.0f };
		s_Data.QuadVertexPositions[3] = {  0.5f,  0.5f, 0.0f };

		uint32_t whiteTextureData = 0xFFFFFFFF;
		rbRef<Texture2D> whiteTexture = renderer->CreateTexture2D(&whiteTextureData, 1, 1);

		for (uint32_t i = 0; i < s_Data.TextureSlots.size(); i++)
			s_Data.TextureSlots[i] = whiteTexture;
		
		Window& window = Application::Get().GetWindow();
		for (uint32_t i = 0; i < 32; i++)
		{
			s_Data.QuadPipelines[0]->UpdateDescriptor(s_Data.TextureSlots[i], 0, i);
		}
		s_Data.TextureSlotIndex++;

		s_Data.DefaultFont = renderer->CreateFont("Resources/fonts/opensans/OpenSans-Regular.ttf");

		s_Data.FontSlots[0] = s_Data.DefaultFont->GetAtlasTexture();
		s_Data.TextPipelines[0]->UpdateDescriptor(s_Data.DefaultFont->GetAtlasTexture(), 0, 0);
		for (uint32_t i = 1; i < 32; i++)
		{
			s_Data.TextPipelines[0]->UpdateDescriptor(s_Data.TextureSlots[0], 0, i);
			s_Data.FontSlots[i] = s_Data.TextureSlots[0];
		}
		s_Data.FontSlotIndex++;

		s_Data.IDBuffer = renderer->CreateStagingBuffer(sizeof(int) * 2560 * 1440);
	}

	void Renderer2D::Release()
	{
		delete[] s_Data.TextVertexBufferBase;
		delete[] s_Data.RoundedQuadVertexBufferBase;
		delete[] s_Data.LineVertexBufferBase;
		delete[] s_Data.CircleVertexBufferBase;
		delete[] s_Data.QuadVertexBufferBase;
	}

	void Renderer2D::Begin(const OrthographicCamera& camera)
	{
		s_Data.CameraPushConstantData.ViewProjection = camera.GetViewProjectionMatrix();
		
		StartBatch();

		rbRef<CommandBuffer> commandBuffer = s_Data.CommandBuffers[m_Renderer->GetFrameIndex()];
		commandBuffer->Begin();

		if (s_Data.CurrentFramebuffer)
		{
			s_Data.CurrentFramebuffer->BeginRenderPass(commandBuffer);
		}
		else
		{
			m_Renderer->BeginRenderPass(commandBuffer);
		}
	}

	void Renderer2D::Begin(const OrthographicCamera& camera, rbRef<Framebuffer> framebuffer)
	{
		s_Data.CameraPushConstantData.ViewProjection = camera.GetViewProjectionMatrix();
		s_Data.CurrentFramebuffer = framebuffer;
		s_Data.FramebufferIndex = 0;

		for (uint32_t i = 0; i < s_Data.SetupFramebuffers.size(); i++)
		{
			if (framebuffer == s_Data.SetupFramebuffers[i])
			{
				s_Data.FramebufferIndex = i + 1;
			}
		}

		if (s_Data.FramebufferIndex == 0)
		{
			CreatePipelines(framebuffer);
			s_Data.SetupFramebuffers.push_back(framebuffer);
			s_Data.FramebufferIndex = (uint32_t)s_Data.SetupFramebuffers.size();
		}

		StartBatch();

		rbRef<CommandBuffer> commandBuffer = s_Data.CommandBuffers[m_Renderer->GetFrameIndex()];
		commandBuffer->Begin();

		if (s_Data.CurrentFramebuffer)
		{
			s_Data.CurrentFramebuffer->BeginRenderPass(commandBuffer);
		}
		else
		{
			m_Renderer->BeginRenderPass(commandBuffer);
		}
	}

	void Renderer2D::End()
	{
		Flush();

		rbRef<CommandBuffer> commandBuffer = s_Data.CommandBuffers[m_Renderer->GetFrameIndex()];

		commandBuffer->End();
		m_Renderer->SubmitCommandBuffer(commandBuffer);

		s_Data.CurrentFramebuffer = nullptr;
	}

	void Renderer2D::StartBatch()
	{
		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		s_Data.CircleIndexCount = 0;
		s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;

		s_Data.LineVertexCount = 0;
		s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;

		s_Data.RoundedQuadIndexCount = 0;
		s_Data.RoundedQuadVertexBufferPtr = s_Data.RoundedQuadVertexBufferBase;

		s_Data.TextIndexCount = 0;
		s_Data.TextVertexBufferPtr = s_Data.TextVertexBufferBase;
	}

	void Renderer2D::Flush()
	{
		rbRef<CommandBuffer> commandBuffer = s_Data.CommandBuffers[m_Renderer->GetFrameIndex()];

		if (s_Data.QuadIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
			s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

			s_Data.QuadPipelines[s_Data.FramebufferIndex]->Bind(commandBuffer);
			s_Data.QuadPipelines[s_Data.FramebufferIndex]->PushConstants(commandBuffer, ShaderStage::Vertex, sizeof(Renderer2DData::CameraData), &s_Data.CameraPushConstantData);
			s_Data.QuadPipelines[s_Data.FramebufferIndex]->BindDescriptor(commandBuffer);

			s_Data.QuadVertexBuffer->Bind(commandBuffer);
			s_Data.IndexBuffer->Bind(commandBuffer);

			m_Renderer->DrawIndexed(commandBuffer, s_Data.QuadIndexCount);
		}
		if (s_Data.CircleIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CircleVertexBufferPtr - (uint8_t*)s_Data.CircleVertexBufferBase);
			s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize);

			s_Data.CirclePipelines[s_Data.FramebufferIndex]->Bind(commandBuffer);
			s_Data.CirclePipelines[s_Data.FramebufferIndex]->PushConstants(commandBuffer, ShaderStage::Vertex, sizeof(Renderer2DData::CameraData), &s_Data.CameraPushConstantData);

			s_Data.CircleVertexBuffer->Bind(commandBuffer);
			s_Data.IndexBuffer->Bind(commandBuffer);

			m_Renderer->DrawIndexed(commandBuffer, s_Data.CircleIndexCount);
		}
		if (s_Data.LineVertexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
			s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize);

			s_Data.LinePipelines[s_Data.FramebufferIndex]->Bind(commandBuffer);
			s_Data.LinePipelines[s_Data.FramebufferIndex]->PushConstants(commandBuffer, ShaderStage::Vertex, sizeof(Renderer2DData::CameraData), &s_Data.CameraPushConstantData);
			s_Data.LinePipelines[s_Data.FramebufferIndex]->SetLineWidth(commandBuffer, s_Data.LineWidth);

			s_Data.LineVertexBuffer->Bind(commandBuffer);

			m_Renderer->Draw(commandBuffer, s_Data.LineVertexCount);
		}
		if (s_Data.RoundedQuadIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.RoundedQuadVertexBufferPtr - (uint8_t*)s_Data.RoundedQuadVertexBufferBase);
			s_Data.RoundedQuadVertexBuffer->SetData(s_Data.RoundedQuadVertexBufferBase, dataSize);

			s_Data.RoundedQuadPipelines[s_Data.FramebufferIndex]->Bind(commandBuffer);
			s_Data.RoundedQuadPipelines[s_Data.FramebufferIndex]->PushConstants(commandBuffer, ShaderStage::Vertex, sizeof(Renderer2DData::CameraData), &s_Data.CameraPushConstantData);

			s_Data.RoundedQuadVertexBuffer->Bind(commandBuffer);
			s_Data.IndexBuffer->Bind(commandBuffer);

			m_Renderer->DrawIndexed(commandBuffer, s_Data.RoundedQuadIndexCount);
		}
		if (s_Data.TextIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.TextVertexBufferPtr - (uint8_t*)s_Data.TextVertexBufferBase);
			s_Data.TextVertexBuffer->SetData(s_Data.TextVertexBufferBase, dataSize);

			s_Data.TextPipelines[s_Data.FramebufferIndex]->Bind(commandBuffer);
			s_Data.TextPipelines[s_Data.FramebufferIndex]->PushConstants(commandBuffer, ShaderStage::Vertex, sizeof(Renderer2DData::CameraData), &s_Data.CameraPushConstantData);
			s_Data.TextPipelines[s_Data.FramebufferIndex]->BindDescriptor(commandBuffer);

			s_Data.TextVertexBuffer->Bind(commandBuffer);
			s_Data.IndexBuffer->Bind(commandBuffer);

			m_Renderer->DrawIndexed(commandBuffer, s_Data.TextIndexCount);
		}

		if (s_Data.CurrentFramebuffer)
		{
			s_Data.CurrentFramebuffer->EndRenderPass(commandBuffer);
		}
		else
		{
			m_Renderer->EndRenderPass(commandBuffer);
		}
	}

	void Renderer2D::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		const glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

		constexpr glm::vec2 textureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (size_t i = 0; i < 4; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = glm::vec3(transform * glm::vec4(s_Data.QuadVertexPositions[i], 1.0f));
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = 0;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, rbRef<Texture2D> texture)
	{
		const glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

		constexpr glm::vec2 textureCoords[4] = { { 1.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } };

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		int texIndex = -1;
		for (int i = 0; i < 32; i++)
		{
			if (texture == s_Data.TextureSlots[i])
				texIndex = i;
		}

		if (texIndex == -1)
		{
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			for (rbRef<GraphicsPipeline> pipeline : s_Data.QuadPipelines)
				pipeline->UpdateDescriptor(texture, 0, s_Data.TextureSlotIndex);

			texIndex = s_Data.TextureSlotIndex;
			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < 4; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = glm::vec3(transform * glm::vec4(s_Data.QuadVertexPositions[i], 1.0f));
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = texIndex;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;
	}

	void Renderer2D::DrawCircle(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, float thickness, float fade)
	{
		const glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

		for (size_t i = 0; i < 4; i++)
		{
			s_Data.CircleVertexBufferPtr->WorldPosition = glm::vec3(transform * glm::vec4(s_Data.QuadVertexPositions[i], 1.0f));
			s_Data.CircleVertexBufferPtr->LocalPosition = s_Data.QuadVertexPositions[i] * 2.0f;
			s_Data.CircleVertexBufferPtr->Color = color;
			s_Data.CircleVertexBufferPtr->Thickness = thickness;
			s_Data.CircleVertexBufferPtr->Fade = fade;
			s_Data.CircleVertexBufferPtr++;
		}

		s_Data.CircleIndexCount += 6;
	}

	void Renderer2D::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
	{
		s_Data.LineVertexBufferPtr->Position = p0;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexBufferPtr->Position = p1;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexCount += 2;
	}

	void Renderer2D::SetLineWidth(float lineWidth)
	{
		s_Data.LineWidth = lineWidth;
	}

	void Renderer2D::DrawRoundedQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, float cornerRadius, float fade)
	{
		const glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

		for (size_t i = 0; i < 4; i++)
		{
			s_Data.RoundedQuadVertexBufferPtr->WorldPosition = glm::vec3(transform * glm::vec4(s_Data.QuadVertexPositions[i], 1.0f));
			s_Data.RoundedQuadVertexBufferPtr->LocalPosition = s_Data.QuadVertexPositions[i] * 2.0f;
			s_Data.RoundedQuadVertexBufferPtr->Color = color;
			s_Data.RoundedQuadVertexBufferPtr->CornerRadius = cornerRadius;
			s_Data.RoundedQuadVertexBufferPtr->Fade = fade;
			s_Data.RoundedQuadVertexBufferPtr++;
		}

		s_Data.RoundedQuadIndexCount += 6;
	}

	rbRef<CommandBuffer>& Renderer2D::GetCurrentCommandBuffer() const
	{
		return s_Data.CommandBuffers[m_Renderer->GetFrameIndex()];
	}

	void Renderer2D::DrawText(const std::string& text, const glm::mat4& transform, const TextParams& textParams)
	{
		DrawText(text, transform, textParams, s_Data.DefaultFont);
	}

	void Renderer2D::DrawText(const std::string& text, const glm::mat4& transform, const TextParams& textParams, rbRef<Font> font)
	{
		const auto& fontGeometry = font->GetMSDFData()->FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();
		rbRef<Texture2D> fontAtlas = font->GetAtlasTexture();

		double x = 0.0;
		double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
		double y = 0.0;

		const float spaceGlyphAdvance = (float)fontGeometry.getGlyph(' ')->getAdvance();

		uint32_t texIndex = -1;
		for (uint32_t i = 0; i < s_Data.FontSlotIndex; i++)
		{
			if (s_Data.FontSlots[i] == font->GetAtlasTexture())
			{
				texIndex = i;
			}
		}
		
		if (texIndex == -1)
		{
			s_Data.FontSlots[s_Data.FontSlotIndex] = font->GetAtlasTexture();
			for (rbRef<GraphicsPipeline>& pipeline : s_Data.TextPipelines)
				pipeline->UpdateDescriptor(s_Data.FontSlots[s_Data.FontSlotIndex], 0, s_Data.FontSlotIndex);

			texIndex = s_Data.FontSlotIndex;
			s_Data.FontSlotIndex++;
		}

		for (size_t i = 0; i < text.size(); i++)
		{
			char character = text[i];
			if (character == '\r')
				continue;

			if (character == '\n')
			{
				x = 0;
				y -= fsScale * metrics.lineHeight + textParams.LineSpacing;
				continue;
			}

			if (character == ' ')
			{
				float advance = spaceGlyphAdvance;
				if (i < text.size() - 1)
				{
					char nextCharacter = text[i + 1];
					double dAdvance;
					fontGeometry.getAdvance(dAdvance, character, nextCharacter);
					advance = (float)dAdvance;
				}

				x += fsScale * advance + textParams.Kerning;
				continue;
			}

			if (character == '\t')
			{
				x += 4.0f * (fsScale * spaceGlyphAdvance + textParams.Kerning);
				continue;
			}

			auto glyph = fontGeometry.getGlyph(character);
			if (!glyph)
				glyph = fontGeometry.getGlyph('?');
			if (!glyph)
				return;

			double al, ab, ar, at;
			glyph->getQuadAtlasBounds(al, ab, ar, at);
			glm::vec2 texCoordMin((float)al, (float)ab);
			glm::vec2 texCoordMax((float)ar, (float)at);

			double pl, pb, pr, pt;
			glyph->getQuadPlaneBounds(pl, pb, pr, pt);
			glm::vec2 quadMin((float)pl, (float)pb);
			glm::vec2 quadMax((float)pr, (float)pt);

			quadMin *= fsScale, quadMax *= fsScale;
			quadMin += glm::vec2(x, y);
			quadMax += glm::vec2(x, y);

			float texelWidth = 1.0f / fontAtlas->GetWidth();
			float texelHeight = 1.0f / fontAtlas->GetHeight();
			texCoordMin *= glm::vec2(texelWidth, texelHeight);
			texCoordMax *= glm::vec2(texelWidth, texelHeight);

			s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMin, 0.0f, 1.0f);
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->TexCoord = texCoordMin;
			s_Data.TextVertexBufferPtr->FontIndex = texIndex;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMin.x, quadMax.y, 0.0f, 1.0f);
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->TexCoord = { texCoordMin.x, texCoordMax.y };
			s_Data.TextVertexBufferPtr->FontIndex = texIndex;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMax, 0.0f, 1.0f);
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->TexCoord = texCoordMax;
			s_Data.TextVertexBufferPtr->FontIndex = texIndex;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMax.x, quadMin.y, 0.0f, 1.0f);
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->TexCoord = { texCoordMax.x, texCoordMin.y };
			s_Data.TextVertexBufferPtr->FontIndex = texIndex;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextIndexCount += 6;

			if (i < text.size() - 1)
			{
				double advance = glyph->getAdvance();
				char nextCharacter = text[i + 1];
				fontGeometry.getAdvance(advance, character, nextCharacter);

				x += fsScale * advance + textParams.Kerning;
			}
		}
	}

	void Renderer2D::DrawText(const std::wstring& text, const glm::mat4& transform, const TextParams& textParams, rbRef<Font> font)
	{
		const auto& fontGeometry = font->GetMSDFData()->FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();
		rbRef<Texture2D> fontAtlas = font->GetAtlasTexture();

		double x = 0.0;
		double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
		double y = 0.0;

		const float spaceGlyphAdvance = (float)fontGeometry.getGlyph(' ')->getAdvance();

		uint32_t texIndex = -1;
		for (uint32_t i = 0; i < s_Data.FontSlotIndex; i++)
		{
			if (s_Data.FontSlots[i] == font->GetAtlasTexture())
			{
				texIndex = i;
			}
		}

		if (texIndex == -1)
		{
			s_Data.FontSlots[s_Data.FontSlotIndex] = font->GetAtlasTexture();
			for (rbRef<GraphicsPipeline>& pipeline : s_Data.TextPipelines)
				pipeline->UpdateDescriptor(s_Data.FontSlots[s_Data.FontSlotIndex], 0, s_Data.FontSlotIndex);

			texIndex = s_Data.FontSlotIndex;
			s_Data.FontSlotIndex++;
		}

		for (size_t i = 0; i < text.size(); i++)
		{
			wchar_t character = text[i];
			if (character == '\r')
				continue;

			if (character == '\n')
			{
				x = 0;
				y -= fsScale * metrics.lineHeight + textParams.LineSpacing;
				continue;
			}

			if (character == ' ')
			{
				float advance = spaceGlyphAdvance;
				if (i < text.size() - 1)
				{
					wchar_t nextCharacter = text[i + 1];
					double dAdvance;
					fontGeometry.getAdvance(dAdvance, character, nextCharacter);
					advance = (float)dAdvance;
				}

				x += fsScale * advance + textParams.Kerning;
				continue;
			}

			if (character == '\t')
			{
				x += 4.0f * (fsScale * spaceGlyphAdvance + textParams.Kerning);
				continue;
			}

			auto glyph = fontGeometry.getGlyph(character);
			if (!glyph)
				glyph = fontGeometry.getGlyph('?');
			if (!glyph)
				return;

			double al, ab, ar, at;
			glyph->getQuadAtlasBounds(al, ab, ar, at);
			glm::vec2 texCoordMin((float)al, (float)ab);
			glm::vec2 texCoordMax((float)ar, (float)at);

			double pl, pb, pr, pt;
			glyph->getQuadPlaneBounds(pl, pb, pr, pt);
			glm::vec2 quadMin((float)pl, (float)pb);
			glm::vec2 quadMax((float)pr, (float)pt);

			quadMin *= fsScale, quadMax *= fsScale;
			quadMin += glm::vec2(x, y);
			quadMax += glm::vec2(x, y);

			float texelWidth = 1.0f / fontAtlas->GetWidth();
			float texelHeight = 1.0f / fontAtlas->GetHeight();
			texCoordMin *= glm::vec2(texelWidth, texelHeight);
			texCoordMax *= glm::vec2(texelWidth, texelHeight);

			s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMin, 0.0f, 1.0f);
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->TexCoord = texCoordMin;
			s_Data.TextVertexBufferPtr->FontIndex = texIndex;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMin.x, quadMax.y, 0.0f, 1.0f);
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->TexCoord = { texCoordMin.x, texCoordMax.y };
			s_Data.TextVertexBufferPtr->FontIndex = texIndex;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMax, 0.0f, 1.0f);
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->TexCoord = texCoordMax;
			s_Data.TextVertexBufferPtr->FontIndex = texIndex;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMax.x, quadMin.y, 0.0f, 1.0f);
			s_Data.TextVertexBufferPtr->Color = textParams.Color;
			s_Data.TextVertexBufferPtr->TexCoord = { texCoordMax.x, texCoordMin.y };
			s_Data.TextVertexBufferPtr->FontIndex = texIndex;
			s_Data.TextVertexBufferPtr++;

			s_Data.TextIndexCount += 6;

			if (i < text.size() - 1)
			{
				double advance = glyph->getAdvance();
				wchar_t nextCharacter = text[i + 1];
				fontGeometry.getAdvance(advance, character, nextCharacter);

				x += fsScale * advance + textParams.Kerning;
			}
		}
	}

	rbRef<Font> Renderer2D::GetDefaultFont() const
	{
		return s_Data.DefaultFont;
	}

	uint32_t Renderer2D::ReadPixel(uint32_t attachmentIndex, uint32_t x, uint32_t y, rbRef<Framebuffer> framebuffer)
	{
		WR_ASSERT(framebuffer->GetWidth() <= 2560 && framebuffer->GetHeight() <= 1440 && "Framebuffer is too large!");
		WR_ASSERT(x <= framebuffer->GetWidth() && y <= framebuffer->GetHeight());

		framebuffer->CopyAttachmentImageToBuffer(attachmentIndex, s_Data.IDBuffer);

		int* data = static_cast<int*>(s_Data.IDBuffer->Map((uint32_t)sizeof(int) * framebuffer->GetWidth() * framebuffer->GetHeight()));
		int value = data[x + y * framebuffer->GetWidth()];
		s_Data.IDBuffer->Unmap();

		return value;
	}

	void Renderer2D::CreatePipelines(rbRef<Framebuffer> framebuffer)
	{
		PushConstantInfo cameraPushConstantInfo{};
		cameraPushConstantInfo.Size = sizeof(Renderer2DData::CameraData);
		cameraPushConstantInfo.Stage = ShaderStage::Vertex;
		cameraPushConstantInfo.Offset = 0;

		// Quad
		{
			InputLayout quadLayout;
			quadLayout.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" },
				{ ShaderDataType::Float2, "a_TexCoord" },
				{ ShaderDataType::Int,	  "a_TexIndex" }
			};

			quadLayout.PushConstants.push_back(cameraPushConstantInfo);

			ShaderResourceInfo samplerInfo{};
			samplerInfo.ResourceType = ShaderResourceType::CombinedImageSampler;
			samplerInfo.Binding = 0;
			samplerInfo.Stage = ShaderStage::Fragment;
			samplerInfo.ResourceCount = 32;

			quadLayout.ShaderResources.push_back(samplerInfo);

			s_Data.QuadPipelines.push_back(s_Data.QuadShader->CreatePipeline(quadLayout, PrimitiveTopology::TriangleList, framebuffer->IsMultiSampled(), framebuffer));
		}

		// Circle
		{
			InputLayout circleLayout;
			circleLayout.VertexLayout = {
				{ ShaderDataType::Float3, "a_WorldPosition" },
				{ ShaderDataType::Float3, "a_LocalPosition" },
				{ ShaderDataType::Float4, "a_Color" },
				{ ShaderDataType::Float,  "a_Thickness" },
				{ ShaderDataType::Float,  "a_Fade" }
			};

			circleLayout.PushConstants.push_back(cameraPushConstantInfo);

			s_Data.CirclePipelines.push_back(s_Data.CircleShader->CreatePipeline(circleLayout, PrimitiveTopology::TriangleList, framebuffer->IsMultiSampled(), framebuffer));
		}

		// Line
		{
			InputLayout lineLayout;
			lineLayout.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" }
			};

			lineLayout.PushConstants.push_back(cameraPushConstantInfo);

			s_Data.LinePipelines.push_back(s_Data.LineShader->CreatePipeline(lineLayout, PrimitiveTopology::LineList, framebuffer->IsMultiSampled(), framebuffer));
		}

		// Rounded quad
		{
			InputLayout roundedQuadLayout;
			roundedQuadLayout.VertexLayout = {
				{ ShaderDataType::Float3, "a_WorldPosition" },
				{ ShaderDataType::Float3, "a_LocalPosition" },
				{ ShaderDataType::Float4, "a_Color" },
				{ ShaderDataType::Float,  "a_CornerRadius" },
				{ ShaderDataType::Float,  "a_Fade" }
			};

			roundedQuadLayout.PushConstants.push_back(cameraPushConstantInfo);

			s_Data.RoundedQuadPipelines.push_back(s_Data.RoundedQuadShader->CreatePipeline(roundedQuadLayout, PrimitiveTopology::TriangleList, framebuffer->IsMultiSampled(), framebuffer));
		}

		// Text
		{
			InputLayout textLayout;
			textLayout.VertexLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" },
				{ ShaderDataType::Float2, "a_TexCoord" },
				{ ShaderDataType::Int,    "a_FontIndex" }
			};

			textLayout.PushConstants.push_back(cameraPushConstantInfo);

			ShaderResourceInfo textResource{};
			textResource.ResourceType = ShaderResourceType::CombinedImageSampler;
			textResource.Binding = 0;
			textResource.Stage = ShaderStage::Fragment;
			textResource.ResourceCount = 32;

			textLayout.ShaderResources.push_back(textResource);

			s_Data.TextPipelines.push_back(s_Data.TextShader->CreatePipeline(textLayout, PrimitiveTopology::TriangleList, framebuffer->IsMultiSampled(), framebuffer));
		}

		for (uint32_t i = 0; i < s_Data.TextureSlots.size(); i++)
		{
			s_Data.QuadPipelines[s_Data.QuadPipelines.size() - 1]->UpdateDescriptor(s_Data.TextureSlots[i], 0, i);
		}
		for (uint32_t i = 0; i < s_Data.FontSlots.size(); i++)
		{
			s_Data.TextPipelines[s_Data.TextPipelines.size() - 1]->UpdateDescriptor(s_Data.FontSlots[i], 0, i);
		}
	}

}

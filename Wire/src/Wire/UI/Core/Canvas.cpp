#include "Canvas.h"

#include "Wire/Core/Application.h"
#include "Wire/UI/Renderer/MSDFData.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <string>
#include <locale>
#include <codecvt>
#include <string>

namespace wire {

	namespace Utils {

		inline static glm::vec2 getAnchorOffset(UIAnchor anchor)
		{
			switch (anchor)
			{
			case UIAnchor::TopLeft: return { 0.0f, 0.0f };
			case UIAnchor::TopRight: return { 1.0f, 0.0f };
			case UIAnchor::BottomLeft: return { 0.0f, 1.0f };
			case UIAnchor::BottomRight: return { 1.0f, 1.0f };
			case UIAnchor::Centre: return { 0.5f, 0.5f };
			default:
				return { 0.0f, 0.0f };
			}
		}

	}

	struct RendererLimits
	{
		constexpr static size_t MaxRects = 20'000;
		constexpr static size_t MaxVertices = MaxRects * 4;
		constexpr static size_t MaxIndices = MaxRects * 6;
	};

	Canvas::Canvas(Renderer* renderer)
	{
		m_Renderer = renderer;
		m_Data = {};
        
        // create render pass
        RenderPassDesc renderPassDesc{};
        AttachmentDesc colorAttachment{};
        colorAttachment.Format = AttachmentFormat::SwapchainColorDefault;
        colorAttachment.Usage = AttachmentLayout::Present;
        colorAttachment.PreviousAttachmentUsage = AttachmentLayout::Undefined;
        colorAttachment.Samples = AttachmentDesc::Count1Bit;
        colorAttachment.LoadOp = LoadOperation::Clear;
        colorAttachment.StoreOp = StoreOperation::Store;
        colorAttachment.StencilLoadOp = LoadOperation::DontCare;
        colorAttachment.StencilStoreOp = StoreOperation::DontCare;
        
        AttachmentDesc depthAttachment{};
        depthAttachment.Format = AttachmentFormat::SwapchainDepthDefault;
        depthAttachment.Usage = AttachmentLayout::Depth;
        depthAttachment.PreviousAttachmentUsage = AttachmentLayout::Undefined;
        depthAttachment.Samples = AttachmentDesc::Count1Bit;
        depthAttachment.LoadOp = LoadOperation::Clear;
        depthAttachment.StoreOp = StoreOperation::Store;
        depthAttachment.StencilLoadOp = LoadOperation::DontCare;
        depthAttachment.StencilStoreOp = StoreOperation::DontCare;
        
        renderPassDesc.Attachments = { colorAttachment, depthAttachment };
        m_RenderPass = renderer->createRenderPass(renderPassDesc, renderer->getSwapchain());

		uint32_t* indices = new uint32_t[RendererLimits::MaxIndices];

		uint32_t offset = 0;
		for (size_t i = 0; i < RendererLimits::MaxIndices; i += 6)
		{
			indices[i + 0] = offset + 0;
			indices[i + 1] = offset + 1;
			indices[i + 2] = offset + 2;

			indices[i + 3] = offset + 2;
			indices[i + 4] = offset + 3;
			indices[i + 5] = offset + 0;

			offset += 4;
		}

		m_IndexBuffer = renderer->createBuffer<IndexBuffer>(RendererLimits::MaxIndices * sizeof(uint32_t), indices);
		delete[] indices;

		m_RectVertexBuffer = renderer->createBuffer<VertexBuffer>(RendererLimits::MaxVertices * sizeof(RectVertex));

		InputLayout layout;
		layout.VertexBufferLayout = {
			{ "POSITION", ShaderDataType::Float3, sizeof(glm::vec3), offsetof(RectVertex, Position) },
			{ "COLOR", ShaderDataType::Float4, sizeof(glm::vec4), offsetof(RectVertex, Color) },
			{ "TEXCOORD0", ShaderDataType::Float2, sizeof(glm::vec2), offsetof(RectVertex, TexCoord) },
			{ "TEXCOORD1", ShaderDataType::Int, sizeof(int), offsetof(RectVertex, TexIndex) }
		};
		layout.Stride = sizeof(RectVertex);
		layout.PushConstantInfos = {
			{ sizeof(glm::mat4), 0, ShaderType::Vertex }
		};

		ShaderResourceInfo rectImagesResource{};
		rectImagesResource.Binding = 0;
		rectImagesResource.ResourceType = ShaderResourceType::SampledImage;
		rectImagesResource.Shader = ShaderType::Pixel;
		rectImagesResource.ResourceCount = 32;

		ShaderResourceInfo rectSamplerResource{};
		rectSamplerResource.Binding = 1;
		rectSamplerResource.ResourceType = ShaderResourceType::Sampler;
		rectSamplerResource.Shader = ShaderType::Pixel;
		rectSamplerResource.ResourceCount = 1;

		layout.ShaderResources.push_back(rectImagesResource);
		layout.ShaderResources.push_back(rectSamplerResource);

		GraphicsPipelineDesc rectDesc{};
		rectDesc.ShaderPath = "shadercache://UIRect.hlsl";
		rectDesc.Topology = PrimitiveTopology::TriangleList;
		rectDesc.Layout = layout;
        rectDesc.RenderPass = m_RenderPass;
        
		m_RectPipeline = renderer->createGraphicsPipeline(rectDesc);

		SamplerDesc textureSamplerDesc{};
		textureSamplerDesc.MinFilter = SamplerFilter::Linear;
		textureSamplerDesc.MagFilter = SamplerFilter::Linear;
		textureSamplerDesc.AddressModeU = AddressMode::Repeat;
		textureSamplerDesc.AddressModeV = AddressMode::Repeat;
		textureSamplerDesc.AddressModeW = AddressMode::Repeat;
		textureSamplerDesc.BorderColor = BorderColor::IntOpaqueBlack;
		textureSamplerDesc.EnableAnisotropy = true;
		textureSamplerDesc.MaxAnisotropy = renderer->getMaxAnisotropy();

		m_TextureSampler = renderer->createSampler(textureSamplerDesc);
		uint32_t whiteTextureData = 0xFFFFFFFF;
		m_WhiteTexture = renderer->createTexture2D(&whiteTextureData, 1, 1);
		m_Data.RectTextures[m_Data.RectTextureIndex] = m_WhiteTexture;

		m_RectPipeline->updateAllDescriptors(m_Data.RectTextures[m_Data.RectTextureIndex], 0, m_Data.RectTextureIndex);
		m_Data.RectTextureIndex++;

		for (uint32_t i = m_Data.RectTextureIndex; i < 32; i++)
		{
			m_RectPipeline->updateAllDescriptors(m_Data.RectTextures[0], 0, i);
		}
		m_RectPipeline->updateAllDescriptors(m_TextureSampler, 1, 0);

		m_Data.RectVertexBase = new RectVertex[RendererLimits::MaxVertices];

		m_TextVertexBuffer = renderer->createBuffer<VertexBuffer>(RendererLimits::MaxVertices * sizeof(TextVertex));
		m_Data.TextVertexBase = new TextVertex[RendererLimits::MaxVertices];

		layout = {};
		layout.VertexBufferLayout = {
			{ "POSITION", ShaderDataType::Float3, sizeof(glm::vec3), offsetof(TextVertex, Position) },
			{ "COLOR", ShaderDataType::Float4, sizeof(glm::vec4), offsetof(TextVertex, Color) },
			{ "TEXCOORD0", ShaderDataType::Float2, sizeof(glm::vec2), offsetof(TextVertex, TexCoord) },
			{ "TEXCOORD1", ShaderDataType::Int, sizeof(int), offsetof(TextVertex, FontIndex) }
		};
		layout.Stride = sizeof(TextVertex);
		layout.PushConstantInfos = {
			{ sizeof(glm::mat4), 0, ShaderType::Vertex }
		};

		ShaderResourceInfo textImagesResource{};
		textImagesResource.Binding = 0;
		textImagesResource.ResourceType = ShaderResourceType::SampledImage;
		textImagesResource.ResourceCount = 32;
		textImagesResource.Shader = ShaderType::Pixel;

		ShaderResourceInfo textSamplerResource{};
		textSamplerResource.Binding = 1;
		textSamplerResource.ResourceType = ShaderResourceType::Sampler;
		textSamplerResource.ResourceCount = 1;
		textSamplerResource.Shader = ShaderType::Pixel;

		layout.ShaderResources.push_back(textImagesResource);
		layout.ShaderResources.push_back(textSamplerResource);

		GraphicsPipelineDesc textDesc{};
		textDesc.ShaderPath = "shadercache://UIText.hlsl";
		textDesc.Topology = PrimitiveTopology::TriangleList;
		textDesc.Layout = layout;
        textDesc.RenderPass = m_RenderPass;

		m_TextPipeline = renderer->createGraphicsPipeline(textDesc);

		SamplerDesc textSamplerDesc{};
		textSamplerDesc.MinFilter = SamplerFilter::Linear;
		textSamplerDesc.MagFilter = SamplerFilter::Linear;
		textSamplerDesc.AddressModeU = AddressMode::ClampToEdge;
		textSamplerDesc.AddressModeV = AddressMode::ClampToEdge;
		textSamplerDesc.AddressModeW = AddressMode::ClampToEdge;
		textSamplerDesc.BorderColor = BorderColor::IntOpaqueBlack;
		textSamplerDesc.EnableAnisotropy = false;
		textSamplerDesc.MaxAnisotropy = 0.0f;

		m_TextSampler = renderer->createSampler(textSamplerDesc);
		m_TextPipeline->updateAllDescriptors(m_TextSampler, 1, 0);

		m_DefaultFont = renderer->getFontFromCache("fontcache://OpenSans-Bold.ttf");

		m_Data.AtlasTextures[m_Data.AtlasTextureIndex] = m_DefaultFont->getAtlasTexture();
		m_TextPipeline->updateAllDescriptors(m_Data.AtlasTextures[m_Data.AtlasTextureIndex], 0, 0);
		m_Data.AtlasTextureIndex++;

		for (uint32_t i = m_Data.AtlasTextureIndex; i < 32; i++)
		{
			m_TextPipeline->updateAllDescriptors(m_WhiteTexture, 0, i);
		}

		m_CircleVertexBuffer = renderer->createBuffer<VertexBuffer>(RendererLimits::MaxVertices * sizeof(CircleVertex));
		m_Data.CircleVertexBase = new CircleVertex[RendererLimits::MaxVertices];

		layout = {};
		layout.VertexBufferLayout = {
			{ "POSITION", ShaderDataType::Float3, sizeof(glm::vec3), offsetof(CircleVertex, WorldPosition) },
			{ "TEXCOORD0", ShaderDataType::Float3, sizeof(glm::vec3), offsetof(CircleVertex, LocalPosition) },
			{ "COLOR", ShaderDataType::Float4, sizeof(glm::vec4), offsetof(CircleVertex, Color) },
			{ "TEXCOORD1", ShaderDataType::Float, sizeof(float), offsetof(CircleVertex, Thickness) },
			{ "TEXCOORD2", ShaderDataType::Float, sizeof(float), offsetof(CircleVertex, Fade) },
		};
		layout.Stride = sizeof(CircleVertex);
		layout.PushConstantInfos = {
			{ sizeof(glm::mat4), 0, ShaderType::Vertex }
		};

		GraphicsPipelineDesc circleDesc{};
		circleDesc.ShaderPath = "shadercache://UICircle.hlsl";
		circleDesc.Topology = PrimitiveTopology::TriangleList;
		circleDesc.Layout = layout;
        circleDesc.RenderPass = m_RenderPass;

		m_CirclePipeline = renderer->createGraphicsPipeline(circleDesc);

		m_RoundedRectVertexBuffer = renderer->createBuffer<VertexBuffer>(RendererLimits::MaxVertices * sizeof(RoundedRectVertex));
		m_Data.RoundedRectVertexBase = new RoundedRectVertex[RendererLimits::MaxVertices];

		layout = {};
		layout.VertexBufferLayout = {
			{ "POSITION", ShaderDataType::Float3, sizeof(glm::vec3), offsetof(RoundedRectVertex, WorldPosition) },
			{ "TEXCOORD0", ShaderDataType::Float3, sizeof(glm::vec3), offsetof(RoundedRectVertex, LocalPosition) },
			{ "TEXCOORD1", ShaderDataType::Float2, sizeof(glm::vec2), offsetof(RoundedRectVertex, RectSize) },
			{ "COLOR", ShaderDataType::Float4, sizeof(glm::vec4), offsetof(RoundedRectVertex, Color) },
			{ "TEXCOORD2", ShaderDataType::Float, sizeof(float), offsetof(RoundedRectVertex, CornerRadius) },
			{ "TEXCOORD3", ShaderDataType::UInt, sizeof(uint32_t), offsetof(RoundedRectVertex, CornerFlags) },
			{ "TEXCOORD4", ShaderDataType::Float, sizeof(float), offsetof(RoundedRectVertex, Fade) },
		};
		layout.Stride = sizeof(RoundedRectVertex);
		layout.PushConstantInfos = {
			{ sizeof(glm::mat4), 0, ShaderType::Vertex }
		};

		GraphicsPipelineDesc roundedRectDesc{};
		roundedRectDesc.ShaderPath = "shadercache://UIRoundedRect.hlsl";
		roundedRectDesc.Topology = PrimitiveTopology::TriangleList;
		roundedRectDesc.Layout = layout;
        roundedRectDesc.RenderPass = m_RenderPass;

		m_RoundedRectPipeline = renderer->createGraphicsPipeline(roundedRectDesc);

		m_LineVertexBuffer = renderer->createBuffer<VertexBuffer>(RendererLimits::MaxVertices * sizeof(LineVertex));
		m_Data.LineVertexBase = new LineVertex[RendererLimits::MaxVertices];

		layout = {};
		layout.VertexBufferLayout = {
			{ "POSITION", ShaderDataType::Float3, sizeof(glm::vec3), offsetof(LineVertex, Position) },
			{ "COLOR", ShaderDataType::Float4, sizeof(glm::vec4), offsetof(LineVertex, Color) }
		};
		layout.Stride = sizeof(LineVertex);
		layout.PushConstantInfos = {
			{ sizeof(glm::mat4), 0, ShaderType::Vertex }
		};

		GraphicsPipelineDesc lineDesc{};
		lineDesc.ShaderPath = "shadercache://UILine.hlsl";
		lineDesc.Topology = PrimitiveTopology::LineList;
		lineDesc.Layout = layout;
        lineDesc.RenderPass = m_RenderPass;
        
		m_LinePipeline = renderer->createGraphicsPipeline(lineDesc);
		
		m_RendererCommandLists.resize(renderer->getNumFramesInFlight());
		for (uint32_t i = 0; i < renderer->getNumFramesInFlight(); i++)
		{
			m_RendererCommandLists[i] = renderer->createCommandList();
		}

		m_Data.RectVertexPositions[0] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_Data.RectVertexPositions[1] = { 1.0f, 0.0f, 0.0f, 1.0f };
		m_Data.RectVertexPositions[2] = { 1.0f, 1.0f, 0.0f, 1.0f };
		m_Data.RectVertexPositions[3] = { 0.0f, 1.0f, 0.0f, 1.0f };
	}

	void Canvas::release()
	{
		delete[] m_Data.LineVertexBase;
		delete[] m_Data.RoundedRectVertexBase;
		delete[] m_Data.CircleVertexBase;
		delete[] m_Data.TextVertexBase;
		delete[] m_Data.RectVertexBase;

		delete m_LinePipeline;
		delete m_LineVertexBuffer;
		delete m_RoundedRectPipeline;
		delete m_RoundedRectVertexBuffer;
		delete m_CirclePipeline;
		delete m_CircleVertexBuffer;
		delete m_TextPipeline;
		delete m_TextVertexBuffer;
		delete m_DefaultFont;
		delete m_WhiteTexture;
		delete m_TextSampler;
		delete m_TextureSampler;
		delete m_RectPipeline;
		delete m_RectVertexBuffer;
		delete m_IndexBuffer;
        delete m_RenderPass;

		m_RendererCommandLists.clear();

		m_Renderer = nullptr;
	}

	void Canvas::beginFrame()
	{
		m_Data.RectVertexPointer = m_Data.RectVertexBase;
		m_Data.RectIndexCount = 0;
		m_Data.RectVertexCount = 0;
		m_Data.RectVertexOffset = 0;

		m_Data.TextVertexPointer = m_Data.TextVertexBase;
		m_Data.TextIndexCount = 0;
		m_Data.TextVertexCount = 0;
		m_Data.TextVertexOffset = 0;

		m_Data.CircleVertexPointer = m_Data.CircleVertexBase;
		m_Data.CircleIndexCount = 0;
		m_Data.CircleVertexCount = 0;
		m_Data.CircleVertexOffset = 0;

		m_Data.RoundedRectVertexPointer = m_Data.RoundedRectVertexBase;
		m_Data.RoundedRectIndexCount = 0;
		m_Data.RoundedRectVertexCount = 0;
		m_Data.RoundedRectVertexOffset = 0;

		m_Data.LineVertexPointer = m_Data.LineVertexBase;
		m_Data.LineVertexCount = 0;
		m_Data.LineVertexOffset = 0;

		m_RendererCommandLists[m_Renderer->getFrameIndex()].begin();
	}

	void Canvas::flush()
	{
		CommandList& commandList = m_RendererCommandLists[m_Renderer->getFrameIndex()];

		const auto& desc = Application::get().getDesc();
		glm::mat4 viewProj = glm::ortho(0.0f, (float)desc.WindowWidth, (float)desc.WindowHeight, 0.0f);

		if (m_Data.RectIndexCount)
		{
			size_t dataSize = sizeof(RectVertex) * m_Data.RectVertexCount;
			m_RectVertexBuffer->setData(m_Data.RectVertexBase, dataSize, m_Data.RectVertexOffset * sizeof(RectVertex));
		}

		if (m_Data.CircleIndexCount)
		{
			size_t dataSize = sizeof(CircleVertex) * m_Data.CircleVertexCount;
			m_CircleVertexBuffer->setData(m_Data.CircleVertexBase, dataSize, m_Data.CircleVertexOffset * sizeof(CircleVertex));
		}

		if (m_Data.RoundedRectIndexCount)
		{
			size_t dataSize = sizeof(RoundedRectVertex) * m_Data.RoundedRectVertexCount;
			m_RoundedRectVertexBuffer->setData(m_Data.RoundedRectVertexBase, dataSize, m_Data.RoundedRectVertexOffset * sizeof(RoundedRectVertex));
		}

		if (m_Data.LineVertexCount)
		{
			size_t dataSize = sizeof(LineVertex) * m_Data.LineVertexCount;
			m_LineVertexBuffer->setData(m_Data.LineVertexBase, dataSize, m_Data.LineVertexOffset * sizeof(LineVertex));
		}

		if (m_Data.TextIndexCount)
		{
			size_t dataSize = sizeof(TextVertex) * m_Data.TextVertexCount;
			m_TextVertexBuffer->setData(m_Data.TextVertexBase, dataSize, m_Data.TextVertexOffset * sizeof(TextVertex));
		}

		glm::vec2 extent = m_Renderer->getExtent();

		commandList.beginRenderPass(m_RenderPass);

		if (m_Data.RectIndexCount)
		{
			commandList.bindPipeline(m_RectPipeline);

			commandList.setViewport(
				{ 0.0f, extent.y },
				{ extent.x, -extent.y },
				0.0f, 1.0f
			);

			commandList.setScissor(
				{ 0.0f, 0.0f },
				{ extent.x, extent.y }
			);

			commandList.pushConstants(ShaderType::Vertex, viewProj);
            commandList.bindDescriptorSet(0, 0);
			commandList.bindVertexBuffers({ m_RectVertexBuffer->getBase() });
			commandList.bindIndexBuffer(m_IndexBuffer->getBase());

			// apply scissors
			if (!m_Data.Scissors.empty())
			{
				const Rect& scissor = m_Data.Scissors.back();
				commandList.setScissor(scissor.Min, scissor.Max);
			}

			commandList.drawIndexed((uint32_t)m_Data.RectIndexCount, (uint32_t)m_Data.RectVertexOffset);
		}

		if (m_Data.CircleIndexCount)
		{
			commandList.bindPipeline(m_CirclePipeline);

			commandList.setViewport(
				{ 0.0f, extent.y },
				{ extent.x, -extent.y },
				0.0f, 1.0f
			);

			commandList.setScissor(
				{ 0.0f, 0.0f },
				{ extent.x, extent.y }
			);

			commandList.pushConstants(ShaderType::Vertex, viewProj);
			commandList.bindVertexBuffers({ m_CircleVertexBuffer->getBase() });
			commandList.bindIndexBuffer(m_IndexBuffer->getBase());

			// apply scissors
			if (!m_Data.Scissors.empty())
			{
				const Rect& scissor = m_Data.Scissors.back();
				commandList.setScissor(scissor.Min, scissor.Max);
			}

			commandList.drawIndexed((uint32_t)m_Data.CircleIndexCount, (uint32_t)m_Data.CircleVertexOffset);
		}

		if (m_Data.RoundedRectIndexCount)
		{
			commandList.bindPipeline(m_RoundedRectPipeline);

			commandList.setViewport(
				{ 0.0f, extent.y },
				{ extent.x, -extent.y },
				0.0f, 1.0f
			);

			commandList.setScissor(
				{ 0.0f, 0.0f },
				{ extent.x, extent.y }
			);

			commandList.pushConstants(ShaderType::Vertex, viewProj);
			commandList.bindVertexBuffers({ m_RoundedRectVertexBuffer->getBase() });
			commandList.bindIndexBuffer(m_IndexBuffer->getBase());

			// apply scissors
			if (!m_Data.Scissors.empty())
			{
				const Rect& scissor = m_Data.Scissors.back();
				commandList.setScissor(scissor.Min, scissor.Max);
			}

			commandList.drawIndexed((uint32_t)m_Data.RoundedRectIndexCount, (uint32_t)m_Data.RoundedRectVertexOffset);
		}

		if (m_Data.LineVertexCount)
		{
			commandList.bindPipeline(m_LinePipeline);

			commandList.setViewport(
				{ 0.0f, extent.y },
				{ extent.x, -extent.y },
				0.0f, 1.0f
			);

			commandList.setScissor(
				{ 0.0f, 0.0f },
				{ extent.x, extent.y }
			);

			commandList.pushConstants(ShaderType::Vertex, viewProj);
			commandList.setLineWidth(1.0f);
			commandList.bindVertexBuffers({ m_LineVertexBuffer->getBase() });

			// apply scissors
			if (!m_Data.Scissors.empty())
			{
				const Rect& scissor = m_Data.Scissors.back();
				commandList.setScissor(scissor.Min, scissor.Max);
			}

			commandList.draw((uint32_t)m_Data.LineVertexCount, (uint32_t)m_Data.LineVertexOffset);
		}

		if (m_Data.TextIndexCount)
		{
			commandList.bindPipeline(m_TextPipeline);

			commandList.setViewport(
				{ 0.0f, extent.y },
				{ extent.x, -extent.y },
				0.0f, 1.0f
			);

			commandList.setScissor(
				{ 0.0f, 0.0f },
				{ extent.x, extent.y }
			);

			commandList.pushConstants(ShaderType::Vertex, viewProj);
			commandList.bindDescriptorSet(0, 0);
			commandList.bindVertexBuffers({ m_TextVertexBuffer->getBase() });
			commandList.bindIndexBuffer(m_IndexBuffer->getBase());

			// apply scissors
			if (!m_Data.Scissors.empty())
			{
				const Rect& scissor = m_Data.Scissors.back();
				commandList.setScissor(scissor.Min, scissor.Max);
			}

			commandList.drawIndexed((uint32_t)m_Data.TextIndexCount, (uint32_t)m_Data.TextVertexOffset);
		}

		commandList.endRenderPass();

		m_Data.RectVertexOffset += m_Data.RectVertexCount;
		m_Data.RectVertexPointer = m_Data.RectVertexBase;
		m_Data.RectIndexCount = 0;
		m_Data.RectVertexCount = 0;

		m_Data.TextVertexOffset += m_Data.TextVertexCount;
		m_Data.TextVertexPointer = m_Data.TextVertexBase;
		m_Data.TextIndexCount = 0;
		m_Data.TextVertexCount = 0;

		m_Data.CircleVertexOffset += m_Data.CircleVertexCount;
		m_Data.CircleVertexPointer = m_Data.CircleVertexBase;
		m_Data.CircleIndexCount = 0;
		m_Data.CircleVertexCount = 0;

		m_Data.RoundedRectVertexOffset += m_Data.RoundedRectVertexCount;
		m_Data.RoundedRectVertexPointer = m_Data.RoundedRectVertexBase;
		m_Data.RoundedRectIndexCount = 0;
		m_Data.RoundedRectVertexCount = 0;

		m_Data.LineVertexOffset += m_Data.LineVertexCount;
		m_Data.LineVertexPointer = m_Data.LineVertexBase;
		m_Data.LineVertexCount = 0;
	}

	void Canvas::endFrame()
	{
		uint32_t frameIndex = m_Renderer->getFrameIndex();
		CommandList& commandList = m_RendererCommandLists[frameIndex];

		flush();

		commandList.end();
		m_Renderer->submitCommandList(commandList);
	}

	void Canvas::drawRect(const glm::mat4& transform, const glm::vec4& color)
	{
		constexpr glm::vec2 textureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		for (size_t i = 0; i < 4; i++)
		{
			glm::vec4 vertexPos = m_Data.RectVertexPositions[i];
			vertexPos.z = -0.00002f;

			m_Data.RectVertexPointer->Position = glm::vec3(transform * vertexPos);
			m_Data.RectVertexPointer->Color = color;
			m_Data.RectVertexPointer->TexCoord = textureCoords[i];
			m_Data.RectVertexPointer->TexIndex = 0;
			m_Data.RectVertexPointer++;
		}

		m_Data.RectIndexCount += 6;
		m_Data.RectVertexCount += 4;
	}

	void Canvas::drawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

		drawRect(transform, color);
	}

	void Canvas::drawRect(const Rect& bounds, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(bounds.Min, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(bounds.Max - bounds.Min, 1.0f));

		drawRect(transform, color);
	}

	void Canvas::drawRect(const glm::mat4& transform, Texture2D* texture, const glm::vec4& color)
	{
		constexpr glm::vec2 textureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		int textureIndex = -1;
		for (uint32_t i = 0; i < m_Data.RectTextureIndex; i++)
		{
			if (texture->getUUID() == m_Data.RectTextures[i]->getUUID())
			{
				textureIndex = i;
				break;
			}
		}
		if (textureIndex == -1)
		{
			m_Data.RectTextures[m_Data.RectTextureIndex] = texture;
			m_RectPipeline->updateAllDescriptors(m_Data.RectTextures[m_Data.RectTextureIndex], 0, m_Data.RectTextureIndex);
			textureIndex = m_Data.RectTextureIndex;

			m_Data.RectTextureIndex++;
		}

		for (size_t i = 0; i < 4; i++)
		{
			m_Data.RectVertexPointer->Position = glm::vec3(transform * m_Data.RectVertexPositions[i]);
			m_Data.RectVertexPointer->Color = color;
			m_Data.RectVertexPointer->TexCoord = textureCoords[i];
			m_Data.RectVertexPointer->TexIndex = textureIndex;
			m_Data.RectVertexPointer++;
		}

		m_Data.RectIndexCount += 6;
		m_Data.RectVertexCount += 4;
	}

	void Canvas::drawRect(const glm::vec2& position, const glm::vec2& size, Texture2D* texture, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

		drawRect(transform, texture, color);
	}

	void Canvas::drawRect(const Rect& bounds, Texture2D* texture, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(bounds.Min, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(bounds.Max - bounds.Min, 1.0f));

		drawRect(transform, texture, color);
	}

	void Canvas::drawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, UIAnchor anchor)
	{
		glm::vec2 offset = Utils::getAnchorOffset(anchor) * size;
		glm::vec2 adjustedPos = position - offset;

		drawRect(adjustedPos, size, color);
	}


	void Canvas::drawRect(const Rect& bounds, const glm::vec4& color, UIAnchor anchor)
	{
		glm::vec2 size = bounds.Max - bounds.Min;
		glm::vec2 offset = Utils::getAnchorOffset(anchor) * size;
		glm::vec2 adjustedPos = bounds.Min - offset;

		drawRect(adjustedPos, size, color);
	}

	void Canvas::drawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness, float fade)
	{
		for (size_t i = 0; i < 4; i++)
		{
			glm::vec3 localPosition = (m_Data.RectVertexPositions[i] - 0.5f) * 2.0f;
			localPosition.z = 0.0f;

			m_Data.CircleVertexPointer->WorldPosition = glm::vec3(transform * m_Data.RectVertexPositions[i]);
			m_Data.CircleVertexPointer->LocalPosition = localPosition;
			m_Data.CircleVertexPointer->Color = color;
			m_Data.CircleVertexPointer->Thickness = thickness;
			m_Data.CircleVertexPointer->Fade = fade;
			m_Data.CircleVertexPointer++;
		}

		m_Data.CircleIndexCount += 6;
		m_Data.CircleVertexCount += 4;
	}

	void Canvas::drawCircle(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, float thickness, float fade)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), { position.x, position.y, 0.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		drawCircle(transform, color, thickness, fade);
	}

	void Canvas::drawCircle(const Rect& bounds, const glm::vec4& color, float thickness, float fade)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(bounds.Min, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(bounds.Max - bounds.Min, 1.0f));

		drawCircle(transform, color, thickness, fade);
	}

	void Canvas::drawRoundedRect(const glm::mat4& transform, const glm::vec4& color, float cornerRadius, RoundedRectFlags flags, float fade)
	{
		glm::vec2 rectSize = {
			glm::length(glm::vec2(transform[0])),
			glm::length(glm::vec2(transform[1]))
		};

		for (size_t i = 0; i < 4; i++)
		{
			glm::vec3 localPosition = (m_Data.RectVertexPositions[i] - 0.5f) * 2.0f;
			localPosition.z = 0.0f;

			glm::vec4 vertexPos = m_Data.RectVertexPositions[i];
			vertexPos.z = -0.00001f;

			m_Data.RoundedRectVertexPointer->WorldPosition = glm::vec3(transform * vertexPos);
			m_Data.RoundedRectVertexPointer->LocalPosition = localPosition;
			m_Data.RoundedRectVertexPointer->RectSize = rectSize;
			m_Data.RoundedRectVertexPointer->Color = color;
			m_Data.RoundedRectVertexPointer->CornerRadius = cornerRadius;
			m_Data.RoundedRectVertexPointer->CornerFlags = (uint32_t)flags;
			m_Data.RoundedRectVertexPointer->Fade = fade;
			m_Data.RoundedRectVertexPointer++;
		}

		m_Data.RoundedRectIndexCount += 6;
		m_Data.RoundedRectVertexCount += 4;
	}

	void Canvas::drawRoundedRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, float cornerRadius, RoundedRectFlags flags, float fade)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), { position.x, position.y, 0.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		drawRoundedRect(transform, color, cornerRadius, flags, fade);
	}

	void Canvas::drawRoundedRect(const Rect& bounds, const glm::vec4& color, float cornerRadius, RoundedRectFlags flags, float fade)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(bounds.Min, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(bounds.Max - bounds.Min, 1.0f));

		drawRoundedRect(transform, color, cornerRadius, flags, fade);
	}

	void Canvas::pushScissor(const Rect& clipBounds)
	{
		flush();

		m_Data.Scissors.push_back(clipBounds);
	}

	void Canvas::popScissor()
	{
		flush();

		m_Data.Scissors.pop_back();
	}

	void Canvas::drawText(const std::string& text, const glm::mat4& transform, const TextParams& params)
	{
		drawText(text, transform, params, m_DefaultFont);
	}

	void Canvas::drawText(const std::wstring& text, const glm::mat4& transform, const TextParams& params)
	{
		drawText(text, transform, params, m_DefaultFont);
	}

	void Canvas::drawText(const std::string& text, const glm::mat4& transform, const Canvas::TextParams& params, Font* font)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wide = converter.from_bytes(text);

		drawText(wide, transform, params, font);
	}

	void Canvas::drawText(const std::wstring& text, const glm::mat4& transform, const TextParams& params, Font* font)
	{
		glm::mat4 newTransform = transform * glm::scale(glm::mat4(1.0f), { 1.0f, -1.0f, 1.0f });

		const auto& fontGeometry = font->getMSDFData().FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();
		Texture2D* fontAtlas = font->getAtlasTexture();

		double x = 0.0;
		double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
		double y = 0.0;

		const float spaceGlyphAdvance = (float)fontGeometry.getGlyph(L' ')->getAdvance();

		int texIndex = -1;
		for (int i = 0; i < (int)m_Data.AtlasTextureIndex; i++)
		{
			if (m_Data.AtlasTextures[i]->getUUID() == fontAtlas->getUUID())
			{
				texIndex = i;
				break;
			}
		}

		if (texIndex == -1)
		{
			m_Data.AtlasTextures[m_Data.AtlasTextureIndex] = fontAtlas;
			m_TextPipeline->updateAllDescriptors(m_Data.AtlasTextures[m_Data.AtlasTextureIndex], 0, m_Data.AtlasTextureIndex);
			texIndex = m_Data.AtlasTextureIndex;

			m_Data.AtlasTextureIndex++;
		}

		for (size_t i = 0; i < text.size(); i++)
		{
			wchar_t character = text[i];
			if (character == L'\r')
				continue;

			if (character == L'\n')
			{
				x = 0;
				y -= fsScale * metrics.lineHeight + params.LineSpacing;
				continue;
			}

			if (character == L' ')
			{
				float advance = spaceGlyphAdvance;
				if (i < text.size() - 1)
				{
					wchar_t nextCharacter = text[i + 1];
					double dAdvance;
					fontGeometry.getAdvance(dAdvance, character, nextCharacter);
					advance = (float)dAdvance;
				}

				x += fsScale * advance + params.Kerning;
				continue;
			}

			if (character == L'\t')
			{
				x += 4.0f * (fsScale * spaceGlyphAdvance + params.Kerning);
				continue;
			}

			auto glyph = fontGeometry.getGlyph(character);
			if (!glyph)
				glyph = fontGeometry.getGlyph(L'?');
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

			float texelWidth = 1.0f / fontAtlas->getWidth();
			float texelHeight = 1.0f / fontAtlas->getHeight();
			texCoordMin *= glm::vec2(texelWidth, texelHeight);
			texCoordMax *= glm::vec2(texelWidth, texelHeight);

			m_Data.TextVertexPointer->Position = newTransform * glm::vec4(quadMin, 0.0f, 1.0f);
			m_Data.TextVertexPointer->Color = params.Color;
			m_Data.TextVertexPointer->TexCoord = { texCoordMin.x, texCoordMin.y };
			m_Data.TextVertexPointer->FontIndex = texIndex;
			m_Data.TextVertexPointer++;

			m_Data.TextVertexPointer->Position = newTransform * glm::vec4(quadMin.x, quadMax.y, 0.0f, 1.0f);
			m_Data.TextVertexPointer->Color = params.Color;
			m_Data.TextVertexPointer->TexCoord = { texCoordMin.x, texCoordMax.y };
			m_Data.TextVertexPointer->FontIndex = texIndex;
			m_Data.TextVertexPointer++;

			m_Data.TextVertexPointer->Position = newTransform * glm::vec4(quadMax, 0.0f, 1.0f);
			m_Data.TextVertexPointer->Color = params.Color;
			m_Data.TextVertexPointer->TexCoord = { texCoordMax.x, texCoordMax.y };
			m_Data.TextVertexPointer->FontIndex = texIndex;
			m_Data.TextVertexPointer++;

			m_Data.TextVertexPointer->Position = newTransform * glm::vec4(quadMax.x, quadMin.y, 0.0f, 1.0f);
			m_Data.TextVertexPointer->Color = params.Color;
			m_Data.TextVertexPointer->TexCoord = { texCoordMax.x, texCoordMin.y };
			m_Data.TextVertexPointer->FontIndex = texIndex;
			m_Data.TextVertexPointer++;

			m_Data.TextIndexCount += 6;
			m_Data.TextVertexCount += 4;

			if (i < text.size() - 1)
			{
				double advance = glyph->getAdvance();
				wchar_t nextCharacter = text[i + 1];
				fontGeometry.getAdvance(advance, character, nextCharacter);

				x += fsScale * advance + params.Kerning;
			}
		}
	}

	void Canvas::drawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
	{
		m_Data.LineVertexPointer->Position = p0;
		m_Data.LineVertexPointer->Color = color;
		m_Data.LineVertexPointer++;

		m_Data.LineVertexPointer->Position = p1;
		m_Data.LineVertexPointer->Color = color;
		m_Data.LineVertexPointer++;

		m_Data.LineVertexCount += 2;
	}

	glm::vec2 Canvas::calculateTextSize(const std::string& text, const glm::mat4& transform, const TextParams& params)
	{
		return calculateTextSize(text, transform, params, m_DefaultFont);
	}

	glm::vec2 Canvas::calculateTextSize(const std::wstring& text, const glm::mat4& transform, const TextParams& params)
	{
		return calculateTextSize(text, transform, params, m_DefaultFont);
	}

	glm::vec2 Canvas::calculateTextSize(const std::string& text, const glm::mat4& transform, const TextParams& params, Font* font)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wide = converter.from_bytes(text);

		return calculateTextSize(wide, transform, params, font);
	}

	glm::vec2 Canvas::calculateTextSize(const std::wstring& text, const glm::mat4& transform, const TextParams& params, Font* font)
	{
		const auto& fontGeometry = font->getMSDFData().FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();

		double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
		double x = 0.0;
		double y = 0.0;

		const float spaceGlyphAdvance = (float)fontGeometry.getGlyph(L' ')->getAdvance();

		glm::mat4 newTransform = transform * glm::scale(glm::mat4(1.0f), { 1.0f, -1.0f, 1.0f });

		bool first = true;
		glm::vec2 minBounds(0.0f), maxBounds(0.0f);

		for (size_t i = 0; i < text.size(); ++i)
		{
			wchar_t character = text[i];
			if (character == L'\r') continue;

			if (character == L'\n')
			{
				x = 0.0;
				y -= fsScale * metrics.lineHeight + params.LineSpacing;
				continue;
			}

			if (character == L' ')
			{
				float advance = spaceGlyphAdvance;
				if (i < text.size() - 1)
				{
					wchar_t nextCharacter = text[i + 1];
					double dAdvance;
					fontGeometry.getAdvance(dAdvance, character, nextCharacter);
					advance = (float)dAdvance;
				}

				x += fsScale * advance + params.Kerning;
				continue;
			}

			if (character == L'\t')
			{
				x += 4.0f * (fsScale * spaceGlyphAdvance + params.Kerning);
				continue;
			}

			auto glyph = fontGeometry.getGlyph(character);
			if (!glyph)
				glyph = fontGeometry.getGlyph(L'?');
			if (!glyph)
				continue;

			double pl, pb, pr, pt;
			glyph->getQuadPlaneBounds(pl, pb, pr, pt);
			glm::vec2 quadMin((float)pl, (float)pb);
			glm::vec2 quadMax((float)pr, (float)pt);

			quadMin *= fsScale; quadMax *= fsScale;
			quadMin += glm::vec2(x, y);
			quadMax += glm::vec2(x, y);

			// Transform the corners
			glm::vec4 corners[4] = {
				newTransform * glm::vec4(quadMin.x, quadMin.y, 0.0f, 1.0f),
				newTransform * glm::vec4(quadMin.x, quadMax.y, 0.0f, 1.0f),
				newTransform * glm::vec4(quadMax.x, quadMax.y, 0.0f, 1.0f),
				newTransform * glm::vec4(quadMax.x, quadMin.y, 0.0f, 1.0f),
			};

			for (const auto& c : corners)
			{
				glm::vec2 pt2D(c.x, c.y);
				if (first)
				{
					minBounds = pt2D;
					maxBounds = pt2D;
					first = false;
				}
				else
				{
					minBounds = glm::min(minBounds, pt2D);
					maxBounds = glm::max(maxBounds, pt2D);
				}
			}

			if (i < text.size() - 1)
			{
				double advance = glyph->getAdvance();
				wchar_t nextCharacter = text[i + 1];
				fontGeometry.getAdvance(advance, character, nextCharacter);

				x += fsScale * advance + params.Kerning;
			}
		}

		glm::vec4 endPoint = newTransform * glm::vec4(x, y, 0.0f, 1.0f);
		glm::vec2 end2D(endPoint.x, endPoint.y);
		minBounds = glm::min(minBounds, end2D);
		maxBounds = glm::max(maxBounds, end2D);

		return maxBounds - minBounds;
	}

	glm::vec2 Canvas::calculateCharacterSize(char c, const glm::mat4& transform)
	{
		return calculateCharacterSize(static_cast<wchar_t>(c), transform, m_DefaultFont);
	}

	glm::vec2 Canvas::calculateCharacterSize(wchar_t c, const glm::mat4& transform)
	{
		return calculateCharacterSize(c, transform, m_DefaultFont);
	}

	glm::vec2 Canvas::calculateCharacterSize(char c, const glm::mat4& transform, Font* font)
	{
		return calculateCharacterSize(static_cast<wchar_t>(c), transform, font);
	}

	glm::vec2 Canvas::calculateCharacterSize(wchar_t c, const glm::mat4& transform, Font* font)
	{
		const auto& fontGeometry = font->getMSDFData().FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();

		double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);

		auto glyph = fontGeometry.getGlyph(c);
		if (!glyph)
			glyph = fontGeometry.getGlyph(L'?');
		if (!glyph)
			return glm::vec2(0.0f);

		double pl, pb, pr, pt;
		glyph->getQuadPlaneBounds(pl, pb, pr, pt);

		glm::vec2 quadMin((float)pl, (float)pb);
		glm::vec2 quadMax((float)pr, (float)pt);

		quadMin *= fsScale;
		quadMax *= fsScale;

		glm::mat4 newTransform = transform * glm::scale(glm::mat4(1.0f), { 1.0f, -1.0f, 1.0f });

		glm::vec4 corners[4] = {
			newTransform * glm::vec4(quadMin.x, quadMin.y, 0.0f, 1.0f),
			newTransform * glm::vec4(quadMin.x, quadMax.y, 0.0f, 1.0f),
			newTransform * glm::vec4(quadMax.x, quadMax.y, 0.0f, 1.0f),
			newTransform * glm::vec4(quadMax.x, quadMin.y, 0.0f, 1.0f),
		};

		glm::vec2 minBounds(corners[0].x, corners[0].y);
		glm::vec2 maxBounds(corners[0].x, corners[0].y);

		for (int i = 1; i < 4; ++i)
		{
			glm::vec2 pt2D(corners[i].x, corners[i].y);
			minBounds = glm::min(minBounds, pt2D);
			maxBounds = glm::max(maxBounds, pt2D);
		}

		return maxBounds - minBounds;
	}

}

module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <string>
#include <locale>
#include <codecvt>
#include <string>

module wire.ui.core:uiRenderer;

import wire.core;

namespace wire {

	struct RendererLimits
	{
		constexpr static size_t MaxRects = 20'000;
		constexpr static size_t MaxVertices = MaxRects * 4;
		constexpr static size_t MaxIndices = MaxRects * 6;
	};

	struct RectVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		int TexIndex;
	};

	struct TextVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		int FontIndex;
	};

	struct CircleVertex
	{
		glm::vec3 WorldPosition;
		glm::vec3 LocalPosition;
		glm::vec4 Color;
		float Thickness;
		float Fade;
	};

	struct RoundedRectVertex
	{
		glm::vec3 WorldPosition;
		glm::vec3 LocalPosition;
		glm::vec4 Color;
		float Thickness;
		float Fade;
	};

	struct RendererData
	{
		RectVertex* RectVertexBase = nullptr;
		RectVertex* RectVertexPointer = nullptr;
		size_t RectIndexCount = 0;
		size_t RectVertexCount = 0;

		std::array<Texture2D*, 32> RectTextures;
		uint32_t RectTextureIndex = 0;

		TextVertex* TextVertexBase = nullptr;
		TextVertex* TextVertexPointer = nullptr;
		size_t TextIndexCount = 0;
		size_t TextVertexCount = 0;

		std::array<Texture2D*, 32> AtlasTextures;
		uint32_t AtlasTextureIndex = 0;

		CircleVertex* CircleVertexBase = nullptr;
		CircleVertex* CircleVertexPointer = nullptr;
		size_t CircleIndexCount = 0;
		size_t CircleVertexCount = 0;

		RoundedRectVertex* RoundedRectVertexBase = nullptr;
		RoundedRectVertex* RoundedRectVertexPointer = nullptr;
		size_t RoundedRectIndexCount = 0;
		size_t RoundedRectVertexCount = 0;

		glm::vec4 RectVertexPositions[4];
	};

	static RendererData s_Data;

	void UIRenderer::init(Renderer* renderer)
	{
		s_Renderer = renderer;
		s_Data = {};

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

		s_IndexBuffer = renderer->createIndexBuffer(RendererLimits::MaxIndices * sizeof(uint32_t), indices);
		delete[] indices;

		s_RectVertexBuffer = renderer->createVertexBuffer(RendererLimits::MaxVertices * sizeof(RectVertex));
		
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

		s_RectPipeline = renderer->createGraphicsPipeline(rectDesc);

		SamplerDesc textureSamplerDesc{};
		textureSamplerDesc.MinFilter = SamplerFilter::Linear;
		textureSamplerDesc.MagFilter = SamplerFilter::Linear;
		textureSamplerDesc.AddressModeU = AddressMode::Repeat;
		textureSamplerDesc.AddressModeV = AddressMode::Repeat;
		textureSamplerDesc.AddressModeW = AddressMode::Repeat;
		textureSamplerDesc.BorderColor = BorderColor::IntOpaqueBlack;
		textureSamplerDesc.EnableAnisotropy = true;
		textureSamplerDesc.MaxAnisotropy = renderer->getMaxAnisotropy();

		s_TextureSampler = renderer->createSampler(textureSamplerDesc);
		uint32_t whiteTextureData = 0xFFFFFFFF;
		s_WhiteTexture = renderer->createTexture2D(&whiteTextureData, 1, 1);
		s_Data.RectTextures[s_Data.RectTextureIndex] = s_WhiteTexture;

		s_RectPipeline->updateDescriptor(s_Data.RectTextures[s_Data.RectTextureIndex], 0, s_Data.RectTextureIndex);
		s_Data.RectTextureIndex++;

		for (uint32_t i = s_Data.RectTextureIndex; i < 32; i++)
		{
			s_RectPipeline->updateDescriptor(s_Data.RectTextures[0], 0, i);
		}
		s_RectPipeline->updateDescriptor(s_TextureSampler, 1, 0);

		s_Data.RectVertexBase = new RectVertex[RendererLimits::MaxVertices];

		s_TextVertexBuffer = renderer->createVertexBuffer(RendererLimits::MaxVertices * sizeof(TextVertex));
		s_Data.TextVertexBase = new TextVertex[RendererLimits::MaxVertices];

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

		s_TextPipeline = renderer->createGraphicsPipeline(textDesc);

		SamplerDesc textSamplerDesc{};
		textSamplerDesc.MinFilter = SamplerFilter::Linear;
		textSamplerDesc.MagFilter = SamplerFilter::Linear;
		textSamplerDesc.AddressModeU = AddressMode::ClampToEdge;
		textSamplerDesc.AddressModeV = AddressMode::ClampToEdge;
		textSamplerDesc.AddressModeW = AddressMode::ClampToEdge;
		textSamplerDesc.BorderColor = BorderColor::IntOpaqueBlack;
		textSamplerDesc.EnableAnisotropy = false;
		textSamplerDesc.MaxAnisotropy = 0.0f;

		s_TextSampler = renderer->createSampler(textSamplerDesc);
		s_TextPipeline->updateDescriptor(s_TextSampler, 1, 0);

		s_DefaultFont = renderer->getFontFromCache("fontcache://OpenSans-Bold.ttf");

		s_Data.AtlasTextures[s_Data.AtlasTextureIndex] = s_DefaultFont->getAtlasTexture();
		s_TextPipeline->updateDescriptor(s_Data.AtlasTextures[s_Data.AtlasTextureIndex], 0, 0);
		s_Data.AtlasTextureIndex++;

		for (uint32_t i = s_Data.AtlasTextureIndex; i < 32; i++)
		{
			s_TextPipeline->updateDescriptor(s_WhiteTexture, 0, i);
		}

		s_CircleVertexBuffer = renderer->createVertexBuffer(RendererLimits::MaxVertices * sizeof(CircleVertex));
		s_Data.CircleVertexBase = new CircleVertex[RendererLimits::MaxVertices];

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

		s_CirclePipeline = renderer->createGraphicsPipeline(circleDesc);

		s_RendererCommandBuffers.resize(renderer->getNumFramesInFlight());
		for (uint32_t i = 0; i < renderer->getNumFramesInFlight(); i++)
		{
			s_RendererCommandBuffers[i] = renderer->allocateCommandBuffer();
		}

		s_Data.RectVertexPositions[0] = { 0.0f, 0.0f, 0.0f, 1.0f };
		s_Data.RectVertexPositions[1] = { 1.0f, 0.0f, 0.0f, 1.0f };
		s_Data.RectVertexPositions[2] = { 1.0f, 1.0f, 0.0f, 1.0f };
		s_Data.RectVertexPositions[3] = { 0.0f, 1.0f, 0.0f, 1.0f };
	}

	void UIRenderer::shutdown()
	{
		delete[] s_Data.CircleVertexBase;
		delete[] s_Data.TextVertexBase;
		delete[] s_Data.RectVertexBase;

		delete s_CirclePipeline;
		delete s_CircleVertexBuffer;
		delete s_TextPipeline;
		delete s_TextVertexBuffer;
		delete s_DefaultFont;
		delete s_WhiteTexture;
		delete s_TextSampler;
		delete s_TextureSampler;
		delete s_RectPipeline;
		delete s_RectVertexBuffer;
		delete s_IndexBuffer;

		s_RendererCommandBuffers.clear();

		s_Renderer = nullptr;
	}

	void UIRenderer::beginFrame()
	{
		s_Data.RectVertexPointer = s_Data.RectVertexBase;
		s_Data.RectIndexCount = 0;
		s_Data.RectVertexCount = 0;

		s_Data.TextVertexPointer = s_Data.TextVertexBase;
		s_Data.TextIndexCount = 0;
		s_Data.TextVertexCount = 0;

		s_Data.CircleVertexPointer = s_Data.CircleVertexBase;
		s_Data.CircleIndexCount = 0;
		s_Data.CircleVertexCount = 0;
	}

	void UIRenderer::endFrame()
	{
		uint32_t frameIndex = s_Renderer->getFrameIndex();

		const auto& desc = Application::get().getDesc();
		glm::mat4 viewProj = glm::ortho(0.0f, (float)desc.WindowWidth, (float)desc.WindowHeight, 0.0f);

		if (s_Data.RectIndexCount)
		{
			size_t dataSize = sizeof(RectVertex) * s_Data.RectVertexCount;
			s_RectVertexBuffer->setData(s_Data.RectVertexBase, dataSize);
		}

		if (s_Data.TextIndexCount)
		{
			size_t dataSize = sizeof(TextVertex) * s_Data.TextVertexCount;
			s_TextVertexBuffer->setData(s_Data.TextVertexBase, dataSize);
		}

		if (s_Data.CircleIndexCount)
		{
			size_t dataSize = sizeof(CircleVertex) * s_Data.CircleVertexCount;
			s_CircleVertexBuffer->setData(s_Data.CircleVertexBase, dataSize);
		}

		s_Renderer->beginCommandBuffer(s_RendererCommandBuffers[frameIndex]);

		if (s_Data.RectIndexCount)
		{
			s_RectPipeline->bind(s_RendererCommandBuffers[frameIndex]);
			s_RectPipeline->pushConstants(s_RendererCommandBuffers[frameIndex], ShaderType::Vertex, viewProj);
			s_RectPipeline->bindDescriptorSet(s_RendererCommandBuffers[frameIndex]);
			s_RectVertexBuffer->bind(s_RendererCommandBuffers[frameIndex]);
			s_IndexBuffer->bind(s_RendererCommandBuffers[frameIndex]);

			s_Renderer->drawIndexed(s_RendererCommandBuffers[frameIndex], (uint32_t)s_Data.RectIndexCount);
		}

		if (s_Data.TextIndexCount)
		{
			s_TextPipeline->bind(s_RendererCommandBuffers[frameIndex]);
			s_TextPipeline->pushConstants(s_RendererCommandBuffers[frameIndex], ShaderType::Vertex, viewProj);
			s_TextPipeline->bindDescriptorSet(s_RendererCommandBuffers[frameIndex]);
			s_TextVertexBuffer->bind(s_RendererCommandBuffers[frameIndex]);
			s_IndexBuffer->bind(s_RendererCommandBuffers[frameIndex]);

			s_Renderer->drawIndexed(s_RendererCommandBuffers[frameIndex], (uint32_t)s_Data.TextIndexCount);
		}

		if (s_Data.CircleIndexCount)
		{
			s_CirclePipeline->bind(s_RendererCommandBuffers[frameIndex]);
			s_CirclePipeline->pushConstants(s_RendererCommandBuffers[frameIndex], ShaderType::Vertex, viewProj);
			s_CircleVertexBuffer->bind(s_RendererCommandBuffers[frameIndex]);
			s_IndexBuffer->bind(s_RendererCommandBuffers[frameIndex]);

			s_Renderer->drawIndexed(s_RendererCommandBuffers[frameIndex], (uint32_t)s_Data.CircleIndexCount);
		}

		s_Renderer->endCommandBuffer(s_RendererCommandBuffers[frameIndex]);
		s_Renderer->submitCommandBuffer(s_RendererCommandBuffers[frameIndex]);
	}

	void UIRenderer::drawRect(const glm::mat4& transform, const glm::vec4& color)
	{
		constexpr glm::vec2 textureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		for (size_t i = 0; i < 4; i++)
		{
			glm::vec4 vertexPos = s_Data.RectVertexPositions[i];
			vertexPos.z = -0.00001f;

			s_Data.RectVertexPointer->Position = glm::vec3(transform * vertexPos);
			s_Data.RectVertexPointer->Color = color;
			s_Data.RectVertexPointer->TexCoord = textureCoords[i];
			s_Data.RectVertexPointer->TexIndex = 0;
			s_Data.RectVertexPointer++;
		}

		s_Data.RectIndexCount += 6;
		s_Data.RectVertexCount += 4;
	}

	void UIRenderer::drawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

		drawRect(transform, color);
	}

	void UIRenderer::drawRect(const Rect& bounds, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(bounds.Min, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(bounds.Max - bounds.Min, 1.0f));

		drawRect(transform, color);
	}

	void UIRenderer::drawRect(const glm::mat4& transform, Texture2D* texture, const glm::vec4& color)
	{
		constexpr glm::vec2 textureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		int textureIndex = -1;
		for (uint32_t i = 0; i < s_Data.RectTextureIndex; i++)
		{
			if (texture->getUUID() == s_Data.RectTextures[i]->getUUID())
			{
				textureIndex = i;
				break;
			}
		}
		if (textureIndex == -1)
		{
			s_Data.RectTextures[s_Data.RectTextureIndex] = texture;
			s_RectPipeline->updateDescriptor(s_Data.RectTextures[s_Data.RectTextureIndex], 0, s_Data.RectTextureIndex);
			textureIndex = s_Data.RectTextureIndex;

			s_Data.RectTextureIndex++;
		}

		for (size_t i = 0; i < 4; i++)
		{
			s_Data.RectVertexPointer->Position = glm::vec3(transform * s_Data.RectVertexPositions[i]);
			s_Data.RectVertexPointer->Color = color;
			s_Data.RectVertexPointer->TexCoord = textureCoords[i];
			s_Data.RectVertexPointer->TexIndex = textureIndex;
			s_Data.RectVertexPointer++;
		}

		s_Data.RectIndexCount += 6;
		s_Data.RectVertexCount += 4;
	}

	void UIRenderer::drawRect(const glm::vec2& position, const glm::vec2& size, Texture2D* texture, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

		drawRect(transform, texture, color);
	}

	void UIRenderer::drawRect(const Rect& bounds, Texture2D* texture, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(bounds.Min, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(bounds.Max - bounds.Min, 1.0f));

		drawRect(transform, texture, color);
	}

	void UIRenderer::drawText(const std::string& text, const glm::mat4& transform, const TextParams& params)
	{
		drawText(text, transform, params, s_DefaultFont);
	}

	void UIRenderer::drawText(const std::wstring& text, const glm::mat4& transform, const TextParams& params)
	{
		drawText(text, transform, params, s_DefaultFont);
	}

	void UIRenderer::drawText(const std::string& text, const glm::mat4& transform, const UIRenderer::TextParams& params, Font* font)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wide = converter.from_bytes(text);

		drawText(wide, transform, params, font);
	}

	void UIRenderer::drawText(const std::wstring& text, const glm::mat4& transform, const TextParams& params, Font* font)
	{
		glm::mat4 newTransform = transform * glm::scale(glm::mat4(1.0f), { 1.0f, -1.0f, 1.0f });

		const auto& fontGeometry = font->getFontGeometry();
		const auto& metrics = fontGeometry.getMetrics();
		Texture2D* fontAtlas = font->getAtlasTexture();

		double x = 0.0;
		double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
		double y = 0.0;

		const float spaceGlyphAdvance = (float)fontGeometry.getGlyph(L' ')->getAdvance();

		int texIndex = -1;
		for (int i = 0; i < (int)s_Data.AtlasTextureIndex; i++)
		{
			if (s_Data.AtlasTextures[i]->getUUID() == fontAtlas->getUUID())
			{
				texIndex = i;
				break;
			}
		}

		if (texIndex == -1)
		{
			s_Data.AtlasTextures[s_Data.AtlasTextureIndex] = fontAtlas;
			s_TextPipeline->updateDescriptor(s_Data.AtlasTextures[s_Data.AtlasTextureIndex], 0, s_Data.AtlasTextureIndex);
			texIndex = s_Data.AtlasTextureIndex;

			s_Data.AtlasTextureIndex++;
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

			s_Data.TextVertexPointer->Position = newTransform * glm::vec4(quadMin, 0.0f, 1.0f);
			s_Data.TextVertexPointer->Color = params.Color;
			s_Data.TextVertexPointer->TexCoord = { texCoordMin.x, texCoordMin.y };
			s_Data.TextVertexPointer->FontIndex = texIndex;
			s_Data.TextVertexPointer++;

			s_Data.TextVertexPointer->Position = newTransform * glm::vec4(quadMin.x, quadMax.y, 0.0f, 1.0f);
			s_Data.TextVertexPointer->Color = params.Color;
			s_Data.TextVertexPointer->TexCoord = { texCoordMin.x, texCoordMax.y };
			s_Data.TextVertexPointer->FontIndex = texIndex;
			s_Data.TextVertexPointer++;

			s_Data.TextVertexPointer->Position = newTransform * glm::vec4(quadMax, 0.0f, 1.0f);
			s_Data.TextVertexPointer->Color = params.Color;
			s_Data.TextVertexPointer->TexCoord = { texCoordMax.x, texCoordMax.y };
			s_Data.TextVertexPointer->FontIndex = texIndex;
			s_Data.TextVertexPointer++;

			s_Data.TextVertexPointer->Position = newTransform * glm::vec4(quadMax.x, quadMin.y, 0.0f, 1.0f);
			s_Data.TextVertexPointer->Color = params.Color;
			s_Data.TextVertexPointer->TexCoord = { texCoordMax.x, texCoordMin.y };
			s_Data.TextVertexPointer->FontIndex = texIndex;
			s_Data.TextVertexPointer++;

			s_Data.TextIndexCount += 6;
			s_Data.TextVertexCount += 4;

			if (i < text.size() - 1)
			{
				double advance = glyph->getAdvance();
				wchar_t nextCharacter = text[i + 1];
				fontGeometry.getAdvance(advance, character, nextCharacter);

				x += fsScale * advance + params.Kerning;
			}
		}
	}

}

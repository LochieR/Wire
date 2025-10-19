#pragma once

#include "Wire/UI/Renderer/Renderer.h"

#include <glm/glm.hpp>

#include <array>
#include <vector>
#include <string>
#include <type_traits>

namespace wire {

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
		glm::vec2 RectSize;
		glm::vec4 Color;
		float CornerRadius;
		uint32_t CornerFlags;
		float Fade;
	};

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	struct Rect
	{
		glm::vec2 Min;
		glm::vec2 Max;
	};

	struct RendererData
	{
		RectVertex* RectVertexBase = nullptr;
		RectVertex* RectVertexPointer = nullptr;
		size_t RectIndexCount = 0;
		size_t RectVertexCount = 0;
		size_t RectVertexOffset = 0;

		std::array<Texture2D*, 32> RectTextures;
		uint32_t RectTextureIndex = 0;

		TextVertex* TextVertexBase = nullptr;
		TextVertex* TextVertexPointer = nullptr;
		size_t TextIndexCount = 0;
		size_t TextVertexCount = 0;
		size_t TextVertexOffset = 0;

		std::array<Texture2D*, 32> AtlasTextures;
		uint32_t AtlasTextureIndex = 0;

		CircleVertex* CircleVertexBase = nullptr;
		CircleVertex* CircleVertexPointer = nullptr;
		size_t CircleIndexCount = 0;
		size_t CircleVertexCount = 0;
		size_t CircleVertexOffset = 0;

		RoundedRectVertex* RoundedRectVertexBase = nullptr;
		RoundedRectVertex* RoundedRectVertexPointer = nullptr;
		size_t RoundedRectIndexCount = 0;
		size_t RoundedRectVertexCount = 0;
		size_t RoundedRectVertexOffset = 0;

		LineVertex* LineVertexBase = nullptr;
		LineVertex* LineVertexPointer = nullptr;
		size_t LineVertexCount = 0;
		size_t LineVertexOffset = 0;

		std::vector<Rect> Scissors;

		glm::vec4 RectVertexPositions[4];
	};

	enum class UIAnchor
	{
		TopLeft,
		TopRight,
		BottomLeft,
		BottomRight,
		Centre,
		Custom
	};

	enum class RoundedRectFlags : uint32_t
	{
		TopLeft = 1 << 0,
		TopRight = 1 << 1,
		BottomLeft = 1 << 2,
		BottomRight = 1 << 3,

		NoCorners = 0,
		AllCorners = TopLeft | TopRight | BottomLeft | BottomRight
	};

	inline RoundedRectFlags operator|(RoundedRectFlags lhs, RoundedRectFlags rhs)
	{
		return static_cast<RoundedRectFlags>(
			static_cast<std::underlying_type<RoundedRectFlags>::type>(lhs) |
			static_cast<std::underlying_type<RoundedRectFlags>::type>(rhs)
		);
	}

	inline RoundedRectFlags& operator|=(RoundedRectFlags& lhs, RoundedRectFlags rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	inline RoundedRectFlags operator&(RoundedRectFlags lhs, RoundedRectFlags rhs)
	{
		return static_cast<RoundedRectFlags>(
			static_cast<std::underlying_type<RoundedRectFlags>::type>(lhs) &
			static_cast<std::underlying_type<RoundedRectFlags>::type>(rhs)
		);
	}

	inline RoundedRectFlags& operator&=(RoundedRectFlags& lhs, RoundedRectFlags rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	inline RoundedRectFlags operator^(RoundedRectFlags lhs, RoundedRectFlags rhs)
	{
		return static_cast<RoundedRectFlags>(
			static_cast<std::underlying_type<RoundedRectFlags>::type>(lhs) ^
			static_cast<std::underlying_type<RoundedRectFlags>::type>(rhs)
		);
	}

	inline RoundedRectFlags& operator^=(RoundedRectFlags& lhs, RoundedRectFlags rhs)
	{
		lhs = lhs ^ rhs;
		return lhs;
	}

	inline RoundedRectFlags operator~(RoundedRectFlags flags)
	{
		return static_cast<RoundedRectFlags>(
			~static_cast<std::underlying_type<RoundedRectFlags>::type>(flags)
		);
	}

	class Canvas
	{
	public:
		Canvas() = default;
		Canvas(Renderer* renderer);
		void release();

		void beginFrame();
		void endFrame();

		void drawRect(const glm::mat4& transform, const glm::vec4& color);
		void drawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		void drawRect(const Rect& bounds, const glm::vec4& color);

		void drawRect(const glm::mat4& transform, Texture2D* texture, const glm::vec4& color);
		void drawRect(const glm::vec2& position, const glm::vec2& size, Texture2D* texture, const glm::vec4& color);
		void drawRect(const Rect& bounds, Texture2D* texture, const glm::vec4& color);

		void drawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, UIAnchor anchor);
		void drawRect(const Rect& bounds, const glm::vec4& color, UIAnchor anchor);

		void drawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness = 1.0f, float fade = 0.0005f);
		void drawCircle(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, float thickness = 1.0f, float fade = 0.0005f);
		void drawCircle(const Rect& bounds, const glm::vec4& color, float thickness = 1.0f, float fade = 0.0005f);

		void drawRoundedRect(const glm::mat4& transform, const glm::vec4& color, float cornerRadius, RoundedRectFlags flags = RoundedRectFlags::AllCorners, float fade = 0.0005f);
		void drawRoundedRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, float cornerRadius, RoundedRectFlags flags = RoundedRectFlags::AllCorners, float fade = 0.0005f);
		void drawRoundedRect(const Rect& bounds, const glm::vec4& color, float cornerRadius, RoundedRectFlags flags = RoundedRectFlags::AllCorners, float fade = 0.0005f);

		void pushScissor(const Rect& clipBounds);
		void popScissor();

		struct TextParams
		{
			glm::vec4 Color{ 1.0f };
			float Kerning = 0.0f;
			float LineSpacing = 0.0f;
		};
		void drawText(const std::string& text, const glm::mat4& transform, const TextParams& params);
		void drawText(const std::wstring& text, const glm::mat4& transform, const TextParams& params);
		void drawText(const std::string& text, const glm::mat4& transform, const TextParams& params, Font* font);
		void drawText(const std::wstring& text, const glm::mat4& transform, const TextParams& params, Font* font);

		void drawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color);

		glm::vec2 calculateTextSize(const std::string& text, const glm::mat4& transform, const TextParams& params);
		glm::vec2 calculateTextSize(const std::wstring& text, const glm::mat4& transform, const TextParams& params);
		glm::vec2 calculateTextSize(const std::string& text, const glm::mat4& transform, const TextParams& params, Font* font);
		glm::vec2 calculateTextSize(const std::wstring& text, const glm::mat4& transform, const TextParams& params, Font* font);

		glm::vec2 calculateCharacterSize(char c, const glm::mat4& transform);
		glm::vec2 calculateCharacterSize(wchar_t c, const glm::mat4& transform);
		glm::vec2 calculateCharacterSize(char c, const glm::mat4& transform, Font* font);
		glm::vec2 calculateCharacterSize(wchar_t c, const glm::mat4& transform, Font* font);
	private:
		void flush();
	private:
		Renderer* m_Renderer = nullptr;
		RendererData m_Data;
        
        RenderPass* m_RenderPass = nullptr;

		Buffer<VertexBuffer>* m_RectVertexBuffer = nullptr;
        ShaderResourceLayout* m_RectResourceLayout = nullptr;
        ShaderResource* m_RectResource = nullptr;
		GraphicsPipeline* m_RectPipeline = nullptr;

        Buffer<VertexBuffer>* m_TextVertexBuffer = nullptr;
        ShaderResourceLayout* m_TextResourceLayout = nullptr;
        ShaderResource* m_TextResource = nullptr;
		GraphicsPipeline* m_TextPipeline = nullptr;

        Buffer<VertexBuffer>* m_CircleVertexBuffer = nullptr;
		GraphicsPipeline* m_CirclePipeline = nullptr;

        Buffer<VertexBuffer>* m_RoundedRectVertexBuffer = nullptr;
		GraphicsPipeline* m_RoundedRectPipeline = nullptr;

        Buffer<VertexBuffer>* m_LineVertexBuffer = nullptr;
		GraphicsPipeline* m_LinePipeline = nullptr;

		Texture2D* m_WhiteTexture = nullptr;
		Font* m_DefaultFont = nullptr;

		Sampler* m_TextureSampler = nullptr;
		Sampler* m_TextSampler = nullptr;
		Buffer<IndexBuffer>* m_IndexBuffer = nullptr;

		std::vector<CommandList> m_RendererCommandLists;
	};

}

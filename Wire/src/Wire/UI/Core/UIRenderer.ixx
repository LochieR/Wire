module;

#include <glm/glm.hpp>

#include <vector>
#include <string>

export module wire.ui.core:uiRenderer;

import wire.ui.renderer;

namespace wire {

	export struct Rect
	{
		glm::vec2 Min;
		glm::vec2 Max;
	};

	export class UIRenderer
	{
	public:
		static void init(Renderer* renderer);
		static void shutdown();

		static void beginFrame();
		static void endFrame();

		static void drawRect(const glm::mat4& transform, const glm::vec4& color);
		static void drawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		static void drawRect(const Rect& bounds, const glm::vec4& color);

		static void drawRect(const glm::mat4& transform, Texture2D* texture, const glm::vec4& color);
		static void drawRect(const glm::vec2& position, const glm::vec2& size, Texture2D* texture, const glm::vec4& color);
		static void drawRect(const Rect& bounds, Texture2D* texture, const glm::vec4& color);

		struct TextParams
		{
			glm::vec4 Color{ 1.0f };
			float Kerning = 0.0f;
			float LineSpacing = 0.0f;
		};
		static void drawText(const std::string& text, const glm::mat4& transform, const TextParams& params);
		static void drawText(const std::wstring& text, const glm::mat4& transform, const TextParams& params);
		static void drawText(const std::string& text, const glm::mat4& transform, const TextParams& params, Font* font);
		static void drawText(const std::wstring& text, const glm::mat4& transform, const TextParams& params, Font* font);
	private:
		inline static Renderer* s_Renderer = nullptr;

		inline static VertexBuffer* s_RectVertexBuffer = nullptr;
		inline static GraphicsPipeline* s_RectPipeline = nullptr;
		
		inline static VertexBuffer* s_TextVertexBuffer = nullptr;
		inline static GraphicsPipeline* s_TextPipeline = nullptr;

		inline static Texture2D* s_WhiteTexture = nullptr;
		inline static Font* s_DefaultFont = nullptr;

		inline static Sampler* s_TextureSampler = nullptr;
		inline static Sampler* s_TextSampler = nullptr;
		inline static IndexBuffer* s_IndexBuffer = nullptr;

		inline static std::vector<CommandBuffer> s_RendererCommandBuffers;
	};

}

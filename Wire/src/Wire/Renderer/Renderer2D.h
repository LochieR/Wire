#pragma once

#include "Texture2D.h"
#include "Framebuffer.h"
#include "OrthographicCamera.h"

#include "Font.h"

#include <glm/glm.hpp>

#ifdef DrawText
#undef DrawText
#endif

namespace Wire {

	class Renderer;

	class Renderer2D
	{
	public:
		Renderer2D() = default;
		Renderer2D(Renderer* renderer);
		~Renderer2D() = default;

		void Release();

		void Begin(const OrthographicCamera& camera);
		void Begin(const OrthographicCamera& camera, rbRef<Framebuffer> framebuffer);
		void End();

		void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, rbRef<Texture2D> texture);

		void DrawCircle(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, float thickness = 1.0f, float fade = 0.005f);
		void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color);

		void SetLineWidth(float lineWidth);

		void DrawRoundedQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, float cornerRadius, float fade = 0.005f);

		struct TextParams
		{
			glm::vec4 Color{ 1.0f };
			float Kerning = 0.0f;
			float LineSpacing = 0.0f;
		};
		void DrawText(const std::string& text, const glm::mat4& transform, const TextParams& textParams);
		void DrawText(const std::string& text, const glm::mat4& transform, const TextParams& textParams, rbRef<Font> font);
		void DrawText(const std::wstring& text, const glm::mat4& transform, const TextParams& textParams, rbRef<Font> font);

		rbRef<Font> GetDefaultFont() const;

		uint32_t ReadPixel(uint32_t attachmentIndex, uint32_t x, uint32_t y, rbRef<Framebuffer> framebuffer = nullptr);

		void NewLayer() { NextBatch(); }
	private:
		void StartBatch();
		void Flush();
		void NextBatch();

		void CreatePipelines(rbRef<Framebuffer> framebuffer);
	private:
		Renderer* m_Renderer = nullptr;
	};

}

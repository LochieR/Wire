#pragma once

#include "GraphRenderer.h"

#include <Wire/Wire.h>
#include "Wire/Renderer/OrthographicCameraController.h"

#include <glm/glm.hpp>

#include <map>

namespace Wire {

	enum class StatusType
	{
		Normal = 0,
		Loading
	};

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer();

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(Timestep ts) override;
		void OnImGuiRender() override;
		void PostRender() override;
		void OnEvent(Event& e) override;
	private:
		Scene* m_Scene;

		rbRef<Framebuffer> m_Framebuffer = nullptr;
		float m_ViewportWidth = 800.0f, m_ViewportHeight = 600.0f;

		rbRef<Texture2D> m_Texture = nullptr;
		rbRef<Font> m_MathsFont = nullptr;

		float m_AspectRatio;
		OrthographicCameraController m_CameraController;

		GraphRenderer* m_GraphRenderer = nullptr;

		bool m_TitlebarHovered = false;

		glm::vec2 m_WindowPos = { 0, 0 };
		uint32_t m_HoveredID = 0;
	};

}

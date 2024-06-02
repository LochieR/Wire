#include "EditorLayer.h"

#include "Wire/Core/Base.h"
#include "Wire/Utils/PlatformUtils.h"

#include "Wire/Renderer/Renderer.h"
#include "Wire/Renderer/Font.h"

#include <imgui.h>

namespace Wire {

	EditorLayer::EditorLayer()
		: Layer("EditorLayer"), m_AspectRatio(800.0f / 600.0f), m_CameraController(m_AspectRatio, -10.0f, 10.0f)
	{
	}

	EditorLayer::~EditorLayer()
	{
	}

	void EditorLayer::OnAttach()
	{
		Renderer* renderer = Application::Get().GetRenderer();

		FramebufferSpecification fbSpec{};
		fbSpec.Width = 800;
		fbSpec.Height = 800;
		fbSpec.Attachments = { AttachmentFormat::Default, AttachmentFormat::R32_SInt, AttachmentFormat::Depth };
		fbSpec.MultiSample = true;

		m_Framebuffer = renderer->CreateFramebuffer(fbSpec);
		m_MathsFont = renderer->CreateFont("Resources/fonts/Maths/NewCM08-Regular.ttf", 0x0020, 0x0D50);

		m_GraphRenderer = new GraphRenderer(renderer, m_Framebuffer);
	}

	void EditorLayer::OnDetach()
	{
		delete m_GraphRenderer;
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		Application& app = Application::Get();

		Renderer* renderer = app.GetRenderer();
		Renderer2D& renderer2D = renderer->GetRenderer2D();

		static bool isFirstFrame = true;
		static bool isSecondFrame = false;

		if ((m_ViewportWidth != 0 && m_ViewportHeight != 0 &&
			(m_ViewportWidth != (float)m_Framebuffer->GetWidth() || m_ViewportHeight != (float)m_Framebuffer->GetHeight())
			&& !Input::IsMouseButtonDown(MouseButton::ButtonLeft)) || isSecondFrame)
		{
			if (!isFirstFrame)
			{
				m_Framebuffer->Resize((uint32_t)m_ViewportWidth, (uint32_t)m_ViewportHeight);
				m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
				m_CameraController.OnResize(m_ViewportWidth, m_ViewportHeight);
			}
		}

		m_GraphRenderer->Draw(m_CameraController.GetCamera(), "hi");

		/*renderer2D.Begin(m_CameraController.GetCamera(), m_Framebuffer);
		renderer2D.DrawText(L"\u222B", glm::mat4{ 1.0f }, {}, m_MathsFont);
		renderer2D.End();*/

		m_CameraController.OnUpdate(ts);

		if (isFirstFrame)
		{
			isFirstFrame = false;
			isSecondFrame = true;
		}
		if (isSecondFrame)
			isSecondFrame = false;
	}

	void EditorLayer::OnImGuiRender()
	{
		ImGui::SetNextWindowSize({ 800, 800 });
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		ImGui::Begin("Viewport");
		float width = ImGui::GetContentRegionAvail().x;
		float height = ImGui::GetContentRegionAvail().y;

		ImGui::Image(m_Framebuffer->GetRenderHandle(), { m_ViewportWidth, m_ViewportHeight });

		ImVec2 windowPos = ImGui::GetWindowPos();
		m_WindowPos = { windowPos.x, windowPos.y };

		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::Begin("Info");
		ImGui::Text("Hovered: %u", m_HoveredID);
		ImGui::End();

		m_ViewportWidth = width, m_ViewportHeight = height;
	}

	void EditorLayer::PostRender()
	{
		Renderer* renderer = Application::Get().GetRenderer();
		Renderer2D& renderer2D = renderer->GetRenderer2D();

		glm::vec2 mousePos = Input::GetMousePosition();
		uint32_t x = (uint32_t)mousePos.x;
		uint32_t y = (uint32_t)mousePos.y;

		x -= (uint32_t)m_WindowPos.x;
		y -= (uint32_t)m_WindowPos.y;

		if (x < m_Framebuffer->GetWidth() && y < m_Framebuffer->GetHeight())
		{
			m_HoveredID = renderer2D.ReadPixel(1, x, y, m_Framebuffer);
		}
	}

	void EditorLayer::OnEvent(Event& e)
	{
		m_CameraController.OnEvent(e);
	}

}

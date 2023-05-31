#include "EditorLayer.h"

#include "Wire/Scene/SceneSerializer.h"
#include "Wire/Utils/PlatformUtils.h"
#include "Wire/Maths/Maths.h"
#include "Wire/ImGui/ImGuiLayer.h"
#include "Wire/Scripting/ScriptGlue.h"
#include "Wire/Scripting/ScriptEngine.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ImGuizmo/ImGuizmo.h"

namespace Wire {

	static bool s_PreferencesWindowHasSettings;

	EditorLayer::EditorLayer()
		: Layer("EditorLayer"), m_CameraController(1280.0f / 720.0f), m_SquareColour({ 0.2f, 0.3f, 0.8f, 1.0f })
	{
	}

	void EditorLayer::OnAttach()
	{
		WR_PROFILE_FUNCTION();

		m_CheckerboardTexture = Texture2D::Create("assets/textures/Checkerboard.png");
		m_IconPlay = Texture2D::Create("Resources/Icons/UIToolbar/PlayButton.png");
		m_IconPause = Texture2D::Create("Resources/Icons/UIToolbar/PauseButton.png");
		m_IconStop = Texture2D::Create("Resources/Icons/UIToolbar/StopButton.png");
		m_IconStep = Texture2D::Create("Resources/Icons/UIToolbar/StepButton.png");

		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
		fbSpec.Width = 1280;
		fbSpec.Height = 720;
		m_Framebuffer = Framebuffer::Create(fbSpec);

		m_ActiveScene = CreateRef<Scene>();

		ApplicationCommandLineArgs commandLineArgs = Application::Get().GetCommandLineArgs();
		if (commandLineArgs.Count > 1)
		{
			bool project = false;
			int projectIndex = -1;
			bool scene = false;
			int sceneIndex = -1;
			int i = 0;
			for (auto arg : commandLineArgs)
			{
				if (std::string(arg) == std::string("--project"))
				{
					project = true;
					projectIndex = i + 1;
				}
				if (std::string(arg) == std::string("--scene"))
				{
					scene = true;
					sceneIndex = i + 1;
				}
				i++;
			}

			if (project)
			{
				auto prjPath = commandLineArgs[projectIndex];
				OpenProject(prjPath);
			}
			if (scene)
			{
				auto sceneFilePath = commandLineArgs[sceneIndex];
				OpenScene(sceneFilePath);
			}
		}

		m_EditorCamera = EditorCamera(30.0f, 16.0f / 9.0f, 0.1f, 1000.0f);

		if (!std::filesystem::exists(std::filesystem::path("imgui.ini")))
			s_PreferencesWindowHasSettings = false;
		else
		{
			std::ifstream file("imgui.ini");

			std::string line;

			while (std::getline(file, line))
			{
				if (line == "[Window][Preferences]")
				{
					s_PreferencesWindowHasSettings = true;
					break;
				}
				else
				{
					s_PreferencesWindowHasSettings = false;
				}
			}

			file.close();
		}

		Application::Get().SetApplicationLogFunction([this](int level, const std::string& message) { m_ConsolePanel.Log((LogLevel)level, message); });

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
	}

	void EditorLayer::OnDetach()
	{
		WR_PROFILE_FUNCTION();

		WR_INFO("Average frame rate was {0} fps", m_ContentBrowserPanel.GetAverageFrameRate());
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		WR_PROFILE_FUNCTION();

		m_Timestep = ts;

		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

		// Resize
		if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
			m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
		{
			m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_CameraController.OnResize(m_ViewportSize.x, m_ViewportSize.y);
			m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
		}

		// Render
		Renderer2D::ResetStats();
		m_Framebuffer->Bind();
		RenderCommand::SetClearColour({ 0.1f, 0.1f, 0.1f, 1 });
		RenderCommand::Clear();

		// Set entity ID attachment to -1
		m_Framebuffer->ClearAttachment(1, -1);

		switch (m_SceneState)
		{
			case SceneState::Edit:
			{
				if (m_ViewportFocused)
					m_CameraController.OnUpdate(ts);

				m_EditorCamera.OnUpdate(ts);

				m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera);
				break;
			}
			case SceneState::Play:
			{
				m_ActiveScene->OnUpdateRuntime(ts);
				break;
			}
		}

		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
		my = viewportSize.y - my;
		int mouseX = (int)mx;
		int mouseY = (int)my;

		if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
		{
			int pixelData = m_Framebuffer->ReadPixel(1, mouseX, mouseY);
			m_HoveredEntity = pixelData == -1 ? Entity{} : Entity{ (entt::entity)pixelData, m_ActiveScene.get() };
		}

		m_Framebuffer->Unbind();
	}

	void EditorLayer::OnImGuiRender()
	{
		WR_PROFILE_FUNCTION();

		// Note: Switch this to true to enable dockspace
		static bool dockspaceOpen = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		style.WindowMinSize.x = minWinSizeX;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::BeginMenu("New"))
				{
					if (ImGui::MenuItem("New Project...", "Ctrl+Shift+N"))
					{
						NewProject();
					}

					if (ImGui::MenuItem("New Patch", "Ctrl+N"))
					{
						NewScene();
					}
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Open"))
				{
					if (ImGui::MenuItem("Open Project...", "Ctrl+O"))
					{
						OpenProject();
					}

					if (ImGui::MenuItem("Open Patch...", "Ctrl+Shift+O"))
					{
						OpenScene();
					}
					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Save Patch", "Ctrl+S"))
				{
					SaveScene();
				}

				if (ImGui::MenuItem("Save Patch As...", "Ctrl+Shift+S"))
				{
					SaveSceneAs();
				}

				if (ImGui::MenuItem("Exit", "Alt+F4")) Application::Get().Close();
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Preferences"))
				{
					m_ShowPreferencesWindow = true;
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Scripts"))
			{
				if (ImGui::MenuItem("Reload Assembly", "Ctrl+R"))
				{
					Mouse::SetMouseIcon(MouseIcon::Loading);
					ScriptEngine::ReloadAssembly();
					Mouse::SetMouseIcon(MouseIcon::Arrow);
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		static bool sceneHierarchyOpen = true;
		static bool contentBrowserOpen = true;
		static bool propertiesPanelOpen = true;
		static bool consolePanelOpen = true;

		m_SceneHierarchyPanel.OnImGuiRender(&sceneHierarchyOpen, &propertiesPanelOpen);
		m_ContentBrowserPanel.OnImGuiRender(&contentBrowserOpen, m_Timestep);
		m_ConsolePanel.OnImGuiRender(&consolePanelOpen);

		ImGui::Begin("Stats");

		std::string name = "None";
		if (m_HoveredEntity)
			name = m_HoveredEntity.GetComponent<TagComponent>().Tag;
		ImGui::Text("Hovered Entity: %s", name.c_str());

		auto stats = Renderer2D::GetStats();
		ImGui::Text("Renderer2D Stats:");
		ImGui::Text("Draw Calls: %d", stats.DrawCalls);
		ImGui::Text("Quads: %d", stats.QuadCount);
		ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
		ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

		ImGui::End();

		if (m_ShowPreferencesWindow)
		{
			ImGui::Begin("Preferences", &m_ShowPreferencesWindow);

			if (!s_PreferencesWindowHasSettings)
				ImGui::SetWindowSize(ImVec2{ 800, 600 });

			ImGui::End();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");
		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		uint64_t textureID = m_Framebuffer->GetColourAttachmentRendererID();
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		if (m_SceneState == SceneState::Edit)
		{
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const wchar_t* path = (wchar_t*)payload->Data;
					OpenScene(std::filesystem::path(path));
				}
				ImGui::EndDragDropTarget();
			}

			Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
			if (selectedEntity && m_GizmoType != -1)
			{
				ImGuizmo::SetOrthographic(false);
				ImGuizmo::SetDrawlist();
				ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);

				// Camera

				// Runtime camera from entity
				// auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
				// const auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
				// const glm::mat4& cameraProjection = camera.GetProjection();
				// glm::mat4 cameraView = glm::inverse(cameraEntity.GetComponent<TransformComponent>().GetTransform());

				// Editor camera

				const glm::mat4& cameraProjection = m_EditorCamera.GetProjection();
				glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();

				auto& tc = selectedEntity.GetComponent<TransformComponent>();
				glm::mat4 transform = tc.GetTransform();

				bool snap = Input::IsKeyPressed(KeyCode::LeftControl);
				float snapValue = 0.5f;
				if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
					snapValue = 45.0f;

				float snapValues[3] = { snapValue, snapValue, snapValue };

				ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
					(ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
					nullptr, snap ? (const float*)snapValues : nullptr);

				if (ImGuizmo::IsUsing())
				{
					glm::vec3 translation, rotation, scale;
					Maths::DecomposeTransform(transform, translation, rotation, scale);

					glm::vec3 deltaRotation = rotation - tc.Rotation;
					tc.Translation = translation;
					tc.Rotation += deltaRotation;
					tc.Scale = scale;
				}
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();

		UIToolbar();

		ImGui::End();
	}

	void EditorLayer::UIToolbar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 2 });
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2{ 0, 0 });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{ 38.0f / 255.0f, 38.0f / 255.0f, 39.0f / 255.0f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0, 0, 0, 0 });
		auto& colours = ImGui::GetStyle().Colors;
		const auto& buttonHovered = colours[ImGuiCol_ButtonHovered];
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
		const auto& buttonActive = colours[ImGuiCol_ButtonActive];
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

		ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		
		bool isPaused = m_ActiveScene->IsPaused();

		float size = ImGui::GetWindowHeight() - 4.0f;
		Ref<Texture2D> icon = m_SceneState == SceneState::Edit || isPaused ? m_IconPlay : m_IconPause;
		ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * (isPaused ? 1.5f : 1.0f)));
		if (ImGui::ImageButton((ImTextureID)(uint64_t)icon->GetRendererID(), ImVec2{ size, size }, ImVec2{ 0, 0 }, ImVec2{ 1, 1 }, 0))
		{
			if (m_SceneState == SceneState::Edit)
			{
				OnScenePlay();
			}
			else
			{
				m_ActiveScene->SetPaused(!isPaused);
			}
		}
		if (m_SceneState != SceneState::Edit)
		{
			if (isPaused)
			{
				ImGui::SameLine();
				if (ImGui::ImageButton((ImTextureID)(uint64_t)m_IconStep->GetRendererID(), ImVec2{ size, size }, ImVec2{ 0, 0 }, ImVec2{ 1, 1 }, 0))
				{
					m_ActiveScene->Step();
				}
			}
		}
		ImGui::SameLine();
		if (ImGui::ImageButton((ImTextureID)(uint64_t)m_IconStop->GetRendererID(), ImVec2{ size, size }, ImVec2{ 0, 0 }, ImVec2{ 1, 1 }, 0))
		{
			OnSceneStop();
		}

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(4);
		ImGui::End();
	}

	void EditorLayer::OnEvent(Event& e)
	{
		m_CameraController.OnEvent(e);
		if (m_SceneState == SceneState::Edit)
			m_EditorCamera.OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(WR_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(WR_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		if (e.IsRepeat())
			return false;

		bool control = Input::IsKeyPressed(KeyCode::LeftControl) || Input::IsKeyPressed(KeyCode::RightControl);
		bool shift = Input::IsKeyPressed(KeyCode::LeftShift) || Input::IsKeyPressed(KeyCode::RightShift);

		switch (e.GetKeyCode())
		{
			case KeyCode::N:
			{
				if (control)
					NewScene();

				break;
			}
			case KeyCode::O:
			{
				if (control); // Open project
				else if (control && shift)
					OpenScene();

				break;
			}
			case KeyCode::S:
			{
				if (control && shift)
					SaveSceneAs();

				break;
			}

			// Gizmos
			case KeyCode::Q:
			{
				if (!ImGuizmo::IsUsing())
					m_GizmoType = -1;
				break;
			}
			case KeyCode::W:
			{
				if (!ImGuizmo::IsUsing())
					m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
				break;
			}
			case KeyCode::E:
			{
				if (!ImGuizmo::IsUsing())
					m_GizmoType = ImGuizmo::OPERATION::ROTATE;
				break;
			}
			case KeyCode::R:
			{
				if (control)
				{
					Mouse::SetMouseIcon(MouseIcon::Loading);
					ScriptEngine::ReloadAssembly();
					Mouse::SetMouseIcon(MouseIcon::Arrow);
				}
				else
				{
					if (!ImGuizmo::IsUsing())
						m_GizmoType = ImGuizmo::OPERATION::SCALE;
				}
				break;
			}
		}

		return false;
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (e.GetMouseButton() == MouseButton::ButtonLeft)
			if (m_ViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(KeyCode::LeftAlt))
				m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity);

		return false;
	}

	void EditorLayer::NewProject()
	{
		std::string filepath = FileDialogs::SaveFile("Wire Project (*.wrpj)\0*.wrpj\0");
		if (!filepath.empty())
		{
			NewProject(std::filesystem::path(filepath));
		}
	}

	void EditorLayer::NewProject(const std::filesystem::path& path)
	{
		m_Project = CreateRef<Project>(path.stem().string(), path);
		std::filesystem::create_directories(m_Project->GetDir().string() + "assets");
		m_EditorCamera = EditorCamera(30.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
		m_ContentBrowserPanel.OnOpenProject(m_Project);
		m_SceneHierarchyPanel.OnOpenProject(m_Project);
	}

	void EditorLayer::OpenProject()
	{
		std::string filepath = FileDialogs::OpenFile("Wire Project (*.wrpj)\0*.wrpj\0");
		if (!filepath.empty())
		{
			OpenProject(std::filesystem::path(filepath));
		}
	}

	void EditorLayer::OpenProject(const std::filesystem::path& path)
	{
		m_Project = Project::OpenProject(path);
		m_EditorCamera = EditorCamera(30.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
		m_ContentBrowserPanel.OnOpenProject(m_Project);
		m_SceneHierarchyPanel.OnOpenProject(m_Project);
		ScriptEngine::OnOpenProject(m_Project);
	}

	void EditorLayer::NewScene()
	{
		m_ActiveScene = CreateRef<Scene>();
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);

		m_ActiveScenePath = std::filesystem::path();
	}

	void EditorLayer::OpenScene()
	{
		std::string filepath = FileDialogs::OpenFile("Wire Scene (*.wire)\0*.wire\0");
		if (!filepath.empty())
		{
			OpenScene(std::filesystem::path(filepath));
		}
	}

	void EditorLayer::OpenScene(const std::filesystem::path& path)
	{
		if (m_SceneState != SceneState::Edit)
			OnSceneStop();

		if (path.extension().string() != ".wire")
		{
			WR_WARN("Could not load {0} - not a scene file.", path.filename().string());
			m_ConsolePanel.Log(LogLevel::Warn, fmt::format("Could not load {0} - not a scene file.", path.filename().string()));
			return;
		}

		Ref<Scene> newScene = CreateRef<Scene>();
		SceneSerializer serializer(newScene);
		if (serializer.Deserialize(path.string()))
		{
			m_ActiveScene = newScene;
			m_ActiveScenePath = path;
			m_SceneHierarchyPanel.SetContext(m_ActiveScene);
		}
	}

	void EditorLayer::SaveScene()
	{
		if (!m_ActiveScenePath.empty())
			SerializeScene(m_ActiveScene, m_ActiveScenePath);
		else
			SaveSceneAs();
	}

	void EditorLayer::SaveSceneAs()
	{
		std::string filepath = FileDialogs::SaveFile("Wire Scene (*.wire)\0*.wire\0");
		if (!filepath.empty())
		{
			SerializeScene(m_ActiveScene, filepath);
		}
	}

	void EditorLayer::SerializeScene(Ref<Scene> scene, const std::filesystem::path& path)
	{
		SceneSerializer serializer(scene);
		serializer.Serialize(path.string());
	}

	void EditorLayer::OnScenePlay()
	{
		m_SceneState = SceneState::Play;
		m_ConsolePanel.OnSceneStart();
		m_ActiveScene->OnSceneStart();
	}

	void EditorLayer::OnSceneStop()
	{
		m_SceneState = SceneState::Edit;
		m_ActiveScene->OnSceneStop();
	}

	void EditorLayer::OnScenePause()
	{
		if (m_SceneState == SceneState::Edit)
			return;

		m_ActiveScene->SetPaused(true);
	}

}

#include "wrpch.h"
#include "SceneRenderer.h"

#include <entt.hpp>

namespace Wire {

	/*void SceneRenderer::Render(Scene* scene, const RenderObject& renderObject, uint32_t renderPassIndex)
	{
		Renderer2D::Begin(scene->m_SceneCamera, scene->m_SceneCameraTransform, renderObject, renderPassIndex);

		auto& registry = scene->m_Registry;

		{
			auto view = registry.view<TransformComponent>();
			for (auto module : view)
			{
				auto& transform = view.get<TransformComponent>(module);

				Renderer2D::DrawQuad(transform.GetTransform(), { 0.3f, 0.3f, 0.3f, 1.0f });
			}
		}

		Renderer2D::End();
	}*/

}

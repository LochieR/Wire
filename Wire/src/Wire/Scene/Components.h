#pragma once

#include "Wire/Core/UUID.h"
#include "Controls/NumberControls.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <string>

namespace Wire {

	struct IDComponent
	{
		UUID ID;
		std::string Name;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
		IDComponent(UUID uuid, const std::string& name = std::string())
			: ID(uuid), Name(name)
		{
		}
	};

	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };		// Rotation is probably not needed, but is here anyway.
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			: Translation(translation)
		{
		}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

			return glm::translate(glm::mat4(1.0f), Translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	struct InputsComponent
	{
		uint32_t InputCount = 0;
		uint32_t InputsConnected = 0;

		InputsComponent() = default;
		InputsComponent(const InputsComponent&) = default;
		InputsComponent(InputsComponent&&) = default;
		InputsComponent(uint32_t count = 0, uint32_t connected = 0)
			: InputCount(count), InputsConnected(connected)
		{
		}
	};

	struct NumberControlsComponent
	{
		std::vector<Controls::INumberControl*> Controls;

		NumberControlsComponent() = default;
		NumberControlsComponent(const NumberControlsComponent&) = default;
	};

}

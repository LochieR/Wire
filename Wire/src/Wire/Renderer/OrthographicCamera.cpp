#include "wrpch.h"
#include "OrthographicCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Wire {

	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
		: m_ViewMatrix(1.0f)
	{
		SetProjection(left, right, bottom, top);
		RecalculateViewMatrix();
	}

	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top, float near, float far)
		: m_ViewMatrix(1.0f)
	{
		SetProjection(left, right, bottom, top, near, far);
		RecalculateViewMatrix();
	}

	void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
	{
		m_ProjectionMatrix = glm::ortho(left, right, top, bottom, -1.0f, 1.0f);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void OrthographicCamera::SetProjection(float left, float right, float bottom, float top, float near, float far)
	{
		m_ProjectionMatrix = glm::ortho(left, right, top, bottom, near, far);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void OrthographicCamera::RecalculateViewMatrix()
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), glm::vec3(0, 0, 1));

		m_ViewMatrix = glm::inverse(transform);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

}

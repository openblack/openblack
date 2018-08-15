#include "Camera.h"

#include <cmath>

using namespace OpenBlack;

glm::vec3 Camera::GetRotation() const
{
	return m_rotation;
}

glm::vec3 Camera::GetPosition() const
{
	return m_position;
}

glm::mat4 Camera::GetViewMatrix() const
{
	glm::mat4 rotM = glm::mat4(1.0f);
	rotM = glm::rotate(rotM, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	glm::mat4 transM = glm::translate(glm::mat4(1.0f), -m_position);

	return rotM * transM;
}

glm::mat4 Camera::GetProjectionMatrix() const
{
	return m_projection;
}

glm::mat4 Camera::GetViewProjectionMatrix() const
{
	return GetProjectionMatrix() * GetViewMatrix();
}

void Camera::SetPosition(const glm::vec3& position)
{
	m_position = position;
}

void Camera::SetRotation(const glm::vec3& rotation)
{
	m_rotation = rotation;
}

void Camera::SetProjectionMatrixPerspective(float fov, float aspect, float nearclip, float farclip)
{
	m_projection = glm::perspective(glm::radians(fov), aspect, nearclip, farclip);
}


void Camera::SetProjectionMatrix(const glm::mat4x4& projection)
{
	m_projection = projection;
}

glm::vec3 Camera::GetForward() const
{
    glm::vec3 front;
    front.x = cos(glm::radians(m_rotation.x)) * cos(glm::radians(m_rotation.y));
    front.y = sin(glm::radians(m_rotation.y));
    front.z = sin(glm::radians(m_rotation.x)) * cos(glm::radians(m_rotation.y));
    return -glm::normalize(front);
}

glm::vec3 Camera::GetRight() const
{
    return glm::normalize(glm::cross(GetForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 Camera::GetUp() const
{
    return glm::normalize(glm::cross(GetRight(), GetForward()));
}

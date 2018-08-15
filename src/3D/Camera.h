#pragma once
#ifndef OPENBLACK_CAMERA_H
#define OPENBLACK_CAMERA_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace OpenBlack
{
	class Camera
	{

	public:
		glm::vec3 GetRotation() const;
		glm::vec3 GetPosition() const;
		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetProjectionMatrix() const;
		glm::mat4 GetViewProjectionMatrix() const;

		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::vec3& rotation);

		void SetProjectionMatrixPerspective(float fov, float aspect, float nearclip, float farcli);
		void SetProjectionMatrix(const glm::mat4x4& projection);

		glm::vec3 GetForward() const;
		glm::vec3 GetRight() const;
		glm::vec3 GetUp() const;
	private:

		glm::vec3 m_position;
		glm::vec3 m_rotation;
		glm::mat4 m_projection;
	};
}

#endif

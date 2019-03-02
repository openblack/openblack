#pragma once
#ifndef OPENBLACK_CAMERA_H
#define OPENBLACK_CAMERA_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SDL_events.h>

namespace OpenBlack {

class Camera
{

public:
	Camera(glm::vec3 position, glm::vec3 rotation) :
		_position(position), _rotation(_rotation), _projectionMatrix(1.0f),
		_velForward(0.0f), _velRight(0.0f) {}
	Camera() : Camera(glm::vec3(0.0f), glm::vec3(0.0f)) {}

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

	void Update(double dt);
	void ProcessSDLEvent(SDL_Event* e);
	void HandleMouseMoveEvent(int x, int y);
	void HandleMouseScrollEvent(int x);
private:

	glm::vec3 _position;
	glm::vec3 _rotation;
	glm::mat4 _projectionMatrix;

	float _velForward, _velRight;
};
}

#endif

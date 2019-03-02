#include "Camera.h"

#include <cmath>
#include <iostream>

using namespace OpenBlack;

glm::vec3 Camera::GetRotation() const
{
	return _rotation;
}

glm::vec3 Camera::GetPosition() const
{
	return _position;
}

glm::mat4 Camera::GetViewMatrix() const
{
	glm::mat4 matRoll = glm::mat4(1.0f);
	glm::mat4 matPitch = glm::mat4(1.0f);
	glm::mat4 matYaw = glm::mat4(1.0f);

	glm::mat4 mRoll =  glm::rotate(glm::mat4(1.0f), glm::radians(_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // roll
	glm::mat4 mPitch = glm::rotate(glm::mat4(1.0f), glm::radians(_rotation.y), glm::vec3(1.0f, 0.0f, 0.0f)); // pitch
	glm::mat4 mYaw =   glm::rotate(glm::mat4(1.0f), glm::radians(_rotation.x), glm::vec3(0.0f, 1.0f, 0.0f)); // yaw

	glm::mat4 mRot = mRoll * mPitch * mYaw;

	/*glm::mat4 rotM = glm::mat4(1.0f);
	rotM = glm::rotate(rotM, glm::radians(_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); 
	rotM = glm::rotate(rotM, glm::radians(_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));*/

	glm::mat4 transM = glm::translate(glm::mat4(1.0f), -_position);

	return mRot * transM;
}

glm::mat4 Camera::GetProjectionMatrix() const
{
	return _projectionMatrix;
}

glm::mat4 Camera::GetViewProjectionMatrix() const
{
	return GetProjectionMatrix() * GetViewMatrix();
}

void Camera::SetPosition(const glm::vec3& position)
{
	_position = position;
}

void Camera::SetRotation(const glm::vec3& rotation)
{
	_rotation = rotation;
}

void Camera::SetProjectionMatrixPerspective(float fov, float aspect, float nearclip, float farclip)
{
	_projectionMatrix = glm::perspective(glm::radians(fov), aspect, nearclip, farclip);
}


void Camera::SetProjectionMatrix(const glm::mat4x4& projection)
{
	_projectionMatrix = projection;
}

glm::vec3 Camera::GetForward() const
{
	glm::vec3 forward =
		(glm::vec3(0.f, 0.f, -1.f) * cos(glm::radians(_rotation.x))) +
		(glm::vec3(0.f, -1.0f, 0.0f) * sin(glm::radians(_rotation.y))) +
		(glm::vec3(1.f, 0.0f, 0.0f) * sin(glm::radians(_rotation.x)));
	return glm::normalize(forward);

    //glm::vec3 front;
    //front.x = cos(glm::radians(_rotation.x)) * cos(glm::radians(_rotation.y));
    //front.y = sin(glm::radians(_rotation.y));
    //front.z = sin(glm::radians(_rotation.x)) * cos(glm::radians(_rotation.y));
	//
    //return glm::normalize(front);
}

glm::vec3 Camera::GetRight() const
{
    return glm::normalize(glm::cross(GetForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 Camera::GetUp() const
{
    return glm::normalize(glm::cross(GetRight(), GetForward()));
}

void Camera::ProcessSDLEvent(SDL_Event* e)
{
	if (e->type == SDL_KEYDOWN)
	{
		if (e->key.repeat > 0)
			return;

		switch (e->key.keysym.sym)
		{
		case SDLK_w: _velForward += 1.0f; break;
		case SDLK_s: _velForward -= 1.0f; break;
		case SDLK_a: _velRight -= 1.0f; break;
		case SDLK_d: _velRight += 1.0f; break;
		case SDLK_SPACE:
			std::cout << "forward: " << GetForward().x << "," << GetForward().y << "," << GetForward().z << std::endl;
			break;
		}
	}
	else if (e->type == SDL_KEYUP)
	{
		if (e->key.repeat > 0)
			return;

		switch (e->key.keysym.sym)
		{
		case SDLK_w: _velForward -= 1.0f; break;
		case SDLK_s: _velForward += 1.0f; break;
		case SDLK_a: _velRight += 1.0f; break;
		case SDLK_d: _velRight -= 1.0f; break;
		}
	}
}

void Camera::Update(double dt)
{
	_position += GetForward() * (float)(_velForward * dt);
	_position += GetRight() * (float)(_velRight * dt);
}

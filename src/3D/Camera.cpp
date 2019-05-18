/* OpenBlack - A reimplementation of Lionheads's Black & White engine
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

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
}

glm::vec3 Camera::GetRight() const
{
    return glm::normalize(glm::cross(GetForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 Camera::GetUp() const
{
    return glm::normalize(glm::cross(GetRight(), GetForward()));
}

void Camera::DeprojectScreenToWorld(const glm::ivec2 screenPosition, const glm::ivec2 screenSize, glm::vec3 & out_worldOrigin, glm::vec3 & out_worldDirection)
{
	const float normalizedX = (float)screenPosition.x / (float)screenSize.x;
	const float normalizedY = (float)screenPosition.y / (float)screenSize.y;

	const float screenSpaceX = (normalizedX - 0.5f) * 2.0f;
	const float screenSpaceY = ((1.0f - normalizedY) - 0.5f) * 2.0f;

	// The start of the ray trace is defined to be at mousex,mousey,1 in projection space (z=0 is near, z=1 is far - this gives us better precision)
	// To get the direction of the ray trace we need to use any z between the near and the far plane, so let's use (mousex, mousey, 0.5)
	const glm::vec4 rayStartProjectionSpace = glm::vec4(screenSpaceX, screenSpaceY, 0.0f, 1.0f);
	const glm::vec4 rayEndProjectionSpace = glm::vec4(screenSpaceX, screenSpaceY, 0.5f, 1.0f);

	// Calculate our inverse view projection matrix
	glm::mat4 inverseViewProj = glm::inverse(GetViewProjectionMatrix());

	// Get our homogeneous coordinates for our start and end ray positions
	const glm::vec4 hgRayStartWorldSpace = inverseViewProj * rayStartProjectionSpace;
	const glm::vec4 hgRayEndWorldSpace = inverseViewProj * rayEndProjectionSpace;

	glm::vec3 rayStartWorldSpace(hgRayStartWorldSpace.x, hgRayStartWorldSpace.y, hgRayStartWorldSpace.z);
	glm::vec3 rayEndWorldSpace(hgRayEndWorldSpace.x, hgRayEndWorldSpace.y, hgRayEndWorldSpace.z);

	// divide vectors by W to undo any projection and get the 3-space coord
	if (hgRayStartWorldSpace.w != 0.0f)
		rayStartWorldSpace /= hgRayStartWorldSpace.w;

	if (hgRayEndWorldSpace.w != 0.0f)
		rayEndWorldSpace /= hgRayEndWorldSpace.w;

	const glm::vec3 rayDirWorldSpace = glm::normalize(rayEndWorldSpace - rayStartWorldSpace);

	// finally, store the results in the outputs
	out_worldOrigin = rayStartWorldSpace;
	out_worldDirection = rayDirWorldSpace;
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

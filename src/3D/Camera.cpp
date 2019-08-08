/* OpenBlack - A reimplementation of Lionhead's Black & White.
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

#include <3D/Camera.h>
#include <Game.h>

using namespace OpenBlack;

glm::mat4 Camera::getRotationMatrix() const
{
	glm::mat4 pitch, yaw, roll = glm::mat4(1.0f);

	pitch = glm::rotate(glm::mat4(1.0f), _rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
	yaw   = glm::rotate(glm::mat4(1.0f), _rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	roll  = glm::rotate(glm::mat4(1.0f), _rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

	return roll * pitch * yaw;
}

glm::mat4 Camera::GetViewProjectionMatrix() const
{
	glm::mat4 mRotation = getRotationMatrix();
	glm::mat4 mView     = mRotation * glm::translate(glm::mat4(1.0f), -_position);

	return GetProjectionMatrix() * mView;
}

void Camera::SetProjectionMatrixPerspective(float fov, float aspect, float nearclip, float farclip)
{
	_projectionMatrix = glm::perspective(glm::radians(fov), aspect, nearclip, farclip);
}

glm::vec3 Camera::GetForward() const
{
	glm::mat4 mRotation = getRotationMatrix();
	return -glm::vec3(mRotation[0][2], mRotation[1][2], mRotation[2][2]);
}

glm::vec3 Camera::GetRight() const
{
	glm::mat4 mRotation = getRotationMatrix();
	return glm::vec3(mRotation[0][0], mRotation[1][0], mRotation[2][0]);
}

glm::vec3 Camera::GetUp() const
{
	return glm::normalize(glm::cross(GetRight(), GetForward()));
}

void Camera::DeprojectScreenToWorld(const glm::ivec2 screenPosition, const glm::ivec2 screenSize, glm::vec3& out_worldOrigin, glm::vec3& out_worldDirection)
{
	const float normalizedX = (float)screenPosition.x / (float)screenSize.x;
	const float normalizedY = (float)screenPosition.y / (float)screenSize.y;

	const float screenSpaceX = (normalizedX - 0.5f) * 2.0f;
	const float screenSpaceY = ((1.0f - normalizedY) - 0.5f) * 2.0f;

	// The start of the ray trace is defined to be at mousex,mousey,1 in projection space (z=0 is near, z=1 is far - this gives us better precision)
	// To get the direction of the ray trace we need to use any z between the near and the far plane, so let's use (mousex, mousey, 0.5)
	const glm::vec4 rayStartProjectionSpace = glm::vec4(screenSpaceX, screenSpaceY, 0.0f, 1.0f);
	const glm::vec4 rayEndProjectionSpace   = glm::vec4(screenSpaceX, screenSpaceY, 0.5f, 1.0f);

	// Calculate our inverse view projection matrix
	glm::mat4 inverseViewProj = glm::inverse(GetViewProjectionMatrix());

	// Get our homogeneous coordinates for our start and end ray positions
	const glm::vec4 hgRayStartWorldSpace = inverseViewProj * rayStartProjectionSpace;
	const glm::vec4 hgRayEndWorldSpace   = inverseViewProj * rayEndProjectionSpace;

	glm::vec3 rayStartWorldSpace(hgRayStartWorldSpace.x, hgRayStartWorldSpace.y, hgRayStartWorldSpace.z);
	glm::vec3 rayEndWorldSpace(hgRayEndWorldSpace.x, hgRayEndWorldSpace.y, hgRayEndWorldSpace.z);

	// divide vectors by W to undo any projection and get the 3-space coord
	if (hgRayStartWorldSpace.w != 0.0f)
		rayStartWorldSpace /= hgRayStartWorldSpace.w;

	if (hgRayEndWorldSpace.w != 0.0f)
		rayEndWorldSpace /= hgRayEndWorldSpace.w;

	const glm::vec3 rayDirWorldSpace = glm::normalize(rayEndWorldSpace - rayStartWorldSpace);

	// finally, store the results in the outputs
	out_worldOrigin    = rayStartWorldSpace;
	out_worldDirection = rayDirWorldSpace;
}

void Camera::ProcessSDLEvent(const SDL_Event& e)
{
	if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
		handleKeyboardInput(e);
	else if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
		handleMouseInput(e);
}

void Camera::handleKeyboardInput(const SDL_Event& e)
{
	// ignore all repeated keys
	if (e.key.repeat > 0)
		return;

	if (e.key.keysym.scancode == SDL_SCANCODE_W)
		_velocity.z += (e.type == SDL_KEYDOWN) ? -1.0f : 1.0f;
	else if (e.key.keysym.scancode == SDL_SCANCODE_S)
		_velocity.z += (e.type == SDL_KEYDOWN) ? 1.0f : -1.0f;
	else if (e.key.keysym.scancode == SDL_SCANCODE_A)
		_velocity.x += (e.type == SDL_KEYDOWN) ? -1.0f : 1.0f;
	else if (e.key.keysym.scancode == SDL_SCANCODE_D)
		_velocity.x += (e.type == SDL_KEYDOWN) ? 1.0f : -1.0f;
	else if (e.key.keysym.scancode == SDL_SCANCODE_LCTRL)
		_velocity.y -= (e.type == SDL_KEYDOWN) ? 1.0f : -1.0f;
	else if (e.key.keysym.scancode == SDL_SCANCODE_SPACE)
		_velocity.y += (e.type == SDL_KEYDOWN) ? 1.0f : -1.0f;
}

void Camera::handleMouseInput(const SDL_Event& e)
{
	if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
	{
		if (e.button.button == SDL_BUTTON_MIDDLE)
			SDL_SetRelativeMouseMode((e.type == SDL_MOUSEBUTTONDOWN) ? SDL_TRUE : SDL_FALSE);
	}

	// Holding down the middle mouse button enables free look.
	if (e.type == SDL_MOUSEMOTION && e.motion.state & SDL_BUTTON(SDL_BUTTON_MIDDLE))
	{
		glm::vec3 rot = GetRotation();

		rot.y -= e.motion.xrel * _freeLookSensitivity * 0.1f;
		rot.x -= e.motion.yrel * _freeLookSensitivity * 0.1f;

		SetRotation(rot);
	}
}

void Camera::Update(double dt)
{
	_position += GetForward() * (float)(_velocity.z * _movementSpeed * dt);
	_position += GetUp() * (float)(_velocity.y * _movementSpeed * dt);
	_position += GetRight() * (float)(_velocity.x * _movementSpeed * dt);
}

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

#pragma once
#ifndef OPENBLACK_CAMERA_H
#define OPENBLACK_CAMERA_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <SDL_events.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace OpenBlack
{

class Camera
{

  public:
	Camera(glm::vec3 position, glm::vec3 rotation):
	    _position(position), _rotation(_rotation), _projectionMatrix(1.0f),
	    _movementSpeed(1.0f), _freeLookSensitivity(1.0f),
	    _velocity(0.0f, 0.0f, 0.0f) {}
	Camera():
	    Camera(glm::vec3(0.0f), glm::vec3(0.0f)) {}

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

	void DeprojectScreenToWorld(const glm::ivec2 screenPosition, const glm::ivec2 screenSize, glm::vec3& out_worldOrigin, glm::vec3& out_worldDirection);

	void Update(double dt);
	void ProcessSDLEvent(const SDL_Event&);

	void handleKeyboardInput(const SDL_Event&);
	void handleMouseInput(const SDL_Event&);

  private:
	glm::vec3 _position;
	glm::vec3 _rotation;
	glm::mat4 _projectionMatrix;

	glm::vec3 _velocity;
	float _movementSpeed;
	float _freeLookSensitivity;
};
} // namespace OpenBlack

#endif

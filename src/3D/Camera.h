/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <SDL_events.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <chrono>

namespace openblack
{

class Camera
{

  public:
	Camera(glm::vec3 position, glm::vec3 rotation)
		: _position(position)
		, _rotation(glm::radians(rotation))
		, _projectionMatrix(1.0f)
		, _velocity(0.0f, 0.0f, 0.0f)
		, _movementSpeed(0.0005f)
		, _freeLookSensitivity(1.0f)
	{
	}
	Camera():
	    Camera(glm::vec3(0.0f), glm::vec3(0.0f)) {}

	virtual ~Camera() = default;

	[[nodiscard]] glm::mat4 GetViewMatrix() const;
	[[nodiscard]] const glm::mat4& GetProjectionMatrix() const { return _projectionMatrix; }
	[[nodiscard]] virtual glm::mat4 GetViewProjectionMatrix() const;

	glm::vec3 GetPosition() const { return _position; }
	glm::vec3 GetRotation() const { return glm::degrees(_rotation); }

	void SetPosition(const glm::vec3& position) { _position = position; }
	void SetRotation(const glm::vec3& eulerDegrees) { _rotation = glm::radians(eulerDegrees); }

	void SetProjectionMatrixPerspective(float fov, float aspect, float nearclip, float farclip);
	void SetProjectionMatrix(const glm::mat4x4& projection) { _projectionMatrix = projection; }

	glm::vec3 GetForward() const;
	glm::vec3 GetRight() const;
	glm::vec3 GetUp() const;

	void DeprojectScreenToWorld(const glm::ivec2 screenPosition, const glm::ivec2 screenSize, glm::vec3& out_worldOrigin, glm::vec3& out_worldDirection);

	void Update(std::chrono::microseconds dt);
	void ProcessSDLEvent(const SDL_Event&);

	void handleKeyboardInput(const SDL_Event&);
	void handleMouseInput(const SDL_Event&);

	glm::mat4 getRotationMatrix() const;

  protected:
	glm::vec3 _position;
	glm::vec3 _rotation;

	glm::mat4 _projectionMatrix;

	glm::vec3 _velocity;
	float _movementSpeed;
	float _freeLookSensitivity;
};
} // namespace openblack

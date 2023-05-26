/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "CameraModel.h"
#include "ECS/Components/Transform.h"

namespace openblack
{

class Camera
{
public:
	Camera(glm::vec3, glm::vec3);
	Camera();

	virtual ~Camera() = default;

	[[nodiscard]] virtual float GetHorizontalFieldOfView() const;
	[[nodiscard]] virtual glm::mat4 GetViewMatrix() const;
	[[nodiscard]] const glm::mat4& GetProjectionMatrix() const;
	[[nodiscard]] virtual glm::mat4 GetViewProjectionMatrix() const;

	[[nodiscard]] std::optional<ecs::components::Transform> RaycastMouseToLand() const;
	[[nodiscard]] std::optional<ecs::components::Transform> RaycastScreenCoordToLand(glm::vec2 screenCoord) const;

	[[nodiscard]] glm::vec3 GetPosition() const;
	[[nodiscard]] glm::vec3 GetVelocity() const;
	/// Get rotation as euler angles in radians
	[[nodiscard]] glm::vec3 GetRotation() const;

	Camera& SetPosition(const glm::vec3& position);
	/// Set rotation as euler angles in radians
	Camera& SetRotation(const glm::vec3& eulerRadians);
	Camera& SetFocus(const glm::vec3& position);

	Camera& SetProjectionMatrixPerspective(float xFov, float aspect, float nearClip, float farClip);
	Camera& SetProjectionMatrix(const glm::mat4& projection);

	[[nodiscard]] glm::vec3 GetForward() const;
	[[nodiscard]] glm::vec3 GetRight() const;
	[[nodiscard]] glm::vec3 GetUp() const;

	[[nodiscard]] std::unique_ptr<Camera> Reflect(const glm::vec4& reflectionPlane) const;

	void DeprojectScreenToWorld(glm::vec2 screenCoord, glm::vec3& outWorldOrigin, glm::vec3& outWorldDirection) const;
	bool ProjectWorldToScreen(glm::vec3 worldPosition, glm::vec4 viewport, glm::vec3& outScreenPosition) const;

	void Update(std::chrono::microseconds dt);
	void HandleActions();

	[[nodiscard]] glm::mat4 GetRotationMatrix() const;

protected:
	glm::vec3 _position;
	glm::vec3 _rotation;
	float _xFov = 0.0f; // TODO(#707): This should be a zoomer for animations
	glm::mat4 _projectionMatrix;
	CameraModel _model;
};

} // namespace openblack

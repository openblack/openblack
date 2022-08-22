/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include <SDL_events.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ECS/Components/Transform.h"

union SDL_Event;

namespace openblack
{

class Camera
{

public:
	Camera(glm::vec3, glm::vec3);
	Camera()
	    : Camera(glm::vec3(0.0f), glm::vec3(0.0f))
	{
	}

	virtual ~Camera() = default;

	[[nodiscard]] virtual glm::mat4 GetViewMatrix() const;
	[[nodiscard]] const glm::mat4& GetProjectionMatrix() const { return _projectionMatrix; }
	[[nodiscard]] virtual glm::mat4 GetViewProjectionMatrix() const;

	[[nodiscard]] std::optional<ecs::components::Transform> RaycastMouseToLand();
	void FlyInit();
	void StartFlight();
	void ResetVelocities();

	[[nodiscard]] glm::vec3 GetPosition() const { return _position; }
	[[nodiscard]] glm::vec3 GetRotation() const { return _rotation; }
	[[nodiscard]] glm::vec3 GetVelocity() const { return _velocity; }
	[[nodiscard]] float GetMaxSpeed() const { return _maxMovementSpeed; }

	void SetPosition(const glm::vec3& position) { _position = position; }
	void SetRotation(const glm::vec3& eulerDegrees) { _rotation = glm::radians(eulerDegrees); }
	void SetRotationRadians(const glm::vec3& radians) { _rotation = radians; }

	void SetProjectionMatrixPerspective(float xFov, float aspect, float nearClip, float farClip);
	void SetProjectionMatrix(const glm::mat4x4& projection) { _projectionMatrix = projection; }

	[[nodiscard]] glm::vec3 GetForward() const;
	[[nodiscard]] glm::vec3 GetRight() const;
	[[nodiscard]] glm::vec3 GetUp() const;

	[[nodiscard]] std::unique_ptr<Camera> Reflect(const glm::vec4& relectionPlane) const;

	void DeprojectScreenToWorld(glm::ivec2 screenPosition, glm::ivec2 screenSize, glm::vec3& outWorldOrigin,
	                            glm::vec3& outWorldDirection);
	bool ProjectWorldToScreen(glm::vec3 worldPosition, glm::vec4 viewport, glm::vec3& outScreenPosition) const;

	void Update(std::chrono::microseconds dt);
	void ProcessSDLEvent(const SDL_Event&);

	void HandleKeyboardInput(const SDL_Event&);
	void HandleMouseInput(const SDL_Event&);

	[[nodiscard]] glm::mat4 GetRotationMatrix() const;

protected:
	glm::vec3 _position;
	glm::vec3 _rotation;
	glm::vec3 _dv;
	glm::vec3 _dwv;
	glm::vec3 _dsv;
	glm::vec3 _ddv;
	glm::vec3 _duv;
	glm::vec3 _drv;
	glm::mat4 _projectionMatrix;
	glm::vec3 _velocity;
	glm::vec3 _hVelocity;
	glm::vec3 _rotVelocity;
	float _accelFactor;
	float _movementSpeed;
	float _maxMovementSpeed;
	float _maxRotationSpeed;
	bool _lmouseIsDown;
	bool _mmouseIsDown;
	bool _mouseIsMoving;
	glm::ivec2 _mouseFirstClick;
	bool _shiftHeld;
	glm::ivec2 _handScreenVec;
	float _handDragMult;
	bool _flyInProgress;
	float _flyDist;
	float _flySpeed;
	float _flyStartAngle;
	float _flyEndAngle;
	float _flyThreshold;
	float _flyProgress;
	glm::vec3 _flyFromPos;
	glm::vec3 _flyToNorm;
	glm::vec3 _flyFromTan;
	glm::vec3 _flyToPos;
	glm::vec3 _flyToTan;
	glm::vec3 _flyPrevPos;
};

class ReflectionCamera: public Camera
{
public:
	ReflectionCamera()
	    : ReflectionCamera(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec4(0.0f))
	{
	}
	ReflectionCamera(glm::vec3 position, glm::vec3 rotation, glm::vec4 reflectionPlane)
	    : Camera(position, rotation)
	    , _reflectionPlane(reflectionPlane)
	{
	}
	[[nodiscard]] glm::mat4 GetViewMatrix() const override;

private:
	glm::vec4 _reflectionPlane;
	void ReflectMatrix(glm::mat4x4& m, const glm::vec4& plane) const;
};
} // namespace openblack

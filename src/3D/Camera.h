/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <SDL_events.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <chrono>
#include <memory>

namespace openblack
{

class Camera
{

public:
	Camera(glm::vec3 position, glm::vec3 rotation)
	    : _position(position)
	    , _viewCenterLand(_position)
	    , _rotation(glm::radians(rotation))
	    , _dwv(0.0f, 0.0f, 0.0f)
	    , _dv(0.0f, 0.0f, 0.0f)
	    , _drv(0.0f, 0.0f, 0.0f)
	    , _projectionMatrix(1.0f)
	    , _wVelocity(0.0f, 0.0f, 0.0f)
	    , _velocity(0.0f, 0.0f, 0.0f)
	    , _rotVelocity(0.0f, 0.0f, 0.0f)
	    , _groundHeight(0.0f)
	    , _groundHeightStart(0.0f)
	    , _movementSpeed(4.0f)
	    , _accelFactor(0.001f)
	    , _rotAccelFactor(0.001f)
	    , _maxMovementSpeed(0.005f)
	    , _maxRotationSpeed(0.005f)
	    , _freeLookSensitivity(1.0f)
	{
	}
	Camera()
	    : Camera(glm::vec3(0.0f), glm::vec3(0.0f))
	{
	}

	virtual ~Camera() = default;

	[[nodiscard]] virtual glm::mat4 GetViewMatrix() const;
	[[nodiscard]] const glm::mat4& GetProjectionMatrix() const { return _projectionMatrix; }
	[[nodiscard]] virtual glm::mat4 GetViewProjectionMatrix() const;

	[[nodiscard]] glm::vec3 GetPosition() const { return _position; }
	[[nodiscard]] glm::vec3 GetRotation() const { return glm::degrees(_rotation); }
	[[nodiscard]] glm::vec3 GetVelocity() const { return _velocity; }
	[[nodiscard]] float GetMaxSpeed() const { return _maxMovementSpeed; }

	void SetPosition(const glm::vec3& position) { _position = position; }
	void SetRotation(const glm::vec3& eulerDegrees) { _rotation = glm::radians(eulerDegrees); }

	void SetProjectionMatrixPerspective(float xFov, float aspect, float nearClip, float farClip);
	void SetProjectionMatrix(const glm::mat4x4& projection) { _projectionMatrix = projection; }

	[[nodiscard]] glm::vec3 GetForward() const;
	[[nodiscard]] glm::vec3 GetRight() const;
	[[nodiscard]] glm::vec3 GetUp() const;
	[[nodiscard]] float GetGroundPlaneAltitude() const;

	[[nodiscard]] std::unique_ptr<Camera> Reflect(const glm::vec4& relectionPlane) const;

	void DeprojectScreenToWorld(const glm::ivec2 screenPosition, const glm::ivec2 screenSize, glm::vec3& out_worldOrigin,
	                            glm::vec3& out_worldDirection);
	bool ProjectWorldToScreen(const glm::vec3 worldPosition, const glm::vec4 viewport, glm::vec3& out_screenPosition) const;

	void Update(std::chrono::microseconds dt);
	void ProcessSDLEvent(const SDL_Event&);

	void handleKeyboardInput(const SDL_Event&);
	void handleMouseInput(const SDL_Event&);

	[[nodiscard]] glm::mat4 GetRotationMatrix() const;

protected:
	glm::vec3 _position;
	glm::vec3 _rotation;
	glm::vec3 _dwv;
	glm::vec3 _dv;
	glm::vec3 _drv;
	glm::vec3 _viewCenterLand;

	glm::mat4 _projectionMatrix;

	glm::vec3 _velocity;
	glm::vec3 _wVelocity;
	glm::vec3 _rotVelocity;
	float _groundHeight;
	float _groundHeightStart;
	float _accelFactor;
	float _rotAccelFactor;
	float _movementSpeed;
	float _maxMovementSpeed;
	float _maxRotationSpeed;
	float _freeLookSensitivity;
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
	void reflectMatrix(glm::mat4x4& m, const glm::vec4& plane) const;
};
} // namespace openblack

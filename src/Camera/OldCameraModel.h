/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec2.hpp>

#include "CameraModel.h"

namespace openblack
{

class OldCameraModel final: public CameraModel
{
public:
	OldCameraModel();
	~OldCameraModel() override;

	std::optional<CameraInterpolationUpdateInfo> Update(std::chrono::microseconds dt, const Camera& camera) final;
	void HandleActions(std::chrono::microseconds dt) final;
	[[nodiscard]] glm::vec3 GetTargetPosition() const final;
	[[nodiscard]] glm::vec3 GetTargetFocus() const final;
	[[nodiscard]] std::chrono::seconds GetIdleTime() const final;

private:
	void ResetVelocities();

	glm::vec3 _targetPosition;
	glm::vec3 _targetRotation;
	glm::vec3 _dv = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _dwv = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _dsv = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _ddv = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _duv = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _drv = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _hVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _rotVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
	float _accelFactor = 0.001f;
	float _movementSpeed = 1.5f;
	float _maxMovementSpeed = 0.005f;
	float _maxRotationSpeed = 0.005f;
	bool _lmouseIsDown = false;
	bool _mmouseIsDown = false;
	bool _mouseIsMoving = false;
	glm::ivec2 _mouseFirstClick = glm::ivec2(0, 0);
	bool _shiftHeld = false;
	glm::ivec2 _handScreenVec = glm::ivec2(0, 0);
	float _handDragMult = 0.0f;
	bool _flyInProgress = false;
	float _flyDist = 0.0f;
	float _flySpeed = 0.5f;
	float _flyStartAngle = 10.0f;
	float _flyEndAngle = 30.0f;
	float _flyThreshold = 50.0f;
	float _flyProgress = 1.0f;
	glm::vec3 _flyFromPos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _flyToNorm = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _flyFromTan = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _flyToPos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _flyToTan = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _flyPrevPos = glm::vec3(0.0f, 0.0f, 0.0f);
};
} // namespace openblack

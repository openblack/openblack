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
	void FlyInit();
	void ResetVelocities();

	glm::vec3 _targetPosition;
	glm::vec3 _targetRotation;
	glm::vec3 _dv;
	glm::vec3 _dwv;
	glm::vec3 _dsv;
	glm::vec3 _ddv;
	glm::vec3 _duv;
	glm::vec3 _drv;
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
} // namespace openblack

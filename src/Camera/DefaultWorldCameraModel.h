/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <glm/vec2.hpp>

#include "CameraModel.h"

class TestDefaultCameraModel;
class TestDefaultCameraModel_single_line_Test;

namespace openblack
{
class DefaultWorldCameraModel final: public CameraModel
{
	enum class Mode : std::uint8_t
	{
		Cartesian,
		Polar,
		DraggingLandscape,
		FlyingToPoint,
	};

public:
	DefaultWorldCameraModel(glm::vec3 origin, glm::vec3 focus);
	~DefaultWorldCameraModel() final;

	std::optional<CameraInterpolationUpdateInfo> Update(std::chrono::microseconds dt, const Camera& camera) final;
	void HandleActions(std::chrono::microseconds dt) final;
	[[nodiscard]] glm::vec3 GetTargetOrigin() const final;
	[[nodiscard]] glm::vec3 GetTargetFocus() const final;
	[[nodiscard]] std::chrono::seconds GetIdleTime() const final;

private:
	void UpdateCameraInterpolationValues(const Camera& camera);
	void UpdateRaycastHitPoints(const Camera& camera);
	void UpdateFocusDistance();

	void UpdateMode(const Camera& camera, glm::vec3 eulerAngles, float zoomDelta, glm::uvec2 mouseCurrent,
	                float mouseMovementDistance);
	void UpdateModeCartesian();
	void UpdateModePolar(glm::vec3 eulerAngles, bool recalculatePoint);
	void UpdateModeDragging(const Camera& camera, glm::u16vec2 mouseCurrent, float mouseMovementDistance);
	void UpdateModeFlying(const Camera& camera, glm::vec3 eulerAngles);

	/// Updates the model's focus point parameters after a change in position or focus point of view
	void UpdateFocusPointInteractionParameters(glm::vec3 origin, glm::vec3 focus, glm::vec3 eulerAngles, const Camera& camera);
	/// Modifies the given Euler angles based on the rotate Around and keyboard Move Deltas for rotation and zoom.
	/// @param eulerAngles A reference representing Euler angles (yaw, pitch, roll) to be adjusted. Roll is always 0.
	void TiltZoom(glm::vec3& eulerAngles, float scalingFactor, float zoomDelta);
	/// Computes the harmonic mean of the distances from a point of origin to a set of points determined by raycasting in screen
	/// space.
	///
	/// The function casts 16 rays from the center of the screen to vertically distributed points on the screen.
	/// The harmonic mean of these distances is then calculated by averaging their reciprocals and taking the reciprocal of that
	/// average.
	///
	/// @return The harmonic mean of the distances from the origin to each hit point.
	[[nodiscard]] float GetVerticalLineInverseDistanceWeighingRayCast(const Camera& camera) const;

	bool ConstrainCamera(std::chrono::microseconds dt, float mouseMovementDistance, glm::vec3 eulerAngles,
	                     const Camera& camera);
	/// Corrects altitude of the camera
	/// @return If a modification to the camera position was applied.
	bool ConstrainAltitude();
	/// Corrects distance of the camera from the island
	/// @return If a modification to the camera position was applied.
	bool ConstrainDisc();

	[[nodiscard]] glm::vec3 GetTargetForwardVector() const;
	[[nodiscard]] glm::vec3 GetTargetForwardUnitVector() const;
	[[nodiscard]] glm::vec3 ProjectPointOnForwardVector(float distanceFromOrigin) const;

	[[nodiscard]] std::optional<CameraInterpolationUpdateInfo> ComputeUpdateReturnInfo(bool originHasBeenAdjusted,
	                                                                                   std::chrono::duration<float> t);

	Mode _mode = Mode::Cartesian;
	Mode _modePrev = _mode;

	// Values from camera state where the camera has interpolated to.
	glm::vec3 _currentOrigin = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _currentFocus = glm::vec3(0.0f, 0.0f, 0.0f);

	// Values from target camera state which the camera may interpolate to. Not the current camera state.
	glm::vec3 _targetOrigin = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _targetFocus = glm::vec3(0.0f, 0.0f, 0.0f);

	std::optional<glm::vec3> _screenSpaceMouseRaycastHit;
	std::optional<glm::vec3> _screenSpaceCenterRaycastHit;

	// State of input Action
	glm::vec3 _rotateAroundDelta = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec2 _keyBoardMoveDelta = glm::vec2(0.0f, 0.0f);

	float _focusDistance = 0.0f;

	// Estimate of camera to island geometry
	float _averageIslandDistance = 0.0f;

	// Updated at the start of a click+drag or keyboard input
	// Only useful for interaction.
	float _originFocusDistanceAtInteractionStart = 0.0f;
	glm::vec3 _originToHandPlaneNormal = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _originAtClick = glm::vec3(0.0f, 0.0f, 0.0f);
	float _alignmentAtInteractionStart = 0.0f;
	glm::vec3 _focusAtClick = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::u16vec2 _mouseAtClick = glm::u16vec2(0.0f, 0.0f);
	std::chrono::duration<float> _elapsedTime = std::chrono::duration<float>::zero();
	std::optional<FlightPath> _flightPath;

	// For unit testing
	friend TestDefaultCameraModel;
	friend TestDefaultCameraModel_single_line_Test;
};
} // namespace openblack

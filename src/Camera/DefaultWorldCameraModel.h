/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
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
#include "CameraPan.h"
#include "Common/Zoomer.h"

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
		ArcBall,
		DraggingLandscape,
		FlyingToPoint,
	};

public:
	DefaultWorldCameraModel();
	~DefaultWorldCameraModel() final;

	std::optional<CameraInterpolationUpdateInfo> Update(std::chrono::microseconds dt, const Camera& camera) final;
	void HandleActions(std::chrono::microseconds dt) final;
	void SetFlight(glm::vec3 origin, glm::vec3 focus) final;
	[[nodiscard]] glm::vec3 GetTargetOrigin() const final;
	[[nodiscard]] glm::vec3 GetTargetFocus() const final;
	[[nodiscard]] std::chrono::seconds GetIdleTime() const final;
	[[nodiscard]] HandCues GetHandCues() const final;

private:
	void UpdateCameraInterpolationValues(const Camera& camera);
	void UpdateRaycastHitPoints(const Camera& camera);
	void UpdateFocusDistance();
	/// What a drag of the land does, from where it was pressed and how the mouse moves: pans, turns round the edge or
	/// tilts. Gives the mode the camera takes for it.
	[[nodiscard]] Mode HandleDrag(bool held);
	/// The height the mouse controls measure by, the cinema bars' picture's while they are in
	[[nodiscard]] static int ViewHeight(glm::ivec2 screenSize);

	void UpdateMode(const Camera& camera, glm::vec3 eulerAngles, float zoomDelta, glm::uvec2 mouseCurrent);
	void UpdateModeCartesian();
	void UpdateModePolar(glm::vec3 eulerAngles, bool recalculatePoint);
	void UpdateModeArcBall(glm::vec3 eulerAngles, glm::u16vec2 mouseCurrent, float xFov);
	void UpdateModeDragging(const Camera& camera, glm::u16vec2 mouseCurrent);
	void UpdateModeFlying(glm::vec3 eulerAngles);

	/// Updates the model's focus point parameters after a change in position or focus point of view
	void UpdateFocusPointInteractionParameters(glm::vec3 origin, glm::vec3 focus, glm::vec3 eulerAngles, const Camera& camera);
	/// Modifies the given Euler angles based on the rotate Around and keyboard Move Deltas for rotation and zoom.
	/// @param eulerAngles A reference representing Euler angles (yaw, pitch, roll) to be adjusted. Roll is always 0.
	void TiltZoom(glm::vec3& eulerAngles, float scalingFactor, float zoomDelta);
	/// How far a unit of zoom input moves the camera, growing with the camera's height above its focus
	[[nodiscard]] float GetZoomScale() const;
	/// Computes the harmonic mean of the distances from a point of origin to a set of points determined by raycasting in screen
	/// space.
	///
	/// The function casts 16 rays from the center of the screen to vertically distributed points on the screen.
	/// The harmonic mean of these distances is then calculated by averaging their reciprocals and taking the reciprocal of that
	/// average.
	///
	/// @return The harmonic mean of the distances from the origin to each hit point.
	[[nodiscard]] float GetVerticalLineInverseDistanceWeighingRayCast(const Camera& camera) const;

	void ComputeDistanceFromBoundY();
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
	                                                                                   std::chrono::microseconds t);

	Mode _mode = Mode::Cartesian;
	Mode _modePrev = _mode;

	// Values from camera state where the camera has interpolated to.
	glm::vec3 _currentOrigin = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _currentFocus = glm::vec3(0.0f, 0.0f, 0.0f);

	// Values from target camera state which the camera may interpolate to. Not the current camera state.
	glm::vec3 _targetOrigin = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _targetFocus = glm::vec3(0.0f, 0.0f, 0.0f);
	float _arcBallRadius = 0.0f;

	std::optional<glm::vec3> _screenSpaceMouseRaycastHit;
	std::optional<glm::vec3> _screenSpaceMouseRaycastHitAtClick;
	std::optional<glm::vec3> _screenSpaceCenterRaycastHit;

	// State of input Action
	glm::vec3 _rotateAroundDelta = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec2 _keyBoardMoveDelta = glm::vec2(0.0f, 0.0f);
	std::optional<glm::vec3> _handPosition;

	float _focusDistance = 0.0f;
	float _distanceFromBoundY = 0.0f;

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
	std::chrono::microseconds _elapsedTime = std::chrono::microseconds::zero();
	std::optional<FlightPath> _flightPath;

	/// The camera hints where the cursor is, with nothing dragged
	uint32_t _tricons {camera_drag::tricon::k_Idle};
	/// A drag of the land, and where the cursor is held dragging round the edge
	bool _dragging {false};
	camera_drag::DragClassifier _drag;
	glm::ivec2 _ringCursor {0, 0};
	/// Where the land was gripped as the drag was pressed: the camera then, the cursor, the plane the land is dragged
	/// across, whether there was land under the cursor and how far ahead it was
	struct LandGrip
	{
		glm::vec3 origin;
		glm::vec3 focus;
		glm::u16vec2 cursor;
		camera_pan::GripPlane plane;
		bool land;
		float depth;
	};
	std::optional<LandGrip> _landGrip;
	/// What the camera lets the player do this frame, as the scripts allow and a fight changes it
	uint32_t _features {camera_drag::k_DefaultFeatures};
	/// A drag gripping land too far ahead is given up until the buttons are let go
	bool _dragGivenUp {false};
	camera_drag::TwoButtonTurn _twoButtonTurn;
	/// How far the clear view of Ctrl and Shift held together has come, easing in and out over half a second
	Zoomer _clearView;
	/// Time spent handling the controls, for timing the start of a drag
	std::chrono::microseconds _controlsTime {std::chrono::microseconds::zero()};

	// For unit testing
	friend TestDefaultCameraModel;
	friend TestDefaultCameraModel_single_line_Test;
};
} // namespace openblack

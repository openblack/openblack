/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "DefaultWorldCameraModel.h"

#include <numeric>

#include <glm/gtc/constants.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/polar_coordinates.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <spdlog/spdlog.h>

#include "3D/LandIsland.h"
#include "Camera.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "Game.h"
#include "Input/GameActionMapInterface.h"
#include "Locator.h"

using namespace openblack;

// TODO(#708): Add to global configurations
constexpr auto k_WheelZoomFactor = 20.0f;
constexpr auto k_InteractionSpeedMultiplier = 400.0f;
// Vanilla black and white uses a pretty bad PI/2 approximation
constexpr auto k_CameraModelHalfPi = 1.53938043f;
constexpr auto k_CameraInteractionStepSize = 3.0f;
constexpr auto k_MinimalCameraAnimationDuration = std::chrono::duration<float> {1.5f};
constexpr auto k_HandDragVectorAlignmentThreshold = 0.001f;
constexpr auto k_HandDragVectorAlignmentRatioThreshold = 0.000'1f;

glm::vec3 EulerFromPoints(glm::vec3 p0, glm::vec3 p1)
{
	const auto diff = p0 - p1;
	// If the camera is directly above the focus point, set pitch to 90 degrees.
	if (glm::all(glm::lessThan(glm::abs(glm::xz(diff)), glm::vec2(0.1f, 0.1f))))
	{
		return {0.0f, k_CameraModelHalfPi, 0.0f};
	}
	// Otherwise, calculate yaw and pitch based on the direction to the focus point.
	return {glm::pi<float>() - glm::atan(diff.x, -diff.z), glm::atan(diff.y, glm::length(glm::xz(diff))), 0.0f};
}

DefaultWorldCameraModel::DefaultWorldCameraModel(glm::vec3 origin, glm::vec3 focus)
    : _targetOrigin(origin)
    , _targetFocus(focus)
    , _focusDistance(glm::distance(_targetOrigin, _targetFocus))
{
}

DefaultWorldCameraModel::~DefaultWorldCameraModel() = default;

void DefaultWorldCameraModel::TiltZoom(glm::vec3& eulerAngles, float scalingFactor, float zoomDelta)
{
	// Update the camera's yaw if there's significant horizontal movement.
	if (glm::abs(_rotateAroundDelta.y) > glm::epsilon<float>())
	{
		const auto windowSize = Locator::windowing::value().GetSize();
		eulerAngles.x += _rotateAroundDelta.y * glm::pi<float>() / windowSize.x;
	}

	// Update the camera's pitch if there's significant vertical movement.
	if (glm::abs(_rotateAroundDelta.x) > glm::epsilon<float>())
	{
		eulerAngles.y -= _rotateAroundDelta.x * 0.002f;
		// Clamp the pitch angle to keep the camera within between -30 and 78.75 degrees.
		eulerAngles.y = glm::clamp(eulerAngles.y, -1.0f / 6.0f * glm::pi<float>(), 7.0f / 16.0f * glm::pi<float>());
		// TODO: special case if rotate_around_pressed is false
	}

	{
		// Compute a keyboard input offset based on the current pitch and yaw.
		const auto clampedTan = glm::clamp(glm::tan(eulerAngles.y), 0.2f, 2.0f);
		const auto localMovement = _keyBoardMoveDelta * glm::vec2(1.0f / clampedTan, -1.0f) * scalingFactor * 0.001f;
		const auto planarMovement = glm::vec2(glm::cos(-eulerAngles.x), glm::sin(-eulerAngles.x));
		const auto offset = glm::vec3(localMovement.y * planarMovement.x - localMovement.x * planarMovement.y, 0.0,
		                              glm::dot(localMovement, planarMovement));
		_targetOrigin += offset;
		_targetFocus += offset;
		_focusAtClick += offset;
	}

	_averageIslandDistance += zoomDelta;
	_averageIslandDistance = glm::max(_averageIslandDistance, k_CameraInteractionStepSize + 0.1f);
}

float DefaultWorldCameraModel::GetVerticalLineInverseDistanceWeighingRayCast(const Camera& camera) const
{
	std::vector<float> inverseHitDistances;
	inverseHitDistances.reserve(0x10);

	for (int i = 0; i < 0x10; ++i)
	{
		const glm::vec2 coord = glm::vec2(0.5f, i / 16.0f);

		if (const auto hit = camera.RaycastScreenCoordToLand(coord, false, Camera::Interpolation::Target))
		{
			inverseHitDistances.push_back(1.0f / glm::length(hit->position - _targetOrigin));
		}
	}

	// Default distance of 50 if no hits happen, therefore divide by size + 1
	const auto average = std::accumulate(inverseHitDistances.cbegin(), inverseHitDistances.cend(), 1.0f / 50.0f) /
	                     (inverseHitDistances.size() + 1);
	return 1.0f / average;
}

bool DefaultWorldCameraModel::ConstrainCamera(std::chrono::microseconds dt, float mouseMovementDistance, glm::vec3 eulerAngles,
                                              const Camera& camera)
{
	const auto originBackup = _targetOrigin;
	bool originHasBeenAdjusted = false;
	originHasBeenAdjusted |= ConstrainAltitude();
	originHasBeenAdjusted |= ConstrainDisc();

	const auto dtSeconds = std::chrono::duration_cast<std::chrono::duration<float>>(dt).count();
	const auto threshold = glm::max(300.0f, 1.6f * _focusDistance * dtSeconds);
	if ((mouseMovementDistance == 0.0f && glm::distance2(_targetOrigin, originBackup) > threshold * threshold) ||
	    originHasBeenAdjusted)
	{
		_originFocusDistanceAtInteractionStart =
		    glm::max(glm::distance(_targetOrigin, _targetFocus), k_CameraInteractionStepSize + 0.1f);
		UpdateFocusPointInteractionParameters(_targetOrigin, _targetFocus, eulerAngles, camera);
	}

	return originHasBeenAdjusted;
}

bool DefaultWorldCameraModel::ConstrainAltitude()
{
	constexpr float k_floatingHeight = 2.9999f; // 3 in vanilla, but less due to fp precision with recorded data in tests
	// constexpr float nearClip = 0.0f;       // TODO sync with game args
	bool hasBeenAdjusted = false;
	// const auto nearPoint = ProjectPointOnForwardVector(nearClip);
	const auto minAltitude = k_floatingHeight + Locator::terrainSystem::value().GetHeightAt(glm::xz(_targetOrigin));
	// if (const auto hit = Locator::dynamicsSystem::value().RayCastClosestHit(_targetOrigin, _targetFocus, 16.0f))
	// {
	// 	if (glm::distance2(hit->first.position, _targetOrigin) < 64.0f)
	// 	{
	// 		minAltitude = glm::max(floatingHeight + hit->first.position.y, minAltitude);
	// 	}
	// }
	if (_targetOrigin.y < minAltitude)
	{
		_targetOrigin.y = minAltitude;
		hasBeenAdjusted = true;
	}

	if (_rotateAroundDelta.x != 0.0f)
	{
		_targetFocus = _targetOrigin + glm::normalize(_targetFocus - _targetOrigin) * _focusDistance;
	}

	return hasBeenAdjusted;
}

bool DefaultWorldCameraModel::ConstrainDisc()
{
	bool hasBeenAdjusted = false;
	constexpr auto k_discCentre = glm::vec3(2560.0f, 0.0f, 2560.0f);
	constexpr auto k_discRadius = 5120.0f;

	const auto delta = _targetOrigin - k_discCentre;
	const auto distance2 = glm::length2(delta);

	if (distance2 > k_discRadius * k_discRadius)
	{
		_targetOrigin = k_discCentre + delta * (k_discRadius / glm::sqrt(distance2));
		hasBeenAdjusted = true;
	}

	return hasBeenAdjusted;
}

void DefaultWorldCameraModel::UpdateFocusPointInteractionParameters(glm::vec3 origin, glm::vec3 focus, glm::vec3 eulerAngles,
                                                                    const Camera& camera)
{
	_originAtClick = _targetOrigin;
	_focusAtClick = _targetFocus;
	_mouseAtClick = Locator::gameActionSystem::value().GetMousePosition();
	_originFocusDistanceAtInteractionStart = glm::distance(origin, focus);
	// TODO(#713): calculate a y-basis based on the projection on land of camera origin and hand
	_originToHandPlaneNormal = glm::vec3(0.0f, 1.0f, 0.0f);
	// TODO(#713): Calculate the with _originToHandPlaneNormal and the mouse hit point to put in _alignmentAtInteractionStart
	_averageIslandDistance = GetVerticalLineInverseDistanceWeighingRayCast(camera);
	{
		// TODO: factor out this bit of code for euler angles
		// diff is different from other
		const auto diff = _targetOrigin - _targetFocus;
		// If the camera is directly above the focus point, set pitch to 90 degrees.
		if (glm::all(glm::lessThan(glm::abs(glm::xz(diff)), glm::vec2(0.1f, 0.1f))))
		{
			eulerAngles = glm::vec3(0.0f, glm::half_pi<float>(), 0.0f);
		}
		// Otherwise, calculate yaw and pitch based on the direction to the focus point.
		else
		{
			eulerAngles =
			    glm::vec3(glm::pi<float>() - glm::atan(diff.x, -diff.z), glm::atan(diff.y, glm::length(glm::xz(diff))), 0.0f);
		}
	}
	const auto extra =
	    (_focusDistance - _averageIslandDistance) * glm::clamp(eulerAngles.y / (6.0f * glm::pi<float>()), 0.0f, 1.0f);
	_averageIslandDistance += extra;
}

// This is a vanilla-style deprojection but with a few math errors
glm::vec3 ScreenToWorld(const Camera& camera, glm::u16vec2 pixelCoord)
{
	const auto& window = Locator::windowing ::value();

	const auto screenSize = window.GetSize();
	const auto aspect = window.GetAspectRatio();
	const auto yFov = 2.0f * glm::atan(1.0f / camera.GetProjectionMatrix()[1][1]);

	const auto screenCoord = static_cast<glm::vec2>(pixelCoord) / static_cast<glm::vec2>(screenSize);
	const auto clipSpaceCoordinates = ((screenCoord - 0.5f) * 2.0f) * glm::vec2(1.0f, -1.0f);

	// The multiplication by aspect within the tan is odd and wrong
	const auto baseTanHalfFov = glm::tan(yFov / 2.0f * aspect);
	// Using baseTanHalfFov directly to scale is also odd
	const auto extents = glm::vec3(baseTanHalfFov, baseTanHalfFov / aspect, 1.0f);
	const auto point = glm::vec4(glm::vec3(clipSpaceCoordinates, 1.0f) * extents, 1.0f);

	return camera.GetRotationMatrix() * point;
}

void DefaultWorldCameraModel::UpdateMode(const Camera& camera, glm::vec3 eulerAngles, float zoomDelta, glm::uvec2 mouseCurrent,
                                         float mouseMovementDistance)
{
	switch (_mode)
	{
	case Mode::Cartesian:
		UpdateModeCartesian();
		break;
	case Mode::Polar:
		UpdateModePolar(eulerAngles, zoomDelta == 0.0f);
		break;
	case Mode::DraggingLandscape:
		UpdateModeDragging(camera, mouseCurrent, mouseMovementDistance);
		break;
	}

	if (_rotateAroundDelta.x != 0.0f)
	{
		const auto diff = _targetFocus - _targetOrigin;
		_targetOrigin = _currentOrigin;
		_targetFocus = _targetOrigin + diff;
		_focusAtClick = _targetFocus;
	}
}

void DefaultWorldCameraModel::UpdateModeCartesian()
{
	// Drag focus on land
	const auto dist =
	    _screenSpaceCenterRaycastHit.has_value() ? glm::distance(*_screenSpaceCenterRaycastHit, _targetOrigin) : _focusDistance;
	_targetFocus = ProjectPointOnForwardVector(glm::max(dist - 1.0f, 0.1f));
	_focusAtClick = _targetFocus;
}

void DefaultWorldCameraModel::UpdateModePolar(glm::vec3 eulerAngles, bool recalculatePoint)
{
	// Pitch should already be clamped in TiltZoom

	// Put average distance point on half-line from camera
	const auto averageIslandPoint = _targetOrigin + _averageIslandDistance * glm::normalize(GetTargetForwardVector());

	// Rotate camera origin based on euler angles as polar coordinates and focus and distance at interaction
	_targetOrigin = _focusAtClick + _originFocusDistanceAtInteractionStart * glm::euclidean(glm::yx(eulerAngles));

	if (recalculatePoint)
	{
		const auto diff = averageIslandPoint - ProjectPointOnForwardVector(_averageIslandDistance);
		_targetOrigin += diff;
		_focusAtClick += diff;
	}
	_targetFocus = _focusAtClick;
}

void DefaultWorldCameraModel::UpdateModeDragging(const Camera& camera, glm::u16vec2 mouseCurrent, float mouseMovementDistance)
{
	// TODO(#713): Check on optional of _originToHandPlaneNormal to see if there even is a valid grabbing

	// deproject with camera at origin (0, 0, 0), you can use the current
	// rotation, because during this operation, we cannot rotate so the
	// current rotation will be the same as rotation at click
	// Use the current projection matrix too because we don't want fov
	// changes to affect the current operation.
	// In fact, using the regular deprojection followed by removing the
	// current translation will have the same effect
	const auto depth = glm::mix(0.3f, 3.5f, glm::clamp(camera.GetOrigin().y / 20.0f, 0.0f, 1.0f));
	const auto mouseDeltaCurrent = ScreenToWorld(camera, mouseCurrent) * depth;
	const auto mouseDeltaAtClick = ScreenToWorld(camera, _mouseAtClick) * depth;

	// Get dot product of mouse-camera vector and the normal of the up maximizing plane between the camera and hand
	// Note that mouseAlignment could be unit vectors, but they are scaled by the near plane
	const auto mouseAlignmentCurrent = glm::dot(mouseDeltaCurrent, _originToHandPlaneNormal);
	const auto mouseAlignmentAtClick = glm::dot(mouseDeltaAtClick, _originToHandPlaneNormal);

	if ((mouseAlignmentCurrent < -k_HandDragVectorAlignmentThreshold &&
	     mouseAlignmentAtClick < -k_HandDragVectorAlignmentThreshold) ||
	    (mouseAlignmentCurrent > k_HandDragVectorAlignmentThreshold &&
	     mouseAlignmentAtClick > k_HandDragVectorAlignmentThreshold))
	{
		const auto originAlignmentAtClick = glm::dot(_originAtClick, _originToHandPlaneNormal);
		const auto alignmentDelta = _alignmentAtInteractionStart - originAlignmentAtClick;

		const auto alignmentRatioCurrent = alignmentDelta / mouseAlignmentCurrent;
		const auto alignmentRatioAtClick = alignmentDelta / mouseAlignmentAtClick;

		if (alignmentRatioCurrent > k_HandDragVectorAlignmentRatioThreshold &&
		    alignmentRatioAtClick > k_HandDragVectorAlignmentRatioThreshold)
		{
			auto movement = mouseDeltaCurrent * alignmentRatioCurrent - mouseDeltaAtClick * alignmentRatioAtClick;
			const auto movementDistance2 = glm::length2(movement);
			if (movementDistance2 > mouseMovementDistance)
			{
				movement *= mouseMovementDistance * glm::sqrt(movementDistance2);
			}

			_targetOrigin = _originAtClick - movement;
			_targetFocus = _focusAtClick - movement;

			_targetOrigin =
			    _targetFocus + glm::normalize(_targetOrigin - _targetFocus) * _originFocusDistanceAtInteractionStart;
		}
	}
}

void DefaultWorldCameraModel::UpdateCameraInterpolationValues(const Camera& camera)
{
	// Get current curve interpolated values from camera
	_currentOrigin = camera.GetOrigin(Camera::Interpolation::Current);
	_currentFocus = camera.GetFocus(Camera::Interpolation::Current);
	_targetOrigin = camera.GetOrigin(Camera::Interpolation::Target);
	_targetFocus = camera.GetFocus(Camera::Interpolation::Target);
}

void DefaultWorldCameraModel::UpdateRaycastHitPoints(const Camera& camera)
{
	// Raycast mouse and screen center
	// TODO(#656) in c++23 use camera.RaycastMouseToLand().and_then
	_screenSpaceMouseRaycastHit = std::nullopt;
	_screenSpaceCenterRaycastHit = std::nullopt;
	if (const auto hit = camera.RaycastMouseToLand(false, Camera::Interpolation::Target))
	{
		_screenSpaceMouseRaycastHit = hit->position;
	}
	if (const auto hit = camera.RaycastScreenCoordToLand({0.5f, 0.5f}, false, Camera::Interpolation::Target))
	{
		_screenSpaceCenterRaycastHit = hit->position;
	}
	else
	{
		_screenSpaceCenterRaycastHit = std::nullopt;
	}
}

void DefaultWorldCameraModel::UpdateFocusDistance()
{
	// TODO(#656) in c++23 use _screenSpaceCenterRaycastHit.and_then
	if (_screenSpaceCenterRaycastHit.has_value())
	{
		_focusDistance = glm::max(10.0f, glm::distance(_screenSpaceCenterRaycastHit.value(), _targetOrigin));
	}
	else
	{
		_focusDistance = glm::max(10.0f, _averageIslandDistance);
	}
}

std::optional<CameraModel::CameraInterpolationUpdateInfo> DefaultWorldCameraModel::Update(std::chrono::microseconds dt,
                                                                                          const Camera& camera)
{
	_elapsedTime += dt;

	UpdateCameraInterpolationValues(camera);
	UpdateRaycastHitPoints(camera);
	UpdateFocusDistance();

	// Get angles (yaw, pitch, roll). Roll is always 0
	glm::vec3 eulerAngles = EulerFromPoints(_targetOrigin, _focusAtClick);

	if (_mode != _modePrev)
	{
		UpdateFocusPointInteractionParameters(camera.GetOrigin(Camera::Interpolation::Target),
		                                      camera.GetFocus(Camera::Interpolation::Target), eulerAngles, camera);
	}

	// Get step size
	const auto scalingFactor = 60.0f;
	const auto zoomDelta = _rotateAroundDelta.z * 0.0015f * scalingFactor;

	if (_mode == Mode::Polar)
	{
		// Adjust camera's orientation based on user input. Call will reset deltas.
		TiltZoom(eulerAngles, scalingFactor, zoomDelta);
	}

	const auto mouseCurrent = Locator::gameActionSystem::value().GetMousePosition();
	_originFocusDistanceAtInteractionStart =
	    glm::max(_originFocusDistanceAtInteractionStart + zoomDelta, k_CameraInteractionStepSize + 0.1f);
	const auto mouseMovementDistance =
	    glm::max(glm::distance(static_cast<glm::vec2>(mouseCurrent), static_cast<glm::vec2>(_mouseAtClick)) *
	                 _originFocusDistanceAtInteractionStart * 0.11f,
	             50.0f);

	UpdateMode(camera, eulerAngles, zoomDelta, mouseCurrent, mouseMovementDistance);

	const bool originHasBeenAdjusted = ConstrainCamera(dt, mouseMovementDistance, eulerAngles, camera);

	return ComputeUpdateReturnInfo(originHasBeenAdjusted, camera.GetInterpolatorTime());
}

std::optional<CameraModel::CameraInterpolationUpdateInfo>
DefaultWorldCameraModel::ComputeUpdateReturnInfo(bool originHasBeenAdjusted, std::chrono::duration<float> t)
{
	static constinit auto kTimeThreshold = std::chrono::duration<float> {1.5f};
	auto duration = std::chrono::duration<float> {0.3f};
	if (originHasBeenAdjusted)
	{
		duration *= 2.0f;
	}
	if (_elapsedTime <= kTimeThreshold)
	{
		duration = std::chrono::duration<float> {
		    glm::mix(k_MinimalCameraAnimationDuration.count(), duration.count(), (_elapsedTime / kTimeThreshold))};
	}
	return {{GetTargetOrigin(), GetTargetFocus(), duration}};
}

void DefaultWorldCameraModel::HandleActions(std::chrono::microseconds dt)
{
	_rotateAroundDelta = glm::vec3();
	_keyBoardMoveDelta = glm::vec2();

	// Compute delta position (dp) based on the elapsed time and speed.
	const auto dp = k_InteractionSpeedMultiplier * std::chrono::duration_cast<std::chrono::duration<float>>(dt).count();
	const auto& actionSystem = Locator::gameActionSystem::value();

	// TODO(#709): Set tricons based on ZOOM or TILT if any move is detected

	if (actionSystem.GetAny(input::BindableActionMap::ROTATE_LEFT, input::BindableActionMap::ROTATE_RIGHT))
	{
		const float distance = (actionSystem.Get(input::BindableActionMap::ROTATE_LEFT) ? -1.0f : 1.0f) * dp;
		_rotateAroundDelta.y += distance;
	}

	if (actionSystem.GetAny(input::BindableActionMap::TILT_UP, input::BindableActionMap::TILT_DOWN))
	{
		const float distance = (actionSystem.Get(input::BindableActionMap::TILT_DOWN) ? -1.0f : 1.0f) * dp;
		_rotateAroundDelta.x += distance;
	}

	if (actionSystem.GetAny(input::BindableActionMap::MOVE_FORWARDS, input::BindableActionMap::MOVE_BACKWARDS))
	{
		const float distance = (actionSystem.Get(input::BindableActionMap::MOVE_FORWARDS) ? -1.0f : 1.0f) * dp;
		// If ZOOM_ON is active, apply the movement as a zoom action.
		if (actionSystem.Get(input::BindableActionMap::ZOOM_ON))
		{
			_rotateAroundDelta.z += distance;
		}
		// If ROTATE_ON is active, apply the movement as a tilt action.
		// TODO(#710): fight will always be rotating
		else if (actionSystem.Get(input::BindableActionMap::ROTATE_ON))
		{
			_rotateAroundDelta.x += distance;
		}
		// Otherwise, apply the movement normally.
		else
		{
			_keyBoardMoveDelta.x += distance;
		}
	}

	if (actionSystem.GetAny(input::BindableActionMap::MOVE_RIGHT, input::BindableActionMap::MOVE_LEFT))
	{
		const float distance = (actionSystem.Get(input::BindableActionMap::MOVE_RIGHT) ? -1.0f : 1.0f) * dp;
		// If ZOOM_ON is active, apply the movement as a zoom action.
		// If ROTATE_ON is active, apply the movement as a tilt action.
		// TODO(#710): fight will always be rotating
		if (actionSystem.GetAny(input::BindableActionMap::ZOOM_ON, input::BindableActionMap::ROTATE_ON))
		{
			_rotateAroundDelta.y += distance;
		}
		// Otherwise, apply the movement normally.
		else
		{
			_keyBoardMoveDelta.y -= distance;
		}
	}

	if (actionSystem.GetAny(input::BindableActionMap::ZOOM_IN, input::BindableActionMap::ZOOM_OUT))
	{
		const float distance =
		    (actionSystem.Get(input::BindableActionMap::ZOOM_IN) ? -k_WheelZoomFactor : k_WheelZoomFactor) * dp;
		_rotateAroundDelta.z += distance;
	}

	_modePrev = _mode;
	if (actionSystem.Get(input::BindableActionMap::MOVE))
	{
		_mode = Mode::DraggingLandscape;
	}
	else if (_keyBoardMoveDelta != glm::vec2() || _rotateAroundDelta != glm::vec3())
	{
		_mode = Mode::Polar;
	}
	else
	{
		_mode = Mode::Cartesian;
	}
}

glm::vec3 DefaultWorldCameraModel::GetTargetOrigin() const
{
	return _targetOrigin;
}

glm::vec3 DefaultWorldCameraModel::GetTargetFocus() const
{
	return _targetFocus;
}

std::chrono::seconds DefaultWorldCameraModel::GetIdleTime() const
{
	SPDLOG_LOGGER_WARN(spdlog::get("game"), "TODO: Idle Time not implemented");
	return {};
}

glm::vec3 DefaultWorldCameraModel::GetTargetForwardVector() const
{
	return GetTargetFocus() - GetTargetOrigin();
}

glm::vec3 DefaultWorldCameraModel::GetTargetForwardUnitVector() const
{
	return glm::normalize(GetTargetForwardVector());
}

glm::vec3 DefaultWorldCameraModel::ProjectPointOnForwardVector(float distanceFromOrigin) const
{
	return _targetOrigin + GetTargetForwardUnitVector() * distanceFromOrigin;
}

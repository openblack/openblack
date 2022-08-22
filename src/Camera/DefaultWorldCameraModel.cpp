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
#include <ranges>

#include <glm/gtc/constants.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/polar_coordinates.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <spdlog/spdlog.h>

#include "3D/LandIslandInterface.h"
#include "Audio/AudioManagerInterface.h"
#include "Camera.h"
#include "Common/RandomNumberManager.h"
#include "ECS/Components/Transform.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "Input/GameActionMapInterface.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

using namespace openblack;
using namespace std::chrono_literals;

template <typename T, size_t S>
    requires std::floating_point<T>
constexpr std::array<T, S> MakeFlyingScoreAngles()
{
	std::array<T, S> result {};
	for (size_t i = 0; i < S; ++i)
	{
		result.at(i) = static_cast<T>(i) * glm::pi<T>() * 2.0f / static_cast<T>(S);
	}
	return result;
}

// TODO(#708): Add to global configurations
constexpr auto k_WheelZoomFactor = 20.0f;
constexpr auto k_InteractionSpeedMultiplier = 400.0f;
// Vanilla black and white uses a pretty bad PI/2 approximation
constexpr auto k_CameraModelHalfPi = 1.53938043f;
constexpr auto k_RotateOnSpeedMultiplier = glm::vec2(1.9f, -1.7f);
constexpr auto k_TwoButtonZoomFactor = 1.9f;
constexpr auto k_CameraInteractionStepSize = 3.0f;
constexpr auto k_MinimalCameraAnimationDuration = 1'500'000us;
constexpr auto k_HandDragVectorAlignmentThreshold = 0.001f;
constexpr auto k_HandDragVectorAlignmentRatioThreshold = 0.000'1f;
constexpr auto k_FlyingDistanceThresholds = std::array<float, 4> {100.0f, 60.0f, 30.0f, 15.0f};
constexpr auto k_FlyingThresholdFactor = 1.5f;
constexpr auto k_GroundDistanceMinimum = 10.0f;
constexpr auto k_FlightHeightFactor = 0.1f;
constexpr auto k_FlyingScoreAngles = MakeFlyingScoreAngles<float, 0x20>();
constexpr auto k_ConstrainDiscCentre = glm::vec3(2560.0f, 0.0f, 2560.0f);
constexpr auto k_ConstrainDiscRadius = 5120.0f;
constexpr auto k_MaxAltitude = 30'000.0f;
constexpr auto k_FloatingHeight = 2.9999f; // 3 in vanilla, but less due to fp precision with recorded data in tests

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

/// Calculates the projection length of a vector onto another vector.
///
/// Given three points `p1`, `p2`, and `p3`, this function computes the projection length
/// of the vector from `p1` to `p3` onto the direction defined by the vector from `p1` to `p2`.
/// The result represents how much `p3` is "along" the direction from `p1` to `p2`, scaled
/// by the magnitude of the vector from `p1` to `p3`.
///
/// # Arguments
///
/// * `p1` - The origin point.
/// * `p2` - The point defining the direction vector.
/// * `p3` - The point whose projection onto the direction from `p1` to `p2` is calculated.
///
/// # Returns
///
/// The projection length of `p3` onto the direction from `p1` to `p2`.
float PointDistanceAlongLineSegment(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3)
{
	const glm::vec3 v12 = p2 - p1; // Vector from p1 to p2
	const glm::vec3 v13 = p3 - p1; // Vector from p1 to p3

	// Normalize v12 to get its direction
	const glm::vec3 u12 = glm::normalize(v12);

	// Project v13 onto u12 to get the projection length from p1
	return glm::abs(glm::dot(v13, u12)); // Using glm::abs to ensure the distance is non-negative
}

DefaultWorldCameraModel::DefaultWorldCameraModel() = default;

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
		const auto pitchStep = _rotateAroundDelta.x * 0.002f;
		eulerAngles.y -= pitchStep;
		// Clamp the pitch angle to keep the camera within between -30 and 78.75 degrees.
		eulerAngles.y = glm::clamp(eulerAngles.y, -1.0f / 6.0f * glm::pi<float>(), 7.0f / 16.0f * glm::pi<float>());
		if (_mode == Mode::ArcBall)
		{
			const auto distanceFromBound = (2.0f * k_CameraInteractionStepSize) - _distanceFromBoundY;
			if (distanceFromBound > glm::epsilon<float>())
			{
				const auto verticalStep = distanceFromBound * pitchStep * _focusDistance * 0.051f;
				_targetFocus += verticalStep;
				_focusAtClick += verticalStep;
			}
		}
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

void DefaultWorldCameraModel::ComputeDistanceFromBoundY()
{
	if (_targetOrigin.y < _targetFocus.y)
	{
		_distanceFromBoundY = 0.0f;
	}
	else
	{
		const auto groundAltitude = Locator::terrainSystem::value().GetHeightAt(glm::xz(_targetOrigin));
		const auto boundsMinY = groundAltitude + k_CameraInteractionStepSize;
		const auto boundsMaxY = glm::min(groundAltitude, 0.0f) + k_MaxAltitude;
		_distanceFromBoundY = glm::min(glm::abs(_targetOrigin.y - boundsMinY), glm::abs(_targetOrigin.y - boundsMaxY));
	}
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
		if (_mode != Mode::DraggingLandscape)
		{
			_originFocusDistanceAtInteractionStart =
			    glm::max(glm::distance(_targetOrigin, _targetFocus), k_CameraInteractionStepSize + 0.1f);
			UpdateFocusPointInteractionParameters(_targetOrigin, _targetFocus, eulerAngles, camera);
		}
	}

	return originHasBeenAdjusted;
}

bool DefaultWorldCameraModel::ConstrainAltitude()
{
	return false;
	bool hasBeenAdjusted = false;
	const auto minAltitude = k_FloatingHeight + Locator::terrainSystem::value().GetHeightAt(glm::xz(_targetOrigin));
	if (_targetOrigin.y < minAltitude)
	{
		_targetOrigin.y = minAltitude;
		hasBeenAdjusted = true;
	}

	if (_mode != Mode::ArcBall && _rotateAroundDelta.x != 0.0f)
	{
		_targetFocus = _targetOrigin + glm::normalize(_targetFocus - _targetOrigin) * _focusDistance;
	}

	return hasBeenAdjusted;
}

bool DefaultWorldCameraModel::ConstrainDisc()
{
	bool hasBeenAdjusted = false;

	const auto delta = _targetOrigin - k_ConstrainDiscCentre;
	const auto distance2 = glm::length2(delta);

	if (distance2 > k_ConstrainDiscRadius * k_ConstrainDiscRadius)
	{
		_targetOrigin = k_ConstrainDiscCentre + delta * (k_ConstrainDiscRadius / glm::sqrt(distance2));
		hasBeenAdjusted = true;
	}

	return hasBeenAdjusted;
}

void DefaultWorldCameraModel::UpdateFocusPointInteractionParameters(glm::vec3 origin, glm::vec3 focus, glm::vec3 eulerAngles,
                                                                    const Camera& camera)
{
	_focusAtClick = _targetFocus;
	_screenSpaceMouseRaycastHitAtClick =
	    camera.RaycastMouseToLand(true, Camera::Interpolation::Target).and_then(ecs::components::GetTransformPosition);
	if (_screenSpaceMouseRaycastHitAtClick.has_value())
	{
		_arcBallRadius = PointDistanceAlongLineSegment(origin, focus, *_screenSpaceMouseRaycastHitAtClick);
	}
	_originAtClick = _targetOrigin;
	_mouseAtClick = Locator::gameActionSystem::value().GetMousePosition();
	_originFocusDistanceAtInteractionStart = glm::distance(origin, focus);
	// TODO(#713): calculate a y-basis based on the projection on land of camera origin and hand
	_originToHandPlaneNormal = glm::vec3(0.0f, 1.0f, 0.0f);
	// TODO(#713): Calculate the with _originToHandPlaneNormal and the mouse hit point to put in _alignmentAtInteractionStart
	_averageIslandDistance = GetVerticalLineInverseDistanceWeighingRayCast(camera);
	{
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

glm::vec3 ScreenToWorld(const Camera& camera, glm::u16vec2 pixelCoord)
{
	const auto& window = Locator::windowing ::value();

	const auto screenSize = window.GetSize();
	const auto aspect = window.GetAspectRatio();
	const auto yFov = 2.0f * glm::atan(1.0f / camera.GetProjectionMatrix()[1][1]);

	const auto screenCoord = static_cast<glm::vec2>(pixelCoord) / static_cast<glm::vec2>(screenSize);
	const auto clipSpaceCoordinates = ((screenCoord - 0.5f) * 2.0f) * glm::vec2(1.0f, -1.0f);

	const auto tanHalfFov = glm::tan(yFov / 2.0f);
	const auto extents = glm::vec3(tanHalfFov * aspect, tanHalfFov, 1.0f);
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
	case Mode::ArcBall:
		UpdateModeArcBall(eulerAngles, mouseCurrent, camera.GetHorizontalFieldOfView());
		break;
	case Mode::DraggingLandscape:
		UpdateModeDragging(camera, mouseCurrent, mouseMovementDistance);
		break;
	case Mode::FlyingToPoint:
		UpdateModeFlying(eulerAngles);
		break;
	}

	if (_mode != Mode::ArcBall && _rotateAroundDelta.x != 0.0f)
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

void DefaultWorldCameraModel::UpdateModeArcBall(glm::vec3 eulerAngles, glm::u16vec2 mouseCurrent, float xFov)
{
	if (!_screenSpaceMouseRaycastHitAtClick.has_value())
	{
		return;
	}
	const auto point = _focusAtClick + _originFocusDistanceAtInteractionStart * glm::euclidean(glm::yx(eulerAngles));

	const auto basisZ = glm::normalize(point - _focusAtClick);
	const auto basisX = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), basisZ));
	const auto basisY = glm::normalize(glm::cross(basisZ, basisX));

	const auto windowSize = static_cast<glm::vec2>(Locator::windowing::value().GetSize());
	const auto aspect = windowSize.y / windowSize.x;
	const auto screenCentreOffset = static_cast<glm::vec2>(mouseCurrent) / windowSize - 0.5f;

	const auto halfTanX = glm::tan(xFov * 0.5f);
	const auto halfTanY = halfTanX * aspect;

	const auto cameraOffsetX = screenCentreOffset.x * halfTanX * basisX * _arcBallRadius * 2.0f;
	const auto cameraOffsetY = screenCentreOffset.y * halfTanY * basisY * _arcBallRadius * 2.0f;

	_targetFocus = _screenSpaceMouseRaycastHitAtClick.value() + cameraOffsetX + cameraOffsetY;
	_targetOrigin = _targetFocus + basisZ * _arcBallRadius;
	_focusAtClick = _targetFocus;
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
				// movement *= mouseMovementDistance *glm::sqrt(movementDistance2); // FIXME: This whole section is broken
			}

			_targetOrigin = _originAtClick - movement;
			_targetFocus = _focusAtClick - movement;

			_targetOrigin =
			    _targetFocus + glm::normalize(_targetOrigin - _targetFocus) * _originFocusDistanceAtInteractionStart;
		}
	}
}

void DefaultWorldCameraModel::UpdateModeFlying(glm::vec3 eulerAngles)
{
	if (!_screenSpaceCenterRaycastHit.has_value())
	{
		return;
	}
	const auto point = _handPosition.value_or(_screenSpaceCenterRaycastHit.value_or(glm::zero<glm::vec3>()));

	const auto distanceFromHitPoint = glm::length(point - *_screenSpaceCenterRaycastHit);
	const auto distanceFromOrigin = glm::length(point - _targetOrigin);

	const auto thresholdDistance = distanceFromOrigin / k_FlyingThresholdFactor;

	const bool wooshingDistance =
	    thresholdDistance > k_FlyingDistanceThresholds[0] && distanceFromHitPoint > k_GroundDistanceMinimum;
	float distanceFromFocus;
	if (wooshingDistance)
	{
		distanceFromFocus = k_FlyingDistanceThresholds[0];
	}
	else if (thresholdDistance > k_FlyingDistanceThresholds[1])
	{
		distanceFromFocus = k_FlyingDistanceThresholds[1];
	}
	else if (thresholdDistance > k_FlyingDistanceThresholds[2])
	{
		distanceFromFocus = k_FlyingDistanceThresholds[2];
	}
	else
	{
		distanceFromFocus = k_FlyingDistanceThresholds[3];
	}

	// Find best angles
	{
		std::array<float, 0x20> scores {};
		for (auto [score, flyingScore] : std::views::zip(scores, k_FlyingScoreAngles))
		{
			for (int j = 0; j < 5; ++j)
			{
				const auto p = point + static_cast<float>(j) + 3.0f * distanceFromFocus * glm::euclidean(glm::yx(eulerAngles));
				score += point.y - Locator::terrainSystem::value().GetHeightAt(glm::xz(p));
			}
			score += 50.0f * std::cos(flyingScore);
		}

		const auto bestAngleIndex = std::distance(scores.begin(), std::max_element(scores.begin(), scores.end()));
		const auto normal = Locator::terrainSystem::value().GetNormalAt(glm::xz(point));
		const auto offsetPoint = point + normal;

		const auto oldAngles = eulerAngles;
		eulerAngles = EulerFromPoints(offsetPoint, point); // Placeholder

		eulerAngles.y = glm::clamp((eulerAngles.y / 2.0f + oldAngles.y) / 5.0f + 3.0f * glm::pi<float>() / 25.0f,
		                           glm::pi<float>() / 8.0f, glm::pi<float>() / 3.0f);
		eulerAngles.x = static_cast<float>(bestAngleIndex) * glm::pi<float>() / 16.0f + oldAngles.x;
	}
	eulerAngles.y = glm::clamp(eulerAngles.y, glm::pi<float>() / 8.0f, glm::pi<float>() * 10.0f / 21.0f);

	_targetFocus = point;
	_targetOrigin = _targetFocus + distanceFromFocus * glm::euclidean(glm::yx(eulerAngles));

	if (wooshingDistance)
	{
		SetFlight(_targetOrigin, _targetFocus);
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
	{
		const auto hit = camera.RaycastMouseToLand(false, Camera::Interpolation::Target);
		_screenSpaceMouseRaycastHit = hit.and_then(ecs::components::GetTransformPosition);
	}
	{
		const auto hit = camera.RaycastScreenCoordToLand({0.5f, 0.5f}, false, Camera::Interpolation::Target);
		_screenSpaceCenterRaycastHit = hit.and_then(ecs::components::GetTransformPosition);
	}
}

void DefaultWorldCameraModel::UpdateFocusDistance()
{
	_focusDistance =
	    _screenSpaceCenterRaycastHit
	        .and_then([this](auto hit) -> std::optional<float> { return glm::max(10.0f, glm::distance(hit, _targetOrigin)); })
	        .value_or(glm::max(10.0f, _averageIslandDistance));
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

	if (_mode != _modePrev && _mode != Mode::FlyingToPoint && _modePrev != Mode::FlyingToPoint)
	{
		UpdateFocusPointInteractionParameters(camera.GetOrigin(Camera::Interpolation::Target),
		                                      camera.GetFocus(Camera::Interpolation::Target), eulerAngles, camera);
	}

	ComputeDistanceFromBoundY();

	// Get step size
	const auto scalingFactor = 60.0f;
	const auto zoomDelta = _rotateAroundDelta.z * 0.0015f * scalingFactor;

	if (_mode == Mode::Polar || _mode == Mode::ArcBall)
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
DefaultWorldCameraModel::ComputeUpdateReturnInfo(bool originHasBeenAdjusted, std::chrono::microseconds t)
{
	if (!_flightPath.has_value())
	{
		static constinit auto kTimeThreshold = 1'500'000us;
		auto duration = 300'000us;
		if (originHasBeenAdjusted)
		{
			duration *= 2;
		}
		if (_elapsedTime <= kTimeThreshold)
		{
			duration =
			    std::chrono::microseconds {glm::mix(k_MinimalCameraAnimationDuration.count(), duration.count(),
			                                        (static_cast<float>(_elapsedTime.count()) / kTimeThreshold.count()))};
		}
		return {{GetTargetOrigin(), GetTargetFocus(), duration}};
	}
	if (t > k_MinimalCameraAnimationDuration / 2)
	{
		std::optional<FlightPath> backup = std::nullopt;
		std::swap(backup, _flightPath);
		_elapsedTime = decltype(_elapsedTime)::zero();
		return {{backup->origin, backup->focus, k_MinimalCameraAnimationDuration}};
	}
	if (_flightPath->midpoint.has_value())
	{
		std::optional<glm::vec3> backup = std::nullopt;
		std::swap(backup, _flightPath->midpoint);
		return {{*backup, _flightPath->focus,
		         std::chrono::duration_cast<std::chrono::microseconds>(k_MinimalCameraAnimationDuration * 0.9f)}};
	}
	return std::nullopt;
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

	if (actionSystem.Get(input::UnbindableActionMap::TWO_BUTTON_CLICK))
	{
		_rotateAroundDelta.z += actionSystem.GetMouseDelta().y * k_TwoButtonZoomFactor;
		// TODO(#711): the mouse has to be reset
	}

	if (actionSystem.Get(input::BindableActionMap::ROTATE_AROUND_MOUSE_ON))
	{
		const auto mouseDelta = static_cast<glm::vec2>(actionSystem.GetMouseDelta());
		_rotateAroundDelta += glm::vec3(glm::yx(mouseDelta * k_RotateOnSpeedMultiplier), 0.0f);
	}

	const auto handPositions = actionSystem.GetHandPositions();
	_handPosition = handPositions[0].or_else([handPositions] { return handPositions[1]; });

	_modePrev = _mode;
	if (_handPosition.has_value() && actionSystem.Get(input::UnbindableActionMap::DOUBLE_CLICK))
	{
		_mode = Mode::FlyingToPoint;
	}
	else if (actionSystem.Get(input::BindableActionMap::ROTATE_AROUND_MOUSE_ON))
	{
		_mode = Mode::ArcBall;
	}
	else if (_handPosition.has_value() && actionSystem.Get(input::BindableActionMap::MOVE))
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

void DefaultWorldCameraModel::SetFlight(glm::vec3 origin, glm::vec3 focus)
{
	_flightPath = CharterFlight(origin, focus, _currentOrigin, k_FlightHeightFactor);
	static constexpr auto k_WooshingNoiseIds = std::array<audio::SoundId, 4> {
	    audio::SoundId::G_Woosh_01,
	    audio::SoundId::G_Woosh_02,
	    audio::SoundId::G_Woosh_03,
	    audio::SoundId::G_Woosh_04,
	};
	const auto wooshNoiseId = static_cast<entt::id_type>(Locator::rng::value().Choose(k_WooshingNoiseIds));
	Locator::audio::value().PlaySound(wooshNoiseId, audio::PlayType::Once);
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

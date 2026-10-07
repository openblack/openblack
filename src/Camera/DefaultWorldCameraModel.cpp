/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "DefaultWorldCameraModel.h"

#include <numeric>
#include <ranges>
#include <tuple>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/polar_coordinates.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <spdlog/spdlog.h>

#include "3D/LandIslandInterface.h"
#include "Audio/AudioManagerInterface.h"
#include "Camera.h"
#include "Common/RandomNumberManager.h"
#include "ECS/Components/Transform.h"
#include "ECS/Systems/CameraHelpSystemInterface.h"
#include "ECS/Systems/CinematicDirectorSystemInterface.h"
#include "ECS/Systems/CreatureFightSystemInterface.h"
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
// Vanilla reads DirectInput's wheel, 120 per notch, and zooms by half of it. Zoom keys without a wheel turn count as one
// notch every frame.
constexpr auto k_WheelZoomPerNotch = 60.0f;
// Zoom and keyboard movement steps are scaled by three times the camera's height above its focus, within these bounds.
// Below the focus the zoom scale stops at k_ZoomScaleMaxBelowFocus.
constexpr auto k_StepScaleHeightFactor = 3.0f;
constexpr auto k_StepScaleMin = 60.0f;
constexpr auto k_ZoomScaleMax = 2000.0f;
constexpr auto k_ZoomScaleMaxBelowFocus = 4.0f * k_StepScaleMin;
constexpr auto k_InteractionSpeedMultiplier = 400.0f;
// Vanilla black and white uses a pretty bad PI/2 approximation
constexpr auto k_CameraModelHalfPi = 1.53938043f;
constexpr auto k_RotateOnSpeedMultiplier = glm::vec2(1.9f, -1.7f);
constexpr auto k_TwoButtonZoomFactor = 1.9f;
// Both buttons turn the camera by the same amount for the mouse's movement across, once it has moved far enough
constexpr auto k_TwoButtonTurnFactor = 1.9f;
// The tilt of a unit of pitch input, in radians
constexpr auto k_PitchPerInput = 0.002f;
// The clear view eases in and out over this long, and the hand grips while it is further in than this
constexpr auto k_ClearViewSeconds = 0.5f;
constexpr auto k_ClearViewGrips = 0.01f;
constexpr auto k_CameraInteractionStepSize = 3.0f;
constexpr auto k_MinimalCameraAnimationDuration = 1'500'000us;
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
		const auto pitchStep = _rotateAroundDelta.x * k_PitchPerInput;
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
	// Dragging the land, the camera, the cursor and the plane are those of the press
	if (_mode == Mode::DraggingLandscape && _dragging && _landGrip.has_value())
	{
		_originAtClick = _landGrip->origin;
		_focusAtClick = _landGrip->focus;
		_mouseAtClick = _landGrip->cursor;
		_originToHandPlaneNormal = _landGrip->plane.normal;
		_alignmentAtInteractionStart = _landGrip->plane.distance;
		_originFocusDistanceAtInteractionStart = glm::distance(_landGrip->origin, _landGrip->focus);
	}
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

void DefaultWorldCameraModel::UpdateMode(const Camera& camera, glm::vec3 eulerAngles, float zoomDelta, glm::uvec2 mouseCurrent)
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
		UpdateModeDragging(camera, mouseCurrent);
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

void DefaultWorldCameraModel::UpdateModeDragging(const Camera& camera, glm::u16vec2 mouseCurrent)
{
	// Panning is strafing, and the land must be under the cursor now and where the land was gripped
	if ((_features & camera_help::feature::k_Strafe) == 0 || !_screenSpaceMouseRaycastHit.has_value() ||
	    (_landGrip.has_value() && !_landGrip->land))
	{
		return;
	}
	// Land gripped too far ahead isn't dragged, until the buttons are let go
	if (_landGrip.has_value() && _landGrip->depth > camera_pan::k_MaxGripDepth)
	{
		_dragGivenUp = true;
		return;
	}

	// The lines of sight through the cursor now and at the grip, both from the camera as it was at the grip
	const auto screenSize = glm::vec2(Locator::windowing::value().GetSize());
	const auto inverseViewProjection = glm::inverse(camera.GetProjectionMatrix(Camera::Projection::Normal) *
	                                                glm::lookAt(_originAtClick, _focusAtClick, glm::vec3(0.0f, 1.0f, 0.0f)));
	const auto rayThrough = [&inverseViewProjection, screenSize](glm::vec2 cursor) {
		const auto ndc = glm::vec2((cursor.x / screenSize.x - 0.5f) * 2.0f, ((1.0f - cursor.y / screenSize.y) - 0.5f) * 2.0f);
		const auto near = inverseViewProjection * glm::vec4(ndc, 0.0f, 1.0f);
		const auto far = inverseViewProjection * glm::vec4(ndc, 0.5f, 1.0f);
		return glm::normalize(glm::vec3(far) / far.w - glm::vec3(near) / near.w);
	};
	const auto place =
	    camera_pan::Pan({.normal = _originToHandPlaneNormal, .distance = _alignmentAtInteractionStart}, _originAtClick,
	                    _focusAtClick, _originFocusDistanceAtInteractionStart, rayThrough(glm::vec2(mouseCurrent)),
	                    rayThrough(glm::vec2(_mouseAtClick)), glm::ivec2(mouseCurrent), glm::ivec2(_mouseAtClick));
	if (!place.has_value())
	{
		return;
	}

	// The camera stops short of land in its way, or of the sea
	auto stopped = *place;
	const auto move = place->origin - _originAtClick;
	if (glm::length(move) > 0.0f && Locator::dynamicsSystem::has_value())
	{
		const auto land =
		    Locator::dynamicsSystem::value().RayCastLand(_originAtClick, glm::normalize(move), k_ConstrainDiscRadius * 4.0f);
		const auto hit = land.has_value() ? land : camera_pan::SeaHit(_originAtClick, place->origin, camera.GetOrigin());
		if (hit.has_value())
		{
			stopped = camera_pan::StopShortOfLand(*place, _originAtClick, *hit);
		}
	}
	_targetOrigin = stopped.origin;
	_targetFocus = stopped.focus;
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

float DefaultWorldCameraModel::GetZoomScale() const
{
	// Zooming speeds up with height, so a wheel notch covers about the same share of the view at any altitude
	const auto heightAboveFocus = _targetOrigin.y - _targetFocus.y;
	const auto maxScale = heightAboveFocus < 0.0f ? k_ZoomScaleMaxBelowFocus : k_ZoomScaleMax;
	return glm::clamp(k_StepScaleHeightFactor * glm::abs(heightAboveFocus), k_StepScaleMin, maxScale);
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
	const auto originAtFrameStart = _targetOrigin;
	UpdateRaycastHitPoints(camera);
	UpdateFocusDistance();

	// A drag just pressed grips the land under the cursor, or the focus with no land there
	if (_dragging && !_landGrip.has_value())
	{
		const auto gripped = _screenSpaceMouseRaycastHit.value_or(_targetFocus);
		const auto ground = Locator::terrainSystem::has_value()
		                        ? Locator::terrainSystem::value().GetHeightAt(glm::xz(_targetOrigin))
		                        : gripped.y;
		_landGrip = LandGrip {
		    .origin = _targetOrigin,
		    .focus = _targetFocus,
		    .cursor = glm::u16vec2(Locator::gameActionSystem::value().GetMousePosition()),
		    .plane = camera_pan::PlaneThrough(gripped, _targetOrigin, ground),
		    .land = _screenSpaceMouseRaycastHit.has_value(),
		    .depth = PointDistanceAlongLineSegment(_targetOrigin, _targetFocus, gripped),
		};
	}

	// Get angles (yaw, pitch, roll). Roll is always 0
	glm::vec3 eulerAngles = EulerFromPoints(_targetOrigin, _focusAtClick);

	if (_mode != _modePrev && _mode != Mode::FlyingToPoint && _modePrev != Mode::FlyingToPoint)
	{
		UpdateFocusPointInteractionParameters(camera.GetOrigin(Camera::Interpolation::Target),
		                                      camera.GetFocus(Camera::Interpolation::Target), eulerAngles, camera);
	}

	ComputeDistanceFromBoundY();

	// Get step size
	const auto scalingFactor = k_StepScaleMin;
	const auto zoomDelta = _rotateAroundDelta.z * 0.0015f * GetZoomScale();

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

	UpdateMode(camera, eulerAngles, zoomDelta, mouseCurrent);

	const bool originHasBeenAdjusted = ConstrainCamera(dt, mouseMovementDistance, eulerAngles, camera);

	// The self-tilting camera keeps to its height over the land, where it was at the start of the frame, looking the way
	// it now does, unless the land is dragged without turning or zooming
	if ((_features & camera_help::feature::k_AutoPitch) != 0 &&
	    (!_dragging || _rotateAroundDelta != glm::vec3() || _mode == Mode::ArcBall))
	{
		const auto height = Locator::cameraHelpSystem::has_value() ? Locator::cameraHelpSystem::value().Get().autoPitchHeight
		                                                           : camera_help::k_DefaultAutoPitchHeight;
		const auto ground = Locator::terrainSystem::has_value()
		                        ? Locator::terrainSystem::value().GetHeightAt(glm::xz(originAtFrameStart))
		                        : 0.0f;
		const auto origin = glm::vec3(originAtFrameStart.x, ground + height, originAtFrameStart.z);
		_targetFocus += origin - _targetOrigin;
		_targetOrigin = origin;
	}

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

	// What the scripts let the player do, less going to watch fights while watching one
	const auto help =
	    Locator::cameraHelpSystem::has_value() ? Locator::cameraHelpSystem::value().Get() : camera_help::CameraHelp {};
	const bool onFight = Locator::creatureFightSystem::has_value() && Locator::creatureFightSystem::value().IsCameraOnFight();
	_features = onFight ? camera_help::DuringFight(help.features) : help.features;
	const bool aroundMouse = (_features & camera_help::feature::k_RotateAroundMouse) != 0;
	const bool rotateAroundMouse = aroundMouse && actionSystem.Get(input::BindableActionMap::ROTATE_AROUND_MOUSE_ON);

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

	// A wheel step is a fixed distance, whatever the frame time
	if (const auto wheelNotches = actionSystem.GetMouseWheelDelta(); wheelNotches != 0.0f)
	{
		_rotateAroundDelta.z -= wheelNotches * k_WheelZoomPerNotch;
	}
	else if (actionSystem.GetAny(input::BindableActionMap::ZOOM_IN, input::BindableActionMap::ZOOM_OUT))
	{
		_rotateAroundDelta.z +=
		    actionSystem.Get(input::BindableActionMap::ZOOM_IN) ? -k_WheelZoomPerNotch : k_WheelZoomPerNotch;
	}

	const bool twoButtons = aroundMouse && actionSystem.Get(input::UnbindableActionMap::TWO_BUTTON_CLICK);
	if (twoButtons)
	{
		_rotateAroundDelta.z += actionSystem.GetMouseDelta().y * k_TwoButtonZoomFactor;
	}
	// Moving across turns the camera too, once the mouse has moved a fortieth of the screen's width across in a frame,
	// or since both were pressed
	{
		const auto width = Locator::windowing::has_value() ? Locator::windowing::value().GetSize().x : 1;
		const auto across = _twoButtonTurn.Update(twoButtons, actionSystem.GetMouseDelta().x,
		                                          static_cast<int>(actionSystem.GetMousePosition().x), width);
		_rotateAroundDelta.y += static_cast<float>(across) * k_TwoButtonTurnFactor;
	}

	if (rotateAroundMouse)
	{
		const auto mouseDelta = static_cast<glm::vec2>(actionSystem.GetMouseDelta());
		_rotateAroundDelta += glm::vec3(glm::yx(mouseDelta * k_RotateOnSpeedMultiplier), 0.0f);
	}

	// Ctrl and Shift together ease the clear view in over half a second, and out again
	_clearView.SetDestination(
	    actionSystem.Get(input::BindableActionMap::ZOOM_ON) && actionSystem.Get(input::BindableActionMap::ROTATE_ON) ? 1.0f
	                                                                                                                 : 0.0f,
	    k_ClearViewSeconds);
	_clearView.Update(std::chrono::duration<float>(dt).count());

	const auto handPositions = actionSystem.GetHandPositions();
	_handPosition = handPositions[0].or_else([handPositions] { return handPositions[1]; });

	_controlsTime += dt;
	if (!(_handPosition.has_value() && actionSystem.Get(input::BindableActionMap::MOVE)) || rotateAroundMouse)
	{
		std::ignore = HandleDrag(false);
	}

	// Without zooming the zoom does nothing, without turning nothing turns, without tilting nothing tilts unless the
	// camera tilts itself, and without strafing the movement keys do nothing
	if ((_features & camera_help::feature::k_Zoom) == 0)
	{
		_rotateAroundDelta.z = 0.0f;
	}
	if ((_features & camera_help::feature::k_Rotate) == 0)
	{
		_rotateAroundDelta.y = 0.0f;
	}
	if ((_features & camera_help::feature::k_Strafe) == 0)
	{
		_keyBoardMoveDelta = glm::vec2();
	}
	// The self-tilting camera tilts towards its pitch while the land isn't dragged
	bool autoTilting = false;
	if ((_features & camera_help::feature::k_AutoPitch) != 0 && !_dragging)
	{
		const auto pitch = EulerFromPoints(_targetOrigin, _focusAtClick).y;
		if (const auto input = camera_help::AutoPitchInput(help.autoPitch, pitch, std::chrono::duration<float>(dt).count()))
		{
			_rotateAroundDelta.x = *input;
			autoTilting = true;
		}
	}
	if ((_features & camera_help::feature::k_Pitch) == 0 && !autoTilting)
	{
		_rotateAroundDelta.x = 0.0f;
	}
	// The hints follow the cursor while nothing is dragged and the camera isn't being turned, moved or zoomed
	if (!_dragging && !rotateAroundMouse && _rotateAroundDelta == glm::vec3() && _keyBoardMoveDelta == glm::vec2() &&
	    Locator::windowing::has_value())
	{
		const auto screenSize = Locator::windowing::value().GetSize();
		const auto cursor =
		    camera_drag::NormalisedCursor(glm::ivec2(actionSystem.GetMousePosition()), screenSize, ViewHeight(screenSize));
		_tricons = camera_drag::IdleTricons(cursor, _screenSpaceMouseRaycastHit.has_value());
		// Watching a fight, the camera offers no tilting
		if (onFight)
		{
			_tricons &= ~camera_drag::tricon::k_Pitch;
		}
	}

	_modePrev = _mode;
	if (_handPosition.has_value() && actionSystem.Get(input::UnbindableActionMap::DOUBLE_CLICK))
	{
		_mode = Mode::FlyingToPoint;
	}
	else if (rotateAroundMouse)
	{
		_mode = Mode::ArcBall;
	}
	else if (_handPosition.has_value() && actionSystem.Get(input::BindableActionMap::MOVE))
	{
		_mode = HandleDrag(true);
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

DefaultWorldCameraModel::Mode DefaultWorldCameraModel::HandleDrag(bool held)
{
	using camera_drag::DragMode;
	if (!held)
	{
		_dragging = false;
		_dragGivenUp = false;
		return _mode;
	}
	if (_dragGivenUp)
	{
		return Mode::Cartesian;
	}
	if (!Locator::windowing::has_value())
	{
		return Mode::DraggingLandscape;
	}
	auto& actionSystem = Locator::gameActionSystem::value();
	const auto screenSize = Locator::windowing::value().GetSize();
	const auto cursor = glm::ivec2(actionSystem.GetMousePosition());
	const auto milliseconds =
	    static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(_controlsTime).count());
	if (!_dragging)
	{
		// The drag takes the hints of the cursor before it was pressed
		_dragging = true;
		_drag.Start(_tricons, camera_drag::NormalisedCursor(cursor, screenSize, ViewHeight(screenSize)), milliseconds);
		_ringCursor = cursor;
		// The land is gripped as the camera next looks at it
		_landGrip.reset();
	}
	else
	{
		_drag.Move(actionSystem.GetMouseDelta(), screenSize, ViewHeight(screenSize), milliseconds,
		           _screenSpaceMouseRaycastHit.has_value(), _features);
	}

	const auto mode = _drag.GetMode();
	if (!mode.has_value())
	{
		// Until it is decided, the camera stays put
		return Mode::Cartesian;
	}
	switch (*mode)
	{
	case DragMode::Pan:
		return Mode::DraggingLandscape;
	case DragMode::EdgeRotate:
	{
		// The cursor is held on the ring, and the camera turns about its focus by the angle swept round the middle
		const auto step = camera_drag::EdgeRotate(cursor, _ringCursor, screenSize, ViewHeight(screenSize));
		_ringCursor = step.cursor;
		if ((_features & camera_help::feature::k_Rotate) != 0)
		{
			_rotateAroundDelta.y += step.angle * static_cast<float>(screenSize.x) / glm::pi<float>();
		}
		actionSystem.WarpCursor(step.cursor);
		return Mode::Polar;
	}
	case DragMode::Pitch:
	case DragMode::PitchFromTop:
	{
		const auto fov = Locator::camera::has_value() ? Locator::camera::value().GetHorizontalFieldOfView() : 0.0f;
		if ((_features & camera_help::feature::k_Pitch) != 0)
		{
			_rotateAroundDelta.x += camera_drag::PitchStep(actionSystem.GetMouseDelta().y, screenSize.y, fov) / k_PitchPerInput;
		}
		return Mode::Polar;
	}
	}
	return Mode::DraggingLandscape;
}

int DefaultWorldCameraModel::ViewHeight(glm::ivec2 screenSize)
{
	const bool bars =
	    Locator::cinematicDirectorSystem::has_value() && Locator::cinematicDirectorSystem::value().IsWideScreenOn();
	return camera_drag::ViewHeight(screenSize, bars);
}

CameraModel::HandCues DefaultWorldCameraModel::GetHandCues() const
{
	if (_dragging)
	{
		return {.tricons = _drag.GetTricons(),
		        .dragging = true,
		        .dragMode = _drag.GetMode(),
		        .clearViewGrip = _clearView.GetValue() > k_ClearViewGrips};
	}
	return {.tricons = _tricons};
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

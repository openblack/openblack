/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "EditorCameraModel.h"

#include <algorithm>

namespace openblack
{

namespace
{
/// The share of the way to where it wants to be the following camera goes in a second
constexpr float k_FollowEasePerSecond = 0.95f;
/// The focus eases quicker, so the thing stays in the middle of the view
constexpr float k_FocusEasePerSecond = 0.995f;
} // namespace

EditorCameraModel::EditorCameraModel(Mode mode, glm::vec3 origin, glm::vec3 target)
    : _mode(mode)
    , _orbit(editor::OrbitFrom(origin, target))
    , _target(target)
    , _origin(origin)
    , _focus(target)
{
}

void EditorCameraModel::SetMode(Mode mode)
{
	if (mode == _mode)
	{
		return;
	}
	// Each takes over from where the camera is now
	if (mode == Mode::Orbit)
	{
		_orbit = editor::OrbitFrom(_origin, GoalFocus());
	}
	else
	{
		_followTurn = 0.0f;
	}
	_mode = mode;
}

void EditorCameraModel::SetTarget(glm::vec3 position, glm::vec2 facing, float height)
{
	_target = position;
	_facing = facing;
	_height = std::max(height, 0.5f);
}

void EditorCameraModel::Turn(glm::vec2 radians)
{
	if (_mode == Mode::Follow)
	{
		_followTurn = editor::WrapAngle(_followTurn + radians.x);
		_orbit = editor::Turn(_orbit, {0.0f, radians.y});
		return;
	}
	_orbit = editor::Turn(_orbit, radians);
}

void EditorCameraModel::Zoom(float steps)
{
	_orbit = editor::Zoom(_orbit, steps);
}

glm::vec3 EditorCameraModel::GoalFocus() const
{
	// The middle of the thing rather than its feet
	return _target + glm::vec3(0.0f, _height * 0.5f, 0.0f);
}

std::optional<CameraModel::CameraInterpolationUpdateInfo> EditorCameraModel::Update(std::chrono::microseconds dt,
                                                                                    [[maybe_unused]] const Camera& camera)
{
	const auto seconds = std::chrono::duration<float>(dt).count();
	const auto focus = GoalFocus();
	if (_mode == Mode::Orbit)
	{
		_focus = focus;
		_origin = editor::OrbitOrigin(focus, _orbit);
	}
	else
	{
		auto behind = _orbit;
		behind.yaw = editor::BehindYaw(_facing, _followTurn);
		_origin = editor::EaseTowards(_origin, editor::OrbitOrigin(focus, behind), seconds, k_FollowEasePerSecond);
		_focus = editor::EaseTowards(_focus, focus, seconds, k_FocusEasePerSecond);
	}
	// The camera goes straight there: the easing is done here
	return CameraInterpolationUpdateInfo {.origin = _origin, .focus = _focus, .duration = std::chrono::microseconds::zero()};
}

void EditorCameraModel::HandleActions([[maybe_unused]] std::chrono::microseconds dt) {}

void EditorCameraModel::SetFlight(glm::vec3 origin, glm::vec3 focus)
{
	_origin = origin;
	_focus = focus;
	_orbit = editor::OrbitFrom(origin, focus);
}

} // namespace openblack

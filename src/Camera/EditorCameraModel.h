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
#include <glm/vec3.hpp>

#include "CameraModel.h"
#include "Editor/EditorMath.h"

namespace openblack
{

/// The editor's cameras on the thing picked in it, which take the camera over from the player's and hand it back:
/// - orbiting, the camera circles the thing at a distance, turned by dragging and drawn in and out by the wheel, and
///   keeps round it as it moves
/// - following, the camera trails behind the thing as it turns and moves, easing after it, and can be turned round it
///   and drawn in and out the same way
/// The editor tells it where the thing is each frame and passes it the drags and the wheel.
class EditorCameraModel final: public CameraModel
{
public:
	enum class Mode : uint8_t
	{
		Orbit,
		Follow,
	};

	/// Starts round a target from where the camera is
	EditorCameraModel(Mode mode, glm::vec3 origin, glm::vec3 target);

	void SetMode(Mode mode);
	[[nodiscard]] Mode GetMode() const { return _mode; }
	/// Where the thing is, the way it faces on the land, and how tall it is
	void SetTarget(glm::vec3 position, glm::vec2 facing, float height);
	/// A drag, in radians across and up
	void Turn(glm::vec2 radians);
	/// Steps of the wheel, closer for positive
	void Zoom(float steps);
	[[nodiscard]] const editor::Orbit& GetOrbit() const { return _orbit; }

	std::optional<CameraInterpolationUpdateInfo> Update(std::chrono::microseconds dt, const Camera& camera) override;
	void HandleActions(std::chrono::microseconds dt) override;
	void SetFlight(glm::vec3 origin, glm::vec3 focus) override;
	[[nodiscard]] glm::vec3 GetTargetOrigin() const override { return _origin; }
	[[nodiscard]] glm::vec3 GetTargetFocus() const override { return _focus; }
	[[nodiscard]] std::chrono::seconds GetIdleTime() const override { return std::chrono::seconds::zero(); }

private:
	[[nodiscard]] glm::vec3 GoalFocus() const;

	Mode _mode;
	editor::Orbit _orbit;
	/// Following, how far round from behind the thing the camera has been turned
	float _followTurn {0.0f};
	glm::vec3 _target;
	glm::vec2 _facing {0.0f, 1.0f};
	float _height {2.0f};
	glm::vec3 _origin;
	glm::vec3 _focus;
};

} // namespace openblack

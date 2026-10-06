/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "CameraModel.h"
#include "CreatureFollow.h"

namespace openblack
{

/// Creature Mode's camera: locked onto a creature, it follows it round the land, easing after it. The player turns it
/// with the keys (see creature_follow), and the cursor keys alone give the camera back. Creature Mode takes the camera
/// over from the player's own camera with it and hands it back afterwards; it tells it each frame where the creature is.
class CreatureCameraModel final: public CameraModel
{
public:
	/// Starts on a creature, by its middle and height, from where the camera is
	CreatureCameraModel(glm::vec3 cameraOrigin, glm::vec3 cameraFocus, glm::vec3 creature, float height);

	/// Where the creature now is, and how tall it is
	void SetTarget(glm::vec3 position, float height);
	/// Swings the camera round and tilts it for a clear view of the creature, as Ctrl and Shift do together
	void ClearView(glm::vec3 landNormal, const creature_follow::GroundHeight& ground);
	/// Turns it by keys, as if held this frame, on a screen so many pixels wide; returns whether they give the camera
	/// back
	creature_follow::KeyOutcome Turn(const creature_follow::Keys& keys, float screenWidth);

	[[nodiscard]] const creature_follow::View& GetView() const { return _view; }
	[[nodiscard]] float GetSecondsInMode() const { return _seconds; }
	/// Whether the player has asked for their camera back with the cursor keys
	[[nodiscard]] bool WantsToLeave() const { return _leaveRequested; }

	std::optional<CameraInterpolationUpdateInfo> Update(std::chrono::microseconds dt, const Camera& camera) override;
	/// Reads the keys: the cursor keys with Shift or Ctrl, the wheel, and Ctrl and Shift together for a clear view
	void HandleActions(std::chrono::microseconds dt) override;
	void SetFlight(glm::vec3 origin, glm::vec3 focus) override;
	[[nodiscard]] glm::vec3 GetTargetOrigin() const override { return creature_follow::Origin(_focus, _view); }
	[[nodiscard]] glm::vec3 GetTargetFocus() const override { return _focus; }
	[[nodiscard]] std::chrono::seconds GetIdleTime() const override { return std::chrono::seconds::zero(); }

private:
	creature_follow::View _view;
	glm::vec3 _focus;
	float _seconds {0.0f};
	bool _leaveRequested {false};
};

} // namespace openblack

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureCameraModel.h"

#include <glm/gtx/vec_swizzle.hpp>

#include "3D/LandIslandInterface.h"
#include "Input/GameActionMapInterface.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

namespace openblack
{

CreatureCameraModel::CreatureCameraModel(glm::vec3 cameraOrigin, glm::vec3 cameraFocus, glm::vec3 creature, float height)
    : _view(creature_follow::Start(cameraOrigin, cameraFocus, creature, height))
    , _focus(creature_follow::Focus(creature, height))
{
}

void CreatureCameraModel::SetTarget(glm::vec3 position, float height)
{
	_focus = creature_follow::Focus(position, height);
}

void CreatureCameraModel::ClearView(glm::vec3 landNormal, const creature_follow::GroundHeight& ground)
{
	_view = creature_follow::Clamped(creature_follow::ClearView(_view, _focus, landNormal, ground));
}

creature_follow::KeyOutcome CreatureCameraModel::Turn(const creature_follow::Keys& keys, float screenWidth)
{
	const auto outcome = creature_follow::Apply(_view, keys, screenWidth);
	if (outcome == creature_follow::KeyOutcome::Leave)
	{
		_leaveRequested = true;
	}
	return outcome;
}

std::optional<CameraModel::CameraInterpolationUpdateInfo> CreatureCameraModel::Update(std::chrono::microseconds dt,
                                                                                      [[maybe_unused]] const Camera& camera)
{
	const auto seconds = std::chrono::duration<float>(dt).count();
	// It sets off afresh each frame for where it wants to be, arriving after the ease's time, so it eases after the
	// creature as it moves
	const auto ease = creature_follow::EaseSeconds(_seconds);
	_seconds += seconds;
	_view = creature_follow::Clamped(_view);
	return CameraInterpolationUpdateInfo {
	    .origin = creature_follow::Origin(_focus, _view),
	    .focus = _focus,
	    .duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<float>(ease)),
	};
}

void CreatureCameraModel::HandleActions(std::chrono::microseconds dt)
{
	using input::BindableActionMap;
	if (!Locator::gameActionSystem::has_value())
	{
		return;
	}
	const auto& actions = Locator::gameActionSystem::value();
	const auto seconds = std::chrono::duration<float>(dt).count();
	const int across =
	    (actions.Get(BindableActionMap::MOVE_RIGHT) ? 1 : 0) - (actions.Get(BindableActionMap::MOVE_LEFT) ? 1 : 0);
	const int along =
	    (actions.Get(BindableActionMap::MOVE_BACKWARDS) ? 1 : 0) - (actions.Get(BindableActionMap::MOVE_FORWARDS) ? 1 : 0);
	auto keys = creature_follow::KeysFor(across, along, seconds);
	keys.shift = actions.Get(BindableActionMap::ROTATE_ON);
	keys.ctrl = actions.Get(BindableActionMap::ZOOM_ON);
	if (actions.Get(BindableActionMap::ZOOM_OUT))
	{
		keys.wheel -= creature_follow::k_WheelNotch;
	}
	if (actions.Get(BindableActionMap::ZOOM_IN))
	{
		keys.wheel += creature_follow::k_WheelNotch;
	}
	const auto width = Locator::windowing::has_value() ? static_cast<float>(Locator::windowing::value().GetSize().x) : 1.0f;
	Turn(keys, width);

	// Ctrl and Shift held together, with the cursor keys still, swing the camera clear of the land in the way
	if (keys.shift && keys.ctrl && !keys.Moving() && Locator::terrainSystem::has_value())
	{
		const auto& land = Locator::terrainSystem::value();
		ClearView(land.GetNormalAt(glm::xz(_focus)), [&land](glm::vec2 point) { return land.GetHeightAt(point); });
	}
}

void CreatureCameraModel::SetFlight(glm::vec3 origin, glm::vec3 focus)
{
	// Sent somewhere, it keeps looking at the creature from the way the flight would have it look
	const auto turned = creature_follow::HeadingPitchOf(origin, focus);
	_view.yaw = turned.heading;
	_view.pitch = turned.pitch;
	_view.distance = glm::distance(origin, focus);
	_view = creature_follow::Clamped(_view);
}

} // namespace openblack

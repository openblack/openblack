/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ZoomToPlaces.h"

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

namespace openblack::zoom_to
{

float HeadingOf(const CameraView& view) noexcept
{
	const auto towards = view.focus - view.origin;
	if (towards.x == 0.0f && towards.z == 0.0f)
	{
		return 0.0f;
	}
	return std::atan2(towards.x, towards.z);
}

glm::vec3 OriginAround(glm::vec3 focus, float heading, float distance, float pitch) noexcept
{
	const float across = distance * std::cos(pitch);
	return focus + glm::vec3(-across * std::sin(heading), distance * std::sin(pitch), -across * std::cos(heading));
}

CameraView ZoomToPlaces::ZoomTo(const CameraView& current, glm::vec3 targetGround, float distance, float pitch) noexcept
{
	const auto focus = targetGround + glm::vec3(0.0f, k_FocusAboveGround, 0.0f);
	// Flown back from, if pressed again while still looking there
	_returnTo = current;
	_lastTap.reset();
	return {.origin = OriginAround(focus, HeadingOf(current), distance, pitch), .focus = focus};
}

std::optional<CameraView> ZoomToPlaces::PressTemple(std::chrono::milliseconds now, const CameraView& current,
                                                    glm::vec3 lookedAtGround, std::optional<glm::vec3> templeGround,
                                                    glm::vec3 realmGround) noexcept
{
	const bool doubleTap = _lastTap.has_value() && now - *_lastTap <= k_DoubleTapTime;
	if (!doubleTap)
	{
		// A single tap keeps the heading and what is looked at, from a pleasing height and angle
		_lastTap = now;
		const auto focus = lookedAtGround + glm::vec3(0.0f, k_FocusAboveGround, 0.0f);
		return CameraView {.origin = OriginAround(focus, HeadingOf(current), k_TapDistance, k_TapPitch), .focus = focus};
	}
	if (!templeGround.has_value())
	{
		return PressRealm(current, realmGround);
	}
	if (_returnTo.has_value())
	{
		const auto target = glm::vec2(templeGround->x, templeGround->z);
		if (glm::distance(target, glm::vec2(current.focus.x, current.focus.z)) <= k_ReturnRadius)
		{
			const auto back = *_returnTo;
			_returnTo.reset();
			_lastTap.reset();
			return back;
		}
	}
	return ZoomTo(current, *templeGround, k_TempleDistance, k_TemplePitch);
}

std::optional<CameraView> ZoomToPlaces::PressRealm(const CameraView& current, glm::vec3 realmGround) noexcept
{
	if (_returnTo.has_value())
	{
		const auto target = glm::vec2(realmGround.x, realmGround.z);
		if (glm::distance(target, glm::vec2(current.focus.x, current.focus.z)) <= k_ReturnRadius)
		{
			const auto back = *_returnTo;
			_returnTo.reset();
			return back;
		}
	}
	return ZoomTo(current, realmGround, k_RealmDistance, k_RealmPitch);
}

} // namespace openblack::zoom_to

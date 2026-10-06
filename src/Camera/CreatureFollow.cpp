/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureFollow.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>

namespace openblack::creature_follow
{

namespace
{
/// A point this close to straight above or below the centre has no heading
constexpr float k_NoHeadingWithin = 0.01f;
/// Straight above, the game takes the pitch as a little short of a right angle
constexpr float k_OverheadPitch = 1.5393804f;
/// The wheel's zoom: each count it moves draws the camera in by this share, about the half-thousandth parts below
constexpr float k_WheelZoomPerCount = -0.15f;
/// Shift and the cursor keys tilt the camera this many radians a pixel; Ctrl and the cursor keys, and the wheel, scale
/// the distance by a five-hundredth part a pixel
constexpr float k_TiltPerPixel = 0.002f;
constexpr float k_ZoomPixels = 500.0f;
constexpr float k_ZoomPerPixel = 0.002f;
/// The clear view looks round at 32 headings, each sampling the land at 8 points out to the distance, and favours the
/// headings nearest the camera's by this many units of fall
constexpr int k_ClearHeadings = 32;
constexpr int k_ClearSamples = 8;
constexpr float k_ClearHeadingStep = std::numbers::pi_v<float> / 16.0f;
constexpr float k_ClearFavourCurrent = 50.0f;
/// The distance it looks at is taken this much of the way towards 50 when nearer, and towards 100 when further
constexpr float k_ClearNear = 50.0f;
constexpr float k_ClearNearShare = 0.8f;
constexpr float k_ClearFar = 100.0f;
constexpr float k_ClearFarShare = 0.1f;
/// The clear pitch: a steady part, a share of the camera's pitch and a share of half the slope's
constexpr float k_ClearPitchBase = 0.37699112f;
constexpr float k_ClearPitchShare = 0.2f;
constexpr float k_ClearSlopeShare = 0.1f;
constexpr float k_ClearMinPitch = std::numbers::pi_v<float> / 8.0f;
constexpr float k_ClearMaxPitch = std::numbers::pi_v<float> / 3.0f;
} // namespace

glm::vec3 Focus(glm::vec3 position, float height)
{
	return position + glm::vec3(0.0f, height * 0.5f, 0.0f);
}

float ViewingDistance(float height)
{
	return height * k_ViewingDistancePerHeight;
}

HeadingPitch HeadingPitchOf(glm::vec3 point, glm::vec3 centre)
{
	const auto offset = point - centre;
	if (std::abs(offset.x) < k_NoHeadingWithin && std::abs(offset.z) < k_NoHeadingWithin)
	{
		return {.heading = 0.0f, .pitch = k_OverheadPitch};
	}
	const auto across = std::sqrt((offset.x * offset.x) + (offset.z * offset.z));
	return {.heading = std::atan2(offset.x, offset.z), .pitch = std::atan2(offset.y, across)};
}

View Start(glm::vec3 cameraOrigin, glm::vec3 cameraFocus, glm::vec3 creature, float height)
{
	const auto turned = HeadingPitchOf(cameraOrigin, cameraFocus);
	auto distance = ViewingDistance(height);
	const glm::vec2 across {creature.x - cameraOrigin.x, creature.z - cameraOrigin.z};
	if (glm::length(across) < k_CloseAcross)
	{
		// Already close, it stays about as far as it is, but no nearer than twice the creature's height and no further
		// than it would start
		const auto now = glm::distance(creature, cameraOrigin);
		const auto nearest = height * 2.0f;
		distance = now > nearest ? std::min(now, distance) : nearest;
	}
	return Clamped({.yaw = turned.heading, .pitch = turned.pitch, .distance = distance});
}

View Clamped(View view)
{
	view.distance = std::clamp(view.distance, k_MinDistance, k_MaxDistance);
	view.pitch = std::max(view.pitch, k_MinPitch);
	return view;
}

float EaseSeconds(float secondsInMode)
{
	if (secondsInMode >= k_EaseSettleSeconds)
	{
		return k_EaseSeconds;
	}
	const auto settled = std::max(secondsInMode, 0.0f) / k_EaseSettleSeconds;
	return k_StartEaseSeconds + ((k_EaseSeconds - k_StartEaseSeconds) * settled);
}

glm::vec3 Origin(glm::vec3 focus, const View& view)
{
	return editor::OrbitOrigin(focus, view);
}

Keys KeysFor(int across, int along, float seconds)
{
	return {
	    .across = static_cast<float>(across) * k_KeyPixelsPerSecond * seconds,
	    .along = static_cast<float>(along) * k_KeyPixelsPerSecond * seconds,
	};
}

KeyOutcome Apply(View& view, const Keys& keys, float screenWidth)
{
	const auto zoom = ((keys.wheel * k_WheelZoomPerCount) + k_ZoomPixels) * k_ZoomPerPixel;
	view.distance = std::max(view.distance * zoom, k_MinDistance);
	if (keys.Moving())
	{
		const auto turn = keys.across * std::numbers::pi_v<float> / std::max(screenWidth, 1.0f);
		if (keys.ctrl)
		{
			view.yaw -= turn;
			view.distance = std::max(view.distance * ((keys.along + k_ZoomPixels) * k_ZoomPerPixel), k_MinDistance);
		}
		else if (keys.shift)
		{
			view.yaw -= turn;
			view.pitch = std::clamp(view.pitch - (keys.along * k_TiltPerPixel), k_MinKeyPitch, k_MaxKeyPitch);
		}
		else
		{
			return KeyOutcome::Leave;
		}
		view.yaw = editor::WrapAngle(view.yaw);
	}
	view.distance = std::clamp(view.distance * zoom, k_MinDistance, k_MaxKeyDistance);
	return KeyOutcome::Stay;
}

float ClearingDistance(float distance)
{
	if (distance < k_ClearNear)
	{
		distance += (k_ClearNear - distance) * k_ClearNearShare;
	}
	if (distance > k_ClearFar)
	{
		distance += (k_ClearFar - distance) * k_ClearFarShare;
	}
	return distance;
}

float ClearHeading(float heading, float distance, glm::vec3 focus, const GroundHeight& ground)
{
	float best = -1e20f;
	int bestStep = 0;
	for (int step = 0; step < k_ClearHeadings; ++step)
	{
		const auto turn = static_cast<float>(step) * k_ClearHeadingStep;
		const glm::vec2 along {std::sin(heading + turn), std::cos(heading + turn)};
		// The further the land falls away below the creature's middle along the heading, the clearer the view
		float fall = 0.0f;
		for (int sample = 0; sample < k_ClearSamples; ++sample)
		{
			const auto reach = static_cast<float>(sample) * distance / static_cast<float>(k_ClearSamples);
			fall += focus.y - ground(glm::vec2(focus.x, focus.z) + (along * reach));
		}
		fall += std::cos(turn) * k_ClearFavourCurrent;
		if (best < fall)
		{
			best = fall;
			bestStep = step;
		}
	}
	return editor::WrapAngle(heading + (static_cast<float>(bestStep) * k_ClearHeadingStep));
}

float SlopePitch(glm::vec3 normal)
{
	return HeadingPitchOf(normal, glm::vec3(0.0f)).pitch;
}

float ClearPitch(float pitch, float slopePitch)
{
	const auto tilted = (slopePitch * k_ClearSlopeShare) + (pitch * k_ClearPitchShare) + k_ClearPitchBase;
	return std::clamp(tilted, k_ClearMinPitch, k_ClearMaxPitch);
}

View ClearView(View view, glm::vec3 focus, glm::vec3 landNormal, const GroundHeight& ground)
{
	view.yaw = ClearHeading(view.yaw, ClearingDistance(view.distance), focus, ground);
	view.pitch = ClearPitch(view.pitch, SlopePitch(landNormal));
	return view;
}

} // namespace openblack::creature_follow

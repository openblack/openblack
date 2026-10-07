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

#include <optional>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include "Camera/CameraDrag.h"

/// The hand's pose and its up while it moves the camera, from the camera's hints: gripping the land it pans, turning
/// the camera round the edge of the screen, tilting it, and offering to do so with the cursor at the edge
namespace openblack::hand_navigation_pose
{

enum class Pose : uint8_t
{
	/// The hand's ordinary pose
	Idle,
	Grip,
	Rotate,
	Pitch,
	Zoom,
};

/// The hand's pose while it drags the land
[[nodiscard]] inline Pose WhileDragging(bool gripsLand, uint32_t tricons, uint32_t features = camera_drag::k_DefaultFeatures)
{
	namespace tricon = camera_drag::tricon;
	namespace feature = camera_drag::feature;
	if (gripsLand)
	{
		return Pose::Grip;
	}
	if ((tricons & tricon::k_Rotate) != 0)
	{
		return (features & feature::k_Rotate) != 0 ? Pose::Rotate : Pose::Idle;
	}
	if ((tricons & tricon::k_Zoom) != 0)
	{
		return (features & feature::k_Zoom) != 0 ? Pose::Zoom : Pose::Idle;
	}
	if ((tricons & tricon::k_Pitch) != 0 && (features & feature::k_Pitch) != 0)
	{
		return Pose::Pitch;
	}
	return Pose::Idle;
}

/// The hand's pose hovering with nothing under it, where the cursor's place offers turning or tilting the camera; none
/// leaves the hand to its ordinary poses
[[nodiscard]] inline std::optional<Pose> WhileHovering(uint32_t tricons, uint32_t features = camera_drag::k_DefaultFeatures)
{
	namespace tricon = camera_drag::tricon;
	namespace feature = camera_drag::feature;
	if ((tricons & tricon::k_Rotate) != 0)
	{
		return (features & feature::k_Rotate) != 0 ? std::optional(Pose::Rotate) : std::nullopt;
	}
	if ((tricons & tricon::k_Pitch) != 0 && (features & feature::k_Pitch) != 0)
	{
		return Pose::Pitch;
	}
	return std::nullopt;
}

/// Offering to turn the camera, the hand stands up towards where the camera looks, leaning up by four tenths of the way
[[nodiscard]] inline glm::vec3 UpTowardsFocus(glm::vec3 from, glm::vec3 focus)
{
	auto towards = focus - from;
	towards.y += glm::length(towards) * 0.4f;
	return glm::length(towards) > 0.0f ? glm::normalize(towards) : glm::vec3(0.0f, 1.0f, 0.0f);
}

/// Showing that it turns the camera while it drags the land, the hand is drawn this share of its height higher
constexpr float k_RotateLiftShare = 0.33f;

} // namespace openblack::hand_navigation_pose

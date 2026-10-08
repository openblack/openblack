/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <limits>
#include <optional>

#include <entt/entity/entity.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "3D/ScreenPick.h"

namespace openblack::ecs::systems
{

/// What the game finds along lines and under the cursor: where a line meets the land, the land or sea under a pixel,
/// the object the interface picks under the cursor as it draws, and where a line meets a model's triangles
class PickingSystemInterface
{
public:
	/// What the interface took to be under the cursor at the last frame drawn
	struct Pick
	{
		/// The object picked, if one is
		std::optional<entt::entity> object;
		/// The point picked: the object's position, else the land or sea under the cursor
		std::optional<glm::vec3> point;
		/// How far along the view the point is
		float distance {std::numeric_limits<float>::max()};
		/// The land or sea under the cursor, whatever was picked
		std::optional<glm::vec3> land;
	};

	/// A frame as it is drawn: the camera, its view and the cursor
	struct Frame
	{
		screen_pick::View view;
		/// The cursor's point on the near plane
		glm::vec3 nearPoint {0.0f};
	};

	virtual ~PickingSystemInterface() = default;

	/// Where the line from one point to another first meets the land (its height there), none over the sea
	[[nodiscard]] virtual std::optional<glm::vec3> LandAlong(glm::vec3 from, glm::vec3 to) const = 0;
	/// The same, else where the line meets the sea's level going down, no further than 7500 across from the camera
	[[nodiscard]] virtual std::optional<glm::vec3> LandOrSeaAlong(glm::vec3 from, glm::vec3 to, glm::vec3 camera) const = 0;
	/// The land under a pixel, from the camera through the pixel's point on the near plane, with the sea or without it,
	/// at the land's height there and within reach of the map's middle
	[[nodiscard]] virtual std::optional<glm::vec3> LandUnderPixel(glm::vec3 camera, glm::vec3 nearPoint,
	                                                              bool withSea) const = 0;

	/// The interface's pick at a frame as it is drawn: the drawn object under the cursor, weighed against the land
	virtual void PickUnderCursor(const Frame& frame) = 0;
	/// The pick of the last frame drawn
	[[nodiscard]] virtual const Pick& GetPick() const = 0;
	/// The pick before that, which the hand goes by
	[[nodiscard]] virtual const Pick& GetHandPick() const = 0;

	/// Where a line meets an object's model as it is drawn, the hand feeling it: the nearest triangle ahead of the line's
	/// start, and its normal turned along the line
	[[nodiscard]] virtual std::optional<screen_pick::MeshHit> FeelModel(entt::entity object, glm::vec3 origin,
	                                                                    glm::vec3 direction) const = 0;
};

} // namespace openblack::ecs::systems

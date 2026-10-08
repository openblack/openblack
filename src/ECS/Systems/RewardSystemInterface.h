/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <entt/entity/fwd.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::particles::draw
{
struct Frame;
}

namespace openblack::ecs::systems
{

/// The reward chests the scripts give: put on the land, or dropped from the sky with a thump that shakes the camera and
/// a cloud of dust
class RewardSystemInterface
{
public:
	virtual ~RewardSystemInterface() = default;

	/// A reward chest of a kind for a player (and a town, or none) at a point: on the land, or falling from the sky
	virtual entt::entity Create(glm::vec3 position, RewardObjectInfo type, std::optional<PlayerNames> player, entt::entity town,
	                            bool fromSky) = 0;
	/// Every frame, by the game's time: the chests from the sky fall and turn, and thump down; their dust fades
	virtual void Update(float milliseconds) = 0;
	/// Every game turn: a chest that has just landed goes into the map's cells
	virtual void ProcessTurn() = 0;
	/// The chests' dust drawn this frame, added to the particles' frame
	virtual void CollectDrawFrame(particles::draw::Frame& frame) const = 0;
};

} // namespace openblack::ecs::systems

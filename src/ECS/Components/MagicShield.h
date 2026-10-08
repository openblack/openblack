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

#include <array>
#include <vector>

#include <entt/core/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "Magic/ShieldRules.h"
#include "Magic/VillagerReactionRules.h"

namespace openblack::ecs::components
{

/// A shield miracle's object in the world, standing on the land where it was cast: the spiritual shield's, which only
/// marks where it is (its sphere is its particle effect's), or the physical shield's dome
struct MagicShield
{
	enum class Kind : uint8_t
	{
		Spiritual,
		Physical,
	};
	Kind kind {Kind::Spiritual};
	/// The miracle keeping it up, none once the miracle has let go of it
	entt::entity spell {entt::null};
	PlayerNames player {PlayerNames::NEUTRAL};
	float radius {0.0f};
	/// The nearest town it protects, if any
	entt::entity town {entt::null};
	/// The people's reactions to it standing and to it being struck, none for none
	uint32_t reaction {0};
	uint32_t struckReaction {0};
};

/// The physical shield's dome: its model grows, spins and bobs, and fades as it dies; its solid shape stops what is
/// thrown at it
struct ShieldDome
{
	magic::shield::DomeShape shape;
	magic::shield::DomeState state;
	/// This turn's pose and the last, drawn between
	magic::shield::DomePose pose;
	magic::shield::DomePose previous;
	/// Seconds since it was raised
	float age {0.0f};
	/// The size its solid shape is at, which follows the drawn size only in steps
	float solidScale {0.0f};
	/// How opaque it is drawn, 0 to 255
	float alpha {255.0f};
	entt::id_type mesh {0};
	/// Half the model's size along each axis, and the middle of its box
	glm::vec3 halfExtent {0.0f};
	/// Its particle effect, 0 for none
	uint32_t effect {0};
	/// Its solid shape's triangles in the world, set once as it is raised at its full size, sunk as it starts and
	/// unturned: the game never moves its solid shape with the dome it draws
	std::vector<std::array<glm::vec3, 3>> hull;
	/// Where that solid shape's model stands and the scale it is at: the physics' body of the dome is built from them
	glm::vec3 hullOrigin {0.0f};
	float hullScale {0.0f};
};

/// Where no player but the owner has any influence: under a shield, an enemy's hand can't cast
struct AntiInfluence
{
	PlayerNames owner {PlayerNames::NEUTRAL};
	float radius {0.0f};
};

/// On a villager reacting to a shield, standing, struck or destroyed: the reaction and the shield it is to, the point it
/// looks towards, and the animation it last chose
struct VillagerShieldReaction
{
	/// What it was doing before, which it goes back to
	VillagerStates previous {VillagerStates::InvalidState};
	uint32_t reaction {0};
	Reaction type {Reaction::None};
	entt::entity shield {entt::null};
	glm::vec2 lookAt {0.0f};
	uint32_t animation {0};
};

/// The kinds of reaction a villager took up lately, and when
struct VillagerReactionMemory
{
	magic::villager_reaction::Memory memory;
};

} // namespace openblack::ecs::components

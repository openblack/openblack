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

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::components
{

/// A fireball in flight: a burning object that follows its particle, whose fire heats what it passes. Its heat capacity
/// grows with its size and its miracle's strength. When its fire cools away, or the object goes, the particle goes too.
struct MagicFireBall
{
	/// The miracle's strength as its particle last moved it, and the ball's radius, which is its sprite's size
	float strength {1.0f};
	float radius {1.0f};
	/// A fireball a script casts is not cooled by the rain
	bool affectedByRain {true};
	/// Whose fireball it is
	PlayerNames player {PlayerNames::NEUTRAL};
	bool hasPlayer {false};
	/// The game turn its particle last moved it
	uint32_t lastTurn {0};
	/// Where the hand may take hold of it, as its particle was last placed; none while the particle is faint (an alpha
	/// of 30 or less), when the hand passes through it
	std::optional<glm::vec3> handTarget;
};

} // namespace openblack::ecs::components

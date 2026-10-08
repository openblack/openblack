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
#include <optional>
#include <string>

#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::components
{

struct Player
{
	PlayerNames name;
	/// The harm each player's miracles did to what is this player's, by player number: their crushing and hitting, and the
	/// burning they would do. A creature's miracles aren't counted.
	std::array<float, 8> damageFrom {};
	/// Where, what and the turn the player last cast a miracle, and how many of each magic type they have cast
	struct Cast
	{
		glm::vec3 position {0.0f};
		MagicType type {MagicType::None};
		uint32_t turn {0};
	};
	std::optional<Cast> lastCast;
	std::array<uint32_t, 42> castsOfType {};
	/// A script made the things the player's hand throws fly without the air's drag
	uint32_t windResistance {0};

	/// Each player's colour, 0xAARRGGBB, by player number; the neutral player's is black
	static constexpr std::array<uint32_t, 8> k_Colours = {
	    0xFFFF4646u, // red
	    0xFF47FF54u, // green
	    0xFFE347FFu, // magenta
	    0xFF47F9FFu, // cyan
	    0xFFFFFD47u, // yellow
	    0xFF4777FFu, // blue
	    0xFFFFA247u, // orange
	    0xFF000000u, // black
	};
};
} // namespace openblack::ecs::components

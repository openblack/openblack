/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>

#include "Enums.h"

namespace openblack::ecs::components
{

struct Town
{
	uint32_t id;
	/// The player whose town it is
	PlayerNames owner {PlayerNames::NEUTRAL};
	std::unordered_map<std::string, float> beliefs;
	bool uninhabitable = false;
	std::set<entt::entity> homelessVillagers;
	/// How many of its people are hurt, their life under seven tenths
	uint32_t injured {0};
	/// What its people's deaths came to, by the player put down for each (by player number) and what killed them
	std::array<std::array<float, static_cast<size_t>(DeathReason::_COUNT)>, 8> deathsByKiller {};
	/// The forest of the lone trees about it, made as the land is laid out; a tree replanted near the town joins it
	std::optional<uint32_t> scenicForest;
	/// Where its scenic forest was made: the town's centre then
	glm::vec2 scenicForestCentre {0.0f};
	/// The things its people play with, the newest first: footballs
	std::vector<entt::entity> playthings;
};

} // namespace openblack::ecs::components

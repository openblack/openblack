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

namespace openblack::ecs::components
{

/// The number of things a town can want (TownDesireInfo)
inline constexpr size_t k_TownDesireCount = 17;

/// One place in a town's order of desires
struct DesireSort
{
	/// The desire's boosts
	float boosts {0.0f};
	/// What the order is sorted by, most first
	float value {0.0f};
	/// Which desire (TownDesireInfo)
	uint32_t index {0};
};

/// What a town knows of its people and buildings, counted afresh each game turn for its desires
struct TownStats
{
	uint32_t adults {0};
	uint32_t children {0};
	/// Abodes with room for anyone, and the room in them for adults, children and both
	uint32_t abodesWithPlaces {0};
	uint32_t adultPlaces {0};
	uint32_t childPlaces {0};
	uint32_t totalPlaces {0};
	uint32_t civicBuildings {0};
	/// Villagers of each discipline
	std::array<uint8_t, 13> disciples {};
	/// The food the town's people need for their dinner
	float foodForDinner {0.0f};
	/// Food and wood its villagers carry, and wood waiting at its building sites
	float foodCarried {0.0f};
	float woodCarried {0.0f};
	float woodAtSites {0.0f};
	/// Abodes of each abode number
	std::array<uint8_t, 16> abodesByNumber {};
};

/// What a town wants, each game turn (see town_desire). Every array has a value for each desire.
struct TownDesire
{
	/// Boosts the town's desires keep: one the game itself never sets, and the scripts'
	std::array<float, k_TownDesireCount> boostA {};
	std::array<float, k_TownDesireCount> boost {};
	/// Each desire with what its villagers are already doing for it taken off, -1 to 1
	std::array<float, k_TownDesireCount> desire {};
	/// The adults and children not off to worship
	float population {0.0f};
	/// Each desire as the town feels it, by its tribe, not limited
	std::array<float, k_TownDesireCount> raw {};
	/// The desires most first, by their desire and by their raw desire
	std::array<DesireSort, k_TownDesireCount> sorted {};
	std::array<DesireSort, k_TownDesireCount> sortedRaw {};
	/// How much the town's villagers are doing for each desire, and how many states that is, now and at the start of
	/// this turn
	std::array<float, k_TownDesireCount> doingNow {};
	std::array<float, k_TownDesireCount> doingNowCount {};
	std::array<float, k_TownDesireCount> doingNowAtStart {};
	std::array<float, k_TownDesireCount> doingNowCountAtStart {};
};

} // namespace openblack::ecs::components

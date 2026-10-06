/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <vector>

#include <glm/vec2.hpp>

#include "Enums.h"

/// The crowds of the testbed's benchmarks: hundreds or thousands of creatures or villagers, all fully simulated, spread
/// over the land in the same way on every run so that runs can be compared. Laying them out is pure; the scenario runner
/// spawns them a batch a frame.
namespace openblack::testbed_scenarios
{

/// A crowd a scenario spawns, rather than listing each of its members
struct Crowd
{
	enum class Kind : uint8_t
	{
		/// Creatures of every species, owned by several players, their minds and bodies left to themselves
		Creatures,
		/// Villagers of every tribe and role, living in towns of homes about a storage pit
		Villagers,
	};
	Kind kind {Kind::Creatures};
	size_t count {0};
	/// The seed of the layout's random choices, so that every run lays it out the same
	uint32_t seed {1};
	/// How many members are spawned each frame, so that spawning them doesn't make one frame take seconds
	size_t perFrame {100};
};

/// A small random number generator that gives the same numbers from the same seed on every platform and build, which
/// the standard library's distributions don't promise
class CrowdRandom
{
public:
	explicit CrowdRandom(uint32_t seed);
	[[nodiscard]] uint32_t Next();
	/// From low up to but not including high
	[[nodiscard]] float Between(float low, float high);
	/// From 0 up to but not including count, which must not be 0
	[[nodiscard]] size_t Below(size_t count);

private:
	uint64_t _state;
};

/// Whether a point, from the middle of the map, is within a margin of the testbed's lake, its shallows and its bank
[[nodiscard]] bool NearLake(glm::vec2 offset, float margin);

/// Points on a square grid of the spacing round the middle of the map, the nearest to the middle first, leaving out
/// those within the margin of the lake, nudged off the grid by up to the jitter, a share of the spacing
[[nodiscard]] std::vector<glm::vec2> SpreadPoints(size_t count, float spacing, float lakeMargin, float jitter,
                                                  CrowdRandom& random);

struct CrowdCreature
{
	glm::vec2 offset;
	CreatureType species;
	PlayerNames owner;
	float facingDegrees;
};

struct CrowdTown
{
	glm::vec2 offset;
	Tribe tribe;
	PlayerNames owner;
};

struct CrowdAbode
{
	/// By its place in the layout's towns
	size_t town;
	glm::vec2 offset;
	AbodeInfo type;
	float yawDegrees;
	/// Whether villagers live in it, rather than it being the town's store
	bool home;
};

struct CrowdVillager
{
	/// The home it lives in, by its place in the layout's abodes
	size_t abode;
	glm::vec2 offset;
	VillagerInfo type;
	uint32_t age;
};

struct VillageLayout
{
	std::vector<CrowdTown> towns;
	std::vector<CrowdAbode> abodes;
	std::vector<CrowdVillager> villagers;
};

/// How far apart the creatures of a crowd stand, about a grown creature's height
constexpr float k_CreatureSpacing = 20.0f;
/// How far apart the towns of a crowd of villagers are, and how many live in each and in each home
constexpr float k_TownSpacing = 150.0f;
constexpr size_t k_VillagersPerTown = 50;
constexpr size_t k_VillagersPerHome = 5;
/// How far from the middle of its town its homes stand, and how far out its villagers start
constexpr float k_HomeRing = 40.0f;
constexpr float k_VillagerSpread = 55.0f;
/// The players who own the crowd's creatures and towns
constexpr std::array<PlayerNames, 4> k_CrowdOwners {PlayerNames::PLAYER_ONE, PlayerNames::PLAYER_TWO, PlayerNames::PLAYER_THREE,
                                                    PlayerNames::PLAYER_FOUR};

/// The creatures of a crowd: every species in turn as the random choices have it, their owners mixed
[[nodiscard]] std::vector<CrowdCreature> LayOutCreatures(size_t count, uint32_t seed);
/// The villagers of a crowd, in towns of each tribe, each with a storage pit in its middle and homes round it, the
/// villagers of each home of mixed roles and ages
[[nodiscard]] VillageLayout LayOutVillagers(size_t count, uint32_t seed);

/// The villager of a tribe in a role: the housewife first, then the forester, fisherman, farmer, shepherd, leader and
/// trader
[[nodiscard]] VillagerInfo VillagerOf(Tribe tribe, size_t role);
/// A tribe's hut and storage pit
[[nodiscard]] AbodeInfo HutOf(Tribe tribe);
[[nodiscard]] AbodeInfo StoragePitOf(Tribe tribe);

} // namespace openblack::testbed_scenarios

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TestbedCrowd.h"

#include <cmath>

#include <algorithm>
#include <numbers>
#include <tuple>

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

#include "3D/FlatLand.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
/// The lake from the middle of the map, and half its size with its shallows and bank
constexpr glm::vec2 k_Lake = flat_land::k_LakeCentre - flat_land::k_MapMiddle;
constexpr glm::vec2 k_LakeWithShore =
    flat_land::k_LakeHalfExtent + glm::vec2(static_cast<float>(flat_land::k_ShoreCells) * flat_land::k_CellSize);
/// Offsets further from the middle of the map than this would be off it, or too near its edge
constexpr float k_MaxOffset = 2400.0f;
/// How many villagers and roles a tribe has, and how its abodes are numbered
constexpr size_t k_RolesPerTribe = 7;
constexpr size_t k_AbodesPerTribe = 16;
constexpr size_t k_HutIndex = 0;
constexpr size_t k_StoragePitIndex = 7;
/// How likely each role is among the grown up villagers of a home, in the order VillagerOf takes them, out of 100
constexpr std::array<size_t, k_RolesPerTribe> k_RoleWeights {30, 15, 10, 20, 10, 5, 10};
/// The share of villagers that are children, out of 100, and the ages of children and grown ups
constexpr size_t k_ChildShare = 15;
constexpr uint32_t k_YoungestChild = 5;
constexpr uint32_t k_GrownUp = 18;
constexpr uint32_t k_Eldest = 60;
/// How far from its home a villager starts
constexpr float k_NearHome = 8.0f;
constexpr float k_FarFromHome = 15.0f;
/// How far a town keeps from the lake
constexpr float k_TownLakeMargin = k_VillagerSpread + 15.0f;

size_t WeightedRole(CrowdRandom& random)
{
	size_t total = 0;
	for (const auto weight : k_RoleWeights)
	{
		total += weight;
	}
	auto pick = random.Below(total);
	for (size_t role = 0; role < k_RoleWeights.size(); ++role)
	{
		if (pick < k_RoleWeights.at(role))
		{
			return role;
		}
		pick -= k_RoleWeights.at(role);
	}
	return 0;
}

glm::vec2 OnCircle(float radius, float angle)
{
	return radius * glm::vec2(std::cos(angle), std::sin(angle));
}
} // namespace

CrowdRandom::CrowdRandom(uint32_t seed)
    : _state(seed)
{
}

uint32_t CrowdRandom::Next()
{
	// SplitMix64: each step mixes a counter into a well spread value
	_state += 0x9E3779B97F4A7C15ULL;
	auto value = _state;
	value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
	value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
	value ^= value >> 31;
	return static_cast<uint32_t>(value >> 32);
}

float CrowdRandom::Between(float low, float high)
{
	// 24 bits, which a float holds exactly, so the result is below 1 before scaling
	const auto unit = static_cast<float>(Next() >> 8) / static_cast<float>(1u << 24);
	return low + ((high - low) * unit);
}

size_t CrowdRandom::Below(size_t count)
{
	return static_cast<size_t>(Next()) % count;
}

bool testbed_scenarios::NearLake(glm::vec2 offset, float margin)
{
	const auto from = glm::abs(offset - k_Lake);
	return from.x < k_LakeWithShore.x + margin && from.y < k_LakeWithShore.y + margin;
}

std::vector<glm::vec2> testbed_scenarios::SpreadPoints(size_t count, float spacing, float lakeMargin, float jitter,
                                                       CrowdRandom& random)
{
	std::vector<glm::vec2> points;
	if (count == 0 || spacing <= 0.0f)
	{
		return points;
	}
	const auto maxRadius = static_cast<int>(k_MaxOffset / spacing);
	// Enough of the grid for the count, with some to spare for the cells the lake takes; more if that's not enough
	auto radius = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count) / std::numbers::pi_v<float>))) + 2;
	struct Cell
	{
		int x;
		int y;
	};
	std::vector<Cell> cells;
	while (true)
	{
		radius = std::min(radius, maxRadius);
		cells.clear();
		for (int y = -radius; y <= radius; ++y)
		{
			for (int x = -radius; x <= radius; ++x)
			{
				const glm::vec2 point {static_cast<float>(x) * spacing, static_cast<float>(y) * spacing};
				if ((x * x) + (y * y) <= radius * radius && !NearLake(point, lakeMargin))
				{
					cells.push_back({x, y});
				}
			}
		}
		if (cells.size() >= count || radius == maxRadius)
		{
			break;
		}
		radius += radius / 4 + 1;
	}
	// The nearest the middle first, ties broken the same way everywhere
	std::ranges::sort(cells, [](const Cell& a, const Cell& b) {
		return std::tuple((a.x * a.x) + (a.y * a.y), a.y, a.x) < std::tuple((b.x * b.x) + (b.y * b.y), b.y, b.x);
	});
	cells.resize(std::min(cells.size(), count));
	points.reserve(cells.size());
	const auto nudge = jitter * spacing;
	for (const auto& cell : cells)
	{
		const glm::vec2 point {static_cast<float>(cell.x) * spacing, static_cast<float>(cell.y) * spacing};
		const glm::vec2 offset {random.Between(-nudge, nudge), random.Between(-nudge, nudge)};
		points.push_back(glm::clamp(point + offset, glm::vec2(-k_MaxOffset), glm::vec2(k_MaxOffset)));
	}
	return points;
}

VillagerInfo testbed_scenarios::VillagerOf(Tribe tribe, size_t role)
{
	return static_cast<VillagerInfo>((static_cast<size_t>(tribe) * k_RolesPerTribe) + (role % k_RolesPerTribe));
}

AbodeInfo testbed_scenarios::HutOf(Tribe tribe)
{
	return static_cast<AbodeInfo>((static_cast<size_t>(tribe) * k_AbodesPerTribe) + k_HutIndex);
}

AbodeInfo testbed_scenarios::StoragePitOf(Tribe tribe)
{
	return static_cast<AbodeInfo>((static_cast<size_t>(tribe) * k_AbodesPerTribe) + k_StoragePitIndex);
}

std::vector<CrowdCreature> testbed_scenarios::LayOutCreatures(size_t count, uint32_t seed)
{
	CrowdRandom random(seed);
	// Creatures are big: keep the lake's bank clear of them by a creature's stride
	const auto points = SpreadPoints(count, k_CreatureSpacing, k_CreatureSpacing, 0.3f, random);
	constexpr auto k_Species = static_cast<size_t>(CreatureType::_COUNT) - 1;
	std::vector<CrowdCreature> creatures;
	creatures.reserve(points.size());
	for (const auto& point : points)
	{
		creatures.push_back({
		    .offset = point,
		    .species = static_cast<CreatureType>(1 + random.Below(k_Species)),
		    .owner = k_CrowdOwners.at(random.Below(k_CrowdOwners.size())),
		    .facingDegrees = random.Between(0.0f, 360.0f),
		});
	}
	return creatures;
}

VillageLayout testbed_scenarios::LayOutVillagers(size_t count, uint32_t seed)
{
	CrowdRandom random(seed);
	VillageLayout layout;
	const auto towns = (count + k_VillagersPerTown - 1) / k_VillagersPerTown;
	const auto centres = SpreadPoints(towns, k_TownSpacing, k_TownLakeMargin, 0.0f, random);
	constexpr auto k_Tribes = static_cast<size_t>(Tribe::_COUNT);
	size_t placed = 0;
	for (size_t town = 0; town < centres.size(); ++town)
	{
		const auto centre = centres.at(town);
		const auto tribe = static_cast<Tribe>(random.Below(k_Tribes));
		layout.towns.push_back({
		    .offset = centre,
		    .tribe = tribe,
		    .owner = k_CrowdOwners.at(random.Below(k_CrowdOwners.size())),
		});
		// The store in the middle, the homes in a ring about it facing in
		layout.abodes.push_back(
		    {.town = town, .offset = centre, .type = StoragePitOf(tribe), .yawDegrees = 0.0f, .home = false});
		const auto people = std::min(k_VillagersPerTown, count - placed);
		const auto homes = (people + k_VillagersPerHome - 1) / k_VillagersPerHome;
		const auto firstHome = layout.abodes.size();
		for (size_t home = 0; home < homes; ++home)
		{
			const auto angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(home) / static_cast<float>(homes);
			layout.abodes.push_back({
			    .town = town,
			    .offset = centre + OnCircle(k_HomeRing, angle),
			    .type = HutOf(tribe),
			    .yawDegrees = 90.0f - glm::degrees(angle),
			    .home = true,
			});
		}
		for (size_t person = 0; person < people; ++person)
		{
			const auto home = firstHome + (person / k_VillagersPerHome);
			const bool child = random.Below(100) < k_ChildShare;
			const auto age = child ? k_YoungestChild + static_cast<uint32_t>(random.Below(k_GrownUp - k_YoungestChild))
			                       : k_GrownUp + static_cast<uint32_t>(random.Below(k_Eldest - k_GrownUp));
			const auto near =
			    OnCircle(random.Between(k_NearHome, k_FarFromHome), random.Between(0.0f, 2.0f * std::numbers::pi_v<float>));
			layout.villagers.push_back({
			    .abode = home,
			    .offset = layout.abodes.at(home).offset + near,
			    .type = VillagerOf(tribe, WeightedRole(random)),
			    .age = age,
			});
		}
		placed += people;
	}
	return layout;
}

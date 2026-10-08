/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of the beam explosion at each of its levels, on land and on the water, of fire spreading, of
// the water putting it out and growing trees, and of fireballs setting a building alight

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
/// The middle of the testbed's lake, from the middle of the map
constexpr glm::vec2 k_Lake {0.0f, 220.0f};
/// Where the blasts land on dry land
constexpr glm::vec2 k_Ground {0.0f, 40.0f};

/// A hut with a few of its people about it, some trees and a statue round a point
std::vector<ObjectSetup> Village(glm::vec2 at)
{
	return {
	    {.type = AbodeInfo::NorseHut, .offset = at + glm::vec2(6.0f, 4.0f), .yawDegrees = 200.0f},
	    {.type = TreeInfo::Oak, .offset = at + glm::vec2(-7.0f, 3.0f)},
	    {.type = TreeInfo::Beech, .offset = at + glm::vec2(-3.0f, -8.0f)},
	    {.type = TreeInfo::Conifer, .offset = at + glm::vec2(10.0f, -6.0f)},
	    {.type = TreeInfo::Pine, .offset = at + glm::vec2(-12.0f, 9.0f)},
	    {.type = FeatureInfo::AztcStatue, .offset = at + glm::vec2(2.0f, 10.0f)},
	    {.type = VillagerInfo::NorseFarmerMale, .offset = at + glm::vec2(3.0f, -3.0f), .joinTown = true},
	    {.type = VillagerInfo::NorseHousewifeFemale, .offset = at + glm::vec2(-4.0f, 2.0f), .joinTown = true},
	    {.type = VillagerInfo::NorseForesterMale, .offset = at + glm::vec2(14.0f, 12.0f), .joinTown = true},
	};
}

/// A forest of a few rows of trees, a few metres apart
std::vector<ObjectSetup> Forest(glm::vec2 at, int rows, int columns, float spacing)
{
	constexpr std::array k_Kinds {TreeInfo::Oak, TreeInfo::Beech, TreeInfo::Conifer, TreeInfo::Birch, TreeInfo::Pine};
	std::vector<ObjectSetup> trees;
	for (int z = 0; z < rows; ++z)
	{
		for (int x = 0; x < columns; ++x)
		{
			const auto kind = k_Kinds.at(static_cast<size_t>(x + z) % k_Kinds.size());
			trees.push_back({.type = kind,
			                 .offset = at + glm::vec2(static_cast<float>(x) - static_cast<float>(columns - 1) * 0.5f,
			                                          static_cast<float>(z)) *
			                                    spacing,
			                 .yawDegrees = static_cast<float>((x * 37 + z * 71) % 360)});
		}
	}
	return trees;
}

void AddBlasts(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.blast",
	    .name = "Blast in a village",
	    .facet = Facet::Miracles,
	    .description = "A blast cast by hand at the land by a hut, trees, a statue and three villagers, every 20 seconds.",
	    .expected =
	        "A white beam drops from high above onto the point in 0.4 s; it lands with a bang, the camera shakes "
	        "for 0.7 s and three cones spread across the land while a soft light flashes on it. Rubble (a heap of "
	        "roots) lies at the centre with a puff of brown dust, rock pieces fly out, and the wave spreading at "
	        "20 m/s shatters the trees, the statue and the hut one a step: the trees and the statue break into pieces and "
	        "go; the hut breaks into pieces too but stays, with no life left. The villagers are not shattered, but "
	        "the heat sets the nearest alight and they run burning. Smoke rises from 1.2 s for 4 s. The effect "
	        "closes at 6 s.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {k_Ground - glm::vec2(25.0f), k_Ground + glm::vec2(25.0f)}},
	    .objects = Village(k_Ground),
	    .miracles = {{.type = MagicType::ExplosionOne,
	                  .point = k_Ground,
	                  .handOffset = k_Ground,
	                  .handHeight = 15.0f,
	                  .delaySeconds = 2.0f,
	                  .repeatSeconds = 20.0f,
	                  .byHand = true}},
	});

	all.push_back({
	    .id = "miracles.blast_pu1",
	    .name = "Blast, first power-up",
	    .facet = Facet::Miracles,
	    .description = "The first power-up's blast at the land by a village: the centre and six more round it.",
	    .expected = "The centre blast as the plain one does, then six more, one every third of a second from 0.9 s, each "
	                "30 to 50 m out at random, each with its own beam, bang, shake, cones, rubble, smoke and wave. All "
	                "stop at 6 s, cutting the later ones' heat short.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview,
	                .include = {k_Ground - glm::vec2(55.0f), k_Ground + glm::vec2(55.0f)},
	                .distance = 1.2f},
	    .objects = Village(k_Ground),
	    .miracles = {{.type = MagicType::ExplosionOnePuOne,
	                  .point = k_Ground,
	                  .handOffset = k_Ground,
	                  .handHeight = 15.0f,
	                  .delaySeconds = 2.0f,
	                  .repeatSeconds = 20.0f}},
	});

	all.push_back({
	    .id = "miracles.blast_pu2",
	    .name = "Blast, extreme",
	    .facet = Facet::Miracles,
	    .description = "The extreme blast by a village: the centre with twice the reach, six more at 25 to 40 m, then a "
	                   "barrage of about 28 at 20 to 60 m from 6.9 s.",
	    .expected = "The centre blast, heat 800 over 10 m; six more at random intervals of half a second to a second from "
	                "0.9 s; then from 6.9 s blasts every quarter second on average all round, until the effect closes at "
	                "13.9 s. The late ones show only their beams.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview,
	                .include = {k_Ground - glm::vec2(65.0f), k_Ground + glm::vec2(65.0f)},
	                .distance = 1.3f},
	    .objects = Village(k_Ground),
	    .miracles = {{.type = MagicType::ExplosionOnePuTwo,
	                  .point = k_Ground,
	                  .handOffset = k_Ground,
	                  .handHeight = 15.0f,
	                  .delaySeconds = 2.0f,
	                  .repeatSeconds = 25.0f}},
	});

	all.push_back({
	    .id = "miracles.blast_water",
	    .name = "Blast on the water",
	    .facet = Facet::Miracles,
	    .description = "A blast at the middle of the testbed's lake, every 15 seconds.",
	    .expected = "The beam drops onto the water; instead of rubble three rings spread on the water to 5, 7 and 10 m "
	                "in 0.7 s, rock pieces fly up and fall into the water without rings or sounds, and white steam rises "
	                "from 1.2 s for 4 s.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {k_Lake - glm::vec2(25.0f), k_Lake + glm::vec2(25.0f)}},
	    .miracles = {{.type = MagicType::ExplosionOne,
	                  .point = k_Lake,
	                  .handOffset = k_Lake,
	                  .handHeight = 15.0f,
	                  .delaySeconds = 2.0f,
	                  .repeatSeconds = 15.0f}},
	});
}

void AddFire(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.forest_fire",
	    .name = "Fire spreading through a forest",
	    .facet = Facet::Miracles,
	    .description = "A fireball thrown into the front row of a forest of 35 trees, 6 m apart.",
	    .expected = "The trees the ball passes catch; flames lick at their lower branches, and over the next minutes the "
	                "fire spreads tree to tree through the forest as one blaze, the land lit and crackling. Trees that "
	                "burn down char grey and go; the fire dies out where nothing is left to burn.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 25.0f}, {20.0f, 70.0f}}},
	    .objects = Forest({0.0f, 30.0f}, 7, 5, 6.0f),
	    .miracles = {{.type = MagicType::Fireball,
	                  .point = {0.0f, 30.0f},
	                  .handOffset = {0.0f, -5.0f},
	                  .handHeight = 10.0f,
	                  .throwVelocity = {0.0f, 4.0f, 30.0f},
	                  .delaySeconds = 2.0f}},
	});

	all.push_back({
	    .id = "miracles.fireball_building",
	    .name = "Fireball at a hut",
	    .facet = Facet::Miracles,
	    .description = "A fireball thrown at a hut with its people round it.",
	    .expected = "The ball rolls into the hut, which catches and burns with flames over its walls, lighting the land; "
	                "its people come out. The villagers nearby react: those of its town come to beat the fire out, the "
	                "rest keep their distance; a villager the fire reaches runs about burning. The hut chars as its life "
	                "runs down.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-15.0f, 25.0f}, {25.0f, 55.0f}}},
	    .objects = Village({0.0f, 40.0f}),
	    .miracles = {{.type = MagicType::Fireball,
	                  .point = {6.0f, 44.0f},
	                  .handOffset = {6.0f, 34.0f},
	                  .handHeight = 6.0f,
	                  .throwVelocity = {0.0f, 0.0f, 8.0f},
	                  .delaySeconds = 2.0f}},
	});

	all.push_back({
	    .id = "miracles.fireball_water",
	    .name = "Fireballs onto the lake",
	    .facet = Facet::Miracles,
	    .description = "The fireball and its two power-ups thrown onto the testbed's lake, one after the other.",
	    .expected = "Each ball makes its splash sound as it meets the bed of the water, leaves a ring and dies at once; "
	                "no cloud of steam rises, and its own fire, quenched below steam's heat in one turn, doesn't sizzle. "
	                "The first power-up throws three balls, the second eight.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {k_Lake - glm::vec2(30.0f, 60.0f), k_Lake + glm::vec2(30.0f)}},
	    .miracles = {{.type = MagicType::Fireball,
	                  .point = k_Lake,
	                  .handOffset = k_Lake - glm::vec2(0.0f, 40.0f),
	                  .handHeight = 10.0f,
	                  .throwVelocity = {0.0f, 2.0f, 40.0f},
	                  .delaySeconds = 1.0f},
	                 {.type = MagicType::FireballPowerUpOne,
	                  .point = k_Lake,
	                  .handOffset = k_Lake - glm::vec2(0.0f, 40.0f),
	                  .handHeight = 10.0f,
	                  .throwVelocity = {0.0f, 2.0f, 40.0f},
	                  .delaySeconds = 5.0f},
	                 {.type = MagicType::FireballPowerUpTwo,
	                  .point = k_Lake,
	                  .handOffset = k_Lake - glm::vec2(0.0f, 40.0f),
	                  .handHeight = 10.0f,
	                  .throwVelocity = {0.0f, 2.0f, 40.0f},
	                  .delaySeconds = 9.0f}},
	});
}

void AddWater(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.water_trees",
	    .name = "Water on young and full grown trees",
	    .facet = Facet::Miracles,
	    .description = "The water held over three young trees on the left, then the extreme water over three full grown "
	                   "ones on the right, each for eight seconds.",
	    .expected = "The rain cloud hangs over each group with its cone of rain for 6 s, the water's time, and its rain "
	                "loop plays while it pours and fades away after. Its drops leave coloured rings. The young trees, still "
	                "growing to their full size, grow a little with each drop near them, "
	                "rustling; the extreme water grows even the full grown trees, past their full size, ever more slowly.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-25.0f, 30.0f}, {25.0f, 50.0f}}},
	    .objects = {{.type = TreeInfo::Oak, .offset = {-18.0f, 38.0f}, .scale = 0.3f, .fullSize = 1.0f},
	                {.type = TreeInfo::Beech, .offset = {-15.0f, 42.0f}, .scale = 0.3f, .fullSize = 1.0f},
	                {.type = TreeInfo::Birch, .offset = {-12.0f, 38.0f}, .scale = 0.3f, .fullSize = 1.0f},
	                {.type = TreeInfo::Oak, .offset = {12.0f, 38.0f}},
	                {.type = TreeInfo::Beech, .offset = {15.0f, 42.0f}},
	                {.type = TreeInfo::Birch, .offset = {18.0f, 38.0f}}},
	    .miracles = {{.type = MagicType::Water,
	                  .point = {-15.0f, 40.0f},
	                  .handOffset = {-15.0f, 40.0f},
	                  .handHeight = 14.0f,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = 8.0f},
	                 {.type = MagicType::WaterPowerUpOne,
	                  .point = {15.0f, 40.0f},
	                  .handOffset = {15.0f, 40.0f},
	                  .handHeight = 14.0f,
	                  .delaySeconds = 11.0f,
	                  .holdSeconds = 8.0f}},
	});
}
} // namespace

void testbed_scenarios::AddBlastFireScenarios(std::vector<Scenario>& all)
{
	AddBlasts(all);
	AddFire(all);
	AddWater(all);
}

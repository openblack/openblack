/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of the tornado, the storm's second power-up: one through a village and a wood, and one that
// meets a creature, both cast through the hand at a drawn circle

#include <array>
#include <vector>

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
/// Where the tornado is cast, north of the camera
constexpr glm::vec2 k_Centre {0.0f, 60.0f};
/// The circle drawn for it: a storm of this radius makes a tornado about two thirds of it tall
constexpr float k_CircleRadius = 60.0f;

std::vector<ObjectSetup> VillageAndWood()
{
	std::vector<ObjectSetup> objects;
	// A huddle of villagers about some piles of food and wood at the middle
	constexpr std::array k_Villagers {
	    glm::vec2 {-6.0f, 56.0f}, glm::vec2 {-2.0f, 62.0f}, glm::vec2 {-8.0f, 66.0f},
	    glm::vec2 {4.0f, 52.0f},  glm::vec2 {6.0f, 64.0f},  glm::vec2 {2.0f, 58.0f},
	};
	for (size_t i = 0; i < k_Villagers.size(); ++i)
	{
		objects.push_back({.type = i % 2 == 0 ? VillagerInfo::CelticFarmerMale : VillagerInfo::CelticForesterMale,
		                   .offset = k_Villagers.at(i)});
	}
	objects.push_back({.type = PotInfo::FoodPile, .offset = {8.0f, 58.0f}, .amount = 2000});
	objects.push_back({.type = PotInfo::WoodPile_1, .offset = {-10.0f, 60.0f}, .amount = 2000});
	// A wood to the north, where the storm drifts the way it was thrown
	constexpr std::array k_Trees {
	    glm::vec2 {-8.0f, 92.0f}, glm::vec2 {-2.0f, 98.0f}, glm::vec2 {4.0f, 90.0f},  glm::vec2 {-4.0f, 106.0f},
	    glm::vec2 {6.0f, 102.0f}, glm::vec2 {2.0f, 112.0f}, glm::vec2 {10.0f, 96.0f}, glm::vec2 {-10.0f, 100.0f},
	};
	for (size_t i = 0; i < k_Trees.size(); ++i)
	{
		objects.push_back(
		    {.type = i % 3 == 0 ? TreeInfo::Oak : (i % 3 == 1 ? TreeInfo::Beech : TreeInfo::Conifer), .offset = k_Trees.at(i)});
	}
	return objects;
}
} // namespace

void testbed_scenarios::AddTornadoScenarios(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.tornado_village",
	    .name = "Tornado through a village and a wood",
	    .facet = Facet::Miracles,
	    .description = "A storm seed powered up twice to a tornado, let go over a circle of 60 drawn round a huddle of "
	                   "villagers with piles of food and wood, with a wood of eight trees to the north.",
	    .expected = "Storm clouds gather and a grey funnel of two spinning shells fades in over eight seconds, its foot "
	                "wandering slowly under the clouds while a loud tornado roar loops. Dust the colour of the land is "
	                "thrown up round its foot and bushes and chickens whirl about it. Once faded in, every few tenths of a "
	                "second it picks up the first thing it reaches, looking cell by cell out from its foot: villagers, the "
	                "dead among them, and trees are lifted whole, the piles give up pots of food and wood such as the hand "
	                "drops. They spin round the funnel, rise and are flung out near its top; the villagers die where they "
	                "fall, falling and lying dead, the rest is gone (most are let go mid-air after fifteen seconds, before "
	                "they reach the top). The funnel's wiggle follows its bend, so its lower part sways less. The storm drifts "
	                "north the way it was thrown, taking the funnel into the wood, whose "
	                "trees go up the same way. The miracle's prayer power drains ten for each thing it "
	                "touches. When the storm ends the funnel fades over three seconds, what it carried falls and its roar "
	                "dies away.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-25.0f, 40.0f}, {25.0f, 120.0f}}},
	    .objects = VillageAndWood(),
	    .miracles = {{.type = MagicType::Tornado,
	                  .point = k_Centre,
	                  .handOffset = {0.0f, 20.0f},
	                  .delaySeconds = 1.0f,
	                  .byHand = true,
	                  .circleRadius = k_CircleRadius}},
	});

	all.push_back({
	    .id = "miracles.tornado_creature",
	    .name = "Tornado meets a creature",
	    .facet = Facet::Miracles,
	    .description = "A tornado cast over a creature at a circle of 60, through the hand with a storm seed powered up "
	                   "twice.",
	    .expected = "Once the funnel has faded in and reaches the creature, the creature faints where it is, eyes closed, "
	                "and is let off the leash; it is too big to be picked up and is never lifted. It lies four seconds for "
	                "each of its size and eight more, is taken home, waits three seconds and for any spells on it to wear "
	                "off, rests if it was tired or hurt when it fainted, and gets up. The tornado's foot keeps battering "
	                "whatever stands at it.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-40.0f, 20.0f}, {40.0f, 110.0f}}, .distance = 1.2f},
	    .creatures = {{.label = "caught",
	                   .species = CreatureType::Tiger,
	                   .offset = k_Centre,
	                   .facingDegrees = 0.0f,
	                   .needs = {.energy = 1.0f, .life = 1.0f}}},
	    .miracles = {{.type = MagicType::Tornado,
	                  .point = k_Centre,
	                  .handOffset = {0.0f, 20.0f},
	                  .delaySeconds = 1.0f,
	                  .byHand = true,
	                  .circleRadius = k_CircleRadius}},
	});

	all.push_back({
	    .id = "miracles.tornado_hand_pile",
	    .name = "Tornado over piles the hand dropped",
	    .facet = Facet::Miracles,
	    .description = "A tornado cast at a circle of 60 over a pile of food and a pile of wood such as the hand drops, "
	                   "beside a food pile and a wood pile of a store.",
	    .expected = "Once the funnel has faded in, the hand's piles are lifted whole when they fit its funnel, rather than "
	                "giving up pots; the store's food and wood piles give up pots of food and wood such as the hand drops, "
	                "so much of each as the tornado's size takes, rounded to the nearest.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-25.0f, 40.0f}, {25.0f, 80.0f}}},
	    .objects = {{.type = PotInfo::HandFood, .offset = {-4.0f, 58.0f}, .amount = 300},
	                {.type = PotInfo::HandWood, .offset = {4.0f, 62.0f}, .amount = 300},
	                {.type = PotInfo::FoodPile, .offset = {10.0f, 56.0f}, .amount = 2000},
	                {.type = PotInfo::WoodPile_1, .offset = {-10.0f, 64.0f}, .amount = 2000}},
	    .miracles = {{.type = MagicType::Tornado,
	                  .point = k_Centre,
	                  .handOffset = {0.0f, 20.0f},
	                  .delaySeconds = 1.0f,
	                  .byHand = true,
	                  .circleRadius = k_CircleRadius}},
	});

	all.push_back({
	    .id = "miracles.tornado_animals",
	    .name = "Tornado over a herd",
	    .facet = Facet::Miracles,
	    .description = "A tornado cast at a circle of 60 over two cows, two sheep and a pig.",
	    .expected = "Once the funnel has faded in, the animals are picked up whole and whirled round it. Each it lets go "
	                "is put down where it was flung with no life left, falls dead in its kind's dying clip and lies dead, "
	                "a cow, sheep or pig on its left side; a minute later the body is gone.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-25.0f, 40.0f}, {25.0f, 90.0f}}},
	    .objects = {{.type = AnimalInfo::Cow, .offset = {-4.0f, 58.0f}},
	                {.type = AnimalInfo::Cow, .offset = {4.0f, 62.0f}},
	                {.type = AnimalInfo::Sheep, .offset = {-8.0f, 64.0f}},
	                {.type = AnimalInfo::Sheep, .offset = {8.0f, 56.0f}},
	                {.type = AnimalInfo::Pig, .offset = {0.0f, 66.0f}}},
	    .miracles = {{.type = MagicType::Tornado,
	                  .point = k_Centre,
	                  .handOffset = {0.0f, 20.0f},
	                  .delaySeconds = 1.0f,
	                  .byHand = true,
	                  .circleRadius = k_CircleRadius}},
	});
}

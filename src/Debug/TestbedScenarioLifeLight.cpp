/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of the heal, the lightning bolt, the creature spells and how the living react to miracles,
// every level of each, all cast through the hand as a player casts them

#include <array>
#include <string_view>
#include <vector>

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
using Target = MiracleCast::Target;

/// The heal is cast in the middle of a cell of the land, so that which cells it looks in shows
constexpr glm::vec2 k_HealPoint {5.0f, 35.0f};

CreatureSetup Hurt(glm::vec2 offset, std::string_view label, float life)
{
	return {.label = label,
	        .species = CreatureType::Tiger,
	        .offset = offset,
	        .facingDegrees = 0.0f,
	        .needs = {.energy = 1.0f, .exhaustion = 0.0f, .dehydration = 0.0f, .poo = 0.0f, .life = life}};
}

ObjectSetup HurtVillager(VillagerInfo type, glm::vec2 offset, float life, bool poisoned = false)
{
	return {.type = type, .offset = offset, .life = life, .poisoned = poisoned};
}

ObjectSetup HurtAnimal(AnimalInfo type, glm::vec2 offset, float life)
{
	return {.type = type, .offset = offset, .life = life};
}

void AddHeal(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.heal_crowd",
	    .name = "Heal: villagers, animals, the dead and the poisoned",
	    .facet = Facet::Miracles,
	    .description = "A heal pressed by hand in the middle of a cell, over hurt villagers, a hurt cow and sheep, a "
	                   "poisoned villager and a dead one; one more hurt villager stands seven metres away in the cell to "
	                   "the east, which the plain heal's four cells don't take in, and another fourteen metres south, in "
	                   "the cell to the south.",
	    .expected = "The heal looks for the living within 10 m of the cast point moved into each of its four cells (the "
	                "cast cell, west, south-west and south) in turn, so the villager fourteen metres south, under 6 m from "
	                "the point moved a cell south, is healed too. A chakra lights over each one healed, sparks rising "
	                "under it, and each glows pale cyan-white, brightest after a second and a half and gone by three; their "
	                "lives fill up and the poisoned villager is cured. One heal sound plays. The dead villager and the one "
	                "in the eastern cell are left alone.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-6.0f, 19.0f}, {14.0f, 40.0f}}, .distance = 0.35f},
	    .objects = {HurtVillager(VillagerInfo::CelticFarmerMale, {3.0f, 33.0f}, 0.3f),
	                HurtVillager(VillagerInfo::CelticForesterMale, {-3.0f, 36.0f}, 0.2f),
	                HurtVillager(VillagerInfo::CelticFarmerMale, {2.0f, 28.0f}, 0.4f, true),
	                HurtVillager(VillagerInfo::CelticForesterMale, {4.0f, 38.0f}, 0.0f),
	                HurtVillager(VillagerInfo::CelticFarmerMale, {12.0f, 35.0f}, 0.3f),
	                HurtVillager(VillagerInfo::CelticForesterMale, {9.0f, 21.0f}, 0.3f),
	                HurtAnimal(AnimalInfo::Cow, {-3.0f, 30.0f}, 0.25f), HurtAnimal(AnimalInfo::Sheep, {6.0f, 27.0f}, 0.5f)},
	    .miracles = {{.type = MagicType::Heal,
	                  .point = k_HealPoint,
	                  .handOffset = k_HealPoint,
	                  .handHeight = 12.0f,
	                  .delaySeconds = 1.0f,
	                  .repeatSeconds = 20.0f,
	                  .byHand = true}},
	});

	all.push_back({
	    .id = "miracles.heal_powerup_crowd",
	    .name = "Heal power-up: the mushroom over a wide crowd",
	    .facet = Facet::Miracles,
	    .description = "The heal's power-up pressed by hand over hurt villagers and animals spread up to thirty metres "
	                   "round it.",
	    .expected = "A glowing mushroom grows from the point, sliding its texture round, to full height by two seconds "
	                "while seven stars shoot up its stem to about 37 metres; its sound plays. After three and a half "
	                "seconds chakras light over everyone hurt in the seven by seven cells round it, all of them healed and "
	                "glowing; the "
	                "mushroom swells and fades by six seconds and the effect is gone by eight.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-30.0f, 5.0f}, {40.0f, 65.0f}}},
	    .objects = {HurtVillager(VillagerInfo::CelticFarmerMale, {-20.0f, 30.0f}, 0.2f),
	                HurtVillager(VillagerInfo::CelticForesterMale, {25.0f, 40.0f}, 0.3f),
	                HurtVillager(VillagerInfo::CelticFarmerMale, {5.0f, 62.0f}, 0.3f),
	                HurtVillager(VillagerInfo::CelticForesterMale, {5.0f, 10.0f}, 0.1f),
	                HurtAnimal(AnimalInfo::Horse, {-15.0f, 50.0f}, 0.2f), HurtAnimal(AnimalInfo::Pig, {30.0f, 20.0f}, 0.4f)},
	    .miracles = {{.type = MagicType::HealPowerUpOne,
	                  .point = k_HealPoint,
	                  .handOffset = k_HealPoint,
	                  .handHeight = 14.0f,
	                  .delaySeconds = 1.0f,
	                  .repeatSeconds = 20.0f,
	                  .byHand = true}},
	});

	all.push_back({
	    .id = "miracles.heal_creatures",
	    .name = "Heal: creatures at each level",
	    .facet = Facet::Miracles,
	    .description = "Two hurt creatures: the plain heal pressed by hand at the feet of the left one, and the "
	                   "power-up at the right one eight seconds later.",
	    .expected = "The left creature's chakra lights over it at once, it glows and its life fills up, its cuts and "
	                "scars mending; the right one's comes after the power-up's mushroom, three and a half seconds on.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 30.0f}, {20.0f, 40.0f}}, .distance = 0.8f},
	    .creatures = {Hurt({-15.0f, 35.0f}, "plain heal", 0.3f), Hurt({15.0f, 35.0f}, "power-up", 0.2f)},
	    .miracles = {{.type = MagicType::Heal,
	                  .target = Target::Creature,
	                  .creature = 0,
	                  .handOffset = {-15.0f, 30.0f},
	                  .delaySeconds = 1.0f,
	                  .repeatSeconds = 25.0f,
	                  .byHand = true},
	                 {.type = MagicType::HealPowerUpOne,
	                  .target = Target::Creature,
	                  .creature = 1,
	                  .handOffset = {15.0f, 30.0f},
	                  .delaySeconds = 9.0f,
	                  .repeatSeconds = 25.0f,
	                  .byHand = true}},
	});
}

/// A crowd of villagers with a tree, a pillar of rock and a barrel among them, north of the hand
std::vector<ObjectSetup> StrikeCrowd()
{
	std::vector<ObjectSetup> crowd;
	constexpr std::array k_Villagers {VillagerInfo::CelticFarmerMale, VillagerInfo::CelticForesterMale};
	for (int row = 0; row < 3; ++row)
	{
		for (int column = 0; column < 4; ++column)
		{
			crowd.push_back({.type = k_Villagers.at(static_cast<size_t>((row + column) % 2)),
			                 .offset = {-9.0f + 6.0f * static_cast<float>(column), 32.0f + 5.0f * static_cast<float>(row)}});
		}
	}
	crowd.push_back({.type = TreeInfo::Oak, .offset = {-6.0f, 48.0f}});
	crowd.push_back({.type = FeatureInfo::FatPilarChalk, .offset = {6.0f, 47.0f}, .scale = 0.5f});
	crowd.push_back({.type = MobileObjectInfo::EgyptBarrel, .offset = {1.0f, 44.0f}});
	return crowd;
}

void AddLightning(std::vector<Scenario>& all)
{
	struct Level
	{
		std::string_view id;
		std::string_view name;
		MagicType type;
		std::string_view expected;
	};
	constexpr std::array k_Levels {
	    Level {"miracles.lightning_crowd", "Lightning: a crowd", MagicType::LightningBolt,
	           "A narrow cone of thin forked bolts, up to four at a time, crackles from the hand to the villagers, the tree, "
	           "the rock and the barrel ahead and to the ground about them, lighting the land where they strike. Each "
	           "strike hurts everyone within reach of where it lands, and the villagers flee it. The tree, the rock and "
	           "the barrel struck are left with electric arcs crawling over them for three or four seconds, fresh arcs "
	           "about every second while the bolt keeps striking them (the debug log names each). The crackle fades out "
	           "within a second of letting go; there is no thunder."},
	    Level {"miracles.lightning_crowd_pu1", "Lightning power-up one: a crowd", MagicType::LightningBoltPowerUpOne,
	           "A wider cone of thicker bolts, up to ten at a time over as many as twelve targets, with the crackle of the "
	           "second level; otherwise as the plain bolt."},
	    Level {"miracles.lightning_crowd_pu2", "Lightning power-up two: a crowd", MagicType::LightningBoltPowerUpTwo,
	           "The widest cone of the thickest bolts, up to twenty at a time over as many as 28 targets, the hand's glow "
	           "biggest and the crackle of the third level; otherwise as the plain bolt."},
	};
	for (const auto& level : k_Levels)
	{
		all.push_back({
		    .id = level.id,
		    .name = level.name,
		    .facet = Facet::Miracles,
		    .description = "Lightning held by hand for four seconds in every ten from the south, looking north over twelve "
		                   "villagers with a tree, a pillar of rock and a barrel among them.",
		    .expected = level.expected,
		    .environment = {.dispenserGrid = false},
		    .framing = {.shot = Shot::Overview, .include = {{-12.0f, 18.0f}, {12.0f, 50.0f}}, .distance = 0.6f},
		    .objects = StrikeCrowd(),
		    .miracles = {{.type = level.type,
		                  .point = {0.0f, 40.0f},
		                  .handOffset = {0.0f, 22.0f},
		                  .handHeight = 12.0f,
		                  .delaySeconds = 1.0f,
		                  .holdSeconds = 4.0f,
		                  .repeatSeconds = 10.0f,
		                  .byHand = true}},
		});
	}

	all.push_back({
	    .id = "miracles.lightning_arcs",
	    .name = "Lightning: arcs over what it struck",
	    .facet = Facet::Miracles,
	    .description = "Lightning held by hand for a second and a half at a pillar of rock and a tree, every eight seconds.",
	    .expected = "Struck more than a fifth of a second after the bolt last looked for its targets, the rock and the "
	                "tree are left with electric arcs: ribbons of lightning jumping between points of their models, "
	                "writhing every step and leaning away from the map's corner, for three or four seconds after the bolt "
	                "has stopped. Each search, about every second, lets a struck one take fresh arcs.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-6.0f, 34.0f}, {6.0f, 46.0f}}, .distance = 0.4f},
	    .objects = {{.type = FeatureInfo::FatPilarChalk, .offset = {2.0f, 40.0f}, .scale = 0.25f},
	                {.type = TreeInfo::Beech, .offset = {-3.0f, 41.0f}}},
	    .miracles = {{.type = MagicType::LightningBoltPowerUpOne,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = {0.0f, 25.0f},
	                  .handHeight = 12.0f,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = 1.5f,
	                  .repeatSeconds = 8.0f,
	                  .byHand = true}},
	});

	all.push_back({
	    .id = "miracles.lightning_draws_to_creature",
	    .name = "Lightning: a creature takes every fork",
	    .facet = Facet::Miracles,
	    .description = "The second power-up held by hand over villagers with a creature standing among them, then over the "
	                   "same with the creature made invisible.",
	    .expected = "While the creature is in the cone every fork goes to it and none to the villagers. Once it has faded "
	                "invisible (half way or more) the forks spread over the villagers and the ground again.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-12.0f, 18.0f}, {12.0f, 50.0f}}, .distance = 0.7f},
	    .creatures = {Hurt({0.0f, 46.0f}, "struck", 1.0f)},
	    .objects = StrikeCrowd(),
	    .miracles =
	        {{.type = MagicType::LightningBoltPowerUpTwo,
	          .point = {0.0f, 40.0f},
	          .handOffset = {0.0f, 22.0f},
	          .handHeight = 12.0f,
	          .delaySeconds = 1.0f,
	          .holdSeconds = 3.0f,
	          .byHand = true},
	         {.type = MagicType::CreatureSpellInvisible, .target = Target::Creature, .creature = 0, .delaySeconds = 5.0f},
	         {.type = MagicType::LightningBoltPowerUpTwo,
	          .point = {0.0f, 40.0f},
	          .handOffset = {0.0f, 22.0f},
	          .handHeight = 12.0f,
	          .delaySeconds = 16.0f,
	          .holdSeconds = 3.0f,
	          .byHand = true}},
	});

	all.push_back({
	    .id = "miracles.lightning_clash",
	    .name = "Lightning: two bolts clash",
	    .facet = Facet::Miracles,
	    .description = "Two bolts from hands west and east of a creature, both looking north at it, the eastern one cast a "
	                   "second after the western.",
	    .expected = "Once both strike, the newer eastern bolt's trunk stops where the two meet, a glow there; the older "
	                "western bolt runs to that point and on from it to one target three times as thick and fully opaque, "
	                "each strike twice as hard and paid for by both miracles.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-25.0f, 10.0f}, {25.0f, 50.0f}}, .distance = 0.8f},
	    .creatures = {Hurt({0.0f, 45.0f}, "between them", 1.0f)},
	    .miracles = {{.type = MagicType::LightningBolt,
	                  .target = Target::Creature,
	                  .creature = 0,
	                  .handOffset = {-15.0f, 15.0f},
	                  .handHeight = 12.0f,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = 6.0f,
	                  .repeatSeconds = 10.0f},
	                 {.type = MagicType::LightningBolt,
	                  .target = Target::Creature,
	                  .creature = 0,
	                  .handOffset = {15.0f, 15.0f},
	                  .handHeight = 12.0f,
	                  .delaySeconds = 2.0f,
	                  .holdSeconds = 5.0f,
	                  .repeatSeconds = 10.0f}},
	});
}

void AddReactions(std::vector<Scenario>& all)
{
	std::vector<ObjectSetup> town;
	constexpr std::array k_Villagers {VillagerInfo::CelticFarmerMale, VillagerInfo::CelticForesterMale,
	                                  VillagerInfo::CelticHousewifeFemale};
	for (int i = 0; i < 9; ++i)
	{
		town.push_back({.type = k_Villagers.at(static_cast<size_t>(i % 3)),
		                .offset = {-16.0f + 4.0f * static_cast<float>(i), 30.0f + 3.0f * static_cast<float>(i % 3)}});
	}
	all.push_back({
	    .id = "miracles.reactions_nice",
	    .name = "Reactions: villagers impressed by a nice miracle",
	    .facet = Facet::Miracles,
	    .description = "The heal's power-up pressed by hand in front of nine villagers and a creature.",
	    .expected = "Every villager within 60 metres stops, stands and turns to face the miracle for eight or nine "
	                "seconds, then goes back to what it was doing. A symbol of belief rises from each in the player's "
	                "colour, wiggling and pulsing, fading out after a second and a half, with its sound; now and then a good "
	                "villager voice of awe is heard near the hand. The creature goes to look at it and points at it.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 20.0f}, {20.0f, 50.0f}}, .distance = 0.7f},
	    .creatures = {Hurt({15.0f, 20.0f}, "curious", 1.0f)},
	    .objects = town,
	    .miracles = {{.type = MagicType::HealPowerUpOne,
	                  .point = {0.0f, 45.0f},
	                  .handOffset = {0.0f, 45.0f},
	                  .handHeight = 14.0f,
	                  .delaySeconds = 1.0f,
	                  .byHand = true}},
	});
	all.push_back({
	    .id = "miracles.reactions_flee",
	    .name = "Reactions: villagers flee a frightening miracle",
	    .facet = Facet::Miracles,
	    .description = "Lightning held by hand for four seconds over nine villagers and a creature.",
	    .expected = "The villagers within 50 metres run ten metres straight away from where the bolt strikes at their "
	                "fleeing speed, then turn to watch it from fifty metres or more, and go back to what they were doing "
	                "once their eight seconds are up. An evil voice of awe may be heard. The creature is frightened: it "
	                "may start back, then runs away.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-30.0f, 0.0f}, {30.0f, 70.0f}}, .distance = 0.8f},
	    .creatures = {Hurt({15.0f, 30.0f}, "frightened", 1.0f)},
	    .objects = town,
	    .miracles = {{.type = MagicType::LightningBolt,
	                  .point = {0.0f, 35.0f},
	                  .handOffset = {0.0f, 15.0f},
	                  .handHeight = 12.0f,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = 4.0f,
	                  .byHand = true}},
	});
}
} // namespace

void testbed_scenarios::AddLifeLightScenarios(std::vector<Scenario>& all)
{
	AddHeal(all);
	AddLightning(all);
	AddReactions(all);
}

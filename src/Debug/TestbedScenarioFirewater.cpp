/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of what the blast spares and does on a coast, of the water over fields and forests and before
// the people watching it put out a fire, and of the hand catching a fireball or taking one into a fire seed

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
/// Where the blasts land on dry land
constexpr glm::vec2 k_Ground {0.0f, 40.0f};
/// A cell of the lake's northern bank whose corner is at sea level although the land's file marks no water in it: dry
/// by its water flag, wet by its altitude
constexpr glm::vec2 k_Coast {5.0f, 295.0f};
/// The land's forest the trees of the forest scenario belong to
constexpr uint32_t k_ScenarioForest = 1;

void AddBlasts(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.blast_spares",
	    .name = "What a blast spares",
	    .facet = Facet::Miracles,
	    .description = "A blast amid a teleport stone, a totem, a spell dispenser, a pot of food, a one-off spell seed and "
	                   "two trees.",
	    .expected = "The wave shatters the two trees, which go. The teleport stone, the totem, the pot and the one-off "
	                "seed's bubble are left as they were; the spell dispenser breaks into pieces but stays standing with "
	                "no life left.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview,
	                .include = {k_Ground - glm::vec2(20.0f), k_Ground + glm::vec2(20.0f)},
	                .distance = 0.6f},
	    .objects = {{.type = AbodeInfo::NorseTotem, .offset = k_Ground + glm::vec2(-8.0f, 4.0f)},
	                {.type = PotInfo::FoodPot, .offset = k_Ground + glm::vec2(4.0f, -6.0f)},
	                {.type = TreeInfo::Oak, .offset = k_Ground + glm::vec2(-4.0f, -8.0f)},
	                {.type = TreeInfo::Beech, .offset = k_Ground + glm::vec2(9.0f, -2.0f)}},
	    .dispensers = {{.type = MagicType::Heal, .offset = k_Ground + glm::vec2(6.0f, 6.0f)},
	                   {.type = MagicType::Food, .offset = k_Ground + glm::vec2(-6.0f, -2.0f), .bubbleHeight = 3.0f}},
	    .miracles = {{.type = MagicType::Teleport,
	                  .point = k_Ground + glm::vec2(0.0f, 8.0f),
	                  .handOffset = k_Ground + glm::vec2(0.0f, 8.0f),
	                  .delaySeconds = 1.0f},
	                 {.type = MagicType::ExplosionOne,
	                  .point = k_Ground,
	                  .handOffset = k_Ground,
	                  .handHeight = 15.0f,
	                  .delaySeconds = 3.0f}},
	});

	all.push_back({
	    .id = "miracles.blast_coast",
	    .name = "Blast on a coast",
	    .facet = Facet::Miracles,
	    .description = "A blast on the lake's northern bank, in a cell at sea level by its altitude that the land marks "
	                   "dry, every 15 seconds.",
	    .expected = "The land there counts as wet by its altitude: three rings spread to 5, 7 and 10 m and white steam "
	                "rises from 1.2 s, with no rubble and no brown smoke.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {k_Coast - glm::vec2(20.0f), k_Coast + glm::vec2(20.0f)}},
	    .miracles = {{.type = MagicType::ExplosionOne,
	                  .point = k_Coast,
	                  .handOffset = k_Coast,
	                  .handHeight = 15.0f,
	                  .delaySeconds = 2.0f,
	                  .repeatSeconds = 15.0f}},
	});
}

void AddWater(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.water_fields",
	    .name = "Water over fields",
	    .facet = Facet::Miracles,
	    .description = "The water held for eight seconds over a field, with a second field 13 m away.",
	    .expected = "The field under the rain is sown at once and ripens a little with each drop that falls within 7.5 m "
	                "of its middle (a field counts as 5 m wide); the far field is never reached and stays bare.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-15.0f, 25.0f}, {25.0f, 55.0f}}},
	    .objects = {{.type = FieldTypeInfo::Wheat, .offset = {0.0f, 40.0f}},
	                {.type = FieldTypeInfo::Corn, .offset = {13.0f, 40.0f}}},
	    .miracles = {{.type = MagicType::Water,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = {0.0f, 40.0f},
	                  .handHeight = 14.0f,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = 8.0f,
	                  .byHand = true}},
	});

	all.push_back({
	    .id = "miracles.water_forest",
	    .name = "Water over a forest",
	    .facet = Facet::Miracles,
	    .description = "The water held over a forest of full grown trees, three times, 12 seconds apart.",
	    .expected = "Each drop by a full grown tree of the forest may plant a young tree of its kind beside it, at a whole "
	                "number of metres from 0 to 9 away, on land free of anything fixed (in water too), but no sooner than "
	                "41 turns after the last tree any forest gained this way: at most one tree each four seconds. It "
	                "starts at a tenth of its size and grows to 0.8 to 1.2.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 25.0f}, {20.0f, 55.0f}}},
	    .objects = {{.type = TreeInfo::Oak, .offset = {-6.0f, 36.0f}, .forest = k_ScenarioForest},
	                {.type = TreeInfo::Beech, .offset = {0.0f, 44.0f}, .forest = k_ScenarioForest},
	                {.type = TreeInfo::Birch, .offset = {6.0f, 36.0f}, .forest = k_ScenarioForest}},
	    .miracles = {{.type = MagicType::Water,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = {0.0f, 40.0f},
	                  .handHeight = 14.0f,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = 8.0f,
	                  .repeatSeconds = 12.0f}},
	});

	all.push_back({
	    .id = "miracles.water_magic_forest",
	    .name = "Water by a forest miracle's trees",
	    .facet = Facet::Miracles,
	    .description = "A forest miracle cast and kept, then the water held over its trees every 12 seconds from 20 s.",
	    .expected = "Once a magic tree is full grown, the water plants plain young trees of its kind beside it, at most "
	                "one each four seconds; they join the miracle's forest, grow and wither with it, and go with it.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 20.0f}, {20.0f, 60.0f}}},
	    .miracles = {{.type = MagicType::Forest,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = {0.0f, 40.0f},
	                  .delaySeconds = 0.5f,
	                  .byHand = true},
	                 {.type = MagicType::Water,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = {0.0f, 40.0f},
	                  .handHeight = 14.0f,
	                  .delaySeconds = 20.0f,
	                  .holdSeconds = 8.0f,
	                  .repeatSeconds = 12.0f}},
	});

	all.push_back({
	    .id = "miracles.water_fire_watchers",
	    .name = "People watch the water put out a fire",
	    .facet = Facet::Miracles,
	    .description = "A fireball sets a hut alight among its people, then the water is held over it for eight seconds.",
	    .expected = "Once the rain falls on the burning hut the people nearby come to watch the water put it out (one "
	                "reaction for the miracle at a time); the hut's fire cools with each drop and goes out, steaming.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-15.0f, 25.0f}, {25.0f, 55.0f}}, .distance = 0.6f},
	    .objects = {{.type = AbodeInfo::NorseHut, .offset = {6.0f, 44.0f}, .yawDegrees = 200.0f},
	                {.type = VillagerInfo::NorseFarmerMale, .offset = {-6.0f, 34.0f}, .joinTown = true},
	                {.type = VillagerInfo::NorseHousewifeFemale, .offset = {-10.0f, 40.0f}, .joinTown = true},
	                {.type = VillagerInfo::NorseForesterMale, .offset = {16.0f, 30.0f}, .joinTown = true},
	                {.type = VillagerInfo::NorseFarmerMale, .offset = {-14.0f, 30.0f}}},
	    .miracles = {{.type = MagicType::Fireball,
	                  .point = {6.0f, 44.0f},
	                  .handOffset = {6.0f, 34.0f},
	                  .handHeight = 6.0f,
	                  .throwVelocity = {0.0f, 0.0f, 8.0f},
	                  .delaySeconds = 2.0f},
	                 {.type = MagicType::Water,
	                  .point = {6.0f, 44.0f},
	                  .handOffset = {6.0f, 44.0f},
	                  .handHeight = 14.0f,
	                  .delaySeconds = 6.0f,
	                  .holdSeconds = 8.0f}},
	});
}

void AddFireballCatching(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.fireball_catch",
	    .name = "Catching another player's fireball",
	    .facet = Facet::Miracles,
	    .description = "Another player throws a fireball towards the middle; the empty hand takes hold of it as it flies.",
	    .expected = "The ball is caught: it goes, and a fireball seed, charged and ready, is in the hand. (The player's "
	                "own ball would slip through, as would a ball faded to an alpha of 30 or less.)",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 15.0f}, {20.0f, 60.0f}}},
	    .miracles = {{.type = MagicType::Fireball,
	                  .point = {0.0f, 25.0f},
	                  .handOffset = {0.0f, 55.0f},
	                  .handHeight = 10.0f,
	                  .throwVelocity = {0.0f, 2.0f, -20.0f},
	                  .delaySeconds = 2.0f,
	                  .player = PlayerNames::PLAYER_TWO}},
	    .commands = {{.kind = Command::Kind::HandTakeFireBall, .delaySeconds = 2.6f}},
	});

	all.push_back({
	    .id = "miracles.fireball_absorb",
	    .name = "A fire seed takes in a fireball",
	    .facet = Facet::Miracles,
	    .description = "The hand holds a fire seed as another player's fireball flies past; the action button is pressed "
	                   "on the ball.",
	    .expected = "The ball goes into the seed, which stays in the hand a twentieth stronger (its power 1.05).",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 15.0f}, {20.0f, 60.0f}}},
	    .miracles = {{.type = MagicType::Fireball,
	                  .point = {0.0f, 25.0f},
	                  .handOffset = {0.0f, 55.0f},
	                  .handHeight = 10.0f,
	                  .throwVelocity = {0.0f, 2.0f, -20.0f},
	                  .delaySeconds = 2.0f,
	                  .player = PlayerNames::PLAYER_TWO}},
	    .commands = {{.kind = Command::Kind::HoldSeed, .delaySeconds = 1.0f, .value = static_cast<size_t>(SpellSeedType::Fire)},
	                 {.kind = Command::Kind::HandTakeFireBall, .delaySeconds = 1.6f}},
	});
}
} // namespace

void testbed_scenarios::AddFirewaterScenarios(std::vector<Scenario>& all)
{
	AddBlasts(all);
	AddWater(all);
	AddFireballCatching(all);
}

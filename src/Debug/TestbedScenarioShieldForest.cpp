/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of the shield and forest miracles: what each shield stops, how the circle sizes it, what keeping
// it up costs, the people sheltering under it and the creatures walking round it; the forest planted on grass, sand and
// snow, growing and withering away, and its flocks

#include <array>
#include <string_view>

#include "3D/FlatLand.h"
#include "Input/BindableActions.h"
#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
using Target = MiracleCast::Target;
using Kind = Command::Kind;

/// Where the shields go, north of the dispenser grid
constexpr glm::vec2 k_ShieldSpot {0.0f, 40.0f};
/// The patches of sand and snow, from the middle of the map
const glm::vec2 k_Sand = flat_land::k_SandPatch.Centre() - flat_land::k_MapMiddle;
const glm::vec2 k_Snow = flat_land::k_SnowPatch.Centre() - flat_land::k_MapMiddle;

CreatureSetup Thrower(glm::vec2 offset, float facing, PlayerNames owner = PlayerNames::PLAYER_ONE)
{
	return {.label = "thrower",
	        .species = CreatureType::Tiger,
	        .offset = offset,
	        .facingDegrees = facing,
	        .owner = owner,
	        .needs = {.energy = 1.0f, .exhaustion = 0.0f, .dehydration = 0.0f, .poo = 0.0f, .life = 1.0f},
	        .hold = true,
	        .pauseMind = true};
}

void AddShields(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.shield_physical_throw",
	    .name = "Physical shield: a thrown ball bounces off",
	    .facet = Facet::Miracles,
	    .description = "A physical shield of radius 30 cast north of the middle; a tiger to its west picks up a ball and "
	                   "throws it at the shield's middle, again and again.",
	    .expected = "Nothing shows for half a second, then the solid dome grows over a second and a half, spinning slowly "
	                "and bobbing, sunk a little into the land, with sparkles trailing round it and its hum. Its solid shape "
	                "stands at its full size from the moment it is cast. Each ball bounces off its faces; a ball costs the "
	                "shield nothing, as only a thrown rock does. The dome is drawn lit and whole; only its sparkles fade "
	                "with the shield's strength, never below 40 of 255.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-60.0f, 10.0f}, {40.0f, 80.0f}}},
	    .creatures = {Thrower({-60.0f, 40.0f}, 270.0f)},
	    .objects = {{.type = MobileObjectInfo::Ball, .offset = {-60.0f, 30.0f}}},
	    .miracles = {{.type = MagicType::PhysicalShield, .point = k_ShieldSpot, .delaySeconds = 0.5f}},
	    .commands =
	        {{.kind = Kind::PickUp, .creature = 0, .delaySeconds = 3.0f, .waitUntilFree = true, .object = 0},
	         {.kind = Kind::ThrowAt, .creature = 0, .delaySeconds = 0.5f, .waitUntilFree = true, .point = k_ShieldSpot}},
	    .repeatFrom = 0,
	});

	all.push_back({
	    .id = "miracles.shield_physical_fade",
	    .name = "Physical shield: raised and let go",
	    .facet = Facet::Miracles,
	    .description = "A physical shield of radius 30 cast north of the middle and closed down after eight seconds, "
	                   "every twelve.",
	    .expected = "The dome is hidden for half a second, grows over a second and a half on its ease, spins down to a "
	                "slow turn over six seconds and bobs gently. Closed down, it shrinks away over a second and a half and "
	                "is gone at two and a quarter; its sparkles stop at a second and a half, and its hum is let go as the "
	                "dome goes, playing on to the end of its pass.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-40.0f, 10.0f}, {40.0f, 80.0f}}},
	    .miracles = {{.type = MagicType::PhysicalShield,
	                  .point = k_ShieldSpot,
	                  .delaySeconds = 0.5f,
	                  .holdSeconds = 8.0f,
	                  .repeatSeconds = 12.0f}},
	});

	all.push_back({
	    .id = "miracles.shield_sizes",
	    .name = "Shields sized by the circle drawn",
	    .facet = Facet::Miracles,
	    .description = "Three spiritual shields cast through the hand, each at a circle drawn for it: radius 10 on the "
	                   "left, 30 in the middle and 60 on the right.",
	    .expected = "Each shield rises where its circle was drawn, its sphere a little bigger than its circle (11, 33 and "
	                "67). The biggest costs 4 times the middle one to keep and the smallest a ninth (the debug window's "
	                "running miracles). A circle smaller than 5 or bigger than 1000 would be held to those.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-60.0f, 0.0f}, {170.0f, 110.0f}}},
	    .miracles = {{.type = MagicType::Shield,
	                  .point = {-40.0f, 40.0f},
	                  .handOffset = {-40.0f, 20.0f},
	                  .delaySeconds = 1.0f,
	                  .byHand = true,
	                  .circleRadius = 10.0f},
	                 {.type = MagicType::Shield,
	                  .point = {20.0f, 40.0f},
	                  .handOffset = {20.0f, 20.0f},
	                  .delaySeconds = 4.0f,
	                  .byHand = true,
	                  .circleRadius = 30.0f},
	                 {.type = MagicType::Shield,
	                  .point = {120.0f, 50.0f},
	                  .handOffset = {120.0f, 20.0f},
	                  .delaySeconds = 7.0f,
	                  .byHand = true,
	                  .circleRadius = 60.0f}},
	});

	all.push_back({
	    .id = "miracles.shield_runs_out",
	    .name = "A shield runs out of prayer power",
	    .facet = Facet::Miracles,
	    .description = "A spiritual shield of radius 60 cast through the hand by a player with only 2000 prayer power.",
	    .expected = "The shield costs 80 a turn to keep, four times a radius 30 one. The player's worship tops it up "
	                "while it lasts, about two and a half seconds; then the shield weakens, its patches dimming to their "
	                "faintest, and after about six seconds more it closes down: its sphere shrinks away over two seconds "
	                "and its hum stops dead as the sphere goes, without fading.",
	    .environment = {.dispenserGrid = false, .prayer = 2000.0f},
	    .framing = {.shot = Shot::Overview, .include = {{-70.0f, -30.0f}, {70.0f, 110.0f}}},
	    .miracles = {{.type = MagicType::Shield,
	                  .point = k_ShieldSpot,
	                  .handOffset = {0.0f, 10.0f},
	                  .delaySeconds = 1.0f,
	                  .byHand = true,
	                  .circleRadius = 60.0f}},
	});

	all.push_back({
	    .id = "miracles.shield_people",
	    .name = "A town's people shelter under a shield after an attack",
	    .facet = Facet::Miracles,
	    .description = "Five villagers of the player's town stand about a hut north of the middle. Another player's lightning "
	                   "strikes by the hut, then the player casts a spiritual shield of radius 30 over it. An enemy player's "
	                   "tiger walks from the shield's west to its east and back.",
	    .expected =
	        "The lightning's harm to the town's people is an attack on the town, which now wants protection. Once the shield "
	        "rises its people take it up, the nearer the sooner (one of the land's reactions is spread each turn): "
	        "those outside four fifths of its radius walk in under it on their own side and face out, turning to "
	        "look about through its wall. They stay while the attack is fresh, then go back to what they were doing. "
	        "The enemy tiger walks round the shield's edge rather than through it, and under the shield no other "
	        "player has any influence.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-80.0f, -10.0f}, {80.0f, 90.0f}}},
	    .creatures = {Thrower({-80.0f, 40.0f}, 270.0f, PlayerNames::PLAYER_TWO)},
	    .objects = {{.type = AbodeInfo::CelticHut, .offset = {0.0f, 40.0f}},
	                {.type = VillagerInfo::CelticFarmerMale, .offset = {-45.0f, 40.0f}, .joinTown = true},
	                {.type = VillagerInfo::CelticForesterMale, .offset = {45.0f, 45.0f}, .joinTown = true},
	                {.type = VillagerInfo::CelticHousewifeFemale, .offset = {0.0f, 82.0f}, .joinTown = true},
	                {.type = VillagerInfo::CelticFishermanMale, .offset = {20.0f, 2.0f}, .joinTown = true},
	                {.type = VillagerInfo::CelticLeaderMale, .offset = {12.0f, 45.0f}, .joinTown = true}},
	    .miracles = {{.type = MagicType::LightningBolt,
	                  .point = {12.0f, 45.0f},
	                  .handOffset = {12.0f, 30.0f},
	                  .handHeight = 15.0f,
	                  .delaySeconds = 0.5f,
	                  .holdSeconds = 2.0f,
	                  .player = PlayerNames::PLAYER_TWO},
	                 {.type = MagicType::Shield, .point = k_ShieldSpot, .delaySeconds = 4.0f}},
	    .commands =
	        {{.kind = Kind::WalkTo, .creature = 0, .delaySeconds = 6.0f, .waitUntilFree = true, .point = {80.0f, 40.0f}},
	         {.kind = Kind::WalkTo, .creature = 0, .delaySeconds = 1.0f, .waitUntilFree = true, .point = {-80.0f, 40.0f}}},
	    .repeatFrom = 0,
	});
}

void AddForests(std::vector<Scenario>& all)
{
	struct Ground
	{
		std::string_view id;
		std::string_view name;
		glm::vec2 spot;
		std::string_view description;
		std::string_view expected;
	};
	const std::array k_Grounds {
	    Ground {"miracles.forest_grass",
	            "Forest on grass",
	            {0.0f, 40.0f},
	            "A forest cast on the testbed's grass north of the middle.",
	            "The seed falls with a trail of blue blobs and lands; 18 trees are planted at once on a spiral from 2 to "
	            "11 out, of beech, birch and cedar, and grow from nothing each turn, those further out to a smaller size, "
	            "the middle ones over about nine seconds. Sparkles burst and vortices spin at three and a half seconds; "
	            "at five and a half, 5 flocks of 10 butterflies circle up over the trees and fade away by 16 seconds. "
	            "The camera glides onto the forest's path over four seconds and follows it for eleven more, the "
	            "player's camera controls doing nothing meanwhile; then the player has the camera back where it is "
	            "(the debug log says the path ended after 15.0 s)."},
	    Ground {"miracles.forest_sand", "Forest on sand", k_Sand, "A forest cast on the patch of sand west of the middle.",
	            "As on grass, but its trees are palms."},
	    Ground {"miracles.forest_snow", "Forest on snow", k_Snow, "A forest cast on the patch of snow east of the middle.",
	            "As on grass, but its trees are conifers."},
	};
	for (const auto& ground : k_Grounds)
	{
		all.push_back({
		    .id = ground.id,
		    .name = ground.name,
		    .facet = Facet::Miracles,
		    .description = ground.description,
		    .expected = ground.expected,
		    .environment = {.dispenserGrid = false},
		    .framing = {.shot = Shot::Overview,
		                .include = {ground.spot + glm::vec2(-20.0f, -20.0f), ground.spot + glm::vec2(20.0f, 20.0f)}},
		    .miracles = {{.type = MagicType::Forest,
		                  .point = ground.spot,
		                  .handOffset = ground.spot,
		                  .delaySeconds = 0.5f,
		                  .byHand = true}},
		});
	}

	all.push_back({
	    .id = "miracles.forest_camera_taken_back",
	    .name = "Forest: the camera taken back from its path",
	    .facet = Facet::Miracles,
	    .description = "A forest cast by hand on grass north of the middle; eight seconds after the cast the player "
	                   "presses the key that moves the camera forwards.",
	    .expected = "The camera glides onto the forest's path and follows it until the key is pressed; then it is the "
	                "player's again at once, from where it was, and moves forwards (the debug log says the player took "
	                "the camera back). Rotating, tilting or zooming would not have taken it back.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 20.0f}, {20.0f, 60.0f}}},
	    .miracles = {{.type = MagicType::Forest,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = {0.0f, 40.0f},
	                  .delaySeconds = 0.5f,
	                  .byHand = true}},
	    .commands = {{.kind = Kind::PressKey,
	                  .delaySeconds = 8.5f,
	                  .value = static_cast<size_t>(input::BindableActionMap::MOVE_FORWARDS)}},
	});

	all.push_back({
	    .id = "miracles.forest_withers",
	    .name = "Forest grows, then withers once let go",
	    .facet = Facet::Miracles,
	    .description = "A forest cast on grass north of the middle and closed down after 15 seconds, every 30.",
	    .expected = "Its trees grow for 15 seconds, costing 5 and 1 for each tree a turn. Closed down, every tree "
	                "shrinks by a twentieth of its size a turn and is gone within two seconds, the forest with its last "
	                "tree; then the miracle goes.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 20.0f}, {20.0f, 60.0f}}},
	    .miracles = {{.type = MagicType::Forest,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = {0.0f, 40.0f},
	                  .delaySeconds = 0.5f,
	                  .holdSeconds = 15.0f,
	                  .repeatSeconds = 30.0f}},
	});

	all.push_back({
	    .id = "miracles.forest_beside_hut",
	    .name = "Forest beside a hut",
	    .facet = Facet::Miracles,
	    .description = "A forest cast on grass 16 units west of a hut, in the cell west of the one the hut stands in.",
	    .expected = "The trees whose places on the spiral fall in a cell the hut stands in are not planted, the others "
	                "are, however close to each other. (A forest cast in a cell the hut stands in is refused.)",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-28.0f, 20.0f}, {20.0f, 60.0f}}},
	    .objects = {{.type = AbodeInfo::CelticHut, .offset = {8.0f, 40.0f}}},
	    .miracles = {{.type = MagicType::Forest,
	                  .point = {-8.0f, 40.0f},
	                  .handOffset = {-8.0f, 40.0f},
	                  .delaySeconds = 0.5f,
	                  .byHand = true}},
	});

	all.push_back({
	    .id = "miracles.forest_bats",
	    .name = "An evil player's forest has bats",
	    .facet = Facet::Miracles,
	    .description = "A forest cast on grass by a player of alignment -0.8.",
	    .expected = "As any forest, but the 5 flocks over it are of bats rather than butterflies.",
	    .environment = {.dispenserGrid = false, .playerAlignment = -0.8f},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 20.0f}, {20.0f, 60.0f}}},
	    .miracles = {{.type = MagicType::Forest,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = {0.0f, 40.0f},
	                  .delaySeconds = 0.5f,
	                  .byHand = true}},
	});
}
} // namespace

void testbed_scenarios::AddShieldForestScenarios(std::vector<Scenario>& all)
{
	AddShields(all);
	AddForests(all);
}

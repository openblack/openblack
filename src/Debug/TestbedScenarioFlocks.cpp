/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of the flock miracles: doves for a good caster, bats for an evil one, and a pack of wolves
// swept out towards villagers, each cast through the hand and swept across the land as a player does

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
/// The hand casts a little south of the spot and sweeps east across it, the camera looking north
constexpr glm::vec2 k_HandFrom {-12.0f, 0.0f};
constexpr glm::vec2 k_Spot {0.0f, 30.0f};
constexpr glm::vec3 k_SweepEast {16.0f, 0.0f, 0.0f};
/// Cast at a point, the hand over the land just south of it, so the animals head north
constexpr glm::vec2 k_HandBehind {0.0f, 25.0f};
/// The hand lets go once armed, then sweeps on for the second or so the flock takes to pour out
constexpr float k_HoldSeconds = 0.4f;
constexpr float k_SweepSeconds = 1.6f;
} // namespace

void testbed_scenarios::AddFlockScenarios(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.flock_doves",
	    .name = "Flying flock: doves",
	    .facet = Facet::Miracles,
	    .description = "A good player (alignment 0.8) casts the flying flock by hand and sweeps the hand east across the "
	                   "land for a second and a half.",
	    .expected = "A dozen white doves, all 2.8 to 3 times a dove's size, pour out of the hand along its sweep with a "
	                "single coo as the first appears, each trailing pale blue sparkles scaled to its size; a sparkling trail "
	                "follows the hand until the last is out. They climb to about 45 m, fanning out north up to two radians "
	                "either side of the view, the first leading and the rest following it, banking as they turn, each "
	                "flapping on its own beat. The leader roams on about wherever it is, not about the cast point. After 25 "
	                "seconds the miracle ends; the doves fly on whole for two seconds, then vanish.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-40.0f, -10.0f}, {40.0f, 120.0f}}},
	    .miracles = {{.type = MagicType::FlockFlying,
	                  .point = k_Spot,
	                  .handOffset = k_HandFrom,
	                  .handHeight = 12.0f,
	                  .throwVelocity = k_SweepEast,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = k_HoldSeconds,
	                  .byHand = true,
	                  .sweepSeconds = k_SweepSeconds,
	                  .casterAlignment = 0.8f}},
	});

	all.push_back({
	    .id = "miracles.flock_bats",
	    .name = "Flying flock: bats",
	    .facet = Facet::Miracles,
	    .description = "An evil player (alignment -0.8) casts the flying flock by hand and sweeps it east, a creature "
	                   "standing to the north.",
	    .expected = "Black bats, 2.8 to 3 times a bat's size, flap out of the hand along the sweep with a single screech "
	                "as the first appears, each trailing black smoke puffs, and a dark smoky trail follows the hand. They fly "
	                "north at about 45 m in a fan, the first leading. The "
	                "bats count as frightening to creatures (doves don't). They always flap. After 25 seconds they fly on "
	                "whole for two seconds, then vanish.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-40.0f, -10.0f}, {40.0f, 120.0f}}},
	    .creatures = {{.label = "watching", .offset = {0.0f, 70.0f}, .facingDegrees = 0.0f}},
	    .miracles = {{.type = MagicType::FlockFlying,
	                  .point = k_Spot,
	                  .handOffset = k_HandFrom,
	                  .handHeight = 12.0f,
	                  .throwVelocity = k_SweepEast,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = k_HoldSeconds,
	                  .byHand = true,
	                  .sweepSeconds = k_SweepSeconds,
	                  .casterAlignment = -0.8f}},
	});

	all.push_back({
	    .id = "miracles.flock_wolves",
	    .name = "Ground flock: wolves hunting villagers",
	    .facet = Facet::Miracles,
	    .description = "The ground flock cast by hand and swept east, with villagers standing in the wolves' way to the "
	                   "north and one well off to the side.",
	    .expected = "Fourteen wolves, 1.5 to 2 times a wolf's size, appear along the sweep, each with a puff and a trail "
	                "of dust the colour of the ground, a single howl as the first appears, and run north. The villagers "
	                "within 45 m either side of their run are chased, pounced on and brought down, and the wolves stop to "
	                "eat; a villager is eaten over 30 seconds and then dies as villagers die, falling and lying dead. "
	                "The villager off to the side is left alone. Fed wolves run on; the miracle ends after 60 seconds and "
	                "the wolves fade away.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 0.0f}, {20.0f, 110.0f}}, .distance = 0.7f},
	    .objects = {{.type = VillagerInfo::CelticFarmerMale, .offset = {-5.0f, 55.0f}},
	                {.type = VillagerInfo::CelticForesterMale, .offset = {8.0f, 70.0f}},
	                {.type = VillagerInfo::CelticFarmerMale, .offset = {0.0f, 90.0f}},
	                {.type = VillagerInfo::CelticForesterMale, .offset = {-12.0f, 105.0f}},
	                {.type = VillagerInfo::CelticFarmerMale, .offset = {110.0f, 40.0f}}},
	    .miracles = {{.type = MagicType::FlockGround,
	                  .point = k_Spot,
	                  .handOffset = k_HandFrom,
	                  .handHeight = 12.0f,
	                  .throwVelocity = k_SweepEast,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = k_HoldSeconds,
	                  .byHand = true,
	                  .sweepSeconds = k_SweepSeconds}},
	});

	all.push_back({
	    .id = "miracles.flock_wolves_fade",
	    .name = "Ground flock: wolves fade as it ends",
	    .facet = Facet::Miracles,
	    .description = "The ground flock cast at a point and closed down three seconds later, before the wolves are far.",
	    .expected = "Each wolf runs its own run, its legs in step with the ground it covers, bigger wolves striding slower. "
	                "The wolves are lit white rather than by the land. When the miracle ends they keep running while they "
	                "fade: slowly at first, fastest after a second and easing out, truly see-through (blended, not "
	                "speckled) until they are gone two seconds later.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-30.0f, 0.0f}, {30.0f, 90.0f}}},
	    .miracles = {{.type = MagicType::FlockGround,
	                  .point = k_Spot,
	                  .handOffset = k_HandBehind,
	                  .handHeight = 12.0f,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = 3.0f}},
	});

	all.push_back({
	    .id = "miracles.flock_doves_vanish",
	    .name = "Flying flock: doves vanish as it ends",
	    .facet = Facet::Miracles,
	    .description = "The flying flock cast at a point by a good player and closed down three seconds later.",
	    .expected = "Each dove flaps on its own beat, set by how fast it flies and how big it is, lit by the brightest of "
	                "the land's light. When the miracle ends the doves fly on whole for two seconds and then are gone at "
	                "once: the game counts their fade down but never draws it.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-40.0f, -10.0f}, {40.0f, 120.0f}}},
	    .miracles = {{.type = MagicType::FlockFlying,
	                  .point = k_Spot,
	                  .handOffset = k_HandBehind,
	                  .handHeight = 12.0f,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = 3.0f,
	                  .casterAlignment = 0.8f}},
	});
}

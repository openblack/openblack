/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of the miracles: the dispensers to take them from by hand, each miracle cast at what it acts
// on, and each creature spell cast on a creature

#include <array>
#include <string_view>

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
using Target = MiracleCast::Target;

/// Where the miracles land, north of the dispenser grid
constexpr glm::vec2 k_Spot {0.0f, 30.0f};
/// A hand a little south of the spot and above it, which throws north
constexpr glm::vec2 k_ThrowFrom {0.0f, -5.0f};
constexpr glm::vec3 k_ThrowNorth {0.0f, 4.0f, 40.0f};
/// A held miracle runs this long in a scenario, then the hand lets go
constexpr float k_HoldSeconds = 4.0f;
/// The creature spells hold for their tables' 25 seconds with a few seconds' easing either side
constexpr float k_CreatureSpellRepeat = 40.0f;

CreatureSetup Subject(glm::vec2 offset, std::string_view label, float life = 1.0f)
{
	return {.label = label,
	        .species = CreatureType::Tiger,
	        .offset = offset,
	        .facingDegrees = 0.0f,
	        .needs = {.energy = 1.0f, .exhaustion = 0.0f, .dehydration = 0.0f, .poo = 0.0f, .life = life}};
}

void AddCasting(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.dispensers",
	    .name = "Miracle dispensers",
	    .facet = Facet::Miracles,
	    .description = "The testbed's grid of a dispenser for every miracle, the creature spells among them, in front "
	                   "of the camera, each labelled with its miracle.",
	    .expected = "Each dispenser floats a bubble with its seed spinning inside over a swirl of sparks (the grid "
	                "starts charged, a testbed set-up step). Tapping a bubble "
	                "with the hand puts its miracle in the hand; pressing over the land casts it (heal, forest), letting "
	                "go throws it (fireball, flocks; storms and shields at a circle drawn) or holding runs it "
	                "(lightning, water, food, wood); a creature spell "
	                "is pressed onto a creature. The left button taps a bubble; the action button (the right) casts. A "
	                "scribble or shake drops the seed. A dispenser makes a new bubble 30 seconds after its last was "
	                "taken.",
	    .framing = {.shot = Shot::Testbed},
	    .creatures = {Subject({0.0f, 30.0f}, "for the creature spells")},
	});

	all.push_back({
	    .id = "miracles.fireball_trees",
	    .name = "Fireball at trees",
	    .facet = Facet::Miracles,
	    .description = "A fireball thrown from a hand south of three trees, every eight seconds.",
	    .expected = "The ball flies north in an arc trailing fire, bounces on the land among the trees and burns out; the "
	                "trees it passes catch, flames licking at their lower branches, and burn on, spreading to their "
	                "neighbours, charring as they burn down.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{0.0f, -5.0f}, {0.0f, 55.0f}}},
	    .objects = {{.type = TreeInfo::Oak, .offset = {-6.0f, 45.0f}},
	                {.type = TreeInfo::Beech, .offset = {0.0f, 50.0f}},
	                {.type = TreeInfo::Conifer, .offset = {6.0f, 45.0f}}},
	    .miracles = {{.type = MagicType::Fireball,
	                  .point = {0.0f, 45.0f},
	                  .handOffset = k_ThrowFrom,
	                  .handHeight = 10.0f,
	                  .throwVelocity = k_ThrowNorth,
	                  .repeatSeconds = 8.0f}},
	});

	all.push_back({
	    .id = "miracles.lightning_creature",
	    .name = "Lightning at a creature",
	    .facet = Facet::Miracles,
	    .description = "A lightning bolt held from a hand south of a creature, looking north at it, for four seconds in "
	                   "every eight.",
	    .expected = "Forked bolts crackle from the hand to the creature and the ground about it, lighting the land where "
	                "they strike; the creature is burnt by each strike. The crackle stops soon after the bolt does (there "
	                "is no thunder).",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{0.0f, 0.0f}, k_Spot}},
	    .creatures = {Subject(k_Spot, "struck")},
	    .miracles = {{.type = MagicType::LightningBolt,
	                  .target = Target::Creature,
	                  .creature = 0,
	                  .handOffset = {0.0f, 5.0f},
	                  .handHeight = 15.0f,
	                  .holdSeconds = k_HoldSeconds,
	                  .repeatSeconds = 8.0f}},
	});

	all.push_back({
	    .id = "miracles.heal_creature",
	    .name = "Heal a hurt creature",
	    .facet = Facet::Miracles,
	    .description = "A creature with a third of its life left, healed by the miracle cast at its feet.",
	    .expected = "A chakra appears over the creature with sparks bursting under it, and the creature's life fills up.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {k_Spot}, .distance = 0.6f},
	    .creatures = {Subject(k_Spot, "hurt", 0.3f)},
	    .miracles = {{.type = MagicType::Heal, .target = Target::Creature, .creature = 0, .repeatSeconds = 25.0f}},
	});

	all.push_back({
	    .id = "miracles.food_wood",
	    .name = "Food and wood piles",
	    .facet = Facet::Miracles,
	    .description = "Food poured from a hand on the left and wood on the right, each for four seconds.",
	    .expected = "Grains of food and logs tumble from the hands; where they land they make piles of food and wood, "
	                "which grow as more lands on them.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-15.0f, 30.0f}, {15.0f, 30.0f}}, .distance = 0.7f},
	    .miracles = {{.type = MagicType::Food,
	                  .point = {-15.0f, 30.0f},
	                  .handOffset = {-15.0f, 30.0f},
	                  .handHeight = 12.0f,
	                  .holdSeconds = k_HoldSeconds},
	                 {.type = MagicType::Wood,
	                  .point = {15.0f, 30.0f},
	                  .handOffset = {15.0f, 30.0f},
	                  .handHeight = 12.0f,
	                  .holdSeconds = k_HoldSeconds}},
	});

	all.push_back({
	    .id = "miracles.water_fire",
	    .name = "Water on burning trees",
	    .facet = Facet::Miracles,
	    .description = "Trees set alight by a fireball, then rained on by the water miracle held over them.",
	    .expected = "The fireball sets the trees burning; a rain cloud forms over them, its drops leave rings on the land "
	                "and cool the fires a little each, until the flames die down, steam hisses up and smoke rises as they "
	                "go out. Villagers would come to watch.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{0.0f, -5.0f}, {0.0f, 55.0f}}},
	    .objects = {{.type = TreeInfo::Oak, .offset = {-4.0f, 45.0f}}, {.type = TreeInfo::Beech, .offset = {4.0f, 45.0f}}},
	    .miracles = {{.type = MagicType::Fireball,
	                  .point = {0.0f, 45.0f},
	                  .handOffset = k_ThrowFrom,
	                  .handHeight = 10.0f,
	                  .throwVelocity = k_ThrowNorth},
	                 {.type = MagicType::Water,
	                  .point = {0.0f, 45.0f},
	                  .handOffset = {0.0f, 45.0f},
	                  .handHeight = 14.0f,
	                  .delaySeconds = 5.0f,
	                  .holdSeconds = 6.0f}},
	});

	all.push_back({
	    .id = "miracles.shield",
	    .name = "Shield blocking",
	    .facet = Facet::Miracles,
	    .description = "A creature under a shield; a fireball is thrown at it and then lightning held at it.",
	    .expected = "A dome of vapour rises over the creature. The fireball bounces off it with a spark; the lightning's "
	                "forks stop at its surface, sparking, and the creature is not struck. The shield weakens with each "
	                "blow and fades when its prayer power runs out.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{0.0f, -5.0f}, {0.0f, 70.0f}}},
	    .creatures = {Subject({0.0f, 40.0f}, "sheltered")},
	    .miracles = {{.type = MagicType::Shield, .point = {0.0f, 40.0f}, .delaySeconds = 0.5f},
	                 {.type = MagicType::Fireball,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = k_ThrowFrom,
	                  .handHeight = 10.0f,
	                  .throwVelocity = k_ThrowNorth,
	                  .delaySeconds = 4.0f},
	                 {.type = MagicType::LightningBolt,
	                  .target = Target::Creature,
	                  .creature = 0,
	                  .handOffset = {0.0f, 0.0f},
	                  .handHeight = 15.0f,
	                  .delaySeconds = 8.0f,
	                  .holdSeconds = k_HoldSeconds}},
	});
}

void AddFoundation(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.hand_fireball_throws",
	    .name = "Fireballs thrown by hand: slow, medium, fast",
	    .facet = Facet::Miracles,
	    .description = "Three fireball seeds put in the hand one after another; each is armed with the action button, the "
	                   "hand swung north at 30, 200 and then 600 a second, and let go.",
	    .expected = "Each press starts the hand's hum, which stops on letting go. The slow ball drops just ahead of the "
	                "hand at 30, the medium one flies at 106 (the hand's 200 through the throw-speed table) with the medium "
	                "throw sample, the fast one at the top speed of 200 however fast the hand; all leave a little above "
	                "the hand's movement, their trails leaving the hand with them, and a bright core where the trail's "
	                "head sits on the ball. The fire seed is gone from the hand once thrown.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{0.0f, -5.0f}, {0.0f, 90.0f}}},
	    .miracles = {{.type = MagicType::Fireball,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = k_ThrowFrom,
	                  .handHeight = 10.0f,
	                  .throwVelocity = {0.0f, 0.0f, 30.0f},
	                  .delaySeconds = 1.0f,
	                  .repeatSeconds = 12.0f,
	                  .byHand = true},
	                 {.type = MagicType::Fireball,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = k_ThrowFrom,
	                  .handHeight = 10.0f,
	                  .throwVelocity = {0.0f, 0.0f, 200.0f},
	                  .delaySeconds = 5.0f,
	                  .repeatSeconds = 12.0f,
	                  .byHand = true},
	                 {.type = MagicType::Fireball,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = k_ThrowFrom,
	                  .handHeight = 10.0f,
	                  .throwVelocity = {0.0f, 0.0f, 600.0f},
	                  .delaySeconds = 9.0f,
	                  .repeatSeconds = 12.0f,
	                  .byHand = true}},
	});

	all.push_back({
	    .id = "miracles.hand_circle_casts",
	    .name = "Storm and shields cast at a circle by hand",
	    .facet = Facet::Miracles,
	    .description = "A storm seed let go after a circle of 40 is drawn on the left, then a storm seed let go with no "
	                   "circle, then a shield over the middle at a circle of 25 and a physical shield on the right at "
	                   "a circle of 15. The circles are put in as if drawn, until the gestures can be.",
	    .expected = "The storm starts at the left circle's middle, not at the hand. The one let go without a circle fails "
	                "with the failure's puff and sound and stays out of the running miracles. The shield rises over the "
	                "middle at radius 25; the physical shield, which has no particle effect, stays among the running "
	                "miracles (the debug window's list) instead of going on its first turn. The storm and shield seeds "
	                "leave the hand with their miracles.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-40.0f, 0.0f}, {40.0f, 60.0f}}},
	    .miracles = {{.type = MagicType::StormWindRain,
	                  .point = {-30.0f, 40.0f},
	                  .handOffset = {-10.0f, 0.0f},
	                  .delaySeconds = 1.0f,
	                  .byHand = true,
	                  .circleRadius = 40.0f},
	                 {.type = MagicType::StormWindRain,
	                  .point = {-30.0f, 40.0f},
	                  .handOffset = {-10.0f, 0.0f},
	                  .delaySeconds = 4.0f,
	                  .byHand = true},
	                 {.type = MagicType::Shield,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = {0.0f, 10.0f},
	                  .delaySeconds = 7.0f,
	                  .byHand = true,
	                  .circleRadius = 25.0f},
	                 {.type = MagicType::PhysicalShield,
	                  .point = {30.0f, 40.0f},
	                  .handOffset = {30.0f, 10.0f},
	                  .delaySeconds = 10.0f,
	                  .byHand = true,
	                  .circleRadius = 15.0f}},
	});

	all.push_back({
	    .id = "miracles.lifetime_without_effects",
	    .name = "Miracles without a particle effect live on",
	    .facet = Facet::Miracles,
	    .description = "Teleport, the flying and the ground flocks and the physical shield cast at points; teleport and "
	                   "the physical shield have no particle effect of their own, and the flocks' effects wait for their "
	                   "animals.",
	    .expected = "All four stay in the debug window's running miracles: the flocks for their 25 and 60 seconds and "
	                "while their animals fade, teleport and the physical shield while their prayer power lasts, paid from "
	                "the player's. Before, each was gone on its first turn.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-30.0f, 20.0f}, {30.0f, 60.0f}}},
	    .miracles = {{.type = MagicType::Teleport, .point = {-30.0f, 40.0f}},
	                 {.type = MagicType::FlockFlying, .point = {-10.0f, 40.0f}},
	                 {.type = MagicType::FlockGround, .point = {10.0f, 40.0f}},
	                 {.type = MagicType::PhysicalShield, .point = {30.0f, 40.0f}}},
	});

	all.push_back({
	    .id = "miracles.hand_held_miracles",
	    .name = "Lightning, food and wood held by hand",
	    .facet = Facet::Miracles,
	    .description = "A lightning seed held over a creature for three seconds, then food poured for six on the left and "
	                   "wood for six on the right, each through the action button held down.",
	    .expected = "The bolt follows the hand while the button is down and stops when it comes up; its crackle fades and "
	                "stops within about a second of letting go, and a second bolt doesn't lose the first's sound. The "
	                "lightning is held in the hand's still rest pose, nothing in it but the bolt's sparks in its fingers. "
	                "The horn of plenty and the logs hang below the hand, held from the side, the hand raised by about "
	                "their height. They pour while held, the hand staying where the pour began and rising and rolling "
	                "over and over in a four second cycle, up to about 20 units and 121 degrees, rolling about the line "
	                "from the hand to the camera, swelling smoothly from rest and back, and raised once more by its hold "
	                "while it pours (the debug window shows the pour); letting go keeps what is left in the seed and the "
	                "hand eases down over a tenth of a second. Their sprinkle sounds stop too.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 20.0f}, {20.0f, 50.0f}}},
	    .creatures = {Subject({0.0f, 40.0f}, "struck")},
	    .miracles = {{.type = MagicType::LightningBolt,
	                  .target = Target::Creature,
	                  .creature = 0,
	                  .handOffset = {0.0f, 30.0f},
	                  .handHeight = 15.0f,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = 3.0f,
	                  .byHand = true},
	                 {.type = MagicType::Food,
	                  .point = {-15.0f, 30.0f},
	                  .handOffset = {-15.0f, 30.0f},
	                  .handHeight = 12.0f,
	                  .delaySeconds = 6.0f,
	                  .holdSeconds = 6.0f,
	                  .byHand = true},
	                 {.type = MagicType::Wood,
	                  .point = {15.0f, 30.0f},
	                  .handOffset = {15.0f, 30.0f},
	                  .handHeight = 12.0f,
	                  .delaySeconds = 14.0f,
	                  .holdSeconds = 6.0f,
	                  .byHand = true}},
	});

	all.push_back({
	    .id = "miracles.hand_press_too_soon",
	    .name = "Pressing before a summoned seed is ready",
	    .facet = Facet::Miracles,
	    .description = "A heal seed summoned from the player's worship and pressed over a hurt creature half a second "
	                   "later, before it is ready.",
	    .expected = "The press fails: a puff where the hand points and the failure's sound, and the seed stays in the hand "
	                "(the log gives the press as not ready). For its first second and a half the hand holds it in its "
	                "rest pose with nothing in it; then the heal's model shows and the hand holds it as the heal is held.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {k_Spot}, .distance = 0.6f},
	    .creatures = {Subject(k_Spot, "hurt", 0.3f)},
	    .miracles = {{.type = MagicType::Heal,
	                  .target = Target::Creature,
	                  .creature = 0,
	                  .handOffset = {0.0f, 25.0f},
	                  .delaySeconds = 1.0f,
	                  .byHand = true,
	                  .fromWorship = true,
	                  .holdBeforePress = 0.5f}},
	});

	all.push_back({
	    .id = "miracles.hand_press_outside_influence",
	    .name = "Pressing outside the player's influence",
	    .facet = Facet::Miracles,
	    .description = "Another player's spiritual shield is cast on the land, which takes the player's influence away "
	                   "under it; then the player presses a food seed under it by hand.",
	    .expected = "Nothing at all happens when the button goes down: no food, and neither the failure's puff nor its "
	                "sound. The seed stays in the hand.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 20.0f}, {20.0f, 60.0f}}},
	    .miracles =
	        {{.type = MagicType::Shield, .point = {0.0f, 40.0f}, .delaySeconds = 0.5f, .player = PlayerNames::PLAYER_TWO},
	         {.type = MagicType::Food,
	          .point = {0.0f, 40.0f},
	          .handOffset = {0.0f, 40.0f},
	          .handHeight = 12.0f,
	          .delaySeconds = 3.0f,
	          .holdSeconds = 2.0f,
	          .byHand = true}},
	});

	all.push_back({
	    .id = "miracles.hand_worship_and_creature",
	    .name = "A seed from worship, and a creature spell by hand",
	    .facet = Facet::Miracles,
	    .description = "A heal seed summoned from the player's worship and pressed over a hurt creature once ready, then a "
	                   "freeze phial pressed onto the creature.",
	    .expected = "The heal seed is charged from the player's prayer power (the debug window's prayer power drops by "
	                "its cost) and is not ready for a second and a half; once ready the press casts the heal at once and "
	                "the seed is gone. The freeze phial casts on the creature when pressed on it and is gone from the "
	                "hand.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {k_Spot}, .distance = 0.6f},
	    .creatures = {Subject(k_Spot, "hurt", 0.3f)},
	    .miracles = {{.type = MagicType::Heal,
	                  .target = Target::Creature,
	                  .creature = 0,
	                  .handOffset = {0.0f, 25.0f},
	                  .delaySeconds = 1.0f,
	                  .byHand = true,
	                  .fromWorship = true},
	                 {.type = MagicType::CreatureSpellFreeze,
	                  .target = Target::Creature,
	                  .creature = 0,
	                  .handOffset = {0.0f, 25.0f},
	                  .delaySeconds = 6.0f,
	                  .byHand = true}},
	});

	all.push_back({
	    .id = "miracles.area_effect",
	    .name = "A miracle's effect over everyone it reaches",
	    .facet = Facet::Miracles,
	    .description = "Lightning held by hand over five villagers huddled within a couple of metres, with two trees beside "
	                   "them, for four seconds.",
	    .expected = "The strikes hurt the villagers, a little at each strike by the bolt's table (a few thousandths of a "
	                "life each, too little to kill in four seconds), and the trees they reach catch light. An effect at a "
	                "point reaches "
	                "everyone near it, found in the land's cells round the point. One brought to no life falls dead. Hurting "
	                "villagers turns the player evil: at the next turn what waits, held to a whole one, scales the game's "
	                "change for a turn, and the rest is gone.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-6.0f, 34.0f}, {6.0f, 46.0f}}, .distance = 0.6f},
	    .objects = {{.type = VillagerInfo::CelticFarmerMale, .offset = {-0.8f, 40.0f}},
	                {.type = VillagerInfo::CelticFarmerMale, .offset = {0.8f, 40.0f}},
	                {.type = VillagerInfo::CelticForesterMale, .offset = {0.0f, 41.0f}},
	                {.type = VillagerInfo::CelticForesterMale, .offset = {0.0f, 39.0f}},
	                {.type = VillagerInfo::CelticLeaderMale, .offset = {1.2f, 41.2f}},
	                {.type = TreeInfo::Oak, .offset = {-3.0f, 42.0f}},
	                {.type = TreeInfo::Beech, .offset = {3.0f, 42.0f}}},
	    .miracles = {{.type = MagicType::LightningBolt,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = {0.0f, 32.0f},
	                  .handHeight = 12.0f,
	                  .holdSeconds = 4.0f,
	                  .repeatSeconds = 12.0f,
	                  .byHand = true}},
	});

	all.push_back({
	    .id = "miracles.miracle_kills",
	    .name = "A miracle takes the last of their life",
	    .facet = Facet::Miracles,
	    .description = "Lightning held by hand over three villagers with almost no life left, and a tree, for two seconds.",
	    .expected = "Within a second the strikes, and the fire they light on them, take the last of the villagers' life: "
	                "each falls dead where it stands. The tree, hurt too, keeps standing and catches light.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-6.0f, 34.0f}, {6.0f, 46.0f}}, .distance = 0.6f},
	    .objects = {{.type = VillagerInfo::CelticFarmerMale, .offset = {-0.8f, 40.0f}, .life = 0.01f},
	                {.type = VillagerInfo::CelticFarmerMale, .offset = {0.8f, 40.0f}, .life = 0.01f},
	                {.type = VillagerInfo::CelticForesterMale, .offset = {0.0f, 41.0f}, .life = 0.01f},
	                {.type = TreeInfo::Oak, .offset = {-3.0f, 42.0f}}},
	    .miracles = {{.type = MagicType::LightningBolt,
	                  .point = {0.0f, 40.0f},
	                  .handOffset = {0.0f, 32.0f},
	                  .handHeight = 12.0f,
	                  .holdSeconds = 2.0f,
	                  .byHand = true}},
	});
}

void AddCreatureSpells(std::vector<Scenario>& all)
{
	struct Spell
	{
		std::string_view id;
		std::string_view name;
		MagicType type;
		std::string_view expected;
	};
	constexpr std::array k_Spells {
	    Spell {"miracles.creature_freeze", "Creature spell: freeze", MagicType::CreatureSpellFreeze,
	           "Two seconds after the cast the freezing sound plays on it and, over two more, the creature slows to a stop, "
	           "darkening to an icy blue with a "
	           "sheen of ice; it gives up what it was doing and its mind stops, learning nothing. It stays frozen for 25 "
	           "seconds, then thaws over two."},
	    Spell {"miracles.creature_small", "Creature spell: small", MagicType::CreatureSpellSmall,
	           "Two seconds after the cast the shrinking sound plays on it and, over four, the creature shrinks to its "
	           "smallest (a fifth of a grown creature's "
	           "size), stays small for 25 seconds, then grows back over four."},
	    Spell {"miracles.creature_big", "Creature spell: big (miracle grow)", MagicType::CreatureSpellBig,
	           "Two seconds after the cast the growing sound plays on it and, over four, the creature grows to its largest "
	           "(2.4 times a grown creature's "
	           "size), stays big for 25 seconds, then shrinks back over four."},
	    Spell {"miracles.creature_weak", "Creature spell: weak", MagicType::CreatureSpellWeak,
	           "The shrinking sound plays on it as, over four seconds, the creature's body wastes to weak, stays so for 25 "
	           "seconds, then builds back up."},
	    Spell {"miracles.creature_strong", "Creature spell: strong", MagicType::CreatureSpellStrong,
	           "The growing sound plays on it as, over four seconds, the creature's body bulks up to fully strong, stays so "
	           "for 25 seconds, then goes back."},
	    Spell {"miracles.creature_invisible", "Creature spell: invisible", MagicType::CreatureSpellInvisible,
	           "With the vanishing sound, over five seconds the creature dissolves through scrolling static until only a "
	           "quarter of it shows, for 25 "
	           "seconds; then it comes back over five. Bolts no longer go to it, and villagers pay it no heed."},
	    Spell {"miracles.creature_nice", "Creature spell: nice", MagicType::CreatureSpellCompassion,
	           "The compassion sound plays on it; the creature gives up what it was doing and wants to be kind above all else, "
	           "every other want held down "
	           "but making friends; on a leash, it becomes the compassion leash. Its alignment eases to fully good over "
	           "three seconds; after 25 seconds it goes back, kindness is what it wants least and its other wants return."},
	    Spell {"miracles.creature_nasty", "Creature spell: nasty", MagicType::CreatureSpellAngry,
	           "The vanishing sound plays on it; the creature gives up what it was doing and wants to be cruel above all else, "
	           "every other want held down; "
	           "on a leash, it becomes the aggression leash. Its alignment swings to fully evil over a second; after 25 "
	           "seconds it goes back, anger is what it wants least and its other wants return."},
	    Spell {"miracles.creature_itchy", "Creature spell: itchy", MagicType::CreatureSpellItchy,
	           "The itching sound starts on it and goes on, stopping only once the camera is 80 or more away; the creature "
	           "gives up what it was doing and wants nothing but to scratch for 25 seconds; any leash on it "
	           "comes off, every turn, and stays off."},
	};
	for (const auto& spell : k_Spells)
	{
		all.push_back({
		    .id = spell.id,
		    .name = spell.name,
		    .facet = Facet::Miracles,
		    .description = "A creature in front of the dispenser grid, with the spell cast on it after a second, and again "
		                   "once it has worn off.",
		    .expected = spell.expected,
		    .framing = {.shot = Shot::Overview, .include = {{0.0f, 0.0f}, k_Spot}, .distance = 0.8f},
		    .creatures = {Subject(k_Spot, "spellbound")},
		    .miracles =
		        {{.type = spell.type, .target = Target::Creature, .creature = 0, .repeatSeconds = k_CreatureSpellRepeat}},
		});
	}
}
} // namespace

void testbed_scenarios::AddMiracleScenarios(std::vector<Scenario>& all)
{
	AddCasting(all);
	AddFoundation(all);
	AddCreatureSpells(all);
	AddShieldForestScenarios(all);
	AddBlastFireScenarios(all);
	AddFirewaterScenarios(all);
}

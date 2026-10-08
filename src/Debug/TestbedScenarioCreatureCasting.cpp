/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of creatures casting miracles: a spell at another creature, a lightning bolt, a heal, a try
// that fizzles and one the creature is too tired for

#include <vector>

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
using Kind = Command::Kind;

Command Know(MagicType type)
{
	// Once its mind has set up what it knows, on its first turns
	return {.kind = Kind::KnowMiracle, .delaySeconds = 0.5f, .value = static_cast<size_t>(type)};
}

Command CastAtCreature(MagicType type, size_t target, float delay)
{
	return {.kind = Kind::CastMiracle, .delaySeconds = delay, .value = static_cast<size_t>(type), .atCreature = target};
}

Command CastAtObject(MagicType type, size_t object, float delay)
{
	return {.kind = Kind::CastMiracle, .delaySeconds = delay, .value = static_cast<size_t>(type), .object = object};
}

CreatureSetup Caster(glm::vec2 offset, std::string_view label, float exhaustion = 0.0f)
{
	return {.label = label,
	        .species = CreatureType::Tiger,
	        .offset = offset,
	        .facingDegrees = 0.0f,
	        .needs = {.energy = 1.0f, .exhaustion = exhaustion, .dehydration = 0.0f, .poo = 0.0f, .life = 1.0f}};
}

CreatureSetup Other(glm::vec2 offset, std::string_view label)
{
	return {.label = label, .species = CreatureType::Cow, .offset = offset, .facingDegrees = 180.0f, .pauseMind = true};
}
} // namespace

void testbed_scenarios::AddCreatureCastingScenarios(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.creature_casts_itchy",
	    .name = "Creature casting: itchy at another creature",
	    .facet = Facet::Miracles,
	    .description = "A tiger that knows the itchy spell is told to cast it at a cow a hundred metres away.",
	    .expected = "One time in five it pulls a playful face first. It walks until it is within five times its height "
	                "(about 75 metres) of the cow, backs off if nearer than twice its height, turns to face it and "
	                "holds still a tenth of a second, then raises its arms into the casting pose; as the pose loops the "
	                "spell is cast with the sound a creature's cast makes, wisps fly to the cow and flies gather round "
	                "its head; three seconds later the tiger lowers its arms. The cow starts scratching two seconds "
	                "after the cast. The tiger tires a little.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 0.0f}, {20.0f, 110.0f}}},
	    .creatures = {Caster({0.0f, 0.0f}, "caster"), Other({0.0f, 100.0f}, "target")},
	    .commands = {Know(MagicType::CreatureSpellItchy), CastAtCreature(MagicType::CreatureSpellItchy, 1, 0.5f)},
	});

	all.push_back({
	    .id = "miracles.creature_casts_lightning",
	    .name = "Creature casting: a lightning bolt at a rock",
	    .facet = Facet::Miracles,
	    .description = "A tiger that knows the lightning bolt is told to cast it at a pillar of rock eighty metres away.",
	    .expected = "One time in five it shows its anger first. It walks up to within fifty metres of the rock, turns "
	                "to it, raises its arms and a bolt crackles from between its hands to the rock for three seconds, "
	                "then stops as it lowers its arms.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 0.0f}, {20.0f, 90.0f}}},
	    .creatures = {Caster({0.0f, 0.0f}, "caster")},
	    .objects = {{.type = FeatureInfo::FatPilarChalk, .offset = {0.0f, 80.0f}, .scale = 0.5f}},
	    .commands = {Know(MagicType::LightningBolt), CastAtObject(MagicType::LightningBolt, 0, 0.5f)},
	});

	all.push_back({
	    .id = "miracles.creature_casts_heal",
	    .name = "Creature casting: a heal at a hurt villager",
	    .facet = Facet::Miracles,
	    .description = "A tiger that knows the heal is told to cast it at a villager hurt to a third of its life, sixty "
	                   "metres away.",
	    .expected = "It walks up to within twice its height of the villager, backing off if nearer, faces it and casts: "
	                "the chakra lights over the villager, whose life fills up. The heal runs its own ten seconds.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 0.0f}, {20.0f, 70.0f}}},
	    .creatures = {Caster({0.0f, 0.0f}, "caster")},
	    .objects = {{.type = VillagerInfo::CelticFarmerMale, .offset = {0.0f, 60.0f}, .life = 0.33f}},
	    .commands = {Know(MagicType::Heal), CastAtObject(MagicType::Heal, 0, 0.5f)},
	});

	all.push_back({
	    .id = "miracles.creature_cast_fizzles",
	    .name = "Creature casting: a spell it hasn't learnt fizzles",
	    .facet = Facet::Miracles,
	    .description = "A tiger that has never seen the freeze spell is told to cast it at a cow.",
	    .expected = "It goes up to the cow and raises its arms, but nothing is cast: it lowers them at once, looks "
	                "embarrassed, and one time in two is sad as well. Its count of sightings of the freeze goes up by "
	                "one.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 0.0f}, {20.0f, 60.0f}}},
	    .creatures = {Caster({0.0f, 0.0f}, "caster"), Other({0.0f, 50.0f}, "target")},
	    .commands = {CastAtCreature(MagicType::CreatureSpellFreeze, 1, 1.0f)},
	});

	all.push_back({
	    .id = "miracles.creature_too_tired_to_cast",
	    .name = "Creature casting: too tired to cast",
	    .facet = Facet::Miracles,
	    .description = "A tiger that knows the freeze spell but is all but exhausted is told to cast it at a cow.",
	    .expected = "It goes up to the cow and raises its arms, but can't pay for the spell: nothing is cast, and it "
	                "wants food above everything else.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 0.0f}, {20.0f, 60.0f}}},
	    .creatures = {Caster({0.0f, 0.0f}, "caster", 0.84f), Other({0.0f, 50.0f}, "target")},
	    .commands = {Know(MagicType::CreatureSpellFreeze), CastAtCreature(MagicType::CreatureSpellFreeze, 1, 0.5f)},
	});

	all.push_back({
	    .id = "miracles.hand_itchy_seed",
	    .name = "Creature spells: the itchy spell held in the hand",
	    .facet = Facet::Miracles,
	    .description = "The player's hand is given the itchy creature spell and held still over the land.",
	    .expected = "Ten flies swarm round the hand's grasp point, circling a point that goes round it on a sphere of two "
	                "metres. The freeze and compassion spells would show nothing in the hand.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-10.0f, 20.0f}, {10.0f, 40.0f}}, .distance = 0.3f},
	    .commands = {{.kind = Kind::HoldSeed,
	                  .delaySeconds = 1.0f,
	                  .value = static_cast<size_t>(SpellSeedType::CreatureSpellItchy)}},
	    .hand = HandHold {.offset = {0.0f, 30.0f}, .height = 8.0f},
	});
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The testbed's scenarios of the miracles' globes and dispensers, close up: the swirl under a dispenser, what floats in
// each globe, and the rings round the extreme ones

#include "TestbedScenarioRegistry.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;

namespace
{
void AddDispensers(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.dispenser_vortex",
	    .name = "Dispenser swirl and its globe",
	    .facet = Facet::Miracles,
	    .description = "Three miracle dispensers close up: fire, lightning and food. The fire and lightning dispensers "
	                   "start charged, a testbed set-up step; the food dispenser is put down as in the game.",
	    .expected = "Under each dispenser a flat disk of starry light six units across lies a little above the land, "
	                "added to what is under it: dark at its middle, brightest a little out, fading at its rim, its stars "
	                "creeping slowly round and quickly outwards. It is the same for every miracle and makes no sound. The "
	                "fire and lightning dispensers float a globe a little above themselves at once; the food dispenser has "
	                "none until 30 seconds after it is put down, when its globe appears with a puff of sparkles.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-12.0f, 20.0f}, {12.0f, 30.0f}}, .distance = 0.5f},
	    .dispensers = {{.type = MagicType::Fireball, .offset = {-10.0f, 25.0f}},
	                   {.type = MagicType::LightningBolt, .offset = {0.0f, 25.0f}},
	                   {.type = MagicType::Food, .offset = {10.0f, 25.0f}, .charged = false}},
	});
}

/// A row of globes on their own, floating this high, this far apart, from the left
std::vector<DispenserSetup> GlobeRow(std::initializer_list<MagicType> types, float north, float spacing = 7.0f)
{
	constexpr float k_GlobeHeight = 5.0f;
	std::vector<DispenserSetup> globes;
	float east = -spacing * static_cast<float>(types.size() - 1) * 0.5f;
	for (const auto type : types)
	{
		globes.push_back({.type = type, .offset = {east, north}, .bubbleHeight = k_GlobeHeight});
		east += spacing;
	}
	return globes;
}

void AddGlobes(std::vector<Scenario>& all)
{
	auto everySeed = GlobeRow({MagicType::StormWindRain, MagicType::Forest, MagicType::Fireball, MagicType::Food,
	                           MagicType::Shield, MagicType::PhysicalShield},
	                          34.0f);
	for (auto& row : {GlobeRow({MagicType::LightningBolt, MagicType::Heal, MagicType::Wood, MagicType::Water,
	                            MagicType::FlockFlying, MagicType::FlockGround},
	                           27.0f),
	                  GlobeRow({MagicType::CreatureSpellFreeze, MagicType::CreatureSpellBig, MagicType::CreatureSpellCompassion,
	                            MagicType::CreatureSpellItchy, MagicType::Teleport, MagicType::ExplosionOne},
	                           20.0f)})
	{
		everySeed.insert(everySeed.end(), row.begin(), row.end());
	}
	all.push_back({
	    .id = "miracles.globes_every_seed",
	    .name = "A globe of every miracle",
	    .facet = Facet::Miracles,
	    .description = "One-shot globes on their own, close up, a row at a time: storm, forest, fire, food, shield, "
	                   "physical shield; lightning, heal, wood, water, flying and ground flocks; the freeze, big, nice and "
	                   "itchy phials, teleport and the beam explosion.",
	    .expected = "Each globe is added over what is behind it, faintly, a glint running over its dome. The storm, fire, "
	                "lightning, water and teleport show only their miracle's own little effect inside (no model): the "
	                "storm cloud, the fireball, the bolt's sparks, the water mist, the teleport's swirl. Shield, heal, the "
	                "freeze, nice and itchy phials show their model with their effect too. The others show their model "
	                "spinning twice a second round, a little below the middle; food and the phials shine with the "
	                "environment map, the phials' texture running and the big phial pulsing larger; the beam's model is "
	                "added without hiding what is behind it. None has rings.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 18.0f}, {20.0f, 36.0f}}, .distance = 0.55f},
	    .dispensers = everySeed,
	});

	auto extreme = GlobeRow({MagicType::Fireball, MagicType::FireballPowerUpOne, MagicType::FireballPowerUpTwo}, 34.0f);
	for (auto& row :
	     {GlobeRow({MagicType::LightningBolt, MagicType::LightningBoltPowerUpOne, MagicType::LightningBoltPowerUpTwo}, 27.0f),
	      GlobeRow({MagicType::StormWindRain, MagicType::StormWindRainLightning, MagicType::Tornado}, 20.0f)})
	{
		extreme.insert(extreme.end(), row.begin(), row.end());
	}
	all.push_back({
	    .id = "miracles.extreme_globes",
	    .name = "Extreme globes side by side",
	    .facet = Facet::Miracles,
	    .description = "Globes of fire, lightning and storm: in each row the plain miracle, then its first and its "
	                   "second power-up.",
	    .expected = "The plain globes have no rings. Each extreme globe has a faint ring in the player's colour spinning "
	                "fast round its miracle, tilted to the camera; the second power-up has two, crossing. The globes are "
	                "otherwise the same: the same dome, glint and effect inside.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-8.0f, 18.0f}, {8.0f, 36.0f}}, .distance = 0.5f},
	    .dispensers = extreme,
	});
}

void AddHand(std::vector<Scenario>& all)
{
	using Kind = Command::Kind;
	all.push_back({
	    .id = "miracles.hand_power_up_bands",
	    .name = "Bands and bracelets on the hand",
	    .facet = Facet::Miracles,
	    .description = "A lightning seed is summoned from the player's worship into the hand, powered up twice with its "
	                   "gestures, then shaken off with a scribble.",
	    .expected = "As the seed comes, five bands fly from in front of the camera onto the hand with the band's sound, "
	                "and after a moment a bracelet settles round the wrist; the hand glows in the player's colour, the glow "
	                "flowing over it. Each power-up sends five bands flying on with the sound and the announcer's voice "
	                "(power up one, then two), and puts another bracelet on further up the arm. The scribble sends a band "
	                "flying off the hand to the camera with the hand's shake; the bracelets and the glow go.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-3.0f, 27.0f}, {3.0f, 33.0f}}, .distance = 0.2f},
	    .commands =
	        {{.kind = Kind::SummonSeed, .delaySeconds = 1.0f, .value = static_cast<size_t>(SpellSeedType::LightningBolt)},
	         {.kind = Kind::DrawGesture, .delaySeconds = 4.0f, .value = static_cast<size_t>(GestureType::InverseSpiral)},
	         {.kind = Kind::DrawGesture, .delaySeconds = 4.0f, .value = static_cast<size_t>(GestureType::Spiral)},
	         {.kind = Kind::DrawGesture, .delaySeconds = 4.0f, .value = static_cast<size_t>(GestureType::Scribble)}},
	    .repeatFrom = 0,
	    .hand = HandHold {.offset = {0.0f, 25.0f}, .height = 6.0f},
	});

	all.push_back({
	    .id = "miracles.hand_hold_poses",
	    .name = "How the hand holds each seed",
	    .facet = Facet::Miracles,
	    .description = "Seeds put in the hand one after another, three seconds each: food, the forest, lightning, the ground "
	                   "flock, wood and the freeze phial; then a food seed summoned from worship.",
	    .expected =
	        "Taking each seed the hand fades over about a tenth of a second into a still pose that doesn't play: "
	        "food and wood held from the side, the hand opened round the horn of plenty or the logs hanging below "
	        "it and raised by about their height; the forest's tree held above, standing a little above the hand, "
	        "turned half round; lightning in the rest pose, the hand raised by its own height, nothing in it but "
	        "the bolt's sparks in the fingers; the wolf held above, turned a quarter round; the phial held from the side "
	        "like the horn, hanging six tenths of its height below the hand point. Moving the mouse sways "
	        "the hand and its seed together by up to about 17 degrees, rolling about the line to the camera and "
	        "pitching. The summoned food is held in the rest pose with nothing in it for a second and a half, then "
	        "snaps to the side hold with the horn showing.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-3.0f, 27.0f}, {3.0f, 33.0f}}, .distance = 0.2f},
	    .commands = {{.kind = Kind::HoldSeed, .delaySeconds = 1.0f, .value = static_cast<size_t>(SpellSeedType::Food)},
	                 {.kind = Kind::HoldSeed, .delaySeconds = 3.0f, .value = static_cast<size_t>(SpellSeedType::Nature)},
	                 {.kind = Kind::HoldSeed, .delaySeconds = 3.0f, .value = static_cast<size_t>(SpellSeedType::LightningBolt)},
	                 {.kind = Kind::HoldSeed, .delaySeconds = 3.0f, .value = static_cast<size_t>(SpellSeedType::FlockGround)},
	                 {.kind = Kind::HoldSeed, .delaySeconds = 3.0f, .value = static_cast<size_t>(SpellSeedType::Wood)},
	                 {.kind = Kind::HoldSeed,
	                  .delaySeconds = 3.0f,
	                  .value = static_cast<size_t>(SpellSeedType::CreatureSpellFreeze)},
	                 {.kind = Kind::SummonSeed, .delaySeconds = 3.0f, .value = static_cast<size_t>(SpellSeedType::Food)}},
	    .repeatFrom = 0,
	    .hand = HandHold {.offset = {0.0f, 25.0f}, .height = 6.0f},
	});

	all.push_back({
	    .id = "miracles.hand_tribal_power",
	    .name = "A tribe's power behind a miracle",
	    .facet = Facet::Miracles,
	    .description = "The player's Norse power is set to twice (a testbed set-up step; worship sets it in the game). A "
	                   "wood seed comes to a hand held over the land and is poured for three seconds, once a second after "
	                   "it comes; again every eight seconds.",
	    .expected = "As the seed comes, \"Norse Power\" flies in from the camera over a second as a ring of letters in "
	                "the player's colour, closing up from trailing one another, and settles round the hand a little above "
	                "it, turning slowly. When the wood is cast the announcer says \"Norse Power\" and the ring is let go: "
	                "its letters stand up and rise from where the hand was in a widening column, turning faster and faster, "
	                "fading after three seconds and gone within five. The wood pours twice as strongly.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-6.0f, 22.0f}, {6.0f, 34.0f}}, .distance = 0.3f},
	    .miracles = {{.type = MagicType::Wood,
	                  .point = {0.0f, 28.0f},
	                  .handOffset = {0.0f, 28.0f},
	                  .handHeight = 8.0f,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = 3.0f,
	                  .repeatSeconds = 8.0f,
	                  .byHand = true,
	                  .holdBeforePress = 2.0f}},
	    .tribalPower = std::pair {Tribe::NORSE, 2.0f},
	});

	all.push_back({
	    .id = "miracles.hand_extreme_lightning",
	    .name = "An extreme lightning held by hand",
	    .facet = Facet::Miracles,
	    .description = "The lightning's second power-up summoned from worship into a hand over the land, held for six "
	                   "seconds and then cast for six; again every fourteen.",
	    .expected = "Bands fly onto the hand from the camera as the seed comes, with the band's sound and the announcer's "
	                "\"power up two\"; after a moment two bracelets settle round the wrist and arm, spinning in the "
	                "player's colour, and the hand glows, its flow running backwards over it. Once ready, its in-hand "
	                "effect is the extreme one's (more and larger sparks). When the seed goes the bracelets go.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-10.0f, 20.0f}, {10.0f, 45.0f}}, .distance = 0.45f},
	    .miracles = {{.type = MagicType::LightningBoltPowerUpTwo,
	                  .point = {0.0f, 32.0f},
	                  .handOffset = {0.0f, 25.0f},
	                  .handHeight = 6.0f,
	                  .delaySeconds = 1.0f,
	                  .holdSeconds = 6.0f,
	                  .repeatSeconds = 15.0f,
	                  .byHand = true,
	                  .fromWorship = true,
	                  .holdBeforePress = 6.0f}},
	});
}
void AddPiles(std::vector<Scenario>& all)
{
	all.push_back({
	    .id = "miracles.food_speedup_and_stores",
	    .name = "Powered food, and a storage pit taking food and wood",
	    .facet = Facet::Miracles,
	    .description = "The food miracle's first power-up poured on open ground on the left; food and then wood poured "
	                   "beside an empty storage pit on the right, each for four seconds.",
	    .expected = "On the left a pile of grain rises out of the ground over a second with a thud, its grain flowing up "
	                "it as it rises, and sparkles keep coming off it for as long as it stands. On the right the food and "
	                "wood land in the pit's own piles, which rise out of the ground for what they hold, thudding as they "
	                "fill; no new pile is made beside the pit while it takes them.",
	    .environment = {.dispenserGrid = false},
	    .framing = {.shot = Shot::Overview, .include = {{-20.0f, 30.0f}, {20.0f, 30.0f}}, .distance = 0.7f},
	    .objects = {{.type = AbodeInfo::CelticStoragePit, .offset = {20.0f, 30.0f}, .amount = 0}},
	    .miracles = {{.type = MagicType::FoodPowerUpOne,
	                  .point = {-20.0f, 30.0f},
	                  .handOffset = {-20.0f, 30.0f},
	                  .handHeight = 12.0f,
	                  .holdSeconds = 4.0f},
	                 {.type = MagicType::Food,
	                  .point = {20.0f, 33.0f},
	                  .handOffset = {20.0f, 33.0f},
	                  .handHeight = 12.0f,
	                  .holdSeconds = 4.0f},
	                 {.type = MagicType::Wood,
	                  .point = {20.0f, 27.0f},
	                  .handOffset = {20.0f, 27.0f},
	                  .handHeight = 12.0f,
	                  .delaySeconds = 6.0f,
	                  .holdSeconds = 4.0f}},
	});
}
} // namespace

void testbed_scenarios::AddGlobeScenarios(std::vector<Scenario>& all)
{
	AddDispensers(all);
	AddGlobes(all);
	AddHand(all);
	AddPiles(all);
}

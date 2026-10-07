/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <numbers>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Camera/CameraZoomer.h"
#include "ECS/TownAggression.h"
#include "InfoConstants.h"
#include "Magic/ForestRules.h"
#include "Magic/ShieldRules.h"
#include "Magic/VillagerReactionRules.h"
#include "Particles/ParticleFlocking.h"

using namespace openblack;
using namespace openblack::magic;

namespace
{
constexpr float k_Epsilon = 1e-4f;
constexpr float k_Turn = 0.1f;

GMagicShieldInfo PhysicalTables()
{
	GMagicShieldInfo info {};
	info.minRadius = 5.0f;
	info.maxRadius = 1000.0f;
	info.radiusForNormalCost = 30.0f;
	info.chantCostPerImpactMomentum = 25.0f;
	info.shieldHeight = 0.0f;
	info.raiseWithScale = -2.0f;
	info.bobMagnitude = 3.0f;
	return info;
}
} // namespace

// The shields

TEST(ShieldRules, TheSpiritualShieldsSphereIsALittleBiggerThanItsCircle)
{
	EXPECT_NEAR(shield::SphereRadius(30.0f), 33.3186f, k_Epsilon);
	EXPECT_FLOAT_EQ(shield::SphereRadius(2000.0f), 1000.0f);
	EXPECT_FLOAT_EQ(shield::ReactionReach(30.0f), 60.0f);
}

TEST(ShieldRules, TheDomeIsSizedAndSunkByItsRadius)
{
	const auto shape = shield::MakeDomeShape(PhysicalTables(), 30.0f, 10.0f);
	EXPECT_NEAR(shape.finalScale, 0.51f, k_Epsilon);
	EXPECT_NEAR(shape.startScale, 0.0051f, k_Epsilon);
	// The hand's spin, no faster than 3 either way, slowing to 0.15 of its sign
	EXPECT_FLOAT_EQ(shape.startSpin, 3.0f);
	EXPECT_FLOAT_EQ(shape.endSpin, 0.15f);
	EXPECT_FLOAT_EQ(shield::MakeDomeShape(PhysicalTables(), 30.0f, -1.0f).endSpin, -0.15f);
}

TEST(ShieldRules, TheDomeIsHiddenThenGrowsSpinsDownAndBobs)
{
	const auto shape = shield::MakeDomeShape(PhysicalTables(), 30.0f, 2.0f);
	shield::DomeState state;
	// Hidden for its first half second, and not yet turning
	auto pose = shield::StepDome(shape, state, 0.2f, k_Turn);
	EXPECT_FALSE(pose.drawn);
	EXPECT_FLOAT_EQ(state.angle, 0.0f);
	// Half way through growing it is at the ease's middle
	pose = shield::StepDome(shape, state, 0.5f + 0.75f, k_Turn);
	EXPECT_TRUE(pose.drawn);
	const float half = shield::Ease(0.5f);
	EXPECT_NEAR(pose.scale, shape.startScale + (shape.finalScale - shape.startScale) * half, k_Epsilon);
	EXPECT_GT(state.angle, 0.0f);
	// Grown, it is full size, sunk by twice its scale and bobbing up to one and a half times it
	pose = shield::StepDome(shape, state, 3.0f, k_Turn);
	EXPECT_FLOAT_EQ(pose.scale, shape.finalScale);
	EXPECT_GE(pose.height, -1.02f - k_Epsilon);
	EXPECT_LE(pose.height, -1.02f + 1.53f + k_Epsilon);
	// Long after, it turns at its end spin
	const float before = state.angle;
	EXPECT_TRUE(shield::StepDome(shape, state, 10.0f, k_Turn).drawn);
	EXPECT_NEAR(state.angle - before, 0.15f * k_Turn, k_Epsilon);
	EXPECT_FLOAT_EQ(shield::Ease(1.0f), 1.0f);
}

TEST(ShieldRules, ADyingDomeFadesOverASecondAndAHalfAndGoesAtTwoAndAQuarter)
{
	const auto shape = shield::MakeDomeShape(PhysicalTables(), 30.0f, 0.0f);
	shield::DomeState state;
	state.dying = true;
	auto pose = shield::StepDome(shape, state, 10.0f, 0.75f);
	EXPECT_FALSE(pose.gone);
	EXPECT_NEAR(pose.scale, shape.startScale + (shape.finalScale - shape.startScale) * 0.5f, k_Epsilon);
	pose = shield::StepDome(shape, state, 10.75f, 0.75f);
	EXPECT_FALSE(pose.gone);
	EXPECT_NEAR(pose.scale, shape.startScale, k_Epsilon);
	pose = shield::StepDome(shape, state, 11.5f, 0.75f + 0.01f);
	EXPECT_TRUE(pose.gone);
	// Its effect keeps its alpha until the fade is over
	EXPECT_FLOAT_EQ(shield::DyingEffectAlpha(200.0f, 1.0f), 200.0f);
	EXPECT_FLOAT_EQ(shield::DyingEffectAlpha(200.0f, 1.6f), 0.0f);
}

TEST(ShieldRules, TheDomeIsDrawnByItsStrengthNeverFainterThanForty)
{
	EXPECT_FLOAT_EQ(shield::DomeAlpha(1.0f), 255.0f);
	EXPECT_FLOAT_EQ(shield::DomeAlpha(2.0f), 255.0f);
	EXPECT_FLOAT_EQ(shield::DomeAlpha(0.5f), 127.0f);
	EXPECT_FLOAT_EQ(shield::DomeAlpha(0.0f), 40.0f);
	EXPECT_FALSE(shield::NeedsRescale(1.0f, 0.8f));
	EXPECT_TRUE(shield::NeedsRescale(1.0f, 0.6f));
}

TEST(ShieldRules, ABlowCostsTheDomeByItsMomentum)
{
	EXPECT_FLOAT_EQ(shield::ImpactCost(25.0f, 10.0f, 100.0f), 2.5f);
	EXPECT_FLOAT_EQ(shield::ImpactCost(0.0f, 10.0f, 100.0f), 0.0f);
}

TEST(ShieldRules, TheDomesSolidShapeIsACone)
{
	const auto volume = shield::VolumeOf({60.0f, 25.0f, 60.0f}, 0.5f);
	EXPECT_FLOAT_EQ(volume.radius, 30.0f);
	EXPECT_FLOAT_EQ(volume.height, 25.0f);
	EXPECT_TRUE(shield::InsideDome(volume, 0.0f, 24.0f));
	EXPECT_FALSE(shield::InsideDome(volume, 0.0f, 26.0f));
	EXPECT_TRUE(shield::InsideDome(volume, 15.0f, 12.0f));
	EXPECT_FALSE(shield::InsideDome(volume, 15.0f, 13.0f));
	EXPECT_FALSE(shield::InsideDome(volume, 31.0f, 0.0f));
}

TEST(ShieldRules, TheDomesSolidShapeIsSetAtItsFullSizeAndSunk)
{
	const std::array<std::array<glm::vec3, 3>, 1> model {
	    {{glm::vec3(0.0f), glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(0.0f, 10.0f, 0.0f)}}};
	const auto shape = shield::MakeDomeShape(PhysicalTables(), 100.0f, 0.0f);
	const auto hull = shield::DomeHull(model, {50.0f, 20.0f, 60.0f}, shape);
	ASSERT_EQ(hull.size(), 1u);
	// Raised by its raise with scale taken at a scale of 1: 2 into the land, at 1.7 times the model
	EXPECT_NEAR(hull[0][0].y, 18.0f, k_Epsilon);
	EXPECT_NEAR(hull[0][1].x, 50.0f + 17.0f, k_Epsilon);
}

TEST(ShieldRules, AThingCrossesTheDomeOnlyMovingAgainstAFace)
{
	// One face looking up the x axis
	const std::array<std::array<glm::vec3, 3>, 1> hull {
	    {{glm::vec3(0.0f, -5.0f, -5.0f), glm::vec3(0.0f, 5.0f, -5.0f), glm::vec3(0.0f, 0.0f, 5.0f)}}};
	const auto n = glm::normalize(glm::cross(hull[0][1] - hull[0][0], hull[0][2] - hull[0][0]));
	const auto in = shield::CrossHull(hull, n * 2.0f, -n * 2.0f);
	ASSERT_TRUE(in.has_value());
	EXPECT_NEAR(glm::length(in->point), 0.0f, k_Epsilon);
	EXPECT_NEAR(glm::dot(in->normal, n), 1.0f, k_Epsilon);
	// From inside it passes out freely
	EXPECT_FALSE(shield::CrossHull(hull, -n * 2.0f, n * 2.0f).has_value());
	// Beside the face it misses
	EXPECT_FALSE(
	    shield::CrossHull(hull, n * 2.0f + glm::vec3(0.0f, 20.0f, 0.0f), -n * 2.0f + glm::vec3(0.0f, 20.0f, 0.0f)).has_value());
	EXPECT_FLOAT_EQ(shield::ThrownMass(10.0f, 2.0f), 80.0f);
	EXPECT_FLOAT_EQ(shield::ThrownMass(0.0f, 1.0f), 0.01f);
}

TEST(ShieldRules, VillagersShelterWellInsideOnTheirOwnSide)
{
	EXPECT_TRUE(shield::NeedsShelter(25.0f, 30.0f));
	EXPECT_FALSE(shield::NeedsShelter(23.0f, 30.0f));
	// With no turn, no random distance, it goes straight towards the middle to 0.8 of the radius
	const auto place = shield::ShelterAt({0.0f, 0.0f}, {100.0f, 0.0f}, 30.0f, shield::k_ShelterTurnRange * 0.5f, 0.0f,
	                                     shield::k_ShelterTurnRange * 0.5f);
	EXPECT_NEAR(place.point.x, 24.0f, k_Epsilon);
	EXPECT_NEAR(place.point.y, 0.0f, k_Epsilon);
	EXPECT_NEAR(place.faceTowards.x, 25.0f, k_Epsilon);
	// The whole random cube brings it to the middle
	EXPECT_NEAR(glm::length(shield::ShelterAt({0.0f, 0.0f}, {100.0f, 0.0f}, 30.0f, 0.0f, 1.0f, 0.0f).point), 0.0f, k_Epsilon);
	EXPECT_TRUE(shield::IsUnder({10.0f, 0.0f}, {0.0f, 0.0f}, 30.0f, 0.0f));
	EXPECT_FALSE(shield::IsUnder({10.0f, 0.0f}, {0.0f, 0.0f}, 30.0f, 25.0f));
}

TEST(ShieldRules, ASheltersAnimationIsPointingLookingOrStanding)
{
	EXPECT_EQ(shield::AmazedAnimation(0), 286u);
	EXPECT_EQ(shield::AmazedAnimation(1), 311u);
	EXPECT_EQ(shield::AmazedAnimation(2), 311u);
	EXPECT_EQ(shield::AmazedAnimation(3), 385u);
	EXPECT_EQ(shield::AmazedAnimation(4), 385u);
}

TEST(ShieldRules, VillagersOfATownReactOnlyWhenItWasAttackedLately)
{
	EXPECT_EQ(shield::VillagerPriority(70, false, 0.0f, 9999, 100), 70);
	EXPECT_EQ(shield::VillagerPriority(70, true, 0.0f, 0, 100), 0);
	EXPECT_EQ(shield::VillagerPriority(70, true, 1.0f, 100, 100), 70);
	EXPECT_EQ(shield::VillagerPriority(70, true, 1.0f, 101, 100), 0);
	// Kept under the shield for ever while the attack is fresh, three turns in four
	EXPECT_EQ(shield::VillagerReactTurns(true, 1, 10, 50, 100, 80), villager_reaction::k_Forever);
	EXPECT_EQ(shield::VillagerReactTurns(true, 0, 10, 50, 100, 80), 80u);
	EXPECT_EQ(shield::VillagerReactTurns(true, 2, 60, 50, 100, 80), 80u);
	EXPECT_EQ(shield::VillagerReactTurns(false, 1, 0, 0, 100, 80), 80u);
	EXPECT_EQ(shield::VillagerAgainTurns(true, 101, 100, true, 30), villager_reaction::k_Forever);
	EXPECT_EQ(shield::VillagerAgainTurns(true, 10, 100, false, 30), 0u);
	EXPECT_EQ(shield::VillagerAgainTurns(false, 9999, 100, true, 30), 30u);
	const auto look = shield::LookOut({0.0f, 0.0f}, {10.0f, 0.0f}, shield::k_ShelterTurnRange * 0.5f);
	EXPECT_NEAR(look.x, 11.0f, k_Epsilon);
}

TEST(ShieldRules, OtherPlayersCreaturesWalkRoundAShield)
{
	EXPECT_TRUE(shield::CreatureMustAvoid(PlayerNames::PLAYER_TWO, PlayerNames::PLAYER_ONE, false));
	EXPECT_FALSE(shield::CreatureMustAvoid(PlayerNames::PLAYER_ONE, PlayerNames::PLAYER_ONE, false));
	EXPECT_FALSE(shield::CreatureMustAvoid(PlayerNames::PLAYER_TWO, PlayerNames::PLAYER_ONE, true));
}

TEST(ShieldRules, ADestroyedShieldImpressesFourTimesAndAnAttackersShieldNotAtAll)
{
	EXPECT_FLOAT_EQ(shield::ImpressiveMultiplier(Reaction::ReactToMagicShieldDestroyed, false, false), 4.0f);
	EXPECT_FLOAT_EQ(shield::ImpressiveMultiplier(Reaction::ReactToMagicShieldDestroyed, false, true), 1.0f);
	EXPECT_FLOAT_EQ(shield::ImpressiveMultiplier(Reaction::ReactToMagicShield, true, true), 0.0f);
	EXPECT_FLOAT_EQ(shield::ImpressiveMultiplier(Reaction::ReactToMagicShieldStruck, true, false), 0.0f);
	EXPECT_FLOAT_EQ(shield::ImpressiveMultiplier(Reaction::ReactToMagicShieldStruck, true, true), 1.0f);
}

// The forest

TEST(ForestRules, EighteenTreesSpiralOutFromTwoToEleven)
{
	EXPECT_NEAR(glm::length(forest::SpiralOffset(0, 18)), 2.0f, k_Epsilon);
	EXPECT_NEAR(glm::length(forest::SpiralOffset(17, 18)), 11.0f, k_Epsilon);
	// Each tree turns 18 * 17/13 / 17 of a turn on from the last: 138.46 degrees
	const auto first = forest::SpiralOffset(1, 18);
	const auto second = forest::SpiralOffset(2, 18);
	const float a1 = std::atan2(first.y, first.x);
	const float a2 = std::atan2(second.y, second.x);
	float step = std::fmod(a2 - a1 + (4.0f * std::numbers::pi_v<float>), 2.0f * std::numbers::pi_v<float>);
	EXPECT_NEAR(step * 180.0f / std::numbers::pi_v<float>, 138.46f, 0.05f);
	// One tree goes in the middle's ring at its start
	EXPECT_NEAR(glm::length(forest::SpiralOffset(0, 1)), 2.0f, k_Epsilon);
}

TEST(ForestRules, TreesFurtherOutGrowSmaller)
{
	EXPECT_FLOAT_EQ(forest::TargetScale(0.0f), 1.0f);
	EXPECT_NEAR(forest::TargetScale(2.0f), 0.90909f, k_Epsilon);
	EXPECT_FLOAT_EQ(forest::TargetScale(11.0f), 0.5f);
}

TEST(ForestRules, ItAffordsItsTreesWhileItHasStrength)
{
	EXPECT_EQ(forest::TreesWeCanAfford(-1, 18, 1.0f), 18u);
	EXPECT_EQ(forest::TreesWeCanAfford(5, 18, 1.0f), 5u);
	EXPECT_EQ(forest::TreesWeCanAfford(-1, 18, 0.0f), 0u);
	EXPECT_FLOAT_EQ(forest::Upkeep(5.0f, 1.0f, 18), 23.0f);
	EXPECT_EQ(forest::MaxObjectsToCreate(-1, 18, false, std::nullopt), 18u);
	EXPECT_EQ(forest::MaxObjectsToCreate(-1, 18, true, std::nullopt), 0u);
	EXPECT_EQ(forest::MaxObjectsToCreate(-1, 18, true, 7u), 7u);
}

TEST(ForestRules, TheGroundsKindsOfTreeAreRolledFor)
{
	const std::array kinds {TreeInfo::Palm, TreeInfo::PalmA, TreeInfo::PalmB, TreeInfo::PalmC};
	EXPECT_EQ(forest::SpeciesFor(kinds, 0), TreeInfo::Palm);
	EXPECT_EQ(forest::SpeciesFor(kinds, 3), TreeInfo::PalmC);
	EXPECT_EQ(forest::SpeciesFor(kinds, 9), TreeInfo::PalmC);
}

TEST(ForestRules, TreesGrowToTheirSizeAndWitherAway)
{
	EXPECT_FLOAT_EQ(forest::Grow(0.5f, 0.01f, 1.0f), 0.51f);
	EXPECT_FLOAT_EQ(forest::Grow(0.995f, 0.01f, 1.0f), 1.0f);
	EXPECT_EQ(forest::Wither(0.5f, 0.05f), std::optional<float>(0.45f));
	EXPECT_FALSE(forest::Wither(0.05f, 0.05f).has_value());
	// About 91 turns grows a tree at 2 from the middle from nothing
	float scale = 0.0f;
	int turns = 0;
	while (scale < forest::TargetScale(2.0f))
	{
		scale = forest::Grow(scale, 0.01f, forest::TargetScale(2.0f));
		++turns;
	}
	EXPECT_EQ(turns, 91);
}

TEST(ForestRules, AMagicTreeGivesAQuarterOfTheWoodByTribalPower)
{
	EXPECT_FLOAT_EQ(forest::WoodMultiplier(0.25f, 1.0f), 0.25f);
	EXPECT_FLOAT_EQ(forest::WoodMultiplier(0.25f, 2.0f), 0.5f);
	EXPECT_FLOAT_EQ(forest::WoodValue(1.0f, 0.25f, 1.0f, 100.0f), 25.0f);
	EXPECT_TRUE(forest::BatsFor(-0.6f));
	EXPECT_FALSE(forest::BatsFor(-0.5f));
}

// The forest's effect

TEST(ForestEffect, TheFlocksCircleOnASquashedSphere)
{
	const auto start = particles::forest_path::PathPoint(11.0f, 0.0f, 0.14f, 0.2f, 0.0f, 0.0f, {1.0f, 0.2f, 1.0f});
	EXPECT_NEAR(start.x, 11.0f, k_Epsilon);
	EXPECT_NEAR(start.y, 0.0f, k_Epsilon);
	const auto later =
	    particles::forest_path::PathPoint(11.0f, 0.0f, 0.14f, 0.2f, 0.0f, std::numbers::pi_v<float> / 2.0f, {1.0f, 0.2f, 1.0f});
	EXPECT_NEAR(later.y, 2.2f, k_Epsilon);
}

TEST(ForestEffect, AFlocksPullFallsOffByItsType)
{
	EXPECT_FLOAT_EQ(particles::flocking::Falloff(2.0f, 0, false, 0.4f), 1.0f);
	EXPECT_FLOAT_EQ(particles::flocking::Falloff(2.0f, 1, false, 0.5f), 1.0f);
	EXPECT_FLOAT_EQ(particles::flocking::Falloff(2.0f, 2, false, 1.0f), 0.25f);
	EXPECT_FLOAT_EQ(particles::flocking::Falloff(2.0f, 2, true, 1.0f), 4.0f);
	// Never closer than a hundredth
	EXPECT_FLOAT_EQ(particles::flocking::Falloff(0.0f, 1, true, 1.0f), 0.01f);
	// Flying level along x it faces along x
	const auto level = particles::flocking::Banking({1.0f, 0.0f, 0.0f}, glm::vec3(0.0f), 0.5f, -2.0f);
	EXPECT_NEAR(glm::length(level[0]), 1.0f, k_Epsilon);
	EXPECT_NEAR(glm::dot(level[1], glm::vec3(0.0f, 1.0f, 0.0f)), 1.0f, k_Epsilon);
}

// How villagers take up reactions

TEST(VillagerReaction, NearerReactionsAreMoreUrgent)
{
	const villager_reaction::Distance reach {.maxDistance = 60.0f, .importance = 1.0f};
	EXPECT_EQ(villager_reaction::Priority(100, true, reach, 0.0f), 150);
	EXPECT_EQ(villager_reaction::Priority(100, true, reach, 60.0f), 100);
	EXPECT_EQ(villager_reaction::Priority(100, true, reach, 61.0f), 0);
	EXPECT_EQ(villager_reaction::Priority(100, false, reach, 0.0f), 0);
	EXPECT_EQ(villager_reaction::Priority(200, true, reach, 0.0f), 255);
	EXPECT_FLOAT_EQ(villager_reaction::SpreadDistance({0.0f, 0.0f}, {10.0f, -4.0f}), 7.0f);
}

TEST(VillagerReaction, AVillagerTakesAKindUpAgainOnlyAfterItsCooldown)
{
	villager_reaction::Memory memory;
	EXPECT_TRUE(memory.MayReactAgain(13, 100, 50));
	EXPECT_EQ(memory.LastReacted(13), 100u);
	EXPECT_FALSE(memory.MayReactAgain(13, 140, 50));
	EXPECT_TRUE(memory.MayReactAgain(13, 151, 50));
	// A fourth kind pushes out the oldest
	EXPECT_TRUE(memory.MayReactAgain(1, 160, 0));
	EXPECT_TRUE(memory.MayReactAgain(2, 170, 0));
	EXPECT_TRUE(memory.MayReactAgain(3, 180, 0));
	EXPECT_EQ(memory.LastReacted(13), 0u);
	// The kind looked for isn't forgotten, however old
	EXPECT_FALSE(memory.MayReactAgain(1, 2000, 10000));
	// Other kinds older than 1800 turns are forgotten as they are passed on the way to it, those after it kept
	EXPECT_TRUE(memory.MayReactAgain(2, 2000, 0));
	EXPECT_EQ(memory.LastReacted(1), 0u);
	EXPECT_EQ(memory.LastReacted(3), 180u);
}

// A town's memory of attacks

TEST(TownAggression, AnAttackAddsByItsMultiplierWhichWearsDown)
{
	ecs::town_aggression::Record record;
	ecs::town_aggression::Attacked(record, PlayerNames::PLAYER_TWO, false, 1.0f, 0.5f, 100);
	EXPECT_FLOAT_EQ(record.aggression.at(static_cast<size_t>(PlayerNames::PLAYER_TWO)), 1.5f);
	EXPECT_FLOAT_EQ(record.protectionMultiplier, 0.9f);
	EXPECT_EQ(record.lastAggressor, PlayerNames::PLAYER_TWO);
	EXPECT_EQ(record.lastTurn, 100u);
	ecs::town_aggression::Attacked(record, PlayerNames::PLAYER_TWO, false, 1.0f, 0.5f, 101);
	EXPECT_FLOAT_EQ(record.aggression.at(static_cast<size_t>(PlayerNames::PLAYER_TWO)), 2.4f);
	// Its own player's attacks are its want of mercy
	ecs::town_aggression::Attacked(record, PlayerNames::PLAYER_ONE, true, 1.0f, 0.0f, 102);
	ecs::town_aggression::ProcessTurn(record, PlayerNames::PLAYER_ONE);
	EXPECT_NEAR(record.protection, 2.4f * 0.999f, k_Epsilon);
	EXPECT_NEAR(record.mercy, 0.999f, k_Epsilon);
	EXPECT_FLOAT_EQ(record.mercyMultiplier, 0.9f);
	// A multiplier recovers while its want is small, twice over as the game works it out
	ecs::town_aggression::Record calm;
	calm.protectionMultiplier = 0.5f;
	ecs::town_aggression::ProcessTurn(calm, PlayerNames::NEUTRAL);
	EXPECT_NEAR(calm.protectionMultiplier, 0.5f * 1.001f * 1.001f, 1e-6f);
}

// The camera along the forest's path

TEST(CameraZoomer, AGlideArrivesStillAtItsDestinationAfterItsTime)
{
	camera::Zoomer zoomer;
	zoomer.SetPosition(0.0f);
	zoomer.SetDestination(10.0f, 0.0f, 2.0f);
	zoomer.Update(1.0f);
	EXPECT_GT(zoomer.Value(), 0.0f);
	EXPECT_LT(zoomer.Value(), 10.0f);
	zoomer.Update(0.999f);
	EXPECT_NEAR(zoomer.Value(), 10.0f, 0.01f);
	EXPECT_NEAR(zoomer.Speed(), 0.0f, 0.1f);
	zoomer.Update(1.0f);
	EXPECT_FLOAT_EQ(zoomer.Value(), 10.0f);
	// Sent on, it starts from where it is at the speed it has
	zoomer.SetDestination(0.0f, 0.0f, 0.0005f);
	EXPECT_FLOAT_EQ(zoomer.Value(), 0.0f);
}

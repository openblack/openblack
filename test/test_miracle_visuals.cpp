/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <gtest/gtest.h>

#include "Magic/HandMotion.h"
#include "Magic/MiracleVisuals.h"
#include "Particles/ParticleMiracleMaths.h"

using namespace openblack::magic;
using namespace openblack::magic::visuals;

namespace
{
constexpr float k_Epsilon = 1e-4f;
}

TEST(MiracleVisuals, TheGlintRunsThroughItsSixteenCellsEighteenASecond)
{
	EXPECT_NEAR(StepGlint(0.0f, 0.5f), 9.0f, k_Epsilon);
	EXPECT_NEAR(StepGlint(15.0f, 0.1f), 0.8f, k_Epsilon);
	// Cell 6 is the third across and the second down
	EXPECT_NEAR(GlintUvOffset(6.4f).x, 0.5f, k_Epsilon);
	EXPECT_NEAR(GlintUvOffset(6.4f).y, 0.25f, k_Epsilon);
	EXPECT_NEAR(GlintUvOffset(15.9f).y, 0.75f, k_Epsilon);
}

TEST(MiracleVisuals, AnExtremeGlobeHasARingForEachPowerUp)
{
	EXPECT_EQ(RingCount(-1), 0);
	EXPECT_EQ(RingCount(0), 1);
	EXPECT_EQ(RingCount(1), 2);
	// 60 of 256 of the globe's 150, and of an icon's full alpha
	EXPECT_EQ(RingAlpha(150), 35);
	EXPECT_EQ(RingAlpha(255), 59);
}

TEST(MiracleVisuals, TheRingsAreTurnedAsTheGameTurnsThem)
{
	// Unspun, the first ring is laid flat, tipped, turned back a radian and tipped again: still a turn, and the two rings
	// lie differently
	const auto first = RingTurn(0, 0.0f);
	const auto second = RingTurn(1, 0.0f);
	EXPECT_NEAR(std::abs(glm::determinant(first)), 1.0f, k_Epsilon);
	EXPECT_GT(glm::length(first[0] - second[0]), 0.1f);
	// Spinning turns it about its own axis: the axis it spins about stays put
	const auto spun = RingTurn(0, 1.0f);
	EXPECT_GT(glm::length(spun[0] - first[0]), 0.1f);
	// Its model sits on the point, scaled by a fifth of the miracle's size
	const auto model = RingModel(0, 0.0f, {1.0f, 2.0f, 3.0f}, 0.6f, {1.0f, 2.0f, 13.0f});
	EXPECT_NEAR(model[3].y, 2.0f, k_Epsilon);
	EXPECT_NEAR(glm::length(glm::vec3(model[0])), 0.12f, k_Epsilon);
}

TEST(MiracleVisuals, RingsAndGlobesFaceTheCamera)
{
	// Seen from further along z and above, the ring's first axis points back to the camera and its second is upright
	const glm::vec3 point {0.0f, 0.0f, 0.0f};
	const glm::vec3 camera {0.0f, 10.0f, 10.0f};
	const auto ring = FacingCamera(point, camera, Facing::Ring);
	EXPECT_NEAR(glm::dot(ring[0], glm::normalize(camera - point)), 1.0f, k_Epsilon);
	EXPECT_NEAR(glm::dot(ring[1], ring[0]), 0.0f, k_Epsilon);
	EXPECT_GT(ring[1].y, 0.0f);
	// The globe's second axis points to the camera and its third is the ring's upright
	const auto globe = FacingCamera(point, camera, Facing::Globe);
	EXPECT_NEAR(glm::dot(globe[1], glm::normalize(camera - point)), 1.0f, k_Epsilon);
	EXPECT_NEAR(glm::length(globe[2] - ring[1]), 0.0f, k_Epsilon);
	EXPECT_NEAR(std::abs(glm::determinant(globe)), 1.0f, k_Epsilon);
}

TEST(MiracleVisuals, PhialsRunBackwardsAndPulse)
{
	EXPECT_NEAR(StepPhialFrame(0.0f, 0.2f), 29.0f, k_Epsilon);
	EXPECT_NEAR(PhialUvOffset(9.0f).x, 0.125f, k_Epsilon);
	EXPECT_NEAR(PhialUvOffset(9.0f).y, 0.125f, k_Epsilon);
	EXPECT_NEAR(PhialPulseSpeed(0), 0.35f, k_Epsilon);
	EXPECT_NEAR(PhialPulseSpeed(5), 0.5f, k_Epsilon);
	// At the top of the pulse one kind swells across and through to two and a half times, another squashes; the height
	// and the rest stay
	EXPECT_NEAR(PhialScale(5, 0.25f).x, 2.5f, k_Epsilon);
	EXPECT_NEAR(PhialScale(5, 0.25f).y, 1.0f, k_Epsilon);
	EXPECT_NEAR(PhialScale(5, 0.25f).z, 2.5f, k_Epsilon);
	EXPECT_NEAR(PhialScale(6, 0.25f).x, 0.2f, k_Epsilon);
	EXPECT_NEAR(PhialScale(6, 0.25f).y, 1.0f, k_Epsilon);
	EXPECT_NEAR(PhialScale(6, 0.25f).z, 0.3f, k_Epsilon);
	EXPECT_NEAR(PhialScale(1, 0.25f).x, 1.0f, k_Epsilon);
}

TEST(MiracleVisuals, BraceletsCountThePowerUpsAndWaitForTheMiracleToSettle)
{
	HandBands bands;
	SetBracelets(bands, 0, true);
	EXPECT_TRUE(bands.bracelets.empty());
	SetBracelets(bands, 2, true);
	ASSERT_EQ(bands.bracelets.size(), 2u);
	EXPECT_EQ(bands.bracelets[1].index, 2);
	EXPECT_FALSE(PoseOf(bands.bracelets[0]).shown);
	StepBands(bands, 2.4f + 0.425f);
	const auto halfway = PoseOf(bands.bracelets[0]);
	EXPECT_TRUE(halfway.shown);
	EXPECT_NEAR(halfway.t, 0.5f, 1e-3f);
	EXPECT_NEAR(halfway.alpha, 75, 1);
	// They stay once on
	StepBands(bands, 5.0f);
	EXPECT_EQ(bands.bracelets.size(), 2u);
	EXPECT_EQ(PoseOf(bands.bracelets[0]).alpha, 130);
	SetBracelets(bands, 9, false);
	EXPECT_EQ(bands.bracelets.size(), static_cast<size_t>(k_MostBracelets));
	SetBracelets(bands, 0, false);
	EXPECT_TRUE(bands.bracelets.empty());
}

TEST(MiracleVisuals, BandsFlyOnAndOffAndGo)
{
	HandBands bands;
	AddFlyIn(bands, false);
	ASSERT_EQ(bands.flying.size(), 5u);
	EXPECT_NEAR(bands.flying[4].delay, 0.5f, k_Epsilon);
	StepBands(bands, 0.2f);
	EXPECT_TRUE(PoseOf(bands.flying[0]).shown);
	EXPECT_FALSE(PoseOf(bands.flying[2]).shown);
	// The last ends, is drawn once there, and goes the step after
	StepBands(bands, 1.2f);
	ASSERT_FALSE(bands.flying.empty());
	EXPECT_TRUE(bands.flying.back().done);
	StepBands(bands, 0.01f);
	EXPECT_TRUE(bands.flying.empty());
	// The band flying off starts on the hand and ends at the camera, fading from 50 to 5
	AddFlyOff(bands);
	StepBands(bands, 0.01f);
	EXPECT_NEAR(PoseOf(bands.flying[0]).t, 0.99f, 1e-3f);
	EXPECT_EQ(PoseOf(bands.flying[0]).alpha, 49);
	StepBands(bands, 1.0f);
	StepBands(bands, 0.01f);
	EXPECT_TRUE(bands.flying.empty());
}

TEST(MiracleVisuals, BandsSpinRoundTheHandFurtherUpTheArmFaster)
{
	HandBand band {.kind = BandKind::Bracelet, .index = 0, .delay = 0.0f};
	HandBands bands {.bracelets = {band}, .flying = {}};
	bands.bracelets.push_back({.kind = BandKind::Bracelet, .index = 1});
	StepBands(bands, 0.1f);
	EXPECT_NEAR(bands.bracelets[0].spin, 1.2f, k_Epsilon);
	EXPECT_NEAR(bands.bracelets[1].spin, 1.44f, k_Epsilon);
	// Round the hand at its radius, along it by its place
	const auto onHand = BandOnHand(bands.bracelets[1]);
	EXPECT_NEAR(onHand[3].z, 50.0f, k_Epsilon);
	EXPECT_NEAR(glm::length(glm::vec3(onHand[0])), 10.0f, k_Epsilon);
	// In front of the camera, beyond the near plane
	EXPECT_NEAR(BandAtCamera(1.0f)[3].z, 4.0f, k_Epsilon);
	EXPECT_NEAR(BandAtCamera(5.0f)[3].z, 5.2f, k_Epsilon);
	// Half way, half the size between
	const auto between = BandBetween(BandAtCamera(1.0f), onHand, 0.5f);
	EXPECT_NEAR(glm::length(glm::vec3(between[0])), 5.25f, 1e-3f);
}

TEST(MiracleVisuals, TheGlowFlowsBackwards)
{
	// Below nothing it comes back into two runs of its 32 cells, and shows the same cell
	EXPECT_NEAR(StepGlowFrame(0.0f, 0.1f), 62.0f, k_Epsilon);
	EXPECT_NEAR(GlowUvOffset(62.0f).x, 0.75f, k_Epsilon);
	EXPECT_NEAR(GlowUvOffset(62.0f).y, 0.375f, k_Epsilon);
	EXPECT_NEAR(GlowUvOffset(30.0f).x, 0.75f, k_Epsilon);
}

TEST(MiracleVisuals, TheAnnouncerNamesThePowerUp)
{
	EXPECT_FALSE(PowerUpVoiceSample(-1).has_value());
	EXPECT_EQ(PowerUpVoiceSample(0), 10);
	EXPECT_EQ(PowerUpVoiceSample(2), 12);
}

TEST(MiracleVisuals, APourThatClampsTheHandKeepsItWhereItBegan)
{
	PourState pour;
	StartPour(pour, k_FoodWoodPour, {1.0f, 2.0f, 3.0f});
	StepPour(pour, 0.1f);
	ASSERT_TRUE(PourPoseAt(pour, 1.0f).pinned.has_value());
	EXPECT_NEAR(PourPoseAt(pour, 1.0f).pinned->z, 3.0f, k_Epsilon);
	StopPour(pour);
	EXPECT_FALSE(PourPoseAt(pour, 1.0f).pinned.has_value());
	StartPour(pour, k_WaterPour, {1.0f, 2.0f, 3.0f});
	EXPECT_FALSE(PourPoseAt(pour, 1.0f).pinned.has_value());
	// The water lifts the hand without tipping it, over eight seconds
	StepPour(pour, 1.6f);
	EXPECT_NEAR(PourPoseAt(pour, 1.0f).raise, 8.0f, 1e-3f);
	EXPECT_NEAR(PourPoseAt(pour, 1.0f).tilt, 0.0f, k_Epsilon);
}

TEST(MiracleVisuals, AFireballIsHeardRushingPastTheCamera)
{
	using openblack::particles::maths::RushedPastCamera;
	const glm::vec3 camera {0.0f, 50.0f, 0.0f};
	// Coming into the 40 round the camera, fast
	EXPECT_TRUE(RushedPastCamera({0.0f, 50.0f, 41.0f}, {0.0f, 50.0f, 39.0f}, {0.0f, 0.0f, -30.0f}, camera));
	// Too slow, already close, or going away from it
	EXPECT_FALSE(RushedPastCamera({0.0f, 50.0f, 41.0f}, {0.0f, 50.0f, 39.0f}, {0.0f, 0.0f, -10.0f}, camera));
	EXPECT_FALSE(RushedPastCamera({0.0f, 50.0f, 35.0f}, {0.0f, 50.0f, 30.0f}, {0.0f, 0.0f, -30.0f}, camera));
	EXPECT_FALSE(RushedPastCamera({0.0f, 50.0f, 39.0f}, {0.0f, 50.0f, 41.0f}, {0.0f, 0.0f, 30.0f}, camera));
}

TEST(MiracleVisuals, TheHandSpinsAMiracleByHowItsWayTurns)
{
	// Round a circle at a radian a second, turning from x towards z: the spin settles at a radian a second
	constexpr float k_Step = 1.0f / 60.0f;
	constexpr float k_Radius = 10.0f;
	HandSpin spin;
	glm::vec3 velocity(0.0f);
	glm::vec3 last(k_Radius, 0.0f, 0.0f);
	for (int frame = 1; frame <= 600; ++frame)
	{
		const float angle = static_cast<float>(frame) * k_Step;
		const glm::vec3 at(k_Radius * std::cos(angle), 0.0f, k_Radius * std::sin(angle));
		velocity = FilterHandVelocity(velocity, (at - last) / k_Step, k_Step);
		StepHandSpin(spin, at - last, velocity, k_Step);
		last = at;
	}
	EXPECT_NEAR(spin.spin, 1.0f, 0.02f);
	// Straight on, it doesn't spin
	HandSpin straight;
	for (int frame = 0; frame < 120; ++frame)
	{
		StepHandSpin(straight, {0.5f, 0.0f, 0.0f}, {30.0f, 0.0f, 0.0f}, k_Step);
	}
	EXPECT_NEAR(straight.spin, 0.0f, 1e-5f);
}

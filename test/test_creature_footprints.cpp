/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <numbers>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <gtest/gtest.h>

#include "Creature/CreatureAudio.h"
#include "Creature/CreatureFootprints.h"

using namespace openblack;
using namespace openblack::creature_footprints;

namespace
{
constexpr float k_Epsilon = 1e-4f;
constexpr float k_Pi = std::numbers::pi_v<float>;

/// Land that slopes up along x
float Slope(float x, float /*z*/)
{
	return 0.5f * x;
}

void ExpectNear(const glm::vec3& actual, const glm::vec3& expected)
{
	EXPECT_NEAR(actual.x, expected.x, k_Epsilon);
	EXPECT_NEAR(actual.y, expected.y, k_Epsilon);
	EXPECT_NEAR(actual.z, expected.z, k_Epsilon);
}

void ExpectNear(const glm::vec2& actual, const glm::vec2& expected)
{
	EXPECT_NEAR(actual.x, expected.x, k_Epsilon);
	EXPECT_NEAR(actual.y, expected.y, k_Epsilon);
}

Footprint PrintWithAlpha(uint8_t alpha)
{
	Footprint print {};
	print.alpha = alpha;
	return print;
}
} // namespace

TEST(CreatureFootprints, FootstepsLeavePrints)
{
	EXPECT_FALSE(creature_audio::IsFootstep(audio::SoundAction::BreatheOut));
	EXPECT_TRUE(creature_audio::IsFootstep(audio::SoundAction::FootstepLight));
	EXPECT_TRUE(creature_audio::IsFootstep(audio::SoundAction::FootstepNormal));
	EXPECT_TRUE(creature_audio::IsFootstep(audio::SoundAction::FootStamp));
	EXPECT_FALSE(creature_audio::IsFootstep(audio::SoundAction::Scream));
}

TEST(CreatureFootprints, SpeciesCells)
{
	EXPECT_EQ(PrintOf(CreatureType::GiantApe, false), (SpeciesPrint {.cell = 0, .scale = 2.0f}));
	EXPECT_EQ(PrintOf(CreatureType::Unknown, false), (SpeciesPrint {.cell = 0, .scale = 2.0f}));
	for (const auto species : {CreatureType::Chimp, CreatureType::Ogre, CreatureType::Mandrill, CreatureType::Gorilla})
	{
		EXPECT_EQ(PrintOf(species, false), (SpeciesPrint {.cell = 0, .scale = 1.0f}));
	}
	for (const auto species : {CreatureType::Tiger, CreatureType::Leopard, CreatureType::Wolf, CreatureType::Lion})
	{
		EXPECT_EQ(PrintOf(species, false), (SpeciesPrint {.cell = 4, .scale = 1.0f}));
	}
	for (const auto species : {CreatureType::Cow, CreatureType::Sheep, CreatureType::Rhino})
	{
		EXPECT_EQ(PrintOf(species, false), (SpeciesPrint {.cell = 2, .scale = 1.0f}));
	}
	EXPECT_EQ(PrintOf(CreatureType::Horse, false).cell, 3);
	EXPECT_EQ(PrintOf(CreatureType::BrownBear, false).cell, 1);
	EXPECT_EQ(PrintOf(CreatureType::PolarBear, false).cell, 1);
	EXPECT_EQ(PrintOf(CreatureType::Tortoise, false).cell, 5);
	EXPECT_EQ(PrintOf(CreatureType::Zebra, false).cell, 6);
}

TEST(CreatureFootprints, AprilFoolsSmileyKeepsTheSpeciesSize)
{
	EXPECT_EQ(PrintOf(CreatureType::Tiger, true), (SpeciesPrint {.cell = k_SmileyCell, .scale = 1.0f}));
	EXPECT_EQ(PrintOf(CreatureType::GiantApe, true), (SpeciesPrint {.cell = k_SmileyCell, .scale = 2.0f}));
	EXPECT_TRUE(IsAprilFools(4, 1));
	EXPECT_FALSE(IsAprilFools(4, 2));
	EXPECT_FALSE(IsAprilFools(1, 4));
}

TEST(CreatureFootprints, Side)
{
	EXPECT_NEAR(Side(1.0f, PrintOf(CreatureType::Tiger, false)), 2.4f, k_Epsilon);
	EXPECT_NEAR(Side(1.5f, PrintOf(CreatureType::Cow, false)), 3.6f, k_Epsilon);
	EXPECT_NEAR(Side(1.0f, PrintOf(CreatureType::GiantApe, false)), 4.8f, k_Epsilon);
}

TEST(CreatureFootprints, LowerFootGetsThePrint)
{
	const Foot right {.position = {0.0f, 1.0f, 0.0f}, .yaw = 0.0f};
	const Foot lowLeft {.position = {1.0f, 0.5f, 0.0f}, .yaw = 0.0f};
	const Foot highLeft {.position = {1.0f, 1.5f, 0.0f}, .yaw = 0.0f};
	EXPECT_TRUE(LeftIsLower(right, lowLeft));
	EXPECT_FALSE(LeftIsLower(right, highLeft));
	// Level feet favour the right
	EXPECT_FALSE(LeftIsLower(right, Foot {.position = {1.0f, 1.0f, 0.0f}, .yaw = 0.0f}));
}

TEST(CreatureFootprints, FootReadFromItsMatrix)
{
	const auto world = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 4.0f, 5.0f)) * glm::eulerAngleYXZ(0.7f, 0.2f, 0.1f) *
	                   glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
	const auto foot = FootOf(world);
	ExpectNear(foot.position, {3.0f, 4.0f, 5.0f});
	// A turn about y of t takes the z axis to (sin t, 0, cos t), which the game's yaw reads as -t
	const auto turned = FootOf(glm::eulerAngleY(0.7f));
	EXPECT_NEAR(turned.yaw, -0.7f, k_Epsilon);
	EXPECT_NEAR(FootOf(glm::mat4(1.0f)).yaw, 0.0f, k_Epsilon);
	// Pitch and roll after the turn leave the yaw as it is, and so does the scale
	EXPECT_NEAR(foot.yaw, turned.yaw, k_Epsilon);
}

TEST(CreatureFootprints, UnturnedPrint)
{
	const auto print = MakeFootprint({10.0f, 0.0f, 20.0f}, 0.0f, 0.6f, 4, false, Slope);
	ExpectNear(print.corners[0], {9.7f, Slope(9.7f, 0.0f) + k_Lift, 20.3f});
	ExpectNear(print.corners[1], {10.3f, Slope(10.3f, 0.0f) + k_Lift, 20.3f});
	ExpectNear(print.corners[2], {10.3f, Slope(10.3f, 0.0f) + k_Lift, 19.7f});
	ExpectNear(print.corners[3], {9.7f, Slope(9.7f, 0.0f) + k_Lift, 19.7f});
	EXPECT_EQ(print.alpha, k_StartAlpha);
	// The cat's paw, the fifth cell of the top row
	ExpectNear(print.uvs[0], {0.5f, 0.0f});
	ExpectNear(print.uvs[1], {0.625f, 0.0f});
	ExpectNear(print.uvs[2], {0.625f, 0.125f});
	ExpectNear(print.uvs[3], {0.5f, 0.125f});
}

TEST(CreatureFootprints, TurnedAndSizedPrint)
{
	// A quarter turn takes the picture's +x to +z; a side of 2.4 puts the corners 1.2 out
	const auto print = MakeFootprint({0.0f, 7.0f, 0.0f}, k_Pi / 2.0f, 2.4f, 0, false, [](float, float) { return 3.0f; });
	ExpectNear(print.corners[0], {-1.2f, 3.15f, -1.2f});
	ExpectNear(print.corners[1], {-1.2f, 3.15f, 1.2f});
	ExpectNear(print.corners[2], {1.2f, 3.15f, 1.2f});
	ExpectNear(print.corners[3], {1.2f, 3.15f, -1.2f});
}

TEST(CreatureFootprints, LeftFootFlipsThePicture)
{
	const auto right = MakeFootprint({0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 2, false, Slope);
	const auto left = MakeFootprint({0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 2, true, Slope);
	for (size_t i = 0; i < 4; ++i)
	{
		ExpectNear(left.corners.at(i), right.corners.at(i));
		EXPECT_NEAR(left.uvs.at(i).x, right.uvs.at(i).x, k_Epsilon);
		EXPECT_NEAR(left.uvs.at(i).y, 0.125f - right.uvs.at(i).y, k_Epsilon);
	}
}

TEST(CreatureFootprints, FullTrailDropsNewPrints)
{
	Trail trail;
	for (size_t i = 0; i < k_Capacity; ++i)
	{
		EXPECT_TRUE(Add(trail, PrintWithAlpha(static_cast<uint8_t>(1 + (i % 100)))));
	}
	EXPECT_FALSE(Add(trail, PrintWithAlpha(77)));
	ASSERT_EQ(trail.prints.size(), k_Capacity);
	EXPECT_EQ(trail.prints.back().alpha, 1 + ((k_Capacity - 1) % 100));
}

TEST(CreatureFootprints, FadesInStepsOfAtLeast200Ms)
{
	Trail trail;
	Add(trail, PrintWithAlpha(k_StartAlpha));
	Fade(trail, 150.0f);
	EXPECT_EQ(trail.prints[0].alpha, k_StartAlpha);
	// 216 ms have gone by: 5.4 is taken off and the fraction dropped
	Fade(trail, 66.0f);
	EXPECT_EQ(trail.prints[0].alpha, 122);
	EXPECT_EQ(trail.sinceFadeMs, 0.0f);
	Fade(trail, 200.0f);
	EXPECT_EQ(trail.prints[0].alpha, 117);
}

TEST(CreatureFootprints, GoneInAboutFiveSeconds)
{
	Trail trail;
	Add(trail, PrintWithAlpha(k_StartAlpha));
	int frames = 0;
	while (!trail.prints.empty() && frames < 1000)
	{
		Fade(trail, 1000.0f / 30.0f);
		++frames;
	}
	const auto seconds = static_cast<float>(frames) / 30.0f;
	EXPECT_GT(seconds, 4.8f);
	EXPECT_LT(seconds, 5.6f);
}

TEST(CreatureFootprints, FadedPrintsAreTakenAwayInOrder)
{
	Trail trail;
	for (const uint8_t alpha : {3, 50, 4, 9, 2, 60})
	{
		Add(trail, PrintWithAlpha(alpha));
	}
	Fade(trail, 200.0f);
	ASSERT_EQ(trail.prints.size(), 3);
	EXPECT_EQ(trail.prints[0].alpha, 45);
	EXPECT_EQ(trail.prints[1].alpha, 4);
	EXPECT_EQ(trail.prints[2].alpha, 55);
	// A full trail has room again once prints have faded
	Trail full;
	for (size_t i = 0; i < k_Capacity; ++i)
	{
		Add(full, PrintWithAlpha(i == 0 ? 1 : k_StartAlpha));
	}
	Fade(full, 200.0f);
	EXPECT_TRUE(Add(full, PrintWithAlpha(k_StartAlpha)));
}

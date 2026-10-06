/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>
#include <cstdint>

#include <numbers>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Creature/CreatureEyes.h"

using namespace openblack;
using namespace openblack::creature_eyes;

namespace
{
constexpr float k_Tolerance = 1e-4f;
constexpr auto k_Pi = std::numbers::pi_v<float>;
constexpr LidAngles k_Lids {.open = 0.45f, .closed = -0.5f, .calm = 0.25f};

/// Always the top of the range, so the wait between blinks is the longest
uint32_t Highest(uint32_t range)
{
	return range - 1;
}
} // namespace

TEST(CreatureEyes, BlinksCloseAndOpenOverTwoHundredMillisecondsEach)
{
	Blink blink;
	blink = AdvanceBlink(blink, 999, Highest);
	EXPECT_EQ(blink.state, Blink::State::Open);
	blink = AdvanceBlink(blink, 1, Highest);
	EXPECT_EQ(blink.state, Blink::State::Closing);
	EXPECT_EQ(blink.timerMs, 200);
	blink = AdvanceBlink(blink, 200, Highest);
	EXPECT_EQ(blink.state, Blink::State::Opening);
	blink = AdvanceBlink(blink, 250, Highest);
	EXPECT_EQ(blink.state, Blink::State::Open);
	// Up to one and a half times the interval until the next
	EXPECT_EQ(blink.timerMs, 4999 + 2500);
}

TEST(CreatureEyes, ThePointIsOnTheTriangleAndItsNormalPointsIn)
{
	const auto point = PointOnTriangle({0, 0, 0}, {2, 0, 0}, {0, 2, 0}, 0.25f, 0.5f);
	EXPECT_NEAR(point.position.x, 0.5f, k_Tolerance);
	EXPECT_NEAR(point.position.y, 1.0f, k_Tolerance);
	// The third edge crossed with the second
	EXPECT_NEAR(point.normal.z, -4.0f, k_Tolerance);
}

TEST(CreatureEyes, SmallCreaturesHaveBiggerEyesForTheirSize)
{
	// At size 1: a tenth, by 1.3 for the head and 1.0 for the eyes
	EXPECT_NEAR(EyeSize(1.0f, 1.0f, 0.0f, Mode::Calm), 0.13f, k_Tolerance);
	EXPECT_NEAR(EyeSize(2.0f, 1.0f, 0.0f, Mode::Calm), 0.2f * 0.8f * 0.9f, k_Tolerance);
	EXPECT_NEAR(EyeSize(1.0f, 1.0f, 1.0f, Mode::Calm), 0.13f * 1.05f, k_Tolerance);
	EXPECT_NEAR(EyeSize(1.0f, 1.0f, 0.0f, Mode::Stoned), 0.13f * 1.1f, k_Tolerance);
}

TEST(CreatureEyes, OpenEyesTurnFaster)
{
	EXPECT_NEAR(LookSeconds(0.0f, Mode::Calm), 0.15f, k_Tolerance);
	EXPECT_NEAR(LookSeconds(1.0f, Mode::Calm), 0.1f, k_Tolerance);
	EXPECT_NEAR(LookSeconds(0.0f, Mode::Stoned), 0.75f, k_Tolerance);
}

TEST(CreatureEyes, EyesNeverLookBackIntoTheHead)
{
	const glm::vec3 inward {0.0f, 0.0f, 1.0f};
	// Looking along the surface is pulled a little inwards
	const auto clamped = ClampLook({1.0f, 0.0f, 0.0f}, inward);
	EXPECT_NEAR(glm::length(clamped), 1.0f, k_Tolerance);
	EXPECT_NEAR(clamped.z, 0.1f / std::sqrt(1.01f), k_Tolerance);
	// Looking well inwards is left alone
	const auto ahead = ClampLook({0.0f, 0.6f, 0.8f}, inward);
	EXPECT_NEAR(ahead.y, 0.6f, k_Tolerance);
	EXPECT_NEAR(ahead.z, 0.8f, k_Tolerance);
}

TEST(CreatureEyes, TheEyeballFacesAwayFromWhereItLooks)
{
	const auto frame = EyeballFrame({1, 2, 3}, {0.0f, 0.0f, 1.0f});
	// Turned half round about z
	EXPECT_NEAR(frame.x.x, -1.0f, k_Tolerance);
	EXPECT_NEAR(frame.y.y, -1.0f, k_Tolerance);
	EXPECT_NEAR(frame.z.z, 1.0f, k_Tolerance);
	const auto matrix = ToMatrix(frame, 0.5f);
	// The pupil, at -2 along z of the mesh, ends up half that far along -z from the centre
	const auto pupil = matrix * glm::vec4(0.0f, 0.0f, -2.0f, 1.0f);
	EXPECT_NEAR(pupil.z, 2.0f, k_Tolerance);
	EXPECT_NEAR(pupil.x, 1.0f, k_Tolerance);
}

TEST(CreatureEyes, TheLeftAndRightLidsTurnTowardsTheirAnchors)
{
	const glm::vec3 inward {0.0f, 0.0f, 1.0f};
	const auto left = EyelidFrame({0, 0, 0}, inward, {1, 0, 0}, false);
	const auto right = EyelidFrame({0, 0, 0}, inward, {1, 0, 0}, true);
	EXPECT_NEAR(left.z.z, -1.0f, k_Tolerance);
	// Squared up through the lid's y, which leaves its x pointing away from the anchor on the left
	EXPECT_NEAR(left.x.x, -1.0f, k_Tolerance);
	EXPECT_NEAR(right.x.x, 1.0f, k_Tolerance);
	EXPECT_NEAR(glm::dot(left.x, left.y), 0.0f, k_Tolerance);
}

TEST(CreatureEyes, LidAnglesByMode)
{
	const Blink open;
	EXPECT_NEAR(EyelidAngle(Mode::Wide, k_Lids, 0.0f, 0.0f, open), 0.45f * k_Pi, k_Tolerance);
	EXPECT_NEAR(EyelidAngle(Mode::Closed, k_Lids, 0.0f, 0.0f, open), -0.5f * k_Pi, k_Tolerance);
	EXPECT_NEAR(EyelidAngle(Mode::Calm, k_Lids, 0.0f, 0.0f, open), 0.25f * k_Pi, k_Tolerance);
	// A calm lid follows the pupil and opens with the eyes
	EXPECT_NEAR(EyelidAngle(Mode::Calm, k_Lids, 0.5f, 0.5f, open), (0.25f * k_Pi) - std::asin(0.5f) + 0.15f, k_Tolerance);
}

TEST(CreatureEyes, CalmLidsSwingShutOverABlink)
{
	const auto calm = 0.25f * k_Pi;
	const auto closed = -0.5f * k_Pi;
	const Blink starting {.state = Blink::State::Closing, .timerMs = 200, .intervalMs = 5000};
	const Blink halfShut {.state = Blink::State::Closing, .timerMs = 100, .intervalMs = 5000};
	const Blink shut {.state = Blink::State::Opening, .timerMs = 200, .intervalMs = 5000};
	EXPECT_NEAR(EyelidAngle(Mode::Calm, k_Lids, 0.0f, 0.0f, starting), calm, k_Tolerance);
	EXPECT_NEAR(EyelidAngle(Mode::Calm, k_Lids, 0.0f, 0.0f, halfShut), (calm + closed) / 2.0f, k_Tolerance);
	EXPECT_NEAR(EyelidAngle(Mode::Calm, k_Lids, 0.0f, 0.0f, shut), closed, k_Tolerance);
	// Wide open eyes don't blink
	EXPECT_NEAR(EyelidAngle(Mode::Wide, k_Lids, 0.0f, 0.0f, halfShut), 0.45f * k_Pi, k_Tolerance);
}

TEST(CreatureEyes, TurningALidKeepsItsAxesSquare)
{
	const auto lid = Turned(EyelidFrame({0, 0, 0}, {0, 0, 1}, {1, 0, 0}, false), 0.7f);
	EXPECT_NEAR(glm::dot(lid.y, lid.z), 0.0f, k_Tolerance);
	EXPECT_NEAR(glm::length(lid.y), 1.0f, k_Tolerance);
	EXPECT_NEAR(LidPitch(lid, EyeballFrame({0, 0, 0}, lid.y)), 1.0f, k_Tolerance);
}

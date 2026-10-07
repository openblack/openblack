/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Magic/TribalPowerSpin.h"

using namespace openblack::magic::tribal_spin;

namespace
{
constexpr float k_Epsilon = 1e-4f;
constexpr float k_Frame = 0.05f;

Runner::Frame HandAt(glm::vec3 position)
{
	return {.handPosition = position,
	        .handZ = {0.0f, -2.0f, 0.0f},
	        .camera = {0.0f, 20.0f, -30.0f},
	        .cameraFocus = {0.0f, 0.0f, 0.0f}};
}
} // namespace

TEST(TribalPowerSpin, NothingShowsUntilTheRingHasTwoSamples)
{
	Runner runner(u"Norse Power", glm::vec3(0.0f), {255, 0, 0, 255}, true);
	EXPECT_TRUE(runner.GetSpin().Letters().empty());
	EXPECT_TRUE(runner.Update(HandAt({0.0f, 10.0f, 0.0f}), k_Frame));
	EXPECT_TRUE(runner.GetSpin().Letters().empty());
}

TEST(TribalPowerSpin, TheRingSettlesRoundTheHandAboveIt)
{
	Runner runner(u"Norse Power", glm::vec3(0.0f), {255, 0, 0, 255}, true);
	const glm::vec3 hand {5.0f, 10.0f, 5.0f};
	for (int frame = 0; frame < 60; ++frame)
	{
		EXPECT_TRUE(runner.Update(HandAt(hand), k_Frame));
	}
	const auto letters = runner.GetSpin().Letters();
	// Ten letters, the space skipped, all fully seen and a share of the ring's radius high
	ASSERT_EQ(letters.size(), 10u);
	const float radius = std::sin(runner.Age() * 0.1f) * 0.5f + 1.3f;
	for (const auto& letter : letters)
	{
		EXPECT_EQ(letter.alpha, 255);
		EXPECT_EQ(letter.down, 2);
		// Half the hand's height above the hand, each letter starting on the ring
		const glm::vec3 centre = hand + glm::vec3(0.0f, 1.0f, 0.0f);
		EXPECT_NEAR(letter.origin.y, centre.y, 1e-3f);
		EXPECT_NEAR(glm::distance(letter.origin, centre), radius, 0.02f);
		EXPECT_NEAR(letter.size, glm::distance(letter.origin, centre) * 0.6f, 0.02f);
		EXPECT_NEAR(glm::length(letter.axes[0]), 1.0f, k_Epsilon);
	}
}

TEST(TribalPowerSpin, ItFliesInFromTheCamera)
{
	Runner runner(u"Celtic Power", glm::vec3(0.0f), {255, 0, 0, 255}, true);
	const auto frame = HandAt({0.0f, 10.0f, 0.0f});
	for (int step = 0; step < 4; ++step)
	{
		(void)runner.Update(frame, k_Frame);
	}
	const auto letters = runner.GetSpin().Letters();
	ASSERT_FALSE(letters.empty());
	// Still close to the camera a fifth of a second in
	EXPECT_LT(glm::distance(letters.front().origin, frame.camera), glm::distance(letters.front().origin, frame.handPosition));
}

TEST(TribalPowerSpin, LetGoItRisesAsAColumnAndIsGoneAfterFourSecondsAndAHalf)
{
	Runner runner(u"Norse Power", glm::vec3(0.0f), {255, 0, 0, 255}, true);
	for (int frame = 0; frame < 40; ++frame)
	{
		(void)runner.Update(HandAt({0.0f, 10.0f, 0.0f}), k_Frame);
	}
	runner.Release({0.0f, 10.0f, 0.0f});
	EXPECT_FALSE(runner.Held());
	int frames = 0;
	float highest = 0.0f;
	while (runner.Update(HandAt({0.0f, 10.0f, 0.0f}), k_Frame))
	{
		++frames;
		for (const auto& letter : runner.GetSpin().Letters())
		{
			EXPECT_EQ(letter.down, 1);
			highest = std::max(highest, letter.origin.y);
		}
	}
	// 4.8 seconds of 0.05
	EXPECT_NEAR(static_cast<float>(frames) * k_Frame, 4.8f, 0.06f);
	// Risen by the age cubed
	EXPECT_GT(highest, 10.0f + 4.0f * 4.0f * 4.0f);
}

TEST(TribalPowerSpin, TheColumnFadesAfterThreeSeconds)
{
	Runner runner(u"Aztec Power", glm::vec3(0.0f), {255, 0, 0, 255}, false);
	for (int frame = 0; frame < 70; ++frame)
	{
		ASSERT_TRUE(runner.Update(HandAt({0.0f, 0.0f, 0.0f}), k_Frame));
	}
	// 3.5 seconds in: the samples taken a little earlier are about half faded
	const auto letters = runner.GetSpin().Letters();
	ASSERT_FALSE(letters.empty());
	EXPECT_LT(letters.front().alpha, 255);
	EXPECT_GT(letters.front().alpha, 0);
}

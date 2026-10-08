/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Creature/CreatureSway.h"

using namespace openblack;
using namespace openblack::creature_sway;

TEST(CreatureSway, AKickHighOnTheBodySwaysTheUpperBodyAcrossTheLand)
{
	Sway sway;
	sway.frameSeconds = 0.05f;
	Kick(sway, glm::vec3(1000.0f, 500.0f, 0.0f), 10.0f, 9.0f, 1000.0f);
	EXPECT_TRUE(sway.active);
	EXPECT_EQ(sway.lowerVelocity, glm::vec3(0.0f));
	// The force over the frame for half the mass, and nothing upwards
	EXPECT_FLOAT_EQ(sway.upperVelocity.x, 0.1f);
	EXPECT_FLOAT_EQ(sway.upperVelocity.y, 0.0f);
}

TEST(CreatureSway, AKickLowOnTheBodySwaysTheLowerBodyNoFasterThanItsLimit)
{
	Sway sway;
	sway.frameSeconds = 1.0f;
	Kick(sway, glm::vec3(0.0f, 0.0f, 1e6f), 2.0f, 9.0f, 100.0f);
	EXPECT_FLOAT_EQ(glm::length(sway.lowerVelocity), k_MaxKickSpeed);
	EXPECT_EQ(sway.upperVelocity, glm::vec3(0.0f));
}

TEST(CreatureSway, TheSpringSwingsBackAndSettles)
{
	Sway sway;
	sway.frameSeconds = 0.02f;
	Kick(sway, glm::vec3(2e5f, 0.0f, 0.0f), 0.0f, 9.0f, 1000.0f);
	bool wentOut = false;
	bool cameBack = false;
	for (int frame = 0; frame < 2000 && sway.active; ++frame)
	{
		Step(sway, glm::vec3(0.0f), 0.02f, 1000.0f);
		wentOut = wentOut || sway.lowerOffset.x > 0.05f;
		cameBack = cameBack || (wentOut && sway.lowerOffset.x < 0.0f);
	}
	EXPECT_TRUE(wentOut);
	EXPECT_TRUE(cameBack);
	EXPECT_FALSE(sway.active);
}

TEST(CreatureSway, ASettledBodyIsNotStepped)
{
	Sway sway;
	sway.lowerOffset = glm::vec3(1.0f, 0.0f, 0.0f);
	Step(sway, glm::vec3(10.0f), 0.1f, 1000.0f);
	EXPECT_EQ(sway.lowerOffset, glm::vec3(1.0f, 0.0f, 0.0f));
	EXPECT_EQ(sway.drive, glm::vec3(0.0f));
}

TEST(CreatureSway, TheLeashPullsTowardsTheHandAndItsDragFades)
{
	const auto force = LeashForce(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 20.0f), 100.0f, 1.0f);
	EXPECT_FLOAT_EQ(force.z, 1500.0f);
	EXPECT_EQ(LeashForce(glm::vec3(1.0f), glm::vec3(1.0f), 100.0f, 1.0f), glm::vec3(0.0f));
	EXPECT_FLOAT_EQ(FadeLeashDrag(1.0f), 0.95f);
	EXPECT_EQ(FadeLeashDrag(0.31f), 0.0f);
	Sway sway;
	SetLeashDrag(sway, 0.1f);
	EXPECT_EQ(sway.leashDrag, 0.0f);
	SetLeashDrag(sway, 1.0f);
	EXPECT_EQ(sway.leashDrag, 1.0f);
}

TEST(CreatureSway, ALeanPicksHowFarThroughItsAnimation)
{
	EXPECT_EQ(LeanTime(0.0f, 1000), 500u);
	EXPECT_EQ(LeanTime(-3.0f, 1000), 0u);
	EXPECT_EQ(LeanTime(1.0f, 1000), 1000u);
	EXPECT_EQ(LeanTime(0.5f, 1000), 750u);
}

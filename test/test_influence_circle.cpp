/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <vector>

#include <gtest/gtest.h>

#include "3D/InfluenceCircle.h"

using namespace openblack;

namespace
{
influence::Ground Flat(float height)
{
	return [height](glm::vec2) { return height; };
}
} // namespace

TEST(InfluenceCircle, CurtainSteps)
{
	// 2 pi 100 x 0.05 = 31 steps, a column each and one closing the ring, three rows each
	const auto curtain = influence::MakeCurtain(Flat(10.0f), {0.0f, 0.0f, 0.0f}, 100.0f);
	ASSERT_EQ(curtain.positions.size(), 32u * 3u);
	EXPECT_EQ(curtain.indices.size(), 31u * 12u);
	// The first column at angle 0, on the ground, 20 and 40 over it
	EXPECT_FLOAT_EQ(curtain.positions[0].x, 100.0f);
	EXPECT_FLOAT_EQ(curtain.positions[0].y, 10.0f);
	EXPECT_FLOAT_EQ(curtain.positions[1].y, 30.0f);
	EXPECT_FLOAT_EQ(curtain.positions[2].y, 50.0f);
	// The texture's rows
	EXPECT_FLOAT_EQ(curtain.uvs[1].y - curtain.uvs[0].y, 0.2f);
	EXPECT_FLOAT_EQ(curtain.uvs[2].y - curtain.uvs[0].y, 0.4f);
	// The ring closes where it started
	EXPECT_FLOAT_EQ(curtain.positions[31 * 3].x, 100.0f);
	EXPECT_FLOAT_EQ(curtain.positions[31 * 3].z, 0.0f);
}

TEST(InfluenceCircle, SmallAndLargeCircles)
{
	EXPECT_EQ(influence::MakeCurtain(Flat(0.0f), {}, 5.0f).positions.size(), 9u * 3u);
	EXPECT_EQ(influence::MakeCurtain(Flat(0.0f), {}, 5000.0f).positions.size(), 251u * 3u);
}

TEST(InfluenceCircle, CurtainFollowsTheLand)
{
	const auto curtain = influence::MakeCurtain([](glm::vec2 p) { return p.x * 0.1f; }, {0.0f, 0.0f, 0.0f}, 100.0f);
	// 10 higher at x = 100 than at the centre
	EXPECT_FLOAT_EQ(curtain.positions[0].y, 10.0f);
}

TEST(InfluenceCircle, CirclesInsideOthersGo)
{
	std::vector<influence::Circle> circles;
	influence::AddCircle(circles, PlayerNames::PLAYER_ONE, {0.0f, 0.0f, 0.0f}, 300.0f, Flat(0.0f));
	influence::AddCircle(circles, PlayerNames::PLAYER_ONE, {50.0f, 0.0f, 0.0f}, 100.0f, Flat(0.0f));
	ASSERT_EQ(circles.size(), 1u);
	EXPECT_FLOAT_EQ(circles[0].radius, 300.0f);
	// Another player's circle stays
	influence::AddCircle(circles, PlayerNames::PLAYER_TWO, {50.0f, 0.0f, 0.0f}, 100.0f, Flat(0.0f));
	EXPECT_EQ(circles.size(), 2u);
}

TEST(InfluenceCircle, OverlappingCirclesHideTheirInsides)
{
	std::vector<influence::Circle> circles;
	influence::AddCircle(circles, PlayerNames::PLAYER_ONE, {0.0f, 0.0f, 0.0f}, 100.0f, Flat(0.0f));
	influence::AddCircle(circles, PlayerNames::PLAYER_ONE, {150.0f, 0.0f, 0.0f}, 100.0f, Flat(0.0f));
	ASSERT_EQ(circles.size(), 2u);
	// The newer circle's column at angle pi, x = 50, lies inside the older one; its column at angle 0 doesn't
	const auto& newer = circles[0];
	EXPECT_FALSE(newer.hidden.front());
	const auto half = (newer.Columns() - 1) / 2;
	EXPECT_LT(newer.curtain.positions[3 * half].x, 100.0f);
	EXPECT_TRUE(newer.hidden.at(half));
	// The older one's column at angle 0, x = 100, lies inside the newer
	EXPECT_TRUE(circles[1].hidden.front());
}

TEST(InfluenceCircle, ShowsFromHighEnough)
{
	EXPECT_FALSE(influence::CurtainAlpha(100.0f).has_value());
	EXPECT_EQ(influence::CurtainAlpha(150.0f), 60);
	EXPECT_EQ(influence::CurtainAlpha(200.0f), 120);
	EXPECT_EQ(influence::CurtainAlpha(5000.0f), 120);
}

TEST(InfluenceCircle, Scrolls)
{
	const auto offset = influence::ScrollOffset(5000);
	EXPECT_FLOAT_EQ(offset.x, 0.5f);
	EXPECT_FLOAT_EQ(offset.y, -1.0f);
	EXPECT_EQ(influence::ScrollOffset(15000), influence::ScrollOffset(5000));
}

TEST(InfluenceCircle, InfluenceFallsWithDistance)
{
	EXPECT_FLOAT_EQ(influence::OnRange(30.0f, 100.0f, 0.4f, 0.2f, 0.2f), 1.0f);
	// Halfway down the falling part: half of 0.8
	EXPECT_FLOAT_EQ(influence::OnRange(50.0f, 100.0f, 0.4f, 0.2f, 0.2f), 0.4f);
	// Halfway out over the last part: half of 0.2
	EXPECT_FLOAT_EQ(influence::OnRange(80.0f, 100.0f, 0.4f, 0.2f, 0.2f), 0.1f);
	EXPECT_FLOAT_EQ(influence::OnRange(100.0f, 100.0f, 0.4f, 0.2f, 0.2f), 0.0f);
}

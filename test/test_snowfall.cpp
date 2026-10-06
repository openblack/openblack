/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <numbers>
#include <vector>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "3D/Snowfall.h"

using namespace openblack;

namespace
{
/// Hands out numbers in turn, recording what each was drawn between
struct Script
{
	std::vector<float> values;
	std::vector<std::pair<float, float>> ranges;
	size_t next {0};

	snowfall::Random Random()
	{
		return [this](float a, float b) {
			ranges.emplace_back(a, b);
			return values.at(next++ % values.size());
		};
	}
};

/// Always the middle of the range
float Middle(float a, float b)
{
	return (a + b) * 0.5f;
}
} // namespace

TEST(Snowfall, PlacesAFlakeInTheGamesOrder)
{
	Script script {.values = {10.0f, 50.0f, -20.0f, 0.5f, 6.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0.1f, 0.2f, 0.3f}};
	const auto flake = snowfall::Place(script.Random(), 100.0f);
	// Its place along z, its height, then along x, each across half of 80 either way
	EXPECT_FLOAT_EQ(flake.position.z, 5.0f);
	EXPECT_FLOAT_EQ(flake.position.y, 50.0f);
	EXPECT_FLOAT_EQ(flake.position.x, -10.0f);
	EXPECT_EQ(script.ranges.at(1), std::make_pair(0.0f, 100.0f));
	EXPECT_FLOAT_EQ(flake.fall, 7.5f);
	EXPECT_FLOAT_EQ(flake.swaySpeed, 1.0f);
	EXPECT_FLOAT_EQ(flake.heading, 1.0f);
	EXPECT_FLOAT_EQ(flake.sway, 2.0f);
	// Its turns, the last drawn first
	EXPECT_FLOAT_EQ(flake.spin.z, 3.0f);
	EXPECT_FLOAT_EQ(flake.spin.x, 5.0f);
	EXPECT_FLOAT_EQ(flake.spinSpeed.z, 0.1f);
	EXPECT_FLOAT_EQ(flake.spinSpeed.x, 0.3f);
	EXPECT_EQ(script.ranges.size(), 13u);
}

TEST(Snowfall, FlakesFallDriftAndTurn)
{
	std::array<snowfall::Flake, 1> flakes {};
	flakes[0].position = {0.0f, 100.0f, 0.0f};
	flakes[0].fall = 8.0f;
	flakes[0].swaySpeed = 0.0f;
	flakes[0].sway = std::numbers::pi_v<float> * 0.5f;
	flakes[0].heading = 0.0f;
	flakes[0].spinSpeed = {1.0f, 2.0f, 3.0f};
	snowfall::Step(flakes, 0.5f, 2.0f, 160.0f, Middle);
	// 8 a second at twice the speed, for half a second; drifting 3 a second along its heading, z
	EXPECT_NEAR(flakes[0].position.y, 92.0f, 1e-4f);
	EXPECT_NEAR(flakes[0].position.z, 1.5f, 1e-5f);
	EXPECT_NEAR(flakes[0].position.x, 0.0f, 1e-5f);
	EXPECT_FLOAT_EQ(flakes[0].spin.z, 1.5f);
}

TEST(Snowfall, FlakesUnderTheGroundStartAgainAtTheTop)
{
	std::array<snowfall::Flake, 1> flakes {};
	flakes[0].position = {0.0f, snowfall::k_Bottom, 0.0f};
	snowfall::Step(flakes, 0.0f, 1.0f, 300.0f, Middle);
	EXPECT_FLOAT_EQ(flakes[0].position.y, 150.0f);
}

TEST(Snowfall, ATurnRoundTheCircleTurnsTheHeading)
{
	std::array<snowfall::Flake, 1> flakes {};
	flakes[0].position = {0.0f, 100.0f, 0.0f};
	flakes[0].swaySpeed = 1.0f;
	flakes[0].sway = 2.0f * std::numbers::pi_v<float> - 0.01f;
	flakes[0].heading = 0.1f;
	// The middle of its turn keeps the heading
	snowfall::Step(flakes, 0.1f, 1.0f, 160.0f, Middle);
	EXPECT_NEAR(flakes[0].sway, 0.09f, 1e-5f);
	EXPECT_NEAR(flakes[0].heading, 0.1f, 1e-5f);
}

TEST(Snowfall, AFlakeIsASmallSquare)
{
	snowfall::Flake flake;
	flake.position = {1.0f, 2.0f, 3.0f};
	flake.spin = {0.3f, 1.1f, 2.0f};
	const auto corners = snowfall::Corners(flake);
	EXPECT_NEAR(glm::distance(corners[0], corners[1]), 0.6f, 1e-5f);
	EXPECT_NEAR(glm::distance(corners[1], corners[2]), 0.6f, 1e-5f);
	EXPECT_NEAR(glm::distance(corners[0], corners[2]), 0.6f * std::numbers::sqrt2_v<float>, 1e-5f);
	const auto middle = (corners[0] + corners[2]) * 0.5f;
	EXPECT_NEAR(glm::distance(middle, flake.position), 0.0f, 1e-5f);
}

TEST(Snowfall, MoreSnowMoreFlakesMoreOpaque)
{
	// The camera over the corner the flakes fall about, near enough for them all
	const glm::vec2 here {40.0f, 40.0f};
	const glm::vec2 camera {0.0f, 0.0f};
	EXPECT_FALSE(snowfall::TileOf(here, 5, camera).has_value());
	const auto light = snowfall::TileOf(here, 10, camera);
	ASSERT_TRUE(light.has_value());
	EXPECT_EQ(light->alpha, 25);
	EXPECT_EQ(light->flakes, 50);
	const auto heavy = snowfall::TileOf(here, 100, camera);
	ASSERT_TRUE(heavy.has_value());
	EXPECT_EQ(heavy->alpha, 255);
	EXPECT_EQ(heavy->flakes, 256);
	// About the quarter's corner
	EXPECT_EQ(heavy->corner, glm::vec2(0.0f, 0.0f));
}

TEST(Snowfall, FainterFurtherAway)
{
	const glm::vec2 centre {120.0f, 40.0f};
	// 175 from its corner: halfway out from 50 to 300
	const auto tile = snowfall::TileOf(centre, 100, {80.0f + 175.0f, 0.0f});
	ASSERT_TRUE(tile.has_value());
	EXPECT_EQ(tile->alpha, 127);
	EXPECT_EQ(tile->flakes, 256);
	EXPECT_FALSE(snowfall::TileOf(centre, 100, {80.0f + 301.0f, 0.0f}).has_value());
}

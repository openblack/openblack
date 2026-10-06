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

#include "3D/SnowCover.h"

#define LOCATOR_IMPLEMENTATIONS
#include "ECS/Systems/Implementations/SnowSystem.h"

using namespace openblack;

namespace
{
std::vector<float> Grid(float depth = 0.0f)
{
	return std::vector<float>(snow_cover::k_Cells, depth);
}

float& Cell(std::vector<float>& grid, int x, int z)
{
	return grid.at((static_cast<size_t>(z) * snow_cover::k_GridSize) + static_cast<size_t>(x));
}

ecs::components::Storm SnowStorm(int8_t snow)
{
	ecs::components::Storm storm {};
	storm.effect.snow = snow;
	storm.currentStrength = 1.0f;
	storm.currentPosition = {400.0f, 300.0f, 400.0f};
	storm.currentInnerRadius = 80.0f;
	storm.outerRadius = 200.0f;
	return storm;
}
} // namespace

TEST(SnowCover, StormsSnowBySnowAndStrength)
{
	EXPECT_NEAR(snow_cover::StormSnowPerTurn(100, 1.0f), 0.3f, 1e-6f);
	EXPECT_NEAR(snow_cover::StormSnowPerTurn(100, 0.5f), 0.15f, 1e-6f);
	EXPECT_FLOAT_EQ(snow_cover::StormSnowPerTurn(0, 1.0f), 0.0f);
}

TEST(SnowCover, StormsSnowLessBeyondTheirInnerRadius)
{
	auto grid = Grid();
	// Centred on cell (10, 10), all of it within 2 cells, none from 5
	snow_cover::AddStorm(grid, {400.0f, 400.0f}, 80.0f, 200.0f, 1.0f);
	EXPECT_FLOAT_EQ(Cell(grid, 10, 10), 1.0f);
	EXPECT_FLOAT_EQ(Cell(grid, 12, 10), 1.0f);
	EXPECT_FLOAT_EQ(Cell(grid, 13, 10), 12.0f / 21.0f);
	EXPECT_FLOAT_EQ(Cell(grid, 14, 10), 5.0f / 21.0f);
	EXPECT_FLOAT_EQ(Cell(grid, 15, 10), 0.0f);
}

TEST(SnowCover, StormsTakeSnowAwayNearTheirEdge)
{
	auto grid = Grid(1.0f);
	// 6 cells out, 2 in: 5 across and 3 down lies beyond what is left of the outer radius once the inner is taken off
	snow_cover::AddStorm(grid, {400.0f, 400.0f}, 80.0f, 240.0f, 1.0f);
	EXPECT_FLOAT_EQ(Cell(grid, 15, 13), 1.0f - (2.0f / 32.0f));
	// And never below none
	auto bare = Grid();
	snow_cover::AddStorm(bare, {400.0f, 400.0f}, 80.0f, 240.0f, 1.0f);
	EXPECT_FLOAT_EQ(Cell(bare, 15, 13), 0.0f);
}

TEST(SnowCover, SnowLiesNoDeeperThanItsMost)
{
	auto grid = Grid(254.5f);
	snow_cover::AddStorm(grid, {400.0f, 400.0f}, 80.0f, 200.0f, 1.0f);
	EXPECT_FLOAT_EQ(Cell(grid, 10, 10), snow_cover::k_MaxDepth);
}

TEST(SnowCover, MeltsABandAtATime)
{
	auto grid = Grid(3.0f);
	snow_cover::Melting melting;
	snow_cover::Melt(grid, melting, 0.31f);
	// The second band of sixteen rows, then the third
	EXPECT_FLOAT_EQ(Cell(grid, 0, 15), 3.0f);
	EXPECT_FLOAT_EQ(Cell(grid, 5, 16), 1.0f);
	EXPECT_FLOAT_EQ(Cell(grid, 5, 31), 1.0f);
	EXPECT_FLOAT_EQ(Cell(grid, 5, 32), 3.0f);
	snow_cover::Melt(grid, melting, 0.3f);
	EXPECT_FLOAT_EQ(Cell(grid, 5, 32), 1.0f);
	// Snow of 2 or less is gone
	melting.band = 0;
	melting.clock = 0.31f;
	snow_cover::Melt(grid, melting, 0.0f);
	EXPECT_FLOAT_EQ(Cell(grid, 5, 16), 0.0f);
}

TEST(SnowCover, DepthBlendsBetweenCells)
{
	auto grid = Grid();
	Cell(grid, 1, 1) = 4.0f;
	Cell(grid, 2, 1) = 8.0f;
	EXPECT_FLOAT_EQ(snow_cover::DepthAt(grid, {60.0f, 40.0f}), 6.0f);
	EXPECT_FLOAT_EQ(snow_cover::DepthAt(grid, {-1.0f, 40.0f}), 0.0f);
	EXPECT_FLOAT_EQ(snow_cover::DepthAt(grid, {127.0f * 40.0f, 40.0f}), 0.0f);
}

TEST(SnowCover, ObjectsShowSnowBeyond20)
{
	EXPECT_EQ(snow_cover::ObjectLevel(10.0f, snow_cover::k_ObjectRate, 255), 0);
	EXPECT_EQ(snow_cover::ObjectLevel(120.0f, snow_cover::k_ObjectRate, 255), 99);
	// A field's crop, at a quarter
	EXPECT_EQ(snow_cover::ObjectLevel(120.0f, 64, 64), 25);
	EXPECT_EQ(snow_cover::ObjectLevel(255.0f, 64, 64), 58);
	// The more snow, the more of the texture shows
	EXPECT_EQ(snow_cover::ObjectThreshold(0), 250);
	EXPECT_EQ(snow_cover::ObjectThreshold(100), 150);
	EXPECT_EQ(snow_cover::ObjectThreshold(300), 0);
}

TEST(SnowCover, LandWhitensOverTheNoise)
{
	EXPECT_EQ(snow_cover::LandLevel(100.0f, 90), 1);
	EXPECT_EQ(snow_cover::LandLevel(90.0f, 100), 0);
	EXPECT_EQ(snow_cover::LandLevel(200.7f, 72), 16);
	EXPECT_EQ(snow_cover::LandLevel(255.0f, 0), 16);
	EXPECT_EQ(snow_cover::LandWhite(0), 14);
	EXPECT_EQ(snow_cover::LandWhite(2), 15);
	EXPECT_EQ(snow_cover::LandWhite(6), 14);
}

TEST(SnowSystem, StormsThatSnowLayItAndItMelts)
{
	ecs::systems::SnowSystem snow;
	const auto revision = snow.GetRevision();
	const std::vector<ecs::components::Storm> storms {SnowStorm(100), SnowStorm(0)};
	snow.ProcessTurn(storms);
	EXPECT_NEAR(snow.GetDepth({400.0f, 400.0f}), 0.3f, 1e-6f);
	EXPECT_NE(snow.GetRevision(), revision);
	// A good while later, with no more snow, it has all melted
	for (int turn = 0; turn < 100; ++turn)
	{
		snow.ProcessTurn({});
	}
	EXPECT_FLOAT_EQ(snow.GetDepth({400.0f, 400.0f}), 0.0f);
}

TEST(SnowSystem, ResetClearsTheSnow)
{
	ecs::systems::SnowSystem snow;
	const std::vector<ecs::components::Storm> storms {SnowStorm(100)};
	snow.ProcessTurn(storms);
	snow.Reset();
	EXPECT_FLOAT_EQ(snow.GetDepth({400.0f, 400.0f}), 0.0f);
}

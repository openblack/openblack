/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <chrono>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "3D/WaterRings.h"

#define LOCATOR_IMPLEMENTATIONS
#include "ECS/Systems/Implementations/WaterRingSystem.h"

using namespace openblack;

TEST(WaterRings, TheHandsSplash)
{
	const auto ring = water_rings::HandSplash({10.0f, 20.0f}, 1.5f, 0x123456u);
	EXPECT_EQ(ring.position, glm::vec3(10.0f, 0.2f, 20.0f));
	EXPECT_FLOAT_EQ(ring.growth, 7.0f);
	EXPECT_FLOAT_EQ(ring.angle, 1.5f);
	EXPECT_EQ(ring.cell, 0x30);
	EXPECT_EQ(ring.argb, 0xB0123456u);
	EXPECT_EQ(ring.age, 0u);
}

TEST(WaterRings, GrowsAndFadesOverItsLife)
{
	water_rings::Ring ring {.growth = 7.0f, .argb = 0xFFFFFFFFu};
	EXPECT_FLOAT_EQ(water_rings::HalfWidth(ring), 0.0001f);
	EXPECT_EQ(water_rings::Alpha(ring), 254);
	ASSERT_TRUE(water_rings::Advance(ring, 350.0f));
	EXPECT_EQ(ring.age, 350u);
	EXPECT_FLOAT_EQ(water_rings::HalfWidth(ring), 3.5f);
	EXPECT_EQ(water_rings::Alpha(ring), 127);
	EXPECT_FALSE(water_rings::Advance(ring, 350.0f));
}

TEST(WaterRings, AgesAtItsRateInWholeMilliseconds)
{
	water_rings::Ring ring {.rate = 0.5f};
	ASSERT_TRUE(water_rings::Advance(ring, 33.0f));
	EXPECT_EQ(ring.age, 16u);
}

TEST(WaterRings, AFlatSquareTurnedAboutItsMiddle)
{
	water_rings::Ring ring {.position = {5.0f, 0.2f, 5.0f}, .age = 700, .growth = 1.0f, .angle = 0.7f, .aspect = 2.0f};
	const auto corners = water_rings::Corners(ring);
	for (const auto& corner : corners)
	{
		EXPECT_FLOAT_EQ(corner.y, 0.2f);
	}
	// Twice as deep as wide
	EXPECT_NEAR(glm::distance(corners[0], corners[1]), 2.0f, 1e-5f);
	EXPECT_NEAR(glm::distance(corners[1], corners[2]), 4.0f, 1e-5f);
	EXPECT_NEAR(glm::distance((corners[0] + corners[2]) * 0.5f, ring.position), 0.0f, 1e-5f);
}

TEST(WaterRings, CellsOfTheSmokeTexture)
{
	const auto uvs = water_rings::CellUvs(0x30);
	EXPECT_EQ(uvs[0], glm::vec2(0.0f, 0.75f));
	EXPECT_EQ(uvs[2], glm::vec2(0.125f, 0.875f));
	// Only the low six bits pick the cell
	EXPECT_EQ(water_rings::CellUvs(0x7F)[0], water_rings::CellUvs(0x3F)[0]);
}

TEST(WaterRingSystem, HoldsAtMostItsRingsAndLetsThemGo)
{
	ecs::systems::WaterRingSystem rings;
	for (size_t i = 0; i < water_rings::k_MostRings; ++i)
	{
		ASSERT_TRUE(rings.Add({}));
	}
	EXPECT_FALSE(rings.Add({}));
	rings.Update(std::chrono::duration<float, std::milli>(699.0f));
	EXPECT_EQ(rings.GetRings().size(), water_rings::k_MostRings);
	rings.Update(std::chrono::duration<float, std::milli>(1.0f));
	EXPECT_TRUE(rings.GetRings().empty());
	ASSERT_TRUE(rings.Add({}));
	rings.Reset();
	EXPECT_TRUE(rings.GetRings().empty());
}

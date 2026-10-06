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

#include "3D/LandNormal.h"

namespace land_normal = openblack::land_normal;

TEST(LandNormal, AFlatCellPointsUp)
{
	for (const bool split : {false, true})
	{
		const auto n = land_normal::OfCell(0x4000, 0x8000, split, 10, 10, 10, 10);
		EXPECT_FLOAT_EQ(n.x, 0.0f);
		EXPECT_NEAR(n.y, 1.0f, 1e-3f);
		EXPECT_FLOAT_EQ(n.z, 0.0f);
	}
}

TEST(LandNormal, ASlopeLeansAwayFromTheRise)
{
	// Rising towards +x
	const auto n = land_normal::OfCell(0x8000, 0x8000, false, 0, 0, 15, 15);
	EXPECT_LT(n.x, 0.0f);
	EXPECT_GT(n.y, 0.0f);
	EXPECT_NEAR(n.z, 0.0f, 1e-6f);
	EXPECT_NEAR(glm::length(n), 1.0f, 2e-3f);
	// 15 units of 0.67 over a 10 m cell
	EXPECT_NEAR(-n.x / n.y, 15.0f * 0.67f / 10.0f, 1e-3f);
}

TEST(LandNormal, TheSplitPicksTheTriangle)
{
	// Only the far corner is raised: the near triangle is flat when the cell is split from x + 1 to z + 1
	const auto nearCorner = land_normal::OfCell(0x1000, 0x1000, true, 0, 0, 0, 20);
	EXPECT_NEAR(nearCorner.y, 1.0f, 1e-3f);
	const auto farCorner = land_normal::OfCell(0xF000, 0xF000, true, 0, 0, 0, 20);
	EXPECT_LT(farCorner.y, 0.99f);
}

TEST(LandNormal, TheTablesFollowTheirFormulas)
{
	EXPECT_FLOAT_EQ(land_normal::EdgeScale(0), 0.1f);
	EXPECT_FLOAT_EQ(land_normal::LengthScale(0), 1.0f);
	EXPECT_NEAR(land_normal::LengthScale(1023), 1.0f, 1e-3f);
}

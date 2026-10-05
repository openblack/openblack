/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

#include "Graphics/SeaRows.h"

namespace sea_rows = openblack::graphics::sea_rows;

namespace
{
constexpr glm::vec2 k_Viewport {1280.0f, 720.0f};
constexpr float k_Near = 1.0f;

glm::mat4 ViewProjection(const glm::vec3& eye, const glm::vec3& focus)
{
	const auto projection = glm::perspective(glm::radians(60.0f), k_Viewport.x / k_Viewport.y, k_Near, 100000.0f);
	return projection * glm::lookAt(eye, focus, {0.0f, 1.0f, 0.0f});
}
} // namespace

TEST(SeaRows, PeriodFollowsTheWaterTiling)
{
	EXPECT_FLOAT_EQ(sea_rows::Period(0.0f), 2000.0f);
	EXPECT_FLOAT_EQ(sea_rows::Period(0.8f), 560.0f);
	EXPECT_FLOAT_EQ(sea_rows::Period(1.0f), 200.0f);
}

TEST(SeaRows, LookingDownTheSeaFillsTheScreen)
{
	const auto range =
	    sea_rows::ComputeScreenRange(ViewProjection({2560.0f, 1000.0f, 2560.0f}, {2560.0f, 0.0f, 2561.0f}), k_Viewport, k_Near);
	ASSERT_TRUE(range.has_value());
	EXPECT_FLOAT_EQ(range->top, 0.0f);
	EXPECT_FLOAT_EQ(range->bottom, k_Viewport.y - 1.0f);
}

TEST(SeaRows, LookingAcrossTheSeaStartsNearTheHorizon)
{
	const auto range = sea_rows::ComputeScreenRange(ViewProjection({2560.0f, 100.0f, 2560.0f}, {2560.0f, 100.0f, 3560.0f}),
	                                                k_Viewport, k_Near);
	ASSERT_TRUE(range.has_value());
	// The square's far edge, 15000 units away, is a few pixels under the horizon in the middle of the screen
	EXPECT_GT(range->top, k_Viewport.y / 2.0f);
	EXPECT_LT(range->top, k_Viewport.y / 2.0f + 10.0f);
	EXPECT_FLOAT_EQ(range->bottom, k_Viewport.y - 1.0f);
	// Further away at the top than at the bottom
	EXPECT_LT(range->inverseDepthTop, range->inverseDepthBottom);
}

TEST(SeaRows, NoRowsWhenTheSeaIsBehind)
{
	// Beyond the square's edge, looking away from it
	EXPECT_FALSE(sea_rows::ComputeScreenRange(ViewProjection({2560.0f, 100.0f, 40000.0f}, {2560.0f, 100.0f, 41000.0f}),
	                                          k_Viewport, k_Near)
	                 .has_value());
	// Looking up at the sky
	EXPECT_FALSE(sea_rows::ComputeScreenRange(ViewProjection({2560.0f, 100.0f, 2560.0f}, {2560.0f, 2000.0f, 2561.0f}),
	                                          k_Viewport, k_Near)
	                 .has_value());
}

TEST(SeaRows, RowsAreTwoPixelsApart)
{
	const auto rows =
	    sea_rows::MakeRows({.top = 100.7f, .bottom = 719.0f, .inverseDepthTop = 0.001f, .inverseDepthBottom = 0.1f});
	EXPECT_EQ(rows.first, 100);
	EXPECT_EQ(rows.count, 310);
	EXPECT_TRUE(rows.softTop);
	EXPECT_FLOAT_EQ(rows.inverseDepth, 0.001f);
	EXPECT_FLOAT_EQ(rows.inverseDepth + static_cast<float>(rows.count - 1) * rows.inverseStep, 0.1f);

	// From the top of the screen the first row isn't faded
	EXPECT_FALSE(
	    sea_rows::MakeRows({.top = 0.0f, .bottom = 719.0f, .inverseDepthTop = 0.001f, .inverseDepthBottom = 0.1f}).softTop);
}

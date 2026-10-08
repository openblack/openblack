/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Common/GUtilsDistance.h"
#include "ECS/ScenicForest.h"

using namespace openblack;
using namespace openblack::ecs;

TEST(ScenicForest, ATownsCentreIsTheMiddleOfTheGroundItsBuildingsCover)
{
	scenic_forest::TownArea area;
	area.Add({100.0f, 200.0f}, 5.0f);
	area.Add({140.0f, 210.0f}, 10.0f);
	EXPECT_FLOAT_EQ(area.Centre().x, 122.5f);
	EXPECT_FLOAT_EQ(area.Centre().y, 207.5f);
}

TEST(ScenicForest, TheWalkStopsAtTheFirstStepBeyondTheReach)
{
	const auto centre = map_coords::FromMetres({1000.0f, 1000.0f});
	const auto cells = scenic_forest::Walk(centre, 25.0f);
	ASSERT_FALSE(cells.empty());
	// Every cell walked is within the reach, and the ring it stops in is entered at a corner before it is complete
	for (const auto& cell : cells)
	{
		EXPECT_LE(gutils::GetDistanceInMetres(cell, centre), 25.0f);
	}
	// A full disc of reach 25 holds 21 cell centres; the spiral is stopped at the corner of its third ring
	EXPECT_LT(cells.size(), 21u);
	EXPECT_EQ(gutils::GetDistanceInMetres(cells.front(), centre), 0.0f);
}

TEST(ScenicForest, ATreeOfAnotherTownsScenicForestComesOnlyWhenStrictlyNearer)
{
	const glm::vec2 centre {0.0f, 0.0f};
	EXPECT_TRUE(scenic_forest::Takes({.at = {50.0f, 0.0f}}, centre));
	EXPECT_FALSE(scenic_forest::Takes({.at = {50.0f, 0.0f}, .inForest = true}, centre));
	EXPECT_TRUE(scenic_forest::Takes({.at = {40.0f, 0.0f}, .inForest = true, .scenicCentre = glm::vec2(100.0f, 0.0f)}, centre));
	EXPECT_FALSE(
	    scenic_forest::Takes({.at = {50.0f, 0.0f}, .inForest = true, .scenicCentre = glm::vec2(100.0f, 0.0f)}, centre));
}

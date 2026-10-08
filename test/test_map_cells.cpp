/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <vector>

#include <gtest/gtest.h>

#include "ECS/MapCells.h"
#include "Magic/AreaEffect.h"
#include "Magic/Impressiveness.h"

using namespace openblack;
using namespace openblack::ecs::map_cells;

namespace
{
constexpr entt::entity E(uint32_t id)
{
	return static_cast<entt::entity>(id);
}

std::vector<entt::entity> ToVector(std::span<const entt::entity> span)
{
	return {span.begin(), span.end()};
}
} // namespace

TEST(MapCells, BuildingsGoToTheFrontAndCarriedThingsToTheBack)
{
	CellLists lists(8);
	const auto cell = *lists.IndexOf({2, 3});
	EXPECT_EQ(cell, 2u * 8u + 3u);
	lists.Insert(cell, E(1), Placement::FixedFront);
	lists.Insert(cell, E(2), Placement::FixedBack);
	lists.Insert(cell, E(3), Placement::FixedFront);
	lists.Insert(cell, E(4), Placement::FixedBack);
	EXPECT_EQ(ToVector(lists.Fixed(cell)), (std::vector {E(3), E(1), E(2), E(4)}));
}

TEST(MapCells, TheLastToComeIntoACellIsMetFirstAfterWhatStaysPut)
{
	CellLists lists(8);
	const auto cell = *lists.IndexOf({0, 0});
	lists.Insert(cell, E(10), Placement::MobileFront);
	lists.Insert(cell, E(1), Placement::FixedFront);
	lists.Insert(cell, E(11), Placement::MobileFront);
	EXPECT_EQ(lists.All(cell), (std::vector {E(1), E(11), E(10)}));
}

TEST(MapCells, TakingOneOutKeepsTheOthersInOrderAndMovingPutsItInFront)
{
	CellLists lists(8);
	const auto from = *lists.IndexOf({1, 1});
	const auto to = *lists.IndexOf({1, 2});
	for (uint32_t id = 1; id <= 3; ++id)
	{
		lists.Insert(from, E(id), Placement::MobileFront);
		lists.Insert(to, E(id + 10), Placement::MobileFront);
	}
	lists.Remove(from, E(2));
	EXPECT_EQ(ToVector(lists.Mobile(from)), (std::vector {E(3), E(1)}));
	lists.Insert(to, E(2), Placement::MobileFront);
	EXPECT_EQ(ToVector(lists.Mobile(to)), (std::vector {E(2), E(13), E(12), E(11)}));
	lists.Clear();
	EXPECT_TRUE(lists.All(to).empty());
}

TEST(MapCells, CellsOffTheMapHaveNoIndex)
{
	CellLists lists(8);
	EXPECT_FALSE(lists.IndexOf({-1, 0}).has_value());
	EXPECT_FALSE(lists.IndexOf({0, 8}).has_value());
}

TEST(MapCells, ARoundBoxIsOneCircleOfItsLongerSide)
{
	const auto circles = CirclesOf({.centre = {50.0f, 50.0f}, .halfSize = {4.0f, 5.0f}, .halfDiagonal = 7.0f});
	EXPECT_TRUE(circles.row.empty());
	EXPECT_FLOAT_EQ(circles.bounds.radius, 5.0f);
	// Neither side counts as less than a unit
	EXPECT_FLOAT_EQ(CirclesOf({.halfSize = {0.2f, 0.3f}}).bounds.radius, 1.0f);
}

TEST(MapCells, ALongBoxIsARowOfCirclesItsWidthAcross)
{
	// 20 long and 5 wide along x: five circles of 5, every 8 from -16 to 16
	const auto circles = CirclesOf({.centre = {100.0f, 200.0f}, .halfSize = {20.0f, 5.0f}, .halfDiagonal = 21.0f});
	ASSERT_EQ(circles.row.size(), 5u);
	EXPECT_FLOAT_EQ(circles.bounds.radius, std::sqrt(425.0f));
	EXPECT_FLOAT_EQ(circles.row.front().centre.x, 84.0f);
	EXPECT_FLOAT_EQ(circles.row.back().centre.x, 116.0f);
	EXPECT_FLOAT_EQ(circles.row.front().centre.y, 200.0f);
	EXPECT_FLOAT_EQ(circles.row.front().radius, 5.0f);
	// Turned a quarter, the row runs along z
	const auto turned =
	    CirclesOf({.centre = {100.0f, 200.0f}, .halfSize = {20.0f, 5.0f}, .halfDiagonal = 21.0f, .axis = {0.0f, 1.0f}});
	EXPECT_NEAR(turned.row.front().centre.x, 100.0f, 1e-4f);
	EXPECT_NEAR(turned.row.front().centre.y, 184.0f, 1e-4f);
}

TEST(MapCells, ABuildingCoversTheCellsItsOutlineTouchesXByX)
{
	// A round building of radius 5 in the middle of cell (10, 10): it and the cells beside it, whose circles of 7.1
	// reach it, but not the corners' (14.1 away)
	const auto cells = CellsCovered({.centre = {105.0f, 105.0f}, .halfSize = {5.0f, 5.0f}, .halfDiagonal = 7.0f});
	EXPECT_EQ(cells, (std::vector<glm::ivec2> {{9, 10}, {10, 9}, {10, 10}, {10, 11}, {11, 10}}));
}

TEST(MapCells, ALongBuildingCoversOnlyTheCellsAlongIt)
{
	const auto cells = CellsCovered({.centre = {105.0f, 105.0f}, .halfSize = {20.0f, 1.0f}, .halfDiagonal = 20.1f});
	for (const auto& cell : cells)
	{
		EXPECT_GE(cell.y, 9);
		EXPECT_LE(cell.y, 11);
	}
	EXPECT_NE(std::ranges::find(cells, glm::ivec2(12, 10)), cells.end());
	EXPECT_EQ(std::ranges::find(cells, glm::ivec2(10, 12)), cells.end());
}

TEST(MapCells, ABuildingAtTheEdgeOfTheMapKeepsToIt)
{
	const auto cells = CellsCovered({.centre = {2.0f, 2.0f}, .halfSize = {5.0f, 5.0f}, .halfDiagonal = 7.0f});
	EXPECT_EQ(cells.front(), glm::ivec2(0, 0));
	for (const auto& cell : cells)
	{
		EXPECT_GE(cell.x, 0);
		EXPECT_GE(cell.y, 0);
	}
}

TEST(AreaEffectCells, TheSquareIsFoundThroughMapPositions)
{
	const auto cells = magic::EffectCellsAround({55.0f, 0.0f, 55.0f}, 12.0f);
	EXPECT_EQ(cells.first, glm::ivec2(4, 4));
	EXPECT_EQ(cells.last, glm::ivec2(6, 6));
	// Off the low edge the cell is -1, kept signed
	EXPECT_EQ(magic::EffectCellsAround({5.0f, 0.0f, 5.0f}, 6.0f).first, glm::ivec2(-1, -1));
}

TEST(ReactionImpressiveness, FoodAndWoodImpressByHowMuchTheTownWantsThem)
{
	EXPECT_FLOAT_EQ(magic::ReactionMultiplier(Reaction::ReactToFood, 0.3f, 0.75f), 0.75f);
	EXPECT_FLOAT_EQ(magic::ReactionMultiplier(Reaction::ReactToWood, 0.3f, 1.5f), 1.5f);
	// Without a town, by exactly one, not by the table
	EXPECT_FLOAT_EQ(magic::ReactionMultiplier(Reaction::ReactToFood, 0.3f, std::nullopt), 1.0f);
	// Everything else by its table
	EXPECT_FLOAT_EQ(magic::ReactionMultiplier(Reaction::LookAtNiceSpell, 0.3f, 0.75f), 0.3f);
}

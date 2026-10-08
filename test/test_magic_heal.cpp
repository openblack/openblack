/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Magic/HealTargets.h"
#include "Magic/MapSpiral.h"

using namespace openblack;
using namespace openblack::magic;

namespace
{
/// Made-up people by the cell of the land they stand in
class FakeCells
{
public:
	void Add(entt::entity entity, glm::vec3 position, bool healable = true)
	{
		_cells[{static_cast<int>(std::floor(position.x / 10.0f)), static_cast<int>(std::floor(position.z / 10.0f))}].push_back(
		    {.entity = entity, .position = position, .living = true, .healable = healable});
	}
	[[nodiscard]] CellContents Contents() const
	{
		return [this](glm::ivec2 cell) -> std::span<const HealCandidate> {
			const auto found = _cells.find({cell.x, cell.y});
			return found != _cells.end() ? std::span<const HealCandidate>(found->second) : std::span<const HealCandidate>();
		};
	}

private:
	std::map<std::pair<int, int>, std::vector<HealCandidate>> _cells;
};

entt::entity Id(uint32_t n)
{
	return static_cast<entt::entity>(n);
}
} // namespace

TEST(MapSpiral, WalksOutFromItsCell)
{
	const auto cells = SpiralCells({5, 5}, 9);
	ASSERT_EQ(cells.size(), 9u);
	EXPECT_EQ(cells[0], glm::ivec2(5, 5));
	// Every cell of the three by three square round it, each once
	for (int z = 4; z <= 6; ++z)
	{
		for (int x = 4; x <= 6; ++x)
		{
			EXPECT_EQ(std::ranges::count(cells, glm::ivec2(x, z)), 1) << x << "," << z;
		}
	}
	// Then the next ring
	const auto more = SpiralCells({0, 0}, 25);
	for (size_t i = 9; i < more.size(); ++i)
	{
		EXPECT_EQ(std::max(std::abs(more[i].x), std::abs(more[i].y)), 2);
	}
}

TEST(HealTargets, CellsAcrossTheRadius)
{
	EXPECT_EQ(HealCellsAcross(10.0f), 2u);
	EXPECT_EQ(HealCellsAcross(35.0f), 7u);
	EXPECT_EQ(HealCellsAcross(0.0f), 0u);
}

TEST(HealTargets, ThePlainHealLooksInOnlyFourCells)
{
	FakeCells cells;
	// Cast in the middle of cell (5, 5)
	const glm::vec3 point(55.0f, 0.0f, 55.0f);
	const auto first = SpiralCells({5, 5}, 4);
	const auto missed = SpiralCells({5, 5}, 9).back();
	// One person just inside the radius in a cell the spiral visits, and one as close in a cell it doesn't
	const glm::vec3 visited(static_cast<float>(first[1].x) * 10.0f + 5.0f, 0.0f, static_cast<float>(first[1].y) * 10.0f + 5.0f);
	const glm::vec3 skipped(static_cast<float>(missed.x) * 10.0f + 5.0f, 0.0f, static_cast<float>(missed.y) * 10.0f + 5.0f);
	cells.Add(Id(1), point + (visited - point) * 0.9f);
	cells.Add(Id(2), point + (skipped - point) * 0.6f);
	const auto found = FindHealTargets(point, 10.0f, 20, cells.Contents());
	ASSERT_EQ(found.size(), 1u);
	EXPECT_EQ(found[0], Id(1));
}

TEST(HealTargets, TakesTheHealableWithinTheRadiusInCellOrder)
{
	FakeCells cells;
	const glm::vec3 point(5.0f, 0.0f, 5.0f);
	cells.Add(Id(1), {6.0f, 0.0f, 6.0f});
	cells.Add(Id(2), {4.0f, 0.0f, 4.0f}, false); // dead, or a dove
	cells.Add(Id(3), {5.0f, 0.0f, 5.5f});
	cells.Add(Id(4), {5.0f, 30.0f, 5.0f}); // far above, but heights don't count
	const auto found = FindHealTargets(point, 10.0f, 20, cells.Contents());
	EXPECT_EQ(found, (std::vector {Id(1), Id(3), Id(4)}));
}

TEST(HealTargets, StopsAtItsMost)
{
	FakeCells cells;
	const glm::vec3 point(5.0f, 0.0f, 5.0f);
	for (uint32_t i = 1; i <= 30; ++i)
	{
		cells.Add(Id(i), {5.0f + static_cast<float>(i) * 0.1f, 0.0f, 5.0f});
	}
	EXPECT_EQ(FindHealTargets(point, 10.0f, 20, cells.Contents()).size(), 20u);
	// The power-up's reach covers its whole radius
	FakeCells wide;
	wide.Add(Id(1), {5.0f + 33.0f, 0.0f, 5.0f});
	wide.Add(Id(2), {5.0f, 0.0f, 5.0f - 30.0f});
	EXPECT_EQ(FindHealTargets(point, 35.0f, 100, wide.Contents()).size(), 2u);
}

TEST(HealTargets, MeasuresEachCellFromTheCastPointMovedIntoIt)
{
	FakeCells cells;
	const glm::vec3 point(5.0f, 0.0f, 35.0f);
	// In the cell to the south, 14.9 m from the cast point but 5.7 m from it moved one cell south
	cells.Add(Id(1), {9.0f, 0.0f, 21.0f});
	// In the cell to the east, 7 m away, but not among the four cells the plain heal looks in
	cells.Add(Id(2), {12.0f, 0.0f, 35.0f});
	EXPECT_EQ(FindHealTargets(point, 10.0f, 20, cells.Contents()), (std::vector {Id(1)}));
}

TEST(HealTargets, TheRadiusIsStrict)
{
	const glm::vec3 point(5.0f, 0.0f, 5.0f);
	// 2.5 m is a whole number of map units, so this one is exactly the radius away: not taken
	FakeCells onEdge;
	onEdge.Add(Id(1), {5.0f, 0.0f, 7.5f});
	EXPECT_TRUE(FindHealTargets(point, 2.5f, 20, onEdge.Contents()).empty());
	FakeCells inside;
	inside.Add(Id(2), {5.0f, 0.0f, 7.4f});
	EXPECT_EQ(FindHealTargets(point, 2.5f, 20, inside.Contents()), (std::vector {Id(2)}));
}

TEST(MapSpiral, GoesWestThenSouthEastAndNorth)
{
	const std::vector<glm::ivec2> expected {{0, 0},  {-1, 0}, {-1, -1}, {0, -1},  {1, -1},  {1, 0},   {1, 1},  {0, 1},
	                                        {-1, 1}, {-2, 1}, {-2, 0},  {-2, -1}, {-2, -2}, {-1, -2}, {0, -2}, {1, -2}};
	EXPECT_EQ(SpiralCells({0, 0}, expected.size()), expected);
}

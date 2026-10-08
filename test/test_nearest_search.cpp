/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <map>

#include <gtest/gtest.h>

#include "ECS/NearestSearch.h"

using namespace openblack;
using namespace openblack::ecs;

namespace
{
/// Things laid in cells, as a fake of the map's cells
struct FakeCells
{
	std::map<std::pair<int, int>, std::vector<nearest_search::Candidate>> cells;
	mutable int visited {0};

	void Put(entt::entity entity, glm::vec2 metres)
	{
		const auto at = map_coords::FromMetres(metres);
		const auto cell = map_coords::Cell(at);
		cells[{cell.x, cell.y}].push_back({.entity = entity, .at = at});
	}

	[[nodiscard]] nearest_search::CellCandidates Lookup() const
	{
		return [this](glm::ivec2 cell) {
			++visited;
			const auto found = cells.find({cell.x, cell.y});
			return found != cells.end() ? found->second : std::vector<nearest_search::Candidate> {};
		};
	}
};

constexpr auto k_A = static_cast<entt::entity>(1);
constexpr auto k_B = static_cast<entt::entity>(2);
constexpr entt::entity k_None {entt::null};
} // namespace

TEST(NearestSearch, AThingCountsOnlyStrictlyWithinTheReach)
{
	FakeCells fake;
	fake.Put(k_A, {1050.0f, 1000.0f});
	const auto from = map_coords::FromMetres({1000.0f, 1000.0f});
	EXPECT_EQ(nearest_search::FindNearest(from, 50.0f, fake.Lookup()), k_None);
	EXPECT_EQ(nearest_search::FindNearest(from, 51.0f, fake.Lookup()), k_A);
}

TEST(NearestSearch, TheFirstMetWinsATieAndANearerOneWinsOutright)
{
	FakeCells fake;
	// Both in the starting cell, the same distance away: the first kept wins
	fake.Put(k_A, {1003.0f, 1000.0f});
	fake.Put(k_B, {1000.0f, 1003.0f});
	const auto from = map_coords::FromMetres({1000.0f, 1000.0f});
	EXPECT_EQ(nearest_search::FindNearest(from, 50.0f, fake.Lookup()), k_A);
	FakeCells nearer;
	nearer.Put(k_A, {1004.0f, 1000.0f});
	nearer.Put(k_B, {1001.0f, 1000.0f});
	EXPECT_EQ(nearest_search::FindNearest(from, 50.0f, nearer.Lookup()), k_B);
}

TEST(NearestSearch, ASmallReachStillWalksThreeByThreeCells)
{
	FakeCells fake;
	const auto from = map_coords::FromMetres({1005.0f, 1005.0f});
	(void)nearest_search::FindNearest(from, 1.0f, fake.Lookup());
	EXPECT_EQ(fake.visited, 9);
}

TEST(NearestSearch, OnceFoundTheWalkStopsBeyondHalfAsFarAgainAndTen)
{
	FakeCells fake;
	// Found at once in the starting cell: the walk goes on only while cells lie within 1.5 times its distance and 10
	fake.Put(k_A, {1001.0f, 1000.0f});
	const auto from = map_coords::FromMetres({1000.0f, 1000.0f});
	EXPECT_EQ(nearest_search::FindNearest(from, 200.0f, fake.Lookup()), k_A);
	// Far fewer than the 40 by 40 cells a 200 m reach covers
	EXPECT_LT(fake.visited, 40 * 40);
}

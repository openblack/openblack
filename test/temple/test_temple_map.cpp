/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <map>
#include <vector>

#include <LNDFile.h>
#include <glm/common.hpp>
#include <gtest/gtest.h>

#include "3D/TempleMap.h"

using namespace openblack;

namespace
{
/// Land of a few blocks, each of its cells at an altitude and brightness, with the blocks from (2, 3) to (5, 7)
struct FakeLand
{
	uint8_t altitude {100};
	uint8_t luminosity {128};
	std::map<std::pair<int, int>, lnd::LNDCell> cells;

	[[nodiscard]] TempleMap::FindCell Finder()
	{
		return [this](glm::u16vec2 cell) -> const lnd::LNDCell* {
			const int blockX = cell.x / 16;
			const int blockZ = cell.y / 16;
			if (blockX < 2 || blockX > 5 || blockZ < 3 || blockZ > 7)
			{
				return nullptr;
			}
			auto& found = cells[{cell.x, cell.y}];
			found.altitude = altitude;
			found.luminosity = luminosity;
			return &found;
		};
	}
};
} // namespace

TEST(TempleMap, FramesTheIslandWithAnElevenUnitDiagonal)
{
	FakeLand land;
	TempleMap map;
	ASSERT_TRUE(map.Frame(land.Finder()));
	// 3 by 4 blocks between the first and last, so a scale of 11 / 5 a vertex
	EXPECT_FLOAT_EQ(map.GetWorldScale(), (11.0f / 5.0f) / 80.0f);
	// The middle of the blocks there are is at the room's centre
	const auto middle = map.ToMap(glm::vec2(4.0f, 5.5f) * 160.0f);
	EXPECT_NEAR(middle.x, 0.0f, 1e-4f);
	EXPECT_NEAR(middle.z, 0.0f, 1e-4f);
}

TEST(TempleMap, HasNoFrameWithoutLand)
{
	TempleMap map;
	EXPECT_FALSE(map.Frame([](glm::u16vec2) -> const lnd::LNDCell* { return nullptr; }));
	std::vector<OrientedTextVertex> triangles;
	map.Build([](glm::u16vec2) -> const lnd::LNDCell* { return nullptr; }, triangles);
	EXPECT_TRUE(triangles.empty());
}

TEST(TempleMap, DrawsTwoTrianglesAQuadOfEachBlock)
{
	FakeLand land;
	TempleMap map;
	map.Frame(land.Finder());
	std::vector<OrientedTextVertex> triangles;
	map.Build(land.Finder(), triangles);
	// 4 by 5 blocks of 2 by 2 quads
	EXPECT_EQ(triangles.size(), 4 * 5 * 4 * 2 * 3);

	// Land is grey by its brightness, opaque above the coast, and as high as the world scaled
	const auto& vertex = triangles[1];
	EXPECT_EQ(vertex.colour, 0xFF7F7F7Fu);
	EXPECT_NEAR(vertex.position.y, 100.0f * 0.67f * map.GetWorldScale(), 1e-5f);
	// Its texture spans the 32 blocks there can be
	EXPECT_EQ(vertex.uv * 64.0f, glm::round(vertex.uv * 64.0f));
}

TEST(TempleMap, FadesAtTheCoast)
{
	FakeLand land;
	land.altitude = 2;
	TempleMap map;
	map.Frame(land.Finder());
	std::vector<OrientedTextVertex> triangles;
	map.Build(land.Finder(), triangles);
	EXPECT_EQ(triangles[1].colour >> 24, 0xAAu);
}

TEST(TempleMap, MapsTheWorldBackAndForth)
{
	FakeLand land;
	TempleMap map;
	map.Frame(land.Finder());
	std::vector<OrientedTextVertex> triangles;
	map.Build(land.Finder(), triangles);
	const glm::vec2 world(700.0f, 900.0f);
	const auto onMap = map.ToMap(world);
	EXPECT_NEAR(onMap.y, 100.0f * 0.67f * map.GetWorldScale(), 1e-4f);
	const auto back = map.ToWorld(onMap);
	EXPECT_NEAR(back.x, world.x, 1e-2f);
	EXPECT_NEAR(back.y, world.y, 1e-2f);
}

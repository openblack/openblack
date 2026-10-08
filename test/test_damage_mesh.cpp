/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Physics/DamageMesh.h"

using namespace openblack::physics::damage;

namespace
{
Triangle Tri(glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
	return MakeTriangle({Corner {.position = a}, Corner {.position = b}, Corner {.position = c}});
}

/// A flat wall of unit squares, each two triangles, from x = 0 to width and y = 0 to height
Primitive Wall(int width, int height, float scale = 1.0f)
{
	Primitive wall;
	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			const glm::vec3 a(static_cast<float>(x) * scale, static_cast<float>(y) * scale, 0.0f);
			const glm::vec3 b = a + glm::vec3(scale, 0.0f, 0.0f);
			const glm::vec3 c = a + glm::vec3(scale, scale, 0.0f);
			const glm::vec3 d = a + glm::vec3(0.0f, scale, 0.0f);
			wall.triangles.push_back(Tri(a, b, c));
			wall.triangles.push_back(Tri(a, c, d));
		}
	}
	LinkNeighbours(wall);
	return wall;
}
} // namespace

TEST(DamageMesh, ATrianglesSizeComesFromItsShortestSide)
{
	EXPECT_EQ(SizeClassOf(Tri({0, 0, 0}, {1, 0, 0}, {0, 1, 0})), 0); // 1 squared
	EXPECT_EQ(SizeClassOf(Tri({0, 0, 0}, {2, 0, 0}, {0, 2, 0})), 1); // 4
	EXPECT_EQ(SizeClassOf(Tri({0, 0, 0}, {3, 0, 0}, {0, 3, 0})), 2); // 9
	EXPECT_EQ(SizeClassOf(Tri({0, 0, 0}, {6, 0, 0}, {0, 6, 0})), 3); // 36
	// Exactly on a threshold falls below it
	EXPECT_EQ(SizeClassOf(Tri({0, 0, 0}, {std::sqrt(2.0f), 0, 0}, {0, std::sqrt(2.0f), 0})), 0);
}

TEST(DamageMesh, CornersAreCountedNearTheBlowsLine)
{
	const auto triangle = Tri({0, 0, 0}, {10, 0, 0}, {0, 10, 0});
	// A line down the z axis through the origin reaches only the first corner
	EXPECT_EQ(CornersNear(triangle, {0, 0, -5}, {0, 0, 1}, 1.0f), 1);
	EXPECT_EQ(CornersNear(triangle, {0, 0, -5}, {0, 0, 2}, 11.0f), 3);
	// Without a direction it is the distance to the point
	EXPECT_EQ(CornersNear(triangle, {0, 0, 5}, {0, 0, 0}, 6.0f), 1);
	EXPECT_EQ(CornersNear(triangle, {0, 0, 5}, {0, 0, 0}, 5.0f), 0);
}

TEST(DamageMesh, HalvingCutsTheLongestSideAndASizeOff)
{
	const auto triangle = Tri({0, 0, 0}, {8, 0, 0}, {0, 2, 0});
	const auto halves = Halve(triangle);
	// The side from the second corner to the third is the longest
	EXPECT_EQ(halves[0].corners[0].position, glm::vec3(8, 0, 0));
	EXPECT_EQ(halves[0].corners[1].position, glm::vec3(4, 1, 0));
	EXPECT_EQ(halves[0].corners[2].position, glm::vec3(0, 0, 0));
	EXPECT_EQ(halves[1].corners[0].position, glm::vec3(4, 1, 0));
	EXPECT_EQ(halves[1].corners[1].position, glm::vec3(0, 2, 0));
	for (const auto& half : halves)
	{
		EXPECT_EQ(half.sizeClass, triangle.sizeClass - 1);
		EXPECT_TRUE(half.CountsAsBuilding());
	}
	// A tie keeps the side from the last corner to the first
	const auto square = Halve(Tri({0, 0, 0}, {4, 0, 0}, {4, 4, 0}));
	EXPECT_EQ(square[0].corners[1].position, glm::vec3(2, 2, 0));
}

TEST(DamageMesh, TheSmallestTrianglesGoWithMostOfTheirCorners)
{
	const auto small = Tri({0, 0, 0}, {1, 0, 0}, {0, 1, 0});
	// Two corners near: broken
	auto sorted = SortByBlow({small}, {0.5f, 0, -1}, {0, 0, 1}, 0.6f);
	EXPECT_EQ(sorted.broken.size(), 1U);
	EXPECT_TRUE(sorted.kept.empty());
	// One corner near: kept
	sorted = SortByBlow({small}, {0, 0, -1}, {0, 0, 1}, 0.5f);
	EXPECT_TRUE(sorted.broken.empty());
	EXPECT_EQ(sorted.kept.size(), 1U);
}

TEST(DamageMesh, ABlowBreaksAHoleAndHalvesTheTrianglesAtItsEdge)
{
	Mesh mesh {.primitives = {Wall(8, 8, 2.0f)}};
	mesh.trianglesAtCreation = static_cast<uint32_t>(mesh.TriangleCount());
	const auto broken = Strike(mesh, {8, 8, -5}, {0, 0, 1}, 5.0f);
	ASSERT_EQ(broken.size(), 1U);
	EXPECT_FALSE(broken.front().triangles.empty());
	// Every broken corner lies near the line, every kept triangle has a corner away from it
	for (const auto& triangle : broken.front().triangles)
	{
		EXPECT_GE(CornersNear(triangle, {8, 8, -5}, {0, 0, 1}, 5.0f), 2);
	}
	const float remaining = RemainingFraction(mesh);
	EXPECT_LT(remaining, 1.0f);
	EXPECT_GT(remaining, 0.5f);
}

TEST(DamageMesh, ABlowThatOnlyHalvesLeavesAllOfTheBuilding)
{
	Mesh mesh {.primitives = {Wall(8, 8, 2.0f)}};
	mesh.trianglesAtCreation = static_cast<uint32_t>(mesh.TriangleCount());
	// The line only grazes corners: triangles are halved, few break
	const auto broken = Strike(mesh, {8, 8, -5}, {0, 0, 1}, 0.5f);
	EXPECT_FLOAT_EQ(RemainingFraction(mesh), 1.0f);
}

TEST(DamageMesh, AWallStandingOnTheLandHoldsAndWhatHangsFallsAway)
{
	Mesh mesh {.primitives = {Wall(4, 4)}};
	// A loose square floating over the wall
	auto loose = Wall(1, 1);
	Offset(loose, {10, 10, 0});
	mesh.primitives.push_back(loose);
	const auto groups = LabelGroups(mesh);
	EXPECT_EQ(groups, 2);
	const auto anchors = Anchors(mesh, groups, [](glm::vec2) { return 0.0f; });
	EXPECT_TRUE(anchors[0]);
	EXPECT_FALSE(anchors[1]);
	const auto parts = TakeAwayLooseGroups(mesh, anchors, groups);
	ASSERT_EQ(parts.size(), 1U);
	EXPECT_EQ(parts.front().primitive.triangles.size(), 2U);
	// Its triangles no longer count as the building
	EXPECT_FALSE(parts.front().primitive.triangles.front().CountsAsBuilding());
	EXPECT_TRUE(mesh.primitives[1].triangles.empty());
	EXPECT_EQ(mesh.primitives[0].triangles.size(), 32U);
}

TEST(DamageMesh, WithoutTheLandOnlyTheFirstGroupHolds)
{
	Mesh mesh {.primitives = {Wall(1, 1)}};
	auto other = Wall(1, 1);
	Offset(other, {5, 0, 0});
	mesh.primitives.front().triangles.insert(mesh.primitives.front().triangles.end(), other.triangles.begin(),
	                                         other.triangles.end());
	const auto groups = LabelGroups(mesh);
	const auto anchors = Anchors(mesh, groups, std::nullopt);
	ASSERT_EQ(anchors.size(), 2U);
	EXPECT_TRUE(anchors[0]);
	EXPECT_FALSE(anchors[1]);
}

TEST(DamageMesh, LoneTrianglesAreLost)
{
	Mesh mesh {.primitives = {Wall(1, 1)}};
	mesh.primitives.front().triangles.push_back(Tri({10, 10, 0}, {11, 10, 0}, {10, 11, 0}));
	const auto groups = LabelGroups(mesh);
	const auto parts = TakeAwayLooseGroups(mesh, Anchors(mesh, groups, std::nullopt), groups);
	EXPECT_TRUE(parts.empty());
	EXPECT_EQ(mesh.primitives.front().triangles.size(), 2U);
}

TEST(DamageMesh, NoMoreThanSixtyFourGroups)
{
	Mesh mesh;
	for (int i = 0; i < 70; ++i)
	{
		auto square = Wall(1, 1);
		Offset(square, {static_cast<float>(i) * 3.0f, 0, 0});
		mesh.primitives.push_back(square);
	}
	EXPECT_EQ(LabelGroups(mesh), k_MostGroups);
	EXPECT_EQ(mesh.primitives.back().triangles.front().group, -1);
}

TEST(DamageMesh, ABuildingsBlowIsJudgedByItsMomentum)
{
	EXPECT_EQ(JudgeBlow(300.0f), Blow::Nothing);
	EXPECT_EQ(JudgeBlow(300.5f), Blow::Knock);
	EXPECT_EQ(JudgeBlow(1000.0f), Blow::Knock);
	EXPECT_EQ(JudgeBlow(1500.0f), Blow::HardKnock);
	EXPECT_EQ(JudgeBlow(2000.0f), Blow::HardKnock);
	EXPECT_EQ(JudgeBlow(2000.5f), Blow::Breaks);
}

TEST(DamageMesh, RepairAndBreakageShares)
{
	EXPECT_TRUE(RebuildsOnBlow(0.2f));
	EXPECT_FALSE(RebuildsOnBlow(1.0f));
	EXPECT_FALSE(RebuildsOnBlow(0.1f));
	EXPECT_FLOAT_EQ(RepairStartLife(1.0f), 1.0f);
	EXPECT_FLOAT_EQ(RepairStartLife(0.5f), 0.45f);
	EXPECT_FLOAT_EQ(BreakageShare(0.9f, 0.6f), 0.3f);
	EXPECT_FLOAT_EQ(BreakageShare(0.5f, 0.6f), 0.0f);
}

TEST(DamageMesh, ABrokenBuildingIsDrawnAsFarAsItIsRepaired)
{
	// Right after a blow its repair starts a little short of its life: it is drawn an eleventh repaired
	const float life = 0.6f;
	EXPECT_NEAR(DrawShare(life, 1.0f, RepairStartLife(life)), 1.0f / 11.0f, 1e-5f);
	// Healed whole, it is drawn whole
	EXPECT_FLOAT_EQ(DrawShare(1.0f, 1.0f, RepairStartLife(life)), 1.0f);
	// Without a repair, a little short of its life
	EXPECT_FLOAT_EQ(DrawShare(0.5f, 1.0f, std::nullopt), 0.49f);
	// Not built, drawn as built
	EXPECT_FLOAT_EQ(DrawShare(1.0f, 0.3f, std::nullopt), 0.3f);
	// A building at no life whose repair started below none is drawn as far as it has regained
	EXPECT_NEAR(DrawShare(0.0f, 1.0f, -0.1f), 0.1f / 1.1f, 1e-6f);
	// Nothing regained since the repair started is drawn as nothing
	EXPECT_FLOAT_EQ(DrawShare(0.45f, 1.0f, 0.45f), 0.0f);
}

TEST(DamageMesh, APartBuiltBuildingStandsAsFarUpAsItsShareWithItsScaffoldRisingThenTakenDown)
{
	using openblack::physics::damage::PartialBuildOf;
	// A model 10 high (half height 5, scale 1) standing at 2
	const auto low = PartialBuildOf(0.1f, 2.0f, 5.0f, 1.0f);
	ASSERT_TRUE(low.modelCut.has_value());
	EXPECT_FLOAT_EQ(*low.modelCut, 3.0f);
	// The scaffold is half risen at a tenth built
	EXPECT_FLOAT_EQ(low.scaffoldSink, 5.0f);
	EXPECT_FALSE(low.scaffoldCut.has_value());
	// Less than a fifth of a metre above its foot, nothing of the model is drawn
	EXPECT_FALSE(PartialBuildOf(0.01f, 2.0f, 5.0f, 1.0f).modelCut.has_value());
	// Halfway the scaffold stands whole
	const auto half = PartialBuildOf(0.5f, 0.0f, 5.0f, 1.0f);
	EXPECT_FLOAT_EQ(half.scaffoldSink, 0.0f);
	EXPECT_FALSE(half.scaffoldCut.has_value());
	// Nine tenths built it is half taken down from its top
	const auto high = PartialBuildOf(0.9f, 0.0f, 5.0f, 1.0f);
	ASSERT_TRUE(high.scaffoldCut.has_value());
	EXPECT_NEAR(*high.scaffoldCut, 5.0f, 1e-5f);
	EXPECT_TRUE(high.scaffoldShown);
	// Almost done, what is left of it is too low to draw
	EXPECT_FALSE(PartialBuildOf(0.999f, 0.0f, 5.0f, 1.0f).scaffoldShown);
}

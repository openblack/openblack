/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <vector>

#include <gtest/gtest.h>

#include "Graphics/PartialBuildCap.h"

using namespace openblack::graphics::partial_build_cap;

namespace
{
// A wall one unit tall facing +x, as two triangles, its normals out of the building
constexpr std::array k_Positions {glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f),
                                  glm::vec3(1.0f, 1.0f, 0.0f)};
constexpr std::array k_Uvs {glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)};
constexpr std::array k_Normals {glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                                glm::vec3(1.0f, 0.0f, 0.0f)};
constexpr std::array<uint16_t, 6> k_Indices {0, 1, 2, 0, 2, 3};
} // namespace

TEST(PartialBuildCap, EachCutTriangleJoinsItsOuterSegmentToItsInnerOne)
{
	const auto cap = Build(k_Positions, k_Uvs, k_Normals, k_Indices, 0.25f, 0.35f);
	// Both triangles cross a quarter of the way up: two segments, two triangles each
	ASSERT_EQ(cap.size(), 12U);
	// The first triangle is cut along its second and third edges, in its order
	EXPECT_FLOAT_EQ(cap[0].position.y, 0.25f);
	EXPECT_FLOAT_EQ(cap[0].position.z, 1.0f);
	EXPECT_FLOAT_EQ(cap[0].uv.y, 0.25f);
	EXPECT_FLOAT_EQ(cap[1].position.z, 0.25f);
	// The inner segment stands in by the inset across the ground, at the same height
	EXPECT_FLOAT_EQ(cap[2].position.x, 1.0f - 0.35f);
	EXPECT_FLOAT_EQ(cap[2].position.y, 0.25f);
	EXPECT_FLOAT_EQ(cap[2].position.z, cap[0].position.z);
	EXPECT_FLOAT_EQ(cap[3].position.x, 0.65f);
	EXPECT_FLOAT_EQ(cap[3].position.z, cap[1].position.z);
	// The quad's second triangle shares the outer segment's end and the inner one's start
	EXPECT_EQ(cap[4].position, cap[1].position);
	EXPECT_EQ(cap[5].position, cap[2].position);
}

TEST(PartialBuildCap, ACornerAtTheCutCountsAsAboveIt)
{
	// Cut at the top of the wall: its top corners are above, so both triangles still cross
	EXPECT_EQ(Build(k_Positions, k_Uvs, k_Normals, k_Indices, 1.0f, 0.2f).size(), 12U);
	// Cut at its foot, every corner is above and nothing is capped
	EXPECT_TRUE(Build(k_Positions, k_Uvs, k_Normals, k_Indices, 0.0f, 0.2f).empty());
	EXPECT_TRUE(Build(k_Positions, k_Uvs, k_Normals, k_Indices, 2.0f, 0.2f).empty());
}

TEST(PartialBuildCap, TooManySegmentsShowNoCap)
{
	std::vector<uint16_t> indices;
	for (size_t i = 0; i < k_MostSegments; ++i)
	{
		indices.insert(indices.end(), k_Indices.begin(), k_Indices.begin() + 3);
	}
	EXPECT_EQ(Build(k_Positions, k_Uvs, k_Normals, indices, 0.5f, 0.2f).size(), k_MostSegments * 6);
	indices.insert(indices.end(), k_Indices.begin(), k_Indices.begin() + 3);
	EXPECT_TRUE(Build(k_Positions, k_Uvs, k_Normals, indices, 0.5f, 0.2f).empty());
}

TEST(PartialBuildCap, OnlyAPrimitiveWithAWholeTriangleBelowTheCutHasWallsWithin)
{
	EXPECT_FALSE(HasWholeTriangleBelow(k_Positions, k_Indices, 0.5f));
	EXPECT_TRUE(HasWholeTriangleBelow(k_Positions, k_Indices, 1.5f));
}

/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <vector>

#include <L3DFile.h>
#include <gtest/gtest.h>

using namespace openblack::l3d;

namespace
{
L3DFile MakeFile()
{
	L3DFile file;
	file.AddSubmesh(L3DSubmeshHeader {});
	file.AddVertices({L3DVertex {.position = {1.0f, 2.0f, 3.0f}}, L3DVertex {.position = {4.0f, 5.0f, 6.0f}}});
	return file;
}
} // namespace

TEST(L3DFileCopy, SpansAreIntoTheCopysOwnData)
{
	const auto original = MakeFile();
	L3DFile copy = original;
	ASSERT_EQ(copy.GetVertexSpan(0).size(), 2u);
	EXPECT_EQ(copy.GetVertexSpan(0).data(), copy.GetVertices().data());

	copy.EditVertices()[1].position.y = 50.0f;
	EXPECT_EQ(copy.GetVertexSpan(0)[1].position.y, 50.0f);
	EXPECT_EQ(original.GetVertexSpan(0)[1].position.y, 5.0f);
}

TEST(L3DFileCopy, AssignedSpansAreIntoTheirOwnData)
{
	const auto original = MakeFile();
	L3DFile assigned;
	assigned = original;
	EXPECT_EQ(assigned.GetVertexSpan(0).data(), assigned.GetVertices().data());
	EXPECT_EQ(assigned.GetVertexSpan(0)[0].position.z, 3.0f);
}

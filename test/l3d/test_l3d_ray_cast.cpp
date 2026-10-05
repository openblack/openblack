/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <L3DFile.h>
#include <gtest/gtest.h>

#include "3D/L3DRayCast.h"

using namespace openblack;
using namespace openblack::l3d;

namespace
{
/// A square across x and y, at z of 5, of two triangles in one primitive
L3DFile Square()
{
	L3DFile file;
	L3DSubmeshHeader header {};
	header.numPrimitives = 1;
	file.AddSubmesh(header);
	L3DPrimitiveHeader primitive {};
	primitive.numVertices = 4;
	primitive.numTriangles = 2;
	file.AddPrimitives({primitive});
	file.AddVertices({
	    L3DVertex {.position = {-1.0f, -1.0f, 5.0f}},
	    L3DVertex {.position = {1.0f, -1.0f, 5.0f}},
	    L3DVertex {.position = {1.0f, 1.0f, 5.0f}},
	    L3DVertex {.position = {-1.0f, 1.0f, 5.0f}},
	});
	file.AddIndices({0, 1, 2, 0, 2, 3});
	return file;
}
} // namespace

TEST(L3DRayCast, MeetsTheMeshFromEitherSide)
{
	const auto square = Square();
	EXPECT_FLOAT_EQ(*RayCast(square, {0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}), 5.0f);
	EXPECT_FLOAT_EQ(*RayCast(square, {-0.5f, 0.5f, 10.0f}, {0.0f, 0.0f, -2.0f}), 2.5f);
}

TEST(L3DRayCast, MissesBesideAndBehind)
{
	const auto square = Square();
	EXPECT_FALSE(RayCast(square, {2.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}).has_value());
	EXPECT_FALSE(RayCast(square, {0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 1.0f}).has_value());
}

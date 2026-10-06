/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstring>

#include <vector>

#include <L3DFile.h>
#include <gtest/gtest.h>

using namespace openblack::l3d;

namespace
{
constexpr uint32_t k_BlockOffset = 0x100;

/// A UV2 block as the temple's meshes have it, from after its size, vertex count and submesh count: the offsets of
/// the coordinates and of the lightmaps, the coordinates of three vertices and the lightmap of one submesh
struct FakeBlock
{
	std::vector<uint8_t> data;
	uint32_t size;
};

template <typename T>
void Append(std::vector<uint8_t>& data, const T& value)
{
	const auto at = data.size();
	data.resize(at + sizeof(T));
	std::memcpy(&data[at], &value, sizeof(T));
}

FakeBlock MakeBlock(uint32_t coordinatesOffset, uint32_t lightmapsOffset)
{
	FakeBlock block;
	Append(block.data, coordinatesOffset);
	Append(block.data, lightmapsOffset);
	for (const float value : {0.25f, 0.5f, 0.75f, 1.0f, 0.0f, 0.125f})
	{
		Append(block.data, value);
	}
	L3DLightmap lightmap {};
	lightmap.material.type = L3DMaterial::Type::Textured;
	lightmap.material.skinID = 0xF1CD84D1;
	lightmap.material.color.raw = 0xFF00BC09;
	Append(block.data, lightmap);
	block.size = static_cast<uint32_t>(block.data.size()) + 3 * sizeof(uint32_t);
	return block;
}
} // namespace

TEST(L3DLightmaps, ReadsTheCoordinatesAndEachSubmeshsLightmap)
{
	const auto block = MakeBlock(k_BlockOffset + 20, k_BlockOffset + 20 + 24);
	std::vector<L3DPoint2D> coordinates;
	std::vector<L3DLightmap> lightmaps;
	ASSERT_TRUE(DecodeLightmaps(block.data, k_BlockOffset, block.size, 3, 1, coordinates, lightmaps));
	ASSERT_EQ(coordinates.size(), 3);
	EXPECT_FLOAT_EQ(coordinates[0].x, 0.25f);
	EXPECT_FLOAT_EQ(coordinates[1].y, 1.0f);
	EXPECT_FLOAT_EQ(coordinates[2].y, 0.125f);
	ASSERT_EQ(lightmaps.size(), 1);
	EXPECT_EQ(lightmaps[0].material.type, L3DMaterial::Type::Textured);
	EXPECT_EQ(lightmaps[0].material.skinID, 0xF1CD84D1);
	EXPECT_EQ(lightmaps[0].material.color.raw, 0xFF00BC09);
}

TEST(L3DLightmaps, RefusesOffsetsOutsideTheBlock)
{
	std::vector<L3DPoint2D> coordinates;
	std::vector<L3DLightmap> lightmaps;
	// Before the block's header ends
	auto block = MakeBlock(k_BlockOffset + 8, k_BlockOffset + 20 + 24);
	EXPECT_FALSE(DecodeLightmaps(block.data, k_BlockOffset, block.size, 3, 1, coordinates, lightmaps));
	// Lightmaps past its end
	block = MakeBlock(k_BlockOffset + 20, k_BlockOffset + 20 + 32);
	EXPECT_FALSE(DecodeLightmaps(block.data, k_BlockOffset, block.size, 3, 1, coordinates, lightmaps));
	// More vertices than the block has room for
	block = MakeBlock(k_BlockOffset + 20, k_BlockOffset + 20 + 24);
	EXPECT_FALSE(DecodeLightmaps(block.data, k_BlockOffset, block.size, 8, 1, coordinates, lightmaps));
	EXPECT_TRUE(coordinates.empty());
	EXPECT_TRUE(lightmaps.empty());
}

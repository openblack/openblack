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
constexpr uint32_t k_DataOffset = 0x200;

/// A name block's data, after its size, count and offset: two records of windows, the second a volume light
std::vector<uint8_t> MakeData()
{
	std::vector<L3DSubmeshName> names(2);
	std::memcpy(names[0].name.data(), "Box118", 7);
	std::memcpy(names[1].name.data(), "Box139", 7);
	names[1].flags = L3DSubmeshName::VolumeLight;
	names[1].volumeLightSource = {-0.375f, 62.197f, 72.015f};
	names[1].volumeLightLength = 75.0f;
	std::vector<uint8_t> data(names.size() * sizeof(L3DSubmeshName));
	std::memcpy(data.data(), names.data(), data.size());
	return data;
}
} // namespace

TEST(L3DSubmeshNames, ReadsEachSubmeshsRecord)
{
	const auto data = MakeData();
	std::vector<L3DSubmeshName> names;
	ASSERT_TRUE(DecodeSubmeshNames(data, k_DataOffset, 2, k_DataOffset, names));
	ASSERT_EQ(names.size(), 2);
	EXPECT_STREQ(names[0].name.data(), "Box118");
	EXPECT_EQ(names[0].flags & L3DSubmeshName::VolumeLight, 0u);
	EXPECT_NE(names[1].flags & L3DSubmeshName::VolumeLight, 0u);
	EXPECT_FLOAT_EQ(names[1].volumeLightSource.y, 62.197f);
	EXPECT_FLOAT_EQ(names[1].volumeLightLength, 75.0f);
}

TEST(L3DSubmeshNames, RefusesRecordsOutsideTheData)
{
	const auto data = MakeData();
	std::vector<L3DSubmeshName> names;
	EXPECT_FALSE(DecodeSubmeshNames(data, k_DataOffset, 2, k_DataOffset - 4, names));
	EXPECT_FALSE(DecodeSubmeshNames(data, k_DataOffset, 3, k_DataOffset, names));
	EXPECT_TRUE(names.empty());
}

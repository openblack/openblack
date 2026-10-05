/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstring>

#include <array>
#include <string_view>
#include <vector>

#include <PackFile.h>
#include <gtest/gtest.h>

using openblack::pack::AudioBankSampleHeader;
using openblack::pack::PackFile;
using openblack::pack::PackResult;

namespace
{

void Append(std::vector<uint8_t>& bytes, const void* data, size_t size)
{
	const auto* begin = static_cast<const uint8_t*>(data);
	bytes.insert(bytes.end(), begin, begin + size);
}

void AppendBlock(std::vector<uint8_t>& bytes, std::string_view name, const std::vector<uint8_t>& body)
{
	std::array<char, 0x20> blockName {};
	std::memcpy(blockName.data(), name.data(), name.size());
	const auto size = static_cast<uint32_t>(body.size());
	Append(bytes, blockName.data(), blockName.size());
	Append(bytes, &size, sizeof(size));
	Append(bytes, body.data(), body.size());
}

/// A sound pack of one 4 byte sample, with a bank info block when given one
std::vector<uint8_t> SoundPack(const std::vector<uint32_t>& bankInfo)
{
	std::vector<uint8_t> bytes;
	Append(bytes, "LiOnHeAd", 8);

	std::vector<uint8_t> table;
	const std::array<uint16_t, 2> counts = {1, 0};
	Append(table, counts.data(), sizeof(counts));
	AudioBankSampleHeader sample {};
	sample.size = 4;
	Append(table, &sample, sizeof(sample));
	AppendBlock(bytes, "LHAudioBankSampleTable", table);
	AppendBlock(bytes, "LHAudioWaveData", {1, 2, 3, 4});

	if (!bankInfo.empty())
	{
		std::vector<uint8_t> info;
		Append(info, bankInfo.data(), bankInfo.size() * sizeof(bankInfo[0]));
		info.resize(532);
		AppendBlock(bytes, "LHFileSegmentBankInfo", info);
	}
	return bytes;
}

} // namespace

TEST(PackBankInfo, TheThirdWordMarksAMusicBank)
{
	PackFile music;
	ASSERT_EQ(music.Open(SoundPack({0, 0, 1})), PackResult::Success);
	EXPECT_TRUE(music.IsAudioMusicBank());

	PackFile ocean;
	ASSERT_EQ(ocean.Open(SoundPack({7, 6, 0})), PackResult::Success);
	EXPECT_FALSE(ocean.IsAudioMusicBank());
	EXPECT_EQ(ocean.GetAudioBankInfo().unknown0, 7);
	EXPECT_EQ(ocean.GetAudioBankInfo().unknown1, 6);
}

TEST(PackBankInfo, IsOptional)
{
	PackFile pack;
	ASSERT_EQ(pack.Open(SoundPack({})), PackResult::Success);
	EXPECT_FALSE(pack.IsAudioMusicBank());
}

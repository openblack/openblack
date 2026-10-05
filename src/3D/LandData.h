/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <vector>

#include <LNDFile.h>

namespace openblack
{
/// Everything a land is built from: its blocks, where each lies on the map, and what paints them. It is read from a
/// landscape file or generated, as the flat testbed is.
struct LandData
{
	/// The map is 32 by 32 blocks of 16 by 16 cells
	static constexpr int k_BlocksPerSide = 32;

	/// For each block of the map at [x * 32 + z], one more than its index in blocks, or 0 where there is no land. A
	/// landscape file holds a byte for each, so has at most 255 blocks; the whole map takes 1024.
	std::array<uint16_t, k_BlocksPerSide * k_BlocksPerSide> blockIndexLookup {};
	std::vector<lnd::LNDBlock> blocks;
	std::vector<lnd::LNDCountry> countries;
	std::vector<lnd::LNDMaterial> materials;
	/// The noise and bump maps, 256 by 256 texels each, [x * 256 + z]
	std::vector<uint8_t> noise;
	std::vector<uint8_t> bump;

	/// The land of a landscape file
	[[nodiscard]] static LandData FromLnd(const lnd::LNDFile& lnd);
};
} // namespace openblack

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <numeric>
#include <vector>

#include <LNDFile.h>
#include <gtest/gtest.h>

#include "3D/BlockTexture.h"

namespace block_texture = openblack::block_texture;
using openblack::lnd::LNDCell;
using openblack::lnd::LNDCountry;

namespace
{
constexpr size_t k_MapTexels = static_cast<size_t>(block_texture::k_Side) * block_texture::k_Side;
constexpr uint16_t k_White = 0x7FFF;

/// A country painting every height with the one material
LNDCountry SingleMaterialCountry(uint32_t material)
{
	LNDCountry country {};
	for (auto& entry : country.materials)
	{
		entry.indices = {material, material};
		entry.coefficient = 0;
	}
	return country;
}

std::vector<LNDCell> FlatCells(uint8_t altitude)
{
	std::vector<LNDCell> cells(static_cast<size_t>(block_texture::k_CellsPerSide * block_texture::k_CellsPerSide));
	for (auto& cell : cells)
	{
		cell.altitude = altitude;
	}
	return cells;
}

std::array<uint8_t, 4> TexelAt(const std::vector<uint8_t>& rgba, int x, int z)
{
	const auto index = static_cast<size_t>((x * block_texture::k_Side) + z) * 4;
	return {rgba[index], rgba[index + 1], rgba[index + 2], rgba[index + 3]};
}
} // namespace

TEST(BlockTexture, ConeWeightsAddUpTo255)
{
	for (const auto& weights : block_texture::ComputeConeWeights())
	{
		EXPECT_EQ(std::accumulate(weights.begin(), weights.end(), 0), 255);
	}
}

TEST(BlockTexture, ConeWeightsFavourTheNearestCorner)
{
	const auto weights = block_texture::ComputeConeWeights();
	EXPECT_EQ(weights[0], (std::array<uint8_t, 4> {255, 0, 0, 0}));
	// In the middle the corners are equally far: what truncation leaves goes to the last of them
	EXPECT_EQ(weights[(8 * 16) + 8], (std::array<uint8_t, 4> {63, 63, 63, 66}));
}

TEST(BlockTexture, CoastAlphaIsClearAtSeaLevelAndOpaqueInland)
{
	EXPECT_EQ(block_texture::CoastAlpha(0xFF, 0), 0);
	EXPECT_EQ(block_texture::CoastAlpha(0x400, 0), 15);
	EXPECT_EQ(block_texture::CoastAlpha(0xFFFF, 0x80), 15);
}

TEST(BlockTexture, CoastAlphaFollowsTheNoise)
{
	EXPECT_EQ(block_texture::CoastAlpha(0x300, 0), 10);
	// The noise counts as signed: high bytes lower the coast, low ones raise it
	EXPECT_EQ(block_texture::CoastAlpha(0x300, 0x80), 0);
	EXPECT_EQ(block_texture::CoastAlpha(0x300, 0x7F), 15);
}

TEST(BlockTexture, CountryTexelIsShadedByTheBumpMap)
{
	const auto country = SingleMaterialCountry(0);
	const std::vector<uint16_t> materials(k_MapTexels, k_White);
	// A bump of a half keeps white just within 4 bits; a quarter halves it again
	EXPECT_EQ(block_texture::CountryTexel(country, 0x400, 0, 0x80, materials, 0), 0xFFFF);
	EXPECT_EQ(block_texture::CountryTexel(country, 0x400, 0, 0x40, materials, 0), 0xF777);
	// Under altitude 1 nothing is painted
	EXPECT_EQ(block_texture::CountryTexel(country, 0xFF, 0, 0x80, materials, 0), 0);
}

TEST(BlockTexture, CountryTexelBlendsTwoMaterialsByTheCoefficient)
{
	LNDCountry country {};
	for (auto& entry : country.materials)
	{
		entry.indices = {0, 1};
		entry.coefficient = 0x100;
	}
	std::vector<uint16_t> materials(k_MapTexels * 2, 0);
	std::fill_n(materials.begin(), k_MapTexels, k_White);
	// All of the first material, none of the second
	EXPECT_EQ(block_texture::CountryTexel(country, 0x400, 0, 0x80, materials, 0), 0xFFFF);
	for (auto& entry : country.materials)
	{
		entry.coefficient = 0;
	}
	EXPECT_EQ(block_texture::CountryTexel(country, 0x400, 0, 0x80, materials, 0), 0xF000);
}

TEST(BlockTexture, BlendCornersWeighsEachChannel)
{
	EXPECT_EQ(block_texture::BlendCorners({0xFFFF, 0, 0, 0}, {255, 0, 0, 0}), 0xFFFF);
	EXPECT_EQ(block_texture::BlendCorners({0xFFFF, 0, 0, 0}, {63, 63, 63, 66}), 0x3333);
}

TEST(BlockTexture, BuildBlockPaintsHighLandOpaque)
{
	const std::array countries = {SingleMaterialCountry(0)};
	const std::vector<uint16_t> materials(k_MapTexels, k_White);
	const std::vector<uint8_t> noise(k_MapTexels, 0);
	const std::vector<uint8_t> bump(k_MapTexels, 0x80);
	const auto cells = FlatCells(10);
	std::vector<uint8_t> rgba(block_texture::k_BlockBytes, 0);
	block_texture::BuildBlock(cells, {countries, materials, noise, bump}, rgba);
	EXPECT_EQ(TexelAt(rgba, 0, 0), (std::array<uint8_t, 4> {255, 255, 255, 255}));
	EXPECT_EQ(TexelAt(rgba, 255, 255), (std::array<uint8_t, 4> {255, 255, 255, 255}));
}

TEST(BlockTexture, BuildBlockLeavesTheSeaClear)
{
	const std::array countries = {SingleMaterialCountry(0)};
	const std::vector<uint16_t> materials(k_MapTexels, k_White);
	const std::vector<uint8_t> noise(k_MapTexels, 0);
	const std::vector<uint8_t> bump(k_MapTexels, 0x80);
	auto cells = FlatCells(10);
	// Open sea in the cell at x 1, z 2
	cells[(1 * block_texture::k_CellsPerSide) + 2].flags = 0x02;
	std::vector<uint8_t> rgba(block_texture::k_BlockBytes, 0xAA);
	block_texture::BuildBlock(cells, {countries, materials, noise, bump}, rgba);
	EXPECT_EQ(TexelAt(rgba, 16, 32), (std::array<uint8_t, 4> {0, 0, 0, 0}));
	EXPECT_EQ(TexelAt(rgba, 31, 47), (std::array<uint8_t, 4> {0, 0, 0, 0}));
	EXPECT_EQ(TexelAt(rgba, 32, 32), (std::array<uint8_t, 4> {255, 255, 255, 255}));

	// Land at sea level is clear too
	const auto sea = FlatCells(0);
	block_texture::BuildBlock(sea, {countries, materials, noise, bump}, rgba);
	EXPECT_EQ(TexelAt(rgba, 100, 100), (std::array<uint8_t, 4> {0, 0, 0, 0}));
}

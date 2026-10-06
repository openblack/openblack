/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// A sound bank's animation effects, built from fake tables, and the tree rustles that play them

#include <cstring>

#include <array>
#include <chrono>
#include <vector>

#include <Audio/AnimEffectTable.h>
#include <ECS/Systems/TreeRustle.h>
#include <gtest/gtest.h>

using namespace openblack::audio;
using openblack::ecs::systems::TreeRustle;

namespace
{
constexpr int32_t k_Any = AnimEffectTable::k_AnyKey;

std::vector<uint8_t> ToBlock(const std::vector<int32_t>& words)
{
	std::vector<uint8_t> block(words.size() * sizeof(int32_t));
	std::memcpy(block.data(), words.data(), block.size());
	return block;
}

/// Five keys and a sample list for each row, as the banks hold them
struct FakeBank
{
	std::vector<std::array<int32_t, 5>> keys;
	std::vector<std::vector<int32_t>> samples;

	[[nodiscard]] AnimEffectTable Build() const
	{
		std::vector<int32_t> table {static_cast<int32_t>(keys.size()), 6};
		std::vector<int32_t> waves;
		for (size_t row = 0; row < keys.size(); ++row)
		{
			table.insert(table.end(), keys[row].begin(), keys[row].end());
			table.push_back(static_cast<int32_t>(waves.size()));
			waves.push_back(static_cast<int32_t>(samples[row].size()));
			waves.insert(waves.end(), samples[row].begin(), samples[row].end());
		}
		auto result = AnimEffectTable::Parse(ToBlock(table), ToBlock(waves));
		EXPECT_TRUE(result.has_value());
		return *result;
	}
};

std::vector<int32_t> Find(const AnimEffectTable& table, const AnimEffectKeys& keys)
{
	const auto found = table.Find(keys.ToArray());
	return {found.begin(), found.end()};
}
} // namespace

TEST(AnimEffectTable, RowsMatchOnEveryKeyOrAny)
{
	const auto table =
	    FakeBank {
	        .keys = {{1, k_Any, k_Any, 10, 75}, {2, k_Any, k_Any, 10, 75}, {k_Any, k_Any, 20, k_Any, 70}},
	        .samples = {{456, 457}, {458}, {307, 308, 309}},
	    }
	        .Build();
	ASSERT_EQ(table.GetKeyCount(), 5);
	ASSERT_EQ(table.GetRowCount(), 3);

	EXPECT_EQ(Find(table, TreeRustle::BendKeys(0.9f)), (std::vector<int32_t> {456, 457}));
	EXPECT_EQ(Find(table, TreeRustle::BendKeys(0.5f)), (std::vector<int32_t> {458}));
	EXPECT_EQ(Find(table, TreeRustle::IdleKeys()), (std::vector<int32_t> {307, 308, 309}));
	// No row for the small collision of a tree
	EXPECT_TRUE(Find(table, TreeRustle::BendKeys(0.1f)).empty());
}

TEST(AnimEffectTable, MostExactRowWinsLaterOnATie)
{
	const auto table =
	    FakeBank {
	        .keys = {{k_Any, k_Any, 20, k_Any, 70}, {k_Any, k_Any, 20, k_Any, 70}, {k_Any, k_Any, 20, 10, 70}},
	        .samples = {{1}, {2}, {3}},
	    }
	        .Build();

	EXPECT_EQ(Find(table, {.object = SoundObject::Tree, .surface = SoundSurface::Tree, .action = SoundAction::Tree}),
	          (std::vector<int32_t> {3}));
	EXPECT_EQ(Find(table, TreeRustle::IdleKeys()), (std::vector<int32_t> {2}));
}

TEST(AnimEffectTable, TooFewKeysFindNothing)
{
	const auto table = FakeBank {.keys = {{k_Any, k_Any, k_Any, k_Any, k_Any}}, .samples = {{1}}}.Build();
	const std::array<int32_t, 4> keys {0, 0, 0, 0};
	EXPECT_TRUE(table.Find(keys).empty());
}

TEST(AnimEffectTable, RejectsTablesThatDontFit)
{
	// More rows than the block holds
	EXPECT_FALSE(AnimEffectTable::Parse(ToBlock({2, 6, 1, 0, 0, 0, 0, 0}), ToBlock({1, 5})).has_value());
	// Samples beyond the end of the sample lists
	EXPECT_FALSE(AnimEffectTable::Parse(ToBlock({1, 6, 1, 0, 0, 0, 0, 1}), ToBlock({1, 5})).has_value());
	EXPECT_FALSE(AnimEffectTable::Parse(ToBlock({1, 6, 1, 0, 0, 0, 0, 0}), ToBlock({3, 5})).has_value());
	// A row with no samples is fine
	EXPECT_TRUE(AnimEffectTable::Parse(ToBlock({1, 6, 1, 0, 0, 0, 0, 0}), ToBlock({0})).has_value());
}

TEST(TreeRustle, BendPicksTheSizeOfTheCollision)
{
	EXPECT_EQ(TreeRustle::BendKeys(0.29f).size, SoundSize::Small);
	EXPECT_EQ(TreeRustle::BendKeys(0.3f).size, SoundSize::Medium);
	EXPECT_EQ(TreeRustle::BendKeys(0.66f).size, SoundSize::Medium);
	EXPECT_EQ(TreeRustle::BendKeys(0.67f).size, SoundSize::Large);
	EXPECT_EQ(TreeRustle::BendKeys(1.0f).ToArray(), (std::array<int32_t, 5> {1, 0, 0, 10, 75}));
	EXPECT_EQ(TreeRustle::IdleKeys().ToArray(), (std::array<int32_t, 5> {0, 0, 20, 0, 70}));
}

TEST(TreeRustle, TallTreesRustleAroundTheCamera)
{
	const glm::vec3 tree {100.0f, 20.0f, 200.0f};
	EXPECT_TRUE(TreeRustle::IsAmongTree(tree, 12.0f, {110.0f, 37.0f, 190.0f}));
	// Too far along x, z or above
	EXPECT_FALSE(TreeRustle::IsAmongTree(tree, 12.0f, {110.5f, 25.0f, 200.0f}));
	EXPECT_FALSE(TreeRustle::IsAmongTree(tree, 12.0f, {100.0f, 25.0f, 189.5f}));
	EXPECT_FALSE(TreeRustle::IsAmongTree(tree, 12.0f, {100.0f, 38.0f, 200.0f}));
	EXPECT_FALSE(TreeRustle::IsAmongTree(tree, 12.0f, {100.0f, 2.0f, 200.0f}));
	// Too short
	EXPECT_FALSE(TreeRustle::IsAmongTree(tree, 10.0f, {100.0f, 25.0f, 200.0f}));
}

TEST(TreeRustle, NearTheCameraIsAcrossTheGroundWhateverTheHeight)
{
	const glm::vec3 tree {100.0f, 20.0f, 200.0f};
	EXPECT_TRUE(TreeRustle::IsNearCamera(tree, {110.0f, 500.0f, 190.0f}));
	EXPECT_FALSE(TreeRustle::IsNearCamera(tree, {110.5f, 20.0f, 200.0f}));
	EXPECT_FALSE(TreeRustle::IsNearCamera(tree, {100.0f, 20.0f, 189.5f}));
	// Every tree the camera is among is near it
	EXPECT_TRUE(TreeRustle::IsNearCamera(tree, {110.0f, 37.0f, 190.0f}));
}

TEST(TreeRustle, AboutOnceASecondOfGameTime)
{
	using Milliseconds = std::chrono::duration<float, std::milli>;
	EXPECT_EQ(TreeRustle::IdleChance(Milliseconds(1000.0f / 30.0f)), 30);
	EXPECT_EQ(TreeRustle::IdleChance(Milliseconds(16.0f)), 62);
	EXPECT_EQ(TreeRustle::IdleChance(Milliseconds(1000.0f)), 1);
	// Never while paused
	EXPECT_EQ(TreeRustle::IdleChance(Milliseconds(0.0f)), 0);
}

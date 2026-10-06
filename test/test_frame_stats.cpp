/*******************************************************************************
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

#include <Common/FrameStats.h>
#include <gtest/gtest.h>

using namespace openblack::frame_stats;

TEST(FrameStats, EmptyRunIsEmpty)
{
	const auto summary = Summarise({});
	EXPECT_EQ(summary.count, 0u);
	EXPECT_FLOAT_EQ(summary.average, 0.0f);
	EXPECT_FLOAT_EQ(summary.p95, 0.0f);
	EXPECT_FLOAT_EQ(summary.max, 0.0f);
}

TEST(FrameStats, OneFrame)
{
	constexpr std::array k_Times {7.5f};
	const auto summary = Summarise(k_Times);
	EXPECT_EQ(summary.count, 1u);
	EXPECT_FLOAT_EQ(summary.average, 7.5f);
	EXPECT_FLOAT_EQ(summary.p95, 7.5f);
	EXPECT_FLOAT_EQ(summary.max, 7.5f);
}

TEST(FrameStats, HundredFramesTakeTheNinetyFifth)
{
	// 1 ms to 100 ms, shuffled out of order
	std::vector<float> times(100);
	std::iota(times.rbegin(), times.rend(), 1.0f);
	const auto summary = Summarise(times);
	EXPECT_EQ(summary.count, 100u);
	EXPECT_FLOAT_EQ(summary.average, 50.5f);
	EXPECT_FLOAT_EQ(summary.p95, 95.0f);
	EXPECT_FLOAT_EQ(summary.max, 100.0f);
}

TEST(FrameStats, OneSlowFrameInTwentyIsOutsideTheNinetyFifth)
{
	std::vector<float> times(20, 10.0f);
	times.at(3) = 50.0f;
	const auto summary = Summarise(times);
	EXPECT_FLOAT_EQ(summary.p95, 10.0f);
	EXPECT_FLOAT_EQ(summary.max, 50.0f);
	EXPECT_FLOAT_EQ(summary.average, 12.0f);
}

TEST(FrameStats, WindowStartsAgainAfterTaking)
{
	Window window;
	window.Add(4.0f);
	window.Add(6.0f);
	EXPECT_EQ(window.Count(), 2u);
	const auto first = window.Take();
	EXPECT_FLOAT_EQ(first.average, 5.0f);
	EXPECT_EQ(window.Count(), 0u);
	window.Add(9.0f);
	const auto second = window.Take();
	EXPECT_EQ(second.count, 1u);
	EXPECT_FLOAT_EQ(second.max, 9.0f);
}

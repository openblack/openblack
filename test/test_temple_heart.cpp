/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <vector>

#include <gtest/gtest.h>

#include "Physics/TempleHeart.h"

using namespace openblack::physics::temple_heart;

namespace
{
entt::entity E(uint32_t id)
{
	return static_cast<entt::entity>(id);
}

Building Standing(uint32_t id, float life)
{
	return {.entity = E(id), .available = true, .life = life, .built = 1.0f};
}
} // namespace

TEST(TempleHeart, ASoundBuildingTakesTheBlowAtOnce)
{
	const std::vector<Town> towns {
	    {.buildings = {Standing(1, 0.2f), Standing(2, 0.3f)}},
	    {.buildings = {Standing(3, 1.0f)}},
	};
	const auto target = Choose(towns);
	EXPECT_EQ(target.kind, TargetKind::Building);
	EXPECT_EQ(target.entity, E(2));
}

TEST(TempleHeart, OtherwiseTheFirstStandingBuildingOfAnyTown)
{
	auto field = Standing(1, 1.0f);
	field.field = true;
	auto pitch = Standing(2, 1.0f);
	pitch.footballPitch = true;
	auto unbuilt = Standing(3, 1.0f);
	unbuilt.built = 0.0f;
	auto ruin = Standing(4, 0.0f);
	const std::vector<Town> towns {
	    {.buildings = {field, pitch, unbuilt, ruin}},
	    {.buildings = {Standing(5, 0.25f), Standing(6, 0.1f)}},
	};
	const auto target = Choose(towns);
	EXPECT_EQ(target.kind, TargetKind::Building);
	EXPECT_EQ(target.entity, E(5));
}

TEST(TempleHeart, ThenAHomelessVillagerThenTheHeart)
{
	const std::vector<Town> homeless {
	    {.homeless = {{.entity = E(7), .available = false}}},
	    {.homeless = {{.entity = E(8), .available = true}, {.entity = E(9), .available = true}}},
	};
	const auto villager = Choose(homeless);
	EXPECT_EQ(villager.kind, TargetKind::Villager);
	EXPECT_EQ(villager.entity, E(8));
	EXPECT_EQ(Choose(std::vector<Town> {{}}).kind, TargetKind::Heart);
}

TEST(TempleHeart, TheHeartIsHarmedByTheBlowsMomentumUpToAFifth)
{
	EXPECT_FLOAT_EQ(Harm({0.0f, 3.0f, 4.0f}, 1000.0f), 0.025f);
	EXPECT_FLOAT_EQ(Harm({0.0f, 30.0f, 40.0f}, 1000.0f), 0.2f);
}

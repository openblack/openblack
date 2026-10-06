/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "3D/CreatureCaveTargets.h"

using namespace openblack;
using namespace openblack::CreatureCaveTargets;

namespace
{
using Places = std::array<std::optional<glm::vec2>, k_Count>;

Places Only(Target target, glm::vec2 place)
{
	Places places {};
	places.at(static_cast<size_t>(target)) = place;
	return places;
}
} // namespace

TEST(CreatureCaveTargets, NothingOnTheScreen)
{
	EXPECT_FALSE(TargetAt(Places {}, {320.0f, 240.0f}).has_value());
}

TEST(CreatureCaveTargets, TheExitWithinItsReach)
{
	const auto places = Only(Target::Exit, {300.0f, 200.0f});
	EXPECT_EQ(TargetAt(places, {300.0f, 200.0f}), Target::Exit);
	EXPECT_EQ(TargetAt(places, {340.0f, 250.0f}), Target::Exit);
	EXPECT_EQ(TargetAt(places, {260.0f, 150.0f}), Target::Exit);
	EXPECT_FALSE(TargetAt(places, {341.0f, 200.0f}).has_value());
	EXPECT_FALSE(TargetAt(places, {300.0f, 251.0f}).has_value());
}

TEST(CreatureCaveTargets, TheBeltsAndMedalsReachDiffer)
{
	EXPECT_EQ(TargetAt(Only(Target::Belts, {100.0f, 100.0f}), {100.0f, 160.0f}), Target::Belts);
	EXPECT_FALSE(TargetAt(Only(Target::Medals, {100.0f, 100.0f}), {100.0f, 160.0f}).has_value());
	EXPECT_EQ(TargetAt(Only(Target::Medals, {100.0f, 100.0f}), {165.0f, 100.0f}), Target::Medals);
}

TEST(CreatureCaveTargets, TheCreatureReachesOnlyItsPixelBelowItsPoint)
{
	const auto places = Only(Target::Creature, {100.0f, 100.0f});
	EXPECT_EQ(TargetAt(places, {100.0f, 110.0f}), Target::Creature);
	EXPECT_FALSE(TargetAt(places, {100.0f, 100.0f}).has_value());
	EXPECT_FALSE(TargetAt(places, {101.0f, 110.0f}).has_value());
}

TEST(CreatureCaveTargets, TheReachGrowsWithTheScreen)
{
	const auto places = Only(Target::Exit, {960.0f, 540.0f});
	// Out of the game's reach, but in it scaled from 600 high to 1080
	EXPECT_FALSE(TargetAt(places, {960.0f, 620.0f}).has_value());
	EXPECT_EQ(TargetAt(places, {960.0f, 620.0f}, 1080.0f), Target::Exit);
	EXPECT_EQ(TargetAt(places, {1032.0f, 630.0f}, 1080.0f), Target::Exit);
	EXPECT_FALSE(TargetAt(places, {1033.0f, 540.0f}, 1080.0f).has_value());
}

TEST(CreatureCaveTargets, TheLastInReachWins)
{
	Places places {};
	places.at(static_cast<size_t>(Target::Belts)) = glm::vec2(100.0f, 100.0f);
	places.at(static_cast<size_t>(Target::Exit)) = glm::vec2(110.0f, 100.0f);
	EXPECT_EQ(TargetAt(places, {105.0f, 100.0f}), Target::Exit);
}

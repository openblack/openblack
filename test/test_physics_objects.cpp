/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <glm/gtx/euler_angles.hpp>
#include <gtest/gtest.h>

#include "Physics/ObjectRules.h"

using namespace openblack::physics::objects;

TEST(PhysicsObjects, ATreePutDownGentlyOnLandTakesRootAgain)
{
	EXPECT_EQ(TreeLandingOf(true, false, true, false), TreeLanding::Replanted);
	// Burning, in the water, or thrown, it falls dead
	EXPECT_EQ(TreeLandingOf(true, false, true, true), TreeLanding::Dies);
	EXPECT_EQ(TreeLandingOf(true, false, false, false), TreeLanding::Dies);
	EXPECT_EQ(TreeLandingOf(false, false, true, false), TreeLanding::Dies);
	// A forest miracle's tree put down gently just stays, wherever it is
	EXPECT_EQ(TreeLandingOf(true, true, false, true), TreeLanding::Stays);
	EXPECT_EQ(TreeLandingOf(false, true, true, false), TreeLanding::Dies);
}

TEST(PhysicsObjects, AReplantedTreeJoinsTheForestOfTheNearestTree)
{
	ForestSearch search;
	EXPECT_TRUE(search.Meet({.edgeDistance = 12.0f, .forest = 3u}));
	EXPECT_TRUE(search.Meet({.edgeDistance = 4.0f, .forest = 7u}));
	EXPECT_TRUE(search.Meet({.edgeDistance = 9.0f, .forest = 5u}));
	// Things that are neither trees of a forest nor a town's change nothing
	EXPECT_TRUE(search.Meet({.edgeDistance = 1.0f}));
	ASSERT_TRUE(search.Forest().has_value());
	EXPECT_EQ(*search.Forest(), 7u);
	EXPECT_FALSE(search.NearTown());
	EXPECT_FALSE(search.StartsForest());
}

TEST(PhysicsObjects, NearATownATreeJoinsTheTownsForestOrNone)
{
	ForestSearch withForest;
	EXPECT_TRUE(withForest.Meet({.edgeDistance = 2.0f, .forest = 7u}));
	// The town's forest wins over trees met before and after, and the rest of the cell is skipped
	EXPECT_FALSE(withForest.Meet({.edgeDistance = 20.0f, .ofTown = true, .townForest = 9u}));
	EXPECT_TRUE(withForest.Meet({.edgeDistance = 1.0f, .forest = 4u}));
	EXPECT_EQ(withForest.Forest(), 9u);
	EXPECT_TRUE(withForest.NearTown());

	// A town without a forest leaves the tree in none, and it starts none either
	ForestSearch withoutForest;
	EXPECT_FALSE(withoutForest.Meet({.edgeDistance = 24.9f, .ofTown = true}));
	EXPECT_FALSE(withoutForest.Forest().has_value());
	EXPECT_FALSE(withoutForest.StartsForest());

	// A town's thing beyond reach of its edge is no matter, and alone the tree starts a forest of its own
	ForestSearch far;
	EXPECT_TRUE(far.Meet({.edgeDistance = 25.0f, .ofTown = true}));
	EXPECT_FALSE(far.NearTown());
	EXPECT_TRUE(far.StartsForest());
}

TEST(PhysicsObjects, HardKnocksWearRocksAway)
{
	EXPECT_FALSE(RockWear(4.0f, 2.0f, false).has_value());
	EXPECT_FALSE(RockWear(10.0f, 0.7f, false).has_value());
	EXPECT_FALSE(RockWear(10.0f, 2.0f, true).has_value());
	ASSERT_TRUE(RockWear(10.0f, 2.0f, false).has_value());
	EXPECT_FLOAT_EQ(*RockWear(10.0f, 2.0f, false), 0.03f);
	// Worn below a hundredth it breaks: a 2 m rock from full life needs a knock of more than 202 g at once
	EXPECT_LT(1.0f - *RockWear(203.0f, 2.0f, false), k_RockBreakLife);
}

TEST(PhysicsObjects, OnlyTallRocksBreakWhenTapped)
{
	EXPECT_FALSE(RockBreaksWhenTapped(0.7f));
	EXPECT_TRUE(RockBreaksWhenTapped(0.71f));
}

TEST(PhysicsObjects, RockHalvesAreHalfItsVolume)
{
	const float volume = k_RockHalfScale * k_RockHalfScale * k_RockHalfScale;
	EXPECT_NEAR(volume, 0.5f, 0.001f);
}

TEST(PhysicsObjects, HeadingOnlyStandsAThingUpright)
{
	const glm::mat3 tilted = glm::mat3(glm::eulerAngleYXZ(0.8f, 0.3f, -0.2f));
	const auto upright = HeadingOnly(tilted);
	EXPECT_NEAR(upright[1].y, 1.0f, 1e-6f);
	EXPECT_NEAR(upright[2].y, 0.0f, 1e-6f);
	// The heading is kept
	EXPECT_NEAR(std::atan2(upright[2].x, upright[2].z), std::atan2(tilted[2].x, tilted[2].z), 1e-5f);
}

TEST(PhysicsObjects, ATreeThatFallsDeadDropsItsRootsToTheLand)
{
	// They fall faster and faster from where the tree lies, and stop just above the land
	EXPECT_FLOAT_EQ(RootsHeight(10.0f, 2.0f, 0.0f), 10.0f);
	EXPECT_FLOAT_EQ(RootsHeight(10.0f, 2.0f, 0.5f), 10.0f - 20.0f * 0.25f);
	EXPECT_FLOAT_EQ(RootsHeight(10.0f, 2.0f, 1.0f), 2.0f);
}

TEST(PhysicsObjects, FallenRootsFadeAfterEighteenSecondsAndGoAfterTwenty)
{
	EXPECT_FALSE(RootsAlpha(18.0f).has_value());
	ASSERT_TRUE(RootsAlpha(19.0f).has_value());
	EXPECT_EQ(*RootsAlpha(19.0f), 127);
	EXPECT_EQ(*RootsAlpha(20.0f), 0);
	EXPECT_FALSE(RootsGone(20.0f));
	EXPECT_TRUE(RootsGone(20.01f));
}

TEST(PhysicsObjects, AFelledTreeFallsAwayFromItsFellerAtAFifthOfItsHeight)
{
	using namespace openblack::physics::objects;
	// The feller stands to the south (lower z) of a tree 10 tall: it falls north, turning about the east-west axis
	const auto north = FellingOf(10.0f, {0.0f, -5.0f});
	EXPECT_NEAR(north.velocity.x, 0.0f, 1e-5f);
	EXPECT_NEAR(north.velocity.z, -2.0f, 1e-5f);
	EXPECT_NEAR(north.spin.x, 0.4f, 1e-5f);
	EXPECT_NEAR(north.spin.z, 0.0f, 1e-5f);
	// Felled from the west it falls east
	const auto east = FellingOf(10.0f, {5.0f, 0.0f});
	EXPECT_NEAR(east.velocity.x, 2.0f, 1e-5f);
	EXPECT_NEAR(east.velocity.z, 0.0f, 1e-5f);
	EXPECT_NEAR(east.spin.z, 0.4f, 1e-5f);
	// From where it stands it has no heading
	const auto still = FellingOf(10.0f, {0.0f, 0.0f});
	EXPECT_NEAR(still.velocity.z, -2.0f, 1e-5f);
}

TEST(PhysicsObjects, AnArtefactImpressesOnlyAnotherTownAndOnlyWhenWorthMoreThanOne)
{
	using namespace openblack::physics::objects;
	EXPECT_TRUE(ArtefactWillImpress(1.5f, false));
	EXPECT_FALSE(ArtefactWillImpress(1.5f, true));
	EXPECT_FALSE(ArtefactWillImpress(1.0f, false));
	EXPECT_FALSE(ArtefactWillImpress(0.01f, false));
}

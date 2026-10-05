/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include "Creature/CreatureMorph.h"

using namespace openblack;
using A = creature::CreatureBody::Appearance;

TEST(CreatureMorph, AlignmentIsTheEvilGoodAxis)
{
	EXPECT_FLOAT_EQ(creature_morph::FromAttributes(-0.4f, 0.5f, 0.5f, 0.5f).evilGood, -0.4f);
	EXPECT_FLOAT_EQ(creature_morph::FromAttributes(3.0f, 0.5f, 0.5f, 0.5f).evilGood, 1.0f);
}

TEST(CreatureMorph, FatnessRunsFromThinToFat)
{
	EXPECT_FLOAT_EQ(creature_morph::FromAttributes(0.0f, 0.0f, 0.5f, 0.5f).thinFat, -1.0f);
	EXPECT_FLOAT_EQ(creature_morph::FromAttributes(0.0f, 0.5f, 0.5f, 0.5f).thinFat, 0.0f);
	EXPECT_FLOAT_EQ(creature_morph::FromAttributes(0.0f, 0.75f, 0.5f, 0.5f).thinFat, 0.5f);
}

TEST(CreatureMorph, StrengthCountsFourTimesTheSpecies)
{
	// 0.2 of the species' strength and 0.8 of the creature's
	EXPECT_FLOAT_EQ(creature_morph::FromAttributes(0.0f, 0.5f, 1.0f, 0.0f).weakStrong, 0.6f);
	EXPECT_FLOAT_EQ(creature_morph::FromAttributes(0.0f, 0.5f, 0.0f, 1.0f).weakStrong, -0.6f);
	EXPECT_FLOAT_EQ(creature_morph::FromAttributes(0.0f, 0.5f, 1.0f, 1.0f).weakStrong, 1.0f);
}

TEST(CreatureMorph, SizeIsKeptBetweenItsLimits)
{
	EXPECT_FLOAT_EQ(creature_morph::ClampScale(0.0f), 0.05f);
	EXPECT_FLOAT_EQ(creature_morph::ClampScale(1.5f), 1.5f);
	EXPECT_FLOAT_EQ(creature_morph::ClampScale(9.0f), 4.0f);
}

TEST(CreatureMorph, EachAxisPullsTowardsTheMeshOnItsSide)
{
	EXPECT_EQ(creature_morph::EvilGoodMesh(-0.1f), A::Evil);
	EXPECT_EQ(creature_morph::EvilGoodMesh(0.0f), A::Good);
	EXPECT_EQ(creature_morph::ThinFatMesh(-1.0f), A::Thin);
	EXPECT_EQ(creature_morph::ThinFatMesh(0.3f), A::Fat);
	EXPECT_EQ(creature_morph::WeakStrongMesh(-0.5f), A::Weak);
	EXPECT_EQ(creature_morph::WeakStrongMesh(1.0f), A::Strong);
}

TEST(CreatureMorph, BlendMovesTheBaseTowardsEachMesh)
{
	const glm::vec3 base {0.0f, 0.0f, 0.0f};
	const glm::vec3 evilGood {1.0f, 0.0f, 0.0f};
	const glm::vec3 thinFat {0.0f, 2.0f, 0.0f};
	const glm::vec3 weakStrong {0.0f, 0.0f, 4.0f};
	const auto blended =
	    creature_morph::Blend(base, evilGood, thinFat, weakStrong, {.evilGood = -0.5f, .thinFat = 0.25f, .weakStrong = 1.0f});
	EXPECT_FLOAT_EQ(blended.x, 0.5f);
	EXPECT_FLOAT_EQ(blended.y, 0.5f);
	EXPECT_FLOAT_EQ(blended.z, 4.0f);
	EXPECT_EQ(creature_morph::Blend(base, evilGood, thinFat, weakStrong, {}), base);
}

TEST(CreatureMorph, NearestMeshIsTheFurthestAxisPastHalfway)
{
	EXPECT_EQ(creature_morph::NearestMesh({}), A::Base);
	EXPECT_EQ(creature_morph::NearestMesh({.evilGood = 0.4f, .thinFat = -0.45f, .weakStrong = 0.2f}), A::Base);
	EXPECT_EQ(creature_morph::NearestMesh({.evilGood = -0.9f, .thinFat = 0.6f}), A::Evil);
	EXPECT_EQ(creature_morph::NearestMesh({.evilGood = 0.2f, .thinFat = -0.7f}), A::Thin);
	EXPECT_EQ(creature_morph::NearestMesh({.weakStrong = 0.8f}), A::Strong);
}

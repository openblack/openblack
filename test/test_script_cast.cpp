/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Magic/ScriptCast.h"

using namespace openblack;

TEST(ScriptCast, TheScriptsCurlSpinsTheMiracle)
{
	const auto cast = magic::MakeScriptCast(150.0f, {100.0f, 5.0f, 200.0f}, {90.0f, 30.0f, 200.0f}, 25.0f, 12.0f, 0.75f);
	EXPECT_FLOAT_EQ(cast.info.spin, 0.75f);
	EXPECT_FLOAT_EQ(cast.cast.magnitude, 25.0f);
	EXPECT_FLOAT_EQ(cast.cast.duration, 12.0f);
	EXPECT_FLOAT_EQ(cast.cast.chants, 150.0f);
	EXPECT_EQ(cast.cast.maxObjectsToCreate, -1);
	// It starts from where it is cast from, looking at the point, still and at full strength
	EXPECT_FLOAT_EQ(cast.info.handPosition.y, 30.0f);
	EXPECT_NEAR(cast.info.cameraForward.x, 10.0f, 1e-3f);
	EXPECT_FLOAT_EQ(cast.info.cameraForward.y, -25.0f);
	EXPECT_EQ(cast.info.direction, glm::vec3(0.0f));
	EXPECT_FLOAT_EQ(cast.info.power, 1.0f);
	EXPECT_TRUE(cast.info.enabled);
}

TEST(ScriptCast, OnlyTheMagicTypesFromOneToFortyOneMayBeCast)
{
	EXPECT_FALSE(magic::IsScriptMagicType(0));
	EXPECT_TRUE(magic::IsScriptMagicType(1));
	EXPECT_TRUE(magic::IsScriptMagicType(41));
	EXPECT_FALSE(magic::IsScriptMagicType(42));
}

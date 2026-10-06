/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Creature/CreatureFace.h"

using namespace openblack::creature_face;

namespace
{
Request Pulled(Cue cue, uint32_t variety = 0, float attitude = 0.0f, uint32_t idlePick = 0)
{
	const auto face = Choose(cue, {.variety = variety, .attitudeToPlayer = attitude, .idlePick = idlePick});
	EXPECT_TRUE(face.has_value());
	return face.value_or(Request {});
}
} // namespace

TEST(CreatureFace, FacesAreTheSpecFilesFaceAnimations)
{
	EXPECT_EQ(AnimationOf(Face::Smile), 16u);
	EXPECT_EQ(AnimationOf(Face::Aah), 25u);
	EXPECT_EQ(AnimationOf(Face::Spare2), 27u);
	EXPECT_EQ(FaceOf(20), Face::Sad);
	EXPECT_FALSE(FaceOf(15).has_value());
	EXPECT_FALSE(FaceOf(28).has_value());
	EXPECT_EQ(Name(Face::Ooh), "ooh");
	EXPECT_EQ(Name(Cue::AttitudeToPlayer), "attitude to player");
	EXPECT_EQ(Name(Cue::Told), "told");
}

TEST(CreatureFace, IdlingPullsAnyOfTenFacesForThreeSeconds)
{
	for (uint32_t pick = 0; pick < k_IdleFaceCount; ++pick)
	{
		const auto face = Pulled(Cue::Idle, 0, 0.0f, pick);
		EXPECT_EQ(static_cast<uint32_t>(face.face), pick);
		EXPECT_FLOAT_EQ(face.milliseconds, 3000.0f);
		EXPECT_EQ(face.cue, Cue::Idle);
	}
}

TEST(CreatureFace, TheAttitudeToThePlayerShowsAsASmileOrSadness)
{
	EXPECT_EQ(Pulled(Cue::AttitudeToPlayer, 0, 0.5f).face, Face::Smile);
	EXPECT_EQ(Pulled(Cue::AttitudeToPlayer, 0, 0.0f).face, Face::Smile);
	EXPECT_EQ(Pulled(Cue::AttitudeToPlayer, 0, -0.1f).face, Face::Sad);
	EXPECT_FLOAT_EQ(Pulled(Cue::AttitudeToPlayer).milliseconds, 2000.0f);
}

TEST(CreatureFace, SomeFeelingsPickBetweenFacesByTheVariety)
{
	EXPECT_EQ(Pulled(Cue::Fear, 0).face, Face::Scared);
	EXPECT_FLOAT_EQ(Pulled(Cue::Fear, 0).milliseconds, 2000.0f);
	EXPECT_EQ(Pulled(Cue::Fear, 1).face, Face::Ooh);
	EXPECT_FLOAT_EQ(Pulled(Cue::Fear, 1).milliseconds, 500.0f);
	EXPECT_EQ(Pulled(Cue::Anger, 2).face, Face::Growl);
	EXPECT_EQ(Pulled(Cue::Anger, 3).face, Face::Grimace);
	EXPECT_EQ(Pulled(Cue::Compassion, 1).face, Face::Aah);
	EXPECT_EQ(Pulled(Cue::Playfulness, 1).face, Face::Puzzled);
	EXPECT_EQ(Pulled(Cue::Curiosity, 0).face, Face::Puzzled);
	EXPECT_EQ(Pulled(Cue::Curiosity, 1).face, Face::Amazed);
	EXPECT_EQ(Pulled(Cue::Curiosity, 2).face, Face::Aah);
	EXPECT_FLOAT_EQ(Pulled(Cue::Curiosity, 2).milliseconds, 800.0f);
}

TEST(CreatureFace, MoodsHaveTheirFaces)
{
	EXPECT_EQ(Pulled(Cue::Smile).face, Face::Smile);
	EXPECT_EQ(Pulled(Cue::Sad).face, Face::Sad);
	EXPECT_EQ(Pulled(Cue::Grimace).face, Face::Grimace);
	EXPECT_EQ(Pulled(Cue::Exhausted).face, Face::Grimace);
	EXPECT_FLOAT_EQ(Pulled(Cue::Exhausted).milliseconds, 1000.0f);
	EXPECT_EQ(Pulled(Cue::Frightened, 0).face, Face::Scared);
	EXPECT_EQ(Pulled(Cue::Frightened, 1).face, Face::Ooh);
	EXPECT_EQ(Pulled(Cue::Amazed).face, Face::Amazed);
	EXPECT_FLOAT_EQ(Pulled(Cue::Puzzled).milliseconds, 3000.0f);
}

TEST(CreatureFace, CuesWithFacesOfTheirOwnPullNoneByThemselves)
{
	EXPECT_FALSE(Choose(Cue::None, {}).has_value());
	EXPECT_FALSE(Choose(Cue::Told, {}).has_value());
	EXPECT_FALSE(Choose(Cue::Stroked, {}).has_value());
}

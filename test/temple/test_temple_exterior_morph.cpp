/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <numeric>
#include <vector>

#include <gtest/gtest.h>

#include "3D/TempleExteriorMorph.h"

using namespace openblack::TempleExteriorMorph;

namespace
{
float WeightOf(const std::array<Corner, 4>& corners, uint32_t size, uint32_t stage)
{
	float weight = 0.0f;
	for (const auto& corner : corners)
	{
		if (corner.size == size && corner.stage == stage)
		{
			weight += corner.weight;
		}
	}
	return weight;
}
} // namespace

TEST(TempleExteriorMorph, StepsTowardWhereItIsToBe)
{
	EXPECT_FLOAT_EQ(Step(0.5f, 1.0f), 0.516f);
	EXPECT_FLOAT_EQ(Step(0.5f, 0.0f), 0.484f);
	EXPECT_FLOAT_EQ(Step(0.99f, 1.0f), 1.0f);
	// Near enough, it is there
	EXPECT_FLOAT_EQ(Step(0.9995f, 1.0f), 1.0f);
}

TEST(TempleExteriorMorph, ASmallNeutralTempleIsTheMiddleStage)
{
	const auto corners = Corners(0.0f, 0.5f);
	EXPECT_FLOAT_EQ(WeightOf(corners, 0, 2), 1.0f);
	EXPECT_FLOAT_EQ(std::accumulate(corners.begin(), corners.end(), 0.0f,
	                                [](float sum, const Corner& corner) { return sum + corner.weight; }),
	                1.0f);
}

TEST(TempleExteriorMorph, BetweenStagesAndSizesItBlendsTheFour)
{
	const auto corners = Corners(0.25f, 0.625f);
	// Half of size 0 and 1, and half of stage 2 and 3
	EXPECT_FLOAT_EQ(WeightOf(corners, 0, 2), 0.25f);
	EXPECT_FLOAT_EQ(WeightOf(corners, 1, 2), 0.25f);
	EXPECT_FLOAT_EQ(WeightOf(corners, 0, 3), 0.25f);
	EXPECT_FLOAT_EQ(WeightOf(corners, 1, 3), 0.25f);
}

TEST(TempleExteriorMorph, TheEndsAreTheirOwnMeshes)
{
	EXPECT_FLOAT_EQ(WeightOf(Corners(0.0f, 0.0f), 0, 0), 1.0f);
	EXPECT_FLOAT_EQ(WeightOf(Corners(1.0f, 1.0f), 2, 4), 1.0f);
}

TEST(TempleExteriorMorph, MeshNames)
{
	EXPECT_EQ(MeshName(1, 4), "b_temple14_l3d");
}

TEST(TempleExteriorMorph, TexturesGoEvilToNeutralToGood)
{
	const auto evil = TextureOf(0.0f);
	EXPECT_EQ(evil.from, Look::Evil);
	EXPECT_EQ(evil.to, Look::Neutral);
	EXPECT_EQ(evil.weight, 0);
	EXPECT_EQ(TextureOf(0.25f).weight, 127);
	EXPECT_EQ(TextureOf(0.5f).to, Look::Neutral);
	EXPECT_EQ(TextureOf(0.5f).weight, 255);
	const auto good = TextureOf(1.0f);
	EXPECT_EQ(good.from, Look::Neutral);
	EXPECT_EQ(good.to, Look::Good);
	EXPECT_EQ(good.weight, 255);
	EXPECT_EQ(ImageName(Look::Good, 3), "good3");
}

TEST(TempleExteriorMorph, TexelsBlendByChannelKeepingTheFirstsAlpha)
{
	const std::vector<uint16_t> from {0xF000, 0x1FFF};
	const std::vector<uint16_t> to {0x0FFF, 0x0000};
	std::vector<uint16_t> blended(2);
	BlendTexels(from, to, 255, blended);
	EXPECT_EQ(blended[0], 0xFFFF);
	EXPECT_EQ(blended[1], 0x1000);
	BlendTexels(from, to, 0, blended);
	EXPECT_EQ(blended, from);
	// Halfway, 15 by 128 over 255 is 7 in each
	BlendTexels(from, to, 128, blended);
	EXPECT_EQ(blended[0], 0xF777);
}

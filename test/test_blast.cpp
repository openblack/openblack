/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Blast/CameraShake.h"
#include "Blast/DustPuff.h"
#include "Graphics/ModelLight.h"
#include "Particles/ParticleBlast.h"
#include "Particles/ParticleObjectEffects.h"

using namespace openblack;
using namespace openblack::particles;

namespace
{
constexpr float k_Epsilon = 1e-4f;
} // namespace

TEST(Blast, ItActsFromItsInitialDelayForItsTimeToDoEvents)
{
	const blast::ExplosionRules rules {.timeToDoEventsFor = 5.0f, .initialDelay = 0.4f};
	EXPECT_FALSE(blast::SendsEvents(0.3f, rules));
	EXPECT_TRUE(blast::SendsEvents(0.45f, rules));
	EXPECT_TRUE(blast::SendsEvents(5.3f, rules));
	EXPECT_FALSE(blast::SendsEvents(5.45f, rules));
}

TEST(Blast, ItsWaveReachesFurtherWithTribalPowerAndReachesObjectsByTheirEdge)
{
	EXPECT_FLOAT_EQ(blast::WaveRange(15.0f, 0.5f), 15.0f);
	EXPECT_FLOAT_EQ(blast::WaveRange(15.0f, 2.0f), 30.0f);
	EXPECT_FLOAT_EQ(blast::WaveRange(15.0f, 9.0f), 75.0f);
	const WaveTarget hut {.point = {10.0f, 0.0f, 0.0f}, .radius = 4.0f};
	EXPECT_FALSE(blast::Reached(5.9f, hut, glm::vec3(0.0f)));
	EXPECT_TRUE(blast::Reached(6.0f, hut, glm::vec3(0.0f)));
}

TEST(Blast, PiecesFlyOutFromBelowTheCentreAtTheBlastsSpeed)
{
	EXPECT_EQ(blast::FragmentOrigin({1.0f, 2.0f, 3.0f}), glm::vec3(1.0f, -3.0f, 3.0f));
	// Straight out at 50, a random part of 0.889 of that, half of it upwards
	const auto velocity =
	    blast::FragmentVelocity({0.0f, 0.0f, 0.0f}, {0.0f, -5.0f, 0.0f}, 50.0f, 0.889381f, {0.0f, 1.0f, 0.0f});
	EXPECT_NEAR(velocity.y, 50.0f + 0.5f * 50.0f * 0.889381f, 1e-3f);
	const auto sideways = blast::FragmentVelocity({3.0f, -5.0f, 4.0f}, {0.0f, -5.0f, 0.0f}, 50.0f, 0.0f, {1.0f, 0.0f, 0.0f});
	EXPECT_NEAR(glm::length(sideways), 50.0f, 1e-3f);
	EXPECT_NEAR(sideways.x, 30.0f, 1e-3f);
}

TEST(Blast, TheBeamDrops120MetresIn04Seconds)
{
	EXPECT_FALSE(blast::MoveFraction(0.5f, 0.1f, 0.0f, 0.4f, false).has_value());
	EXPECT_NEAR(*blast::MoveFraction(0.1f, 0.1f, 0.0f, 0.4f, false), 0.25f, k_Epsilon);
	// The step that takes it past the end puts it there
	EXPECT_FLOAT_EQ(*blast::MoveFraction(0.35f, 0.1f, 0.0f, 0.4f, false), 1.0f);
	EXPECT_NEAR(*blast::MoveFraction(0.1f, 0.1f, 0.0f, 0.4f, true), 0.25f * 0.25f * 2.5f, k_Epsilon);
}

TEST(Blast, TheConesKeepTheirHeightWhateverTheirWidth)
{
	const auto scale = blast::ScaleXYZ(0.1f, 8.0f);
	EXPECT_FLOAT_EQ(scale.across, 0.1f);
	EXPECT_NEAR(scale.across * scale.stretch, 8.0f, k_Epsilon);
	EXPECT_FLOAT_EQ(blast::ScaleXYZ(0.0f, 8.0f).stretch, 0.0f);
}

TEST(Blast, AModelBreaksIntoChainsOfAtMostSixteenJoinedTriangles)
{
	// A strip of 20 triangles sharing edges, and one triangle on its own
	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> uvs;
	for (int i = 0; i < 22; ++i)
	{
		positions.emplace_back(static_cast<float>(i / 2), static_cast<float>(i % 2), 0.0f);
		uvs.emplace_back(0.0f);
	}
	positions.emplace_back(100.0f, 0.0f, 0.0f);
	positions.emplace_back(101.0f, 0.0f, 0.0f);
	positions.emplace_back(100.0f, 1.0f, 0.0f);
	uvs.resize(positions.size());
	std::vector<uint16_t> indices;
	for (uint16_t i = 0; i < 20; ++i)
	{
		indices.insert(indices.end(), {i, static_cast<uint16_t>(i + 1), static_cast<uint16_t>(i + 2)});
	}
	indices.insert(indices.end(), {22, 23, 24});
	const std::array primitives {blast::SourcePrimitive {.positions = positions, .uvs = uvs, .indices = indices, .skin = 7}};
	const auto pieces = blast::BreakIntoPieces(primitives);
	ASSERT_EQ(pieces.size(), 3u);
	EXPECT_EQ(pieces[0].skins.size(), 16u);
	EXPECT_EQ(pieces[1].skins.size(), 4u);
	EXPECT_EQ(pieces[2].skins.size(), 1u);
	EXPECT_EQ(pieces[0].positions.size(), 48u);
	EXPECT_EQ(pieces[0].skins[0], 7u);
	// Placed, a piece's corners are about its middle
	const auto placed = blast::PlaceFragment(pieces[2], glm::mat4(1.0f), 0);
	EXPECT_NEAR(placed.centre.x, 100.0f + 1.0f / 3.0f, k_Epsilon);
	glm::vec3 sum(0.0f);
	for (const auto& corner : placed.shape->positions)
	{
		sum += corner;
	}
	EXPECT_NEAR(glm::length(sum), 0.0f, k_Epsilon);
}

TEST(CameraShake, OnlyTheNearestCountsAtFullStrengthWithinItsRadiusFallingOverItsTime)
{
	std::vector<camera_shake::Shake> shakes {
	    {.position = {0.0f, 0.0f, 0.0f},
	     .radius = 200.0f,
	     .strength = 1.0f,
	     .milliseconds = 700.0f,
	     .millisecondsLeft = 700.0f},
	    {.position = {500.0f, 0.0f, 0.0f},
	     .radius = 200.0f,
	     .strength = 1.0f,
	     .milliseconds = 700.0f,
	     .millisecondsLeft = 700.0f},
	};
	EXPECT_FLOAT_EQ(camera_shake::Amplitude(shakes, {150.0f, 0.0f, 0.0f}), 1.0f);
	EXPECT_FLOAT_EQ(camera_shake::Amplitude(shakes, {250.0f, 0.0f, 0.0f}), 0.0f);
	// Exactly at the radius is outside it
	EXPECT_FLOAT_EQ(camera_shake::Amplitude(shakes, {-200.0f, 0.0f, 0.0f}), 0.0f);
	camera_shake::Advance(shakes, 350.0f);
	EXPECT_NEAR(camera_shake::Amplitude(shakes, glm::vec3(0.0f)), 0.5f, k_Epsilon);
	camera_shake::Advance(shakes, 400.0f);
	EXPECT_TRUE(shakes.empty());
	const auto offsets = camera_shake::Jitter(0.5f, true, [](float a, float b) { return b + 0.0f * a; });
	EXPECT_FLOAT_EQ(offsets.eye.y, 0.5f);
	EXPECT_FLOAT_EQ(offsets.focus.y, 0.5f);
	EXPECT_FLOAT_EQ(offsets.eye.x, 0.0f);
	// A shake along every axis draws z, y, then x, for the eye and then what it looks at
	float draw = 0.0f;
	const auto all = camera_shake::Jitter(1.0f, false, [&draw](float, float) { return draw += 1.0f; });
	EXPECT_EQ(all.eye, glm::vec3(3.0f, 2.0f, 1.0f));
	EXPECT_EQ(all.focus, glm::vec3(6.0f, 5.0f, 4.0f));
}

TEST(DustPuff, FifteenSpritesFlyUpAndOutGrowingAndFadingOverASecondAndAHalf)
{
	auto puff = dust_puff::Make({0.0f, 0.0f, 0.0f}, 1.0f, [](float a, float b) { return (a + b) * 0.5f; });
	EXPECT_NEAR(glm::length(puff.velocities[0]), 1.5f, k_Epsilon);
	EXPECT_GT(puff.velocities[0].y, 0.0f);
	auto look = dust_puff::Look(puff);
	EXPECT_FLOAT_EQ(look[0].halfWidth, 0.5f);
	EXPECT_FLOAT_EQ(look[0].alpha, 255.0f);
	EXPECT_TRUE(dust_puff::Advance(puff, 0.75f));
	look = dust_puff::Look(puff);
	EXPECT_NEAR(look[0].halfWidth, 1.0f, k_Epsilon);
	EXPECT_NEAR(look[0].alpha, 0.5f / 0.7f * 255.0f, 0.01f);
	EXPECT_FALSE(dust_puff::Advance(puff, 0.76f));
}

TEST(Blast, AShieldOverTheBlastIsStruckWhereTheWayDownEntersIt)
{
	const auto hit = blast::EntersSphere({0.0f, 200.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 30.0f);
	EXPECT_NEAR(hit.y, 30.0f, k_Epsilon);
	// Starting inside, it is struck where the way starts
	EXPECT_EQ(blast::EntersSphere({0.0f, 1.0f, 0.0f}, glm::vec3(0.0f), glm::vec3(0.0f), 30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

TEST(Blast, BrokenPiecesAreShadedByTheModelLightInTheirOwnFrame)
{
	using namespace openblack::model_light;
	// Facing the light: the ambient and almost all the rest; facing away: the ambient alone
	const glm::vec3 up(0.0f, 1.0f, 0.0f);
	EXPECT_FLOAT_EQ(Factor(up, up, k_Ambient), 90.0f + 164.0f);
	EXPECT_FLOAT_EQ(Factor(-up, up, k_Ambient), 90.0f);
	// The light's way is taken from the piece's middle into its own frame, which is turned a quarter about y
	const auto local = LocalDirection({10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, up, {1.0f, 0.0f, 0.0f}, glm::vec3(0.0f));
	EXPECT_NEAR(local.z, 1.0f, 1e-6f);
	EXPECT_NEAR(local.x, 0.0f, 1e-6f);
}

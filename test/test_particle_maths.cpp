/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <numbers>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Particles/KeyPointSpline.h"
#include "Particles/ParticleMaths.h"
#include "Particles/ParticleSprites.h"
#include "Particles/ParticleTypes.h"

using namespace openblack;
using namespace openblack::particles;

namespace
{
constexpr float k_Epsilon = 1e-4f;
constexpr float k_HalfPi = std::numbers::pi_v<float> * 0.5f;

void ExpectNear(const glm::vec3& a, const glm::vec3& b)
{
	EXPECT_NEAR(a.x, b.x, k_Epsilon);
	EXPECT_NEAR(a.y, b.y, k_Epsilon);
	EXPECT_NEAR(a.z, b.z, k_Epsilon);
}
} // namespace

TEST(ParticleMaths, TimedValueRunsInsideItsWindow)
{
	EXPECT_FALSE(maths::TimedValue(0.5f, 0.1f, 1.0f, 2.0f, 0.0f, 10.0f).has_value());
	EXPECT_FLOAT_EQ(*maths::TimedValue(1.0f, 0.1f, 1.0f, 2.0f, 0.0f, 10.0f), 0.0f);
	EXPECT_FLOAT_EQ(*maths::TimedValue(1.5f, 0.1f, 1.0f, 2.0f, 0.0f, 10.0f), 5.0f);
	// The first step past the end still lands on it, later ones leave the value alone
	EXPECT_FLOAT_EQ(*maths::TimedValue(2.05f, 0.1f, 1.0f, 2.0f, 0.0f, 10.0f), 10.0f);
	EXPECT_FALSE(maths::TimedValue(2.5f, 0.1f, 1.0f, 2.0f, 0.0f, 10.0f).has_value());
	// An empty window jumps to the end
	EXPECT_FLOAT_EQ(*maths::TimedValue(1.0f, 0.1f, 1.0f, 1.0f, 0.0f, 10.0f), 10.0f);
}

TEST(ParticleMaths, BytesKeepTheirLowBits)
{
	EXPECT_EQ(maths::TruncateToByte(200.9f), 200);
	EXPECT_EQ(maths::TruncateToByte(256.0f), 0);
	EXPECT_EQ(maths::TruncateToByte(-1.0f), 255);
}

TEST(ParticleMaths, PlayerColourTints)
{
	// Red at full blend leaves the red and darkens green and blue by the player's
	const auto red = maths::TintWithPlayerColour({255, 255, 255, 255}, 0xFF4646u, 1.0f);
	EXPECT_EQ(red, (std::array<uint8_t, 4> {254, 69, 69, 254}));
	// Half blend moves the player's colour halfway to white first
	const auto pale = maths::TintWithPlayerColour({255, 255, 255, 255}, 0xFF4646u, 0.5f);
	EXPECT_EQ(pale[0], 254);
	EXPECT_EQ(pale[1], 162);
	// The neutral player's black is white
	const auto neutral = maths::TintWithPlayerColour({100, 100, 100, 100}, 0x000000u, 1.0f);
	EXPECT_EQ(neutral, (std::array<uint8_t, 4> {99, 99, 99, 99}));
}

TEST(ParticleMaths, EmittersKeepTheirSchedule)
{
	maths::EmitterClock clock;
	const maths::EmitterLimits limits {.frequency = 2.0f, .maxAlive = -1, .maxTotal = 3, .randomise = false};
	const auto never = [] {
		ADD_FAILURE() << "not randomised";
		return 0.0f;
	};
	// Two a second, stepped every tenth of a second: one at once, then one every half second, three in all
	int made = 0;
	for (int step = 0; step < 30; ++step)
	{
		if (maths::ShouldEmit(clock, limits, static_cast<float>(step) * 0.1f, 0.1f, 0, never))
		{
			++made;
			EXPECT_TRUE(step == 0 || step == 5 || step == 10) << step;
		}
	}
	EXPECT_EQ(made, 3);

	// None while too many are alive
	maths::EmitterClock full;
	const maths::EmitterLimits capped {.frequency = 10.0f, .maxAlive = 4, .randomise = false};
	EXPECT_FALSE(maths::ShouldEmit(full, capped, 0.0f, 0.1f, 5, never));
	EXPECT_TRUE(maths::ShouldEmit(full, capped, 0.0f, 0.1f, 4, never));

	// A randomised gap is between half and all of the period
	maths::EmitterClock random;
	const maths::EmitterLimits randomised {.frequency = 1.0f, .randomise = true};
	EXPECT_TRUE(maths::ShouldEmit(random, randomised, 0.0f, 0.1f, 0, [] { return 0.25f; }));
	EXPECT_FLOAT_EQ(random.next, 0.75f);
}

TEST(ParticleMaths, FramesStepAndWrap)
{
	float previous = 0.0f;
	float current = 0.0f;
	maths::AdvanceFrame(previous, current, 0.1f, 10.0f, 4, true);
	EXPECT_FLOAT_EQ(previous, 0.0f);
	EXPECT_FLOAT_EQ(current, 1.0f);
	// Paused, it holds
	maths::AdvanceFrame(previous, current, 0.1f, 10.0f, 4, false);
	EXPECT_FLOAT_EQ(previous, 1.0f);
	EXPECT_FLOAT_EQ(current, 1.0f);
	// Both are kept within two cycles, moved by whole cycles together
	previous = 8.5f;
	current = 8.5f;
	maths::AdvanceFrame(previous, current, 0.1f, 10.0f, 4, true);
	EXPECT_FLOAT_EQ(current - previous, 1.0f);
	EXPECT_LE(current, 8.0f + 1.0f);
	// Backwards below zero goes up by two cycles
	previous = 0.5f;
	current = 0.5f;
	maths::AdvanceFrame(previous, current, 0.1f, -10.0f, 4, true);
	EXPECT_FLOAT_EQ(current, 7.5f);
	EXPECT_FLOAT_EQ(previous, 8.5f);

	EXPECT_FLOAT_EQ(maths::LerpFrame(1.0f, 2.0f, 0.5f, true), 1.5f);
	EXPECT_FLOAT_EQ(maths::LerpFrame(1.0f, 2.0f, 3.0f, true), 4.0f);
	EXPECT_FLOAT_EQ(maths::LerpFrame(1.0f, 2.0f, 3.0f, false), 2.0f);

	EXPECT_EQ(maths::FrameIndex(5.5f, 4, true), 1);
	EXPECT_EQ(maths::FrameIndex(-0.5f, 4, true), 3);
	EXPECT_EQ(maths::FrameIndex(5.5f, 4, false), 3);
	EXPECT_EQ(maths::FrameIndex(-2.0f, 4, false), 0);
}

TEST(ParticleMaths, SpriteCellsTileTheSheet)
{
	const auto first = maths::SpriteCellUv(0, 8);
	EXPECT_FLOAT_EQ(first.corner.x, 0.0f);
	EXPECT_FLOAT_EQ(first.size.x, 0.125f);
	const auto tenth = maths::SpriteCellUv(10, 8);
	EXPECT_FLOAT_EQ(tenth.corner.x, 0.25f);
	EXPECT_FLOAT_EQ(tenth.corner.y, 0.125f);
	// Only the low six bits count
	const auto wrapped = maths::SpriteCellUv(64 + 10, 8);
	EXPECT_FLOAT_EQ(wrapped.corner.x, tenth.corner.x);
	EXPECT_FLOAT_EQ(wrapped.corner.y, tenth.corner.y);
}

TEST(ParticleMaths, RotationsTurnTheGamesWay)
{
	// A quarter turn about the vertical takes x to -z, the other way to glm's
	ExpectNear(maths::AngleY(k_HalfPi) * glm::vec3(1.0f, 0.0f, 0.0f), {0.0f, 0.0f, 1.0f});
	ExpectNear(maths::AngleXYZ(0.0f, k_HalfPi, 0.0f) * glm::vec3(1.0f, 0.0f, 0.0f), {0.0f, 0.0f, 1.0f});
	const auto turned = maths::TurnAboutAxis(glm::mat3(1.0f), 2, k_HalfPi);
	ExpectNear(turned * glm::vec3(1.0f, 0.0f, 0.0f), {0.0f, -1.0f, 0.0f});
	EXPECT_NEAR(maths::SpriteAngle(maths::AngleY(0.3f)), 0.3f, k_Epsilon);
	// Moving up the screen is no roll; moving right is a quarter turn
	EXPECT_NEAR(maths::ScreenVelocityAngle({0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}),
	            std::numbers::pi_v<float>, k_Epsilon);
	EXPECT_NEAR(maths::ScreenVelocityAngle({1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}), k_HalfPi, k_Epsilon);
}

TEST(ParticleMaths, ValueNoiseBlendsItsLattice)
{
	const maths::ValueNoise noise;
	// The same every time it is made
	const maths::ValueNoise again;
	EXPECT_FLOAT_EQ(noise.At({1.3f, 2.7f, -4.1f}), again.At({1.3f, 2.7f, -4.1f}));
	// At a lattice point it is the lattice's value, halfway between two it is their mean
	EXPECT_FLOAT_EQ(noise.At({3.0f, 5.0f, 7.0f}), noise.Lattice(3, 5, 7));
	EXPECT_NEAR(noise.At({3.5f, 5.0f, 7.0f}), (noise.Lattice(3, 5, 7) + noise.Lattice(4, 5, 7)) * 0.5f, k_Epsilon);
	for (int i = 0; i < 100; ++i)
	{
		const float value = noise.At(glm::vec3(static_cast<float>(i) * 0.37f, static_cast<float>(i) * 0.11f, 0.5f));
		EXPECT_LE(std::abs(value), 1.0f);
	}
	const auto wind = noise.Wind({1.0f, 2.0f, 3.0f});
	EXPECT_FLOAT_EQ(wind.x, noise.At({1.0f, 2.0f, 3.0f}));
	EXPECT_FLOAT_EQ(wind.y, noise.At({2.0f, 3.0f, 1.0f}));
}

TEST(ParticleMaths, SoundsAreSizedByRadius)
{
	EXPECT_EQ(maths::SoundSizeFromRadius(100.0f, 200.0f, 500.0f), 3);
	EXPECT_EQ(maths::SoundSizeFromRadius(300.0f, 200.0f, 500.0f), 2);
	EXPECT_EQ(maths::SoundSizeFromRadius(500.0f, 200.0f, 500.0f), 1);
}

TEST(KeyPointSpline, PassesThroughItsKeys)
{
	const std::array<float, 6> pairs {0.0f, 1.0f, 1.0f, 3.0f, 2.0f, 2.0f};
	const KeyPointSpline spline(pairs);
	ASSERT_EQ(spline.Keys().size(), 3u);
	EXPECT_NEAR(spline.Evaluate(0.0f, -1.0f), 1.0f, k_Epsilon);
	EXPECT_NEAR(spline.Evaluate(1.0f, -1.0f), 3.0f, k_Epsilon);
	EXPECT_NEAR(spline.Evaluate(2.0f, -1.0f), 2.0f, k_Epsilon);
	// Flat at the ends: just past the first key it has barely moved
	EXPECT_NEAR(spline.Evaluate(0.01f, -1.0f), 1.0f, 1e-2f);
	// A flat line stays flat
	const std::array<float, 4> flat {0.0f, 5.0f, 5.0f, 5.0f};
	EXPECT_NEAR(KeyPointSpline(flat).Evaluate(2.5f, -1.0f), 5.0f, k_Epsilon);
}

TEST(KeyPointSpline, LeavesTheValueWithoutACurve)
{
	EXPECT_FLOAT_EQ(KeyPointSpline().Evaluate(1.0f, 7.0f), 7.0f);
	const std::array<float, 3> oneAndAHalf {0.0f, 1.0f, 2.0f};
	EXPECT_FLOAT_EQ(KeyPointSpline(oneAndAHalf).Evaluate(1.0f, 7.0f), 7.0f);
	const std::array<float, 4> sameTime {1.0f, 1.0f, 1.0f, 2.0f};
	EXPECT_FLOAT_EQ(KeyPointSpline(sameTime).Evaluate(1.0f, 7.0f), 7.0f);
}

TEST(ParticleTypes, NameTheirFiles)
{
	EXPECT_EQ(ParticleTypeFile(ParticleType::Smoke), "SF_Smoke");
	EXPECT_EQ(ParticleTypeFile(ParticleType::Bonfire), "SF_Bonfire");
	// Shared files
	EXPECT_EQ(ParticleTypeFile(ParticleType::Food), ParticleTypeFile(ParticleType::FoodPoisoned));
	EXPECT_EQ(ParticleTypeFile(ParticleType::Heal), ParticleTypeFile(ParticleType::HealFx));
	EXPECT_EQ(ParticleTypeFile(ParticleType::LandscapeVortexOutBefore), "SF_LandscapeVortexInBefore");
	// None for those other code draws
	EXPECT_TRUE(ParticleTypeFile(ParticleType::Tornado).empty());
	EXPECT_TRUE(ParticleTypeFile(static_cast<ParticleType>(k_ParticleTypeCount)).empty());
	EXPECT_FALSE(ParticleTypeName(ParticleType::Smoke).empty());
}

TEST(ParticleSprites, AtomsBecomeInstances)
{
	Creator creator;
	creator.kind = Creator::Kind::Sprite;
	creator.numFrames = 4;
	creator.fileOffset = 8;
	creator.scaleAlpha = 128;
	creator.centreAtBase = true;
	creator.origin = {0.5f, 0.25f};
	const Effect::DrawAtom atom {
	    .creator = &creator,
	    .position = {1.0f, 2.0f, 3.0f},
	    .rotation = maths::AngleY(0.5f),
	    .scale = 2.0f,
	    .stretch = 1.5f,
	    .alpha = 255.0f,
	    .frame = 1.2f,
	    .rgb = {255, 0, 51},
	};
	const auto sprite = sprites::InstanceOf(atom);
	// Raised by half its half height when centred at its base
	ExpectNear(glm::vec3(sprite.positionHalfWidth), {1.0f, 2.0f + 1.5f, 3.0f});
	EXPECT_FLOAT_EQ(sprite.positionHalfWidth.w, 2.0f);
	EXPECT_FLOAT_EQ(sprite.shape.x, 3.0f);
	EXPECT_NEAR(sprite.shape.y, 0.5f, k_Epsilon);
	EXPECT_FLOAT_EQ(sprite.shape.z, 1.0f);
	EXPECT_FLOAT_EQ(sprite.shape.w, 0.75f);
	// Cell 9 of an eight wide sheet
	EXPECT_FLOAT_EQ(sprite.uv.x, 0.125f);
	EXPECT_FLOAT_EQ(sprite.uv.y, 0.125f);
	EXPECT_NEAR(sprite.colour.a, 128.0f / 255.0f, k_Epsilon);
	EXPECT_NEAR(sprite.colour.b, 0.2f, k_Epsilon);
	EXPECT_FLOAT_EQ(sprite.flags.x, 0.0f);

	creator.additive = true;
	EXPECT_EQ(sprites::RenderMode(creator), graphics::render_modes::Mode::AlphaTexturedAlphaAdditiveNz);
	creator.additive = false;
	creator.writeDepth = true;
	EXPECT_EQ(sprites::RenderMode(creator), graphics::render_modes::Mode::AlphaTexturedAlpha);
}

TEST(ParticleSprites, CornersFaceTheScreenOrLieFlat)
{
	sprites::SpriteInstance sprite {
	    .positionHalfWidth = {10.0f, 0.0f, 0.0f, 1.0f},
	    .shape = {2.0f, 0.0f, 0.0f, 0.0f},
	    .uv = {},
	    .colour = {},
	    .flags = {},
	};
	const glm::vec3 right(1.0f, 0.0f, 0.0f);
	const glm::vec3 up(0.0f, 1.0f, 0.0f);
	auto corners = sprites::Corners(sprite, right, up);
	ExpectNear(corners[0], {9.0f, 2.0f, 0.0f});
	ExpectNear(corners[2], {11.0f, -2.0f, 0.0f});
	// A quarter roll turns its top to the right of the screen
	sprite.shape.y = k_HalfPi;
	corners = sprites::Corners(sprite, right, up);
	ExpectNear((corners[0] + corners[1]) * 0.5f, {12.0f, 0.0f, 0.0f});
	// Flat, its top edge lies towards -z
	sprite.shape.y = 0.0f;
	sprite.flags.x = 1.0f;
	corners = sprites::Corners(sprite, right, up);
	ExpectNear(corners[0], {9.0f, 0.0f, -2.0f});
	ExpectNear(corners[2], {11.0f, 0.0f, 2.0f});
}

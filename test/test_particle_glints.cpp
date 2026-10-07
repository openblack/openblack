/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Common/GameRandom.h"
#include "Particles/ParticleClassRegistry.h"
#include "Particles/ParticleEffect.h"

using namespace openblack;
using namespace openblack::particles;

namespace
{
const auto k_Seed = static_cast<entt::entity>(5);

/// A seed with four points of its model, drawn at a scale of 2
class FakeWorld final: public ParticleWorldInterface
{
public:
	[[nodiscard]] float LandHeight(glm::vec2 /*xz*/) const override { return 0.0f; }
	[[nodiscard]] uint32_t PlayerColour(int /*player*/) const override { return 0xFFFFFFu; }
	[[nodiscard]] glm::vec3 CameraRight() const override { return {1.0f, 0.0f, 0.0f}; }
	[[nodiscard]] glm::vec3 CameraUp() const override { return {0.0f, 1.0f, 0.0f}; }
	[[nodiscard]] std::optional<glm::vec3> ObjectPosition(entt::entity object) const override
	{
		return object == k_Seed && !gone ? std::optional(glm::vec3(10.0f, 5.0f, 10.0f)) : std::nullopt;
	}
	[[nodiscard]] uint32_t TargetPointCount(entt::entity object) const override
	{
		return object == k_Seed ? static_cast<uint32_t>(points.size()) : 0;
	}
	[[nodiscard]] std::optional<glm::vec3> TargetPoint(entt::entity object, uint32_t index) const override
	{
		return object == k_Seed && index < points.size() ? std::optional(points[index]) : std::nullopt;
	}
	[[nodiscard]] float TargetScale(entt::entity /*object*/) const override { return 2.0f; }

	std::vector<glm::vec3> points {{10.0f, 5.0f, 10.0f}, {11.0f, 5.0f, 10.0f}, {10.0f, 6.0f, 10.0f}, {10.0f, 5.0f, 11.0f}};
	bool gone {false};
};

class FakeRandom final: public GameRandomInterface
{
public:
	uint32_t GameRand(uint32_t n) override { return n == 0 ? 0 : game_random::LHRand(n, _seeds.synced); }
	float GameFloatRand(float x) override { return game_random::FloatRand(x, _seeds.synced); }
	uint32_t LocalRand(int32_t n) override { return n == 0 ? 0 : game_random::LHRand(static_cast<uint32_t>(n), _seeds.local); }
	float LocalFloatRand(float x) override { return game_random::FloatRand(x, _seeds.local); }
	int32_t CrtRand() override { return game_random::CrtRand(_crt); }
	void CrtSrand(uint32_t seed) override { _crt = seed; }
	[[nodiscard]] GameRandomSeeds GetSeeds() const override { return _seeds; }
	void SetSeeds(GameRandomSeeds seeds) override { _seeds = seeds; }
	[[nodiscard]] ParticleRandomStream GetParticleStream() const override { return _stream; }
	void SetParticleStream(ParticleRandomStream stream) override { _stream = stream; }

private:
	GameRandomSeeds _seeds;
	ParticleRandomStream _stream {ParticleRandomStream::None};
	uint32_t _crt {1};
};

/// The frozen creature spell's holder effect, as the game's file has it
std::string FreezeOnHolder()
{
	return "BEGINPROPERTIES\nPROPERTY DeleteOnCloseDown BOOL 1\nPROPERTY Hierarchies ARRAY SIZE 2 0 0\n"
	       "PROPERTY InitiallyCreated ARRAY SIZE 2 1 0\nPROPERTY MaxSpellAge FLOAT -1\nENDPROPERTIES\n"
	       "BEGINCLASS ParticlePointCreator ParticlePointCreator0\nBEGINPROPERTIES\nPROPERTY InitialScale FLOAT 1\n"
	       "ENDPROPERTIES\nENDCLASS\n"
	       "BEGINCLASS ParticleSpriteCreator ParticleSpriteCreator0\nBEGINPROPERTIES\nPROPERTY ColorA INTEGER 255\n"
	       "PROPERTY ColorR INTEGER 180\nPROPERTY ColorG INTEGER 220\nPROPERTY ColorB INTEGER 255\n"
	       "PROPERTY InitialScale FLOAT 1.5\nPROPERTY NumFrames INTEGER 16\nPROPERTY RandomiseInitFrame BOOL 1\n"
	       "PROPERTY TextureFileName STRING .\\Data\\Textures\\S_SpriteSheet3.raw\nENDPROPERTIES\nENDCLASS\n"
	       "BEGINCLASS ER_GlintsOnTarget ER_GlintsOnTarget0\nBEGINPROPERTIES\nPROPERTY Group INTEGER 0\n"
	       "PROPERTY NextGroups ARRAY SIZE 0\nPROPERTY PCreator PERSIS_PNTR ParticlePointCreator0\n"
	       "PROPERTY MaxAtoms INTEGER 3\nPROPERTY MaxAlpha INTEGER 80\nPROPERTY GlintGroup INTEGER 1\n"
	       "PROPERTY GlintCreator PERSIS_PNTR ParticleSpriteCreator0\nPROPERTY PulseMagnitude FLOAT 1\n"
	       "PROPERTY PulseSpeed FLOAT 1\nPROPERTY AtomAgeMaxSize FLOAT 0.15\nPROPERTY AtomAgeZeroSize FLOAT 0.9\n"
	       "ENDPROPERTIES\nENDCLASS\n";
}

class ParticleGlints: public ::testing::Test
{
protected:
	std::unique_ptr<Effect> Make()
	{
		auto file = psys::ParticleFile::Parse(FreezeOnHolder());
		EXPECT_TRUE(file.has_value());
		auto effect = std::make_unique<Effect>(std::make_shared<const psys::ParticleFile>(std::move(*file)),
		                                       EffectServices {classes, world, random, noise}, glm::vec3(10.0f, 5.0f, 10.0f),
		                                       1.0f, false);
		effect->AddTarget(k_Seed);
		return effect;
	}

	static std::vector<Effect::DrawAtom> Glints(const Effect& effect)
	{
		std::vector<Effect::DrawAtom> atoms;
		effect.Collect(1.0f, atoms, Creator::Kind::Sprite);
		return atoms;
	}

	ParticleClassRegistry classes {ParticleClassRegistry::WithAllClasses()};
	FakeWorld world;
	FakeRandom random;
	maths::ValueNoise noise;
};
} // namespace

TEST_F(ParticleGlints, NoMoreThanTheMostOnThePointsOfTheModelAtAFixedAlpha)
{
	auto effect = Make();
	for (int step = 0; step < 20; ++step)
	{
		effect->Step(0.1f);
		const auto glints = Glints(*effect);
		EXPECT_LE(glints.size(), 3u);
		for (const auto& glint : glints)
		{
			EXPECT_NEAR(glint.alpha, 80.0f, 1e-3f);
			EXPECT_EQ(glint.rgb[0], 180);
			// On one of the points; drawn at the seed's scale times its pulsing growth and shrinking
			const bool onAPoint = std::ranges::any_of(
			    world.points, [&](const glm::vec3& point) { return glm::distance(point, glint.position) < 1e-4f; });
			EXPECT_TRUE(onAPoint);
			EXPECT_LE(glint.age, 0.9f + 1e-4f);
			EXPECT_LE(glint.scale, 3.0f * 1.5f + 1e-4f);
		}
	}
	EXPECT_FALSE(Glints(*effect).empty());
}

TEST_F(ParticleGlints, TheyGoWithTheirObject)
{
	auto effect = Make();
	for (int step = 0; step < 5; ++step)
	{
		effect->Step(0.1f);
	}
	EXPECT_FALSE(Glints(*effect).empty());
	world.gone = true;
	effect->Step(0.1f);
	EXPECT_TRUE(Glints(*effect).empty());
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Common/GameRandom.h"
#include "Particles/GestureTrail.h"
#include "Particles/LightSheet.h"
#include "Particles/ParticleClassRegistry.h"
#include "Particles/ParticleEffect.h"

using namespace openblack;
using namespace openblack::particles;

namespace
{
TrailPath Line(float length, int points)
{
	std::vector<glm::vec3> line;
	for (int i = 0; i < points; ++i)
	{
		line.emplace_back(length * static_cast<float>(i) / static_cast<float>(points - 1), 0.0f, 0.0f);
	}
	return TrailPath(line);
}
} // namespace

namespace
{
class FakeWorld final: public ParticleWorldInterface
{
public:
	[[nodiscard]] float LandHeight(glm::vec2 /*xz*/) const override { return 0.0f; }
	[[nodiscard]] uint32_t PlayerColour(int /*player*/) const override { return 0x2040FFu; }
	[[nodiscard]] glm::vec3 CameraRight() const override { return {1.0f, 0.0f, 0.0f}; }
	[[nodiscard]] glm::vec3 CameraUp() const override { return {0.0f, 1.0f, 0.0f}; }
	[[nodiscard]] glm::vec3 CameraPosition() const override { return {50.0f, 1000.0f, 50.0f}; }
	void PlayListenerSound(uint32_t sample) override { sounds.push_back(sample); }
	[[nodiscard]] std::shared_ptr<GestureTrail> TakeGestureTrail() override
	{
		auto taken = std::move(waiting);
		waiting.reset();
		return taken;
	}
	void AddLightSheet(const std::shared_ptr<LightSheet>& sheet) override { sheets.push_back(sheet); }
	void SetHandGlow(uint32_t rgb) override { glow = rgb; }

	std::shared_ptr<GestureTrail> waiting;
	std::vector<uint32_t> sounds;
	std::vector<std::weak_ptr<LightSheet>> sheets;
	uint32_t glow {0xDEADu};
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

/// The game's gesture trail effect, as its file has it
std::string GestureFile()
{
	return "BEGINPROPERTIES\nPROPERTY DeleteOnCloseDown BOOL 1\nPROPERTY Hierarchies ARRAY SIZE 2 0 0\n"
	       "PROPERTY InitiallyCreated ARRAY SIZE 2 1 0\nPROPERTY MaxSpellAge FLOAT -1\nENDPROPERTIES\n"
	       "BEGINCLASS ParticlePointCreator ParticlePointCreator0\nBEGINPROPERTIES\nPROPERTY InitialScale FLOAT 1\n"
	       "ENDPROPERTIES\nENDCLASS\n"
	       "BEGINCLASS UR_GesturingRecognised UR_GesturingRecognised0\nBEGINPROPERTIES\n"
	       "PROPERTY CollectionAlphaInit INTEGER 50\nPROPERTY CollectionAlphaPulse INTEGER 255\n"
	       "PROPERTY DieAge FLOAT 7\nPROPERTY DispersalTime FLOAT 3\nPROPERTY DoTransition BOOL 1\n"
	       "PROPERTY GoToIdeal BOOL 1\nPROPERTY Group INTEGER 0\nPROPERTY HandPulseDuration FLOAT 0.5\n"
	       "PROPERTY InterpGain FLOAT 0.9\nPROPERTY LightSheetDieAge FLOAT 4\nPROPERTY LightSheetHeightScale FLOAT 9\n"
	       "PROPERTY MaxAlpha FLOAT 176.087\nPROPERTY NextGroups ARRAY SIZE 1 1\nPROPERTY NumAtoms INTEGER 234\n"
	       "PROPERTY PCreator PERSIS_PNTR ParticlePointCreator0\nPROPERTY ShrinkTimeAfterDispersal FLOAT 2\n"
	       "PROPERTY SparkleGroup INTEGER 3\nPROPERTY SpriteCreator PERSIS_PNTR ParticleSpriteCreator_Blob\n"
	       "PROPERTY TimeToIdeal FLOAT 0.6\nPROPERTY WiggleFreq FLOAT 10\nPROPERTY WiggleMag FLOAT 0.1\n"
	       "PROPERTY WiggleMagY FLOAT 0.01\nPROPERTY WigglePhaseSpeed FLOAT 1.5\nPROPERTY WiggleSpeed FLOAT 2\n"
	       "ENDPROPERTIES\nENDCLASS\n"
	       "BEGINCLASS ParticleSpriteCreator ParticleSpriteCreator_Blob\nBEGINPROPERTIES\nPROPERTY ColorA INTEGER 250\n"
	       "PROPERTY ColorB INTEGER 255\nPROPERTY ColorG INTEGER 100\nPROPERTY ColorR INTEGER 100\n"
	       "PROPERTY FileOffset INTEGER 38\nPROPERTY InitialScale FLOAT 1\nPROPERTY NumFrames INTEGER 32\n"
	       "PROPERTY TextureFileName STRING .\\Data\\Textures\\S_SpriteSheet3.raw\nPROPERTY UseAdditiveAlpha BOOL 1\n"
	       "ENDPROPERTIES\nENDCLASS\n";
}

/// The chain drawn behind the gesturing hand, as its file has it
std::string ChainFile()
{
	return "BEGINPROPERTIES\nPROPERTY DeleteOnCloseDown BOOL 1\nPROPERTY Hierarchies ARRAY SIZE 3 0 0 0\n"
	       "PROPERTY InitiallyCreated ARRAY SIZE 3 1 0 0\nPROPERTY MaxSpellAge FLOAT -1\nENDPROPERTIES\n"
	       "BEGINCLASS ParticlePointCreator ParticlePointCreator0\nBEGINPROPERTIES\nPROPERTY InitialScale FLOAT 1\n"
	       "ENDPROPERTIES\nENDCLASS\n"
	       "BEGINCLASS CreateRuleMakeChain CreateRuleMakeChain0\nBEGINPROPERTIES\nPROPERTY Group INTEGER 2\n"
	       "PROPERTY NextGroups ARRAY SIZE 0\nPROPERTY NumAtoms INTEGER 81\n"
	       "PROPERTY PCreator PERSIS_PNTR ParticleChainCreator0\nENDPROPERTIES\nENDCLASS\n"
	       "BEGINCLASS UR_FollowCastPosn UR_FollowCastPosn0\nBEGINPROPERTIES\nPROPERTY Group INTEGER 0\n"
	       "ENDPROPERTIES\nENDCLASS\n"
	       "BEGINCLASS ZR_ChainGesture ZR_ChainGesture0\nBEGINPROPERTIES\n"
	       "PROPERTY AdjustInitialScale PERSIS_PNTR MagnitudeFloatProvider0\nPROPERTY DieAge FLOAT 2.32389\n"
	       "PROPERTY Group INTEGER 0\nPROPERTY InTestMode BOOL 0\nPROPERTY MinEmitDist FLOAT 2\n"
	       "PROPERTY NextGroups ARRAY SIZE 1 2\nPROPERTY PCreator PERSIS_PNTR ParticlePointCreator0\n"
	       "ENDPROPERTIES\nENDCLASS\n"
	       "BEGINCLASS MagnitudeFloatProvider MagnitudeFloatProvider0\nBEGINPROPERTIES\nPROPERTY Maximum FLOAT 100\n"
	       "PROPERTY Minimum FLOAT 0\nPROPERTY ScaleBy FLOAT 0.7\nENDPROPERTIES\nENDCLASS\n"
	       "BEGINCLASS ParticleChainCreator ParticleChainCreator0\nBEGINPROPERTIES\nPROPERTY ColorA INTEGER 10\n"
	       "PROPERTY InitialScale FLOAT 1.0\nPROPERTY NumTexturesForWholeChain INTEGER 5\n"
	       "PROPERTY TextureFileName STRING .\\Data\\Textures\\S_Lightning.raw\nPROPERTY UsePlayerColor BOOL 1\n"
	       "ENDPROPERTIES\nENDCLASS\n";
}

class GestureEffects: public ::testing::Test
{
protected:
	std::unique_ptr<Effect> Make(const std::string& text)
	{
		auto file = psys::ParticleFile::Parse(text);
		EXPECT_TRUE(file.has_value());
		auto effect = std::make_unique<Effect>(std::make_shared<const psys::ParticleFile>(std::move(*file)),
		                                       EffectServices {classes, world, random, noise}, glm::vec3(0.0f), 1.0f, false);
		effect->SetPlayer(0);
		return effect;
	}

	static std::shared_ptr<GestureTrail> ATrail()
	{
		auto made = std::make_shared<GestureTrail>();
		made->drawn = Line(40.0f, 20);
		std::vector<glm::vec3> arc;
		for (int i = 0; i < 20; ++i)
		{
			const float a = static_cast<float>(i) / 19.0f * std::numbers::pi_v<float>;
			arc.emplace_back(20.0f - (20.0f * std::cos(a)), 0.0f, 20.0f * std::sin(a));
		}
		made->ideal = TrailPath(arc);
		return made;
	}

	static std::vector<Effect::DrawAtom> Sprites(const Effect& effect)
	{
		std::vector<Effect::DrawAtom> atoms;
		effect.Collect(1.0f, atoms, Creator::Kind::Sprite);
		return atoms;
	}

	static std::vector<Effect::DrawAtom> Joints(const Effect& effect)
	{
		Effect::DrawWalk walk;
		effect.Walk(1.0f, walk);
		return walk.joints;
	}

	ParticleClassRegistry classes {ParticleClassRegistry::WithAllClasses()};
	FakeWorld world;
	FakeRandom random;
	maths::ValueNoise noise;
};
} // namespace

TEST_F(GestureEffects, ATrailIsHeardAndItsParticlesFlowToTheShape)
{
	auto effect = Make(GestureFile());
	effect->Step(0.1f);
	EXPECT_TRUE(world.sounds.empty());
	world.waiting = ATrail();
	effect->Step(0.1f);
	EXPECT_EQ(world.sounds, std::vector<uint32_t> {36});
	ASSERT_EQ(world.sheets.size(), 1u);
	EXPECT_FALSE(world.sheets.front().expired());
	// Nothing shows until the trail starts to grow
	EXPECT_TRUE(Sprites(*effect).empty());
	for (int step = 0; step < 7; ++step)
	{
		effect->Step(0.1f);
	}
	const auto sprites = Sprites(*effect);
	ASSERT_EQ(sprites.size(), 234u);
	for (const auto& sprite : sprites)
	{
		EXPECT_EQ(sprite.rgb[0], 0x20);
		EXPECT_EQ(sprite.rgb[1], 0x40);
		EXPECT_EQ(sprite.rgb[2], 0xFF);
		// On the arc, a little above it as the camera is above, wiggling across its box by a tenth of its size
		EXPECT_LE(glm::distance(glm::vec2(sprite.position.x, sprite.position.z), glm::vec2(20.0f, 0.0f)), 26.0f);
	}
	EXPECT_EQ(world.glow, 0xDEADu);
}

TEST_F(GestureEffects, TheHandGlowsAsTheTrailFlashesAndTheTrailGoes)
{
	auto effect = Make(GestureFile());
	world.waiting = ATrail();
	effect->Step(0.1f);
	// 2.5 seconds in, the hand glows almost the player's whole colour
	for (int step = 0; step < 25; ++step)
	{
		effect->Step(0.1f);
	}
	EXPECT_NE(world.glow, 0xDEADu);
	EXPECT_GT(world.glow & 0xFFu, 0x80u);
	for (int step = 0; step < 6; ++step)
	{
		effect->Step(0.1f);
	}
	EXPECT_EQ(world.glow, 0u);
	for (int step = 0; step < 50; ++step)
	{
		effect->Step(0.1f);
	}
	EXPECT_TRUE(world.sheets.front().expired());
	EXPECT_EQ(effect->AtomCount(), 0u);
	EXPECT_FALSE(effect->Finished());
}

TEST_F(GestureEffects, TheChainFollowsTheGesturingHand)
{
	auto effect = Make(ChainFile());
	ProcessInfo info {.handPosition = {10.0f, 5.0f, 10.0f}, .enabled = false};
	effect->SetProcessInfo(info);
	effect->Step(0.05f);
	EXPECT_EQ(effect->AtomCount(), 0u);
	info.enabled = true;
	effect->SetProcessInfo(info);
	effect->Step(0.05f);
	// The chain's atom and its 81 joints
	EXPECT_EQ(effect->AtomCount(), 82u);
	for (int step = 0; step < 20; ++step)
	{
		info.handPosition.x += 1.0f;
		effect->SetProcessInfo(info);
		effect->Step(0.05f);
	}
	const auto joints = Joints(*effect);
	ASSERT_EQ(joints.size(), 81u);
	// The newest joint is at the hand, the older ones behind it
	EXPECT_NEAR(joints.back().position.x, info.handPosition.x, 1.0f);
	EXPECT_LT(joints.front().position.x, joints.back().position.x);
	// The hand stops gesturing: the chain stays a while, then goes
	info.enabled = false;
	effect->SetProcessInfo(info);
	effect->Step(0.05f);
	EXPECT_EQ(effect->AtomCount(), 82u);
	for (int step = 0; step < 110; ++step)
	{
		effect->Step(0.05f);
	}
	EXPECT_EQ(effect->AtomCount(), 0u);
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <algorithm>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Common/GameRandom.h"
#include "Particles/ParticleClassRegistry.h"
#include "Particles/ParticleEffect.h"
#include "Particles/TornadoMaths.h"

using namespace openblack;
using namespace openblack::particles;

namespace
{
constexpr float k_Step = 0.1f;
constexpr float k_Epsilon = 1e-3f;

// The maths

TEST(TornadoMaths, BiasPutsTheMiddleAtItsValue)
{
	EXPECT_NEAR(tornado::Bias(0.5f, 0.632743f), 0.632743f, k_Epsilon);
	EXPECT_NEAR(tornado::Bias(1.0f, 0.2f), 1.0f, k_Epsilon);
	EXPECT_FLOAT_EQ(tornado::Bias(0.0f, 0.2f), 0.0f);
}

TEST(TornadoMaths, GainIsTwoBiasesBackToBack)
{
	constexpr float k_Bend = 0.668142f;
	EXPECT_NEAR(tornado::Gain(0.5f, k_Bend), 0.5f, k_Epsilon);
	EXPECT_NEAR(tornado::Gain(0.0f, k_Bend), 0.0f, k_Epsilon);
	EXPECT_NEAR(tornado::Gain(1.0f, k_Bend), 1.0f, k_Epsilon);
	// A quarter of the way up is half of the bias of a half by 1 - g
	EXPECT_NEAR(tornado::Gain(0.25f, k_Bend), 0.5f * (1.0f - k_Bend), k_Epsilon);
	EXPECT_NEAR(tornado::Gain(0.75f, k_Bend), 1.0f - 0.5f * (1.0f - k_Bend), k_Epsilon);
}

TEST(TornadoMaths, TheWallWidensWithTheSquareOfTheHeight)
{
	tornado::Funnel funnel;
	funnel.scale = 2.0f;
	EXPECT_NEAR(tornado::WallRadius(funnel, 0.0f), 4.38053f * 2.0f, k_Epsilon);
	EXPECT_NEAR(tornado::WallRadius(funnel, 1.0f), 36.7624f * 2.0f, k_Epsilon);
	EXPECT_NEAR(tornado::WallRadius(funnel, 0.5f), (4.38053f + (36.7624f - 4.38053f) * 0.25f) * 2.0f, k_Epsilon);
	// Held within the funnel
	EXPECT_NEAR(tornado::WallRadius(funnel, 2.0f), 36.7624f * 2.0f, k_Epsilon);
	EXPECT_NEAR(tornado::WallScale(funnel, 1.0f), 5.13938f * 2.0f, k_Epsilon);
}

TEST(TornadoMaths, TheAxisBendsAndWiggles)
{
	tornado::Funnel funnel;
	const glm::vec3 base {0.0f, 0.0f, 0.0f};
	const glm::vec3 top {100.0f, 100.0f, 0.0f};
	const auto foot = tornado::AxisCentre(funnel, base, top, 0.0f);
	EXPECT_NEAR(foot.x, funnel.wiggleAmplitude, k_Epsilon);
	EXPECT_NEAR(foot.y, 0.0f, k_Epsilon);
	const auto middle = tornado::AxisCentre(funnel, base, top, 0.5f);
	EXPECT_NEAR(middle.y, 50.0f, k_Epsilon);
	// Five half turns at a half: the wiggle points along z
	EXPECT_NEAR(middle.x, 50.0f + std::cos(2.5f * std::numbers::pi_v<float>) * funnel.wiggleAmplitude, k_Epsilon);
	EXPECT_NEAR(middle.z, std::sin(2.5f * std::numbers::pi_v<float>) * funnel.wiggleAmplitude, k_Epsilon);
	// Lower down the wiggle follows the bent share of the height, not the height
	const auto low = tornado::AxisCentre(funnel, base, top, 0.25f);
	const float bent = tornado::Gain(0.25f, funnel.bend);
	const float angle = bent * static_cast<float>(funnel.wiggleCount) * std::numbers::pi_v<float>;
	EXPECT_NEAR(low.x, 100.0f * bent + std::cos(angle) * funnel.wiggleAmplitude, k_Epsilon);
	EXPECT_NEAR(low.z, std::sin(angle) * funnel.wiggleAmplitude, k_Epsilon);
}

TEST(TornadoMaths, ThingsSpinFasterLowDownAndSlowerOutsideTheWall)
{
	const tornado::Funnel funnel;
	EXPECT_NEAR(tornado::SpinRate(funnel, 0.0f, 1.0f), 12.5699f, k_Epsilon);
	EXPECT_NEAR(tornado::SpinRate(funnel, 1.0f, 1.0f), 5.89912f, k_Epsilon);
	EXPECT_NEAR(tornado::SpinRate(funnel, 0.5f, 1.5f), (12.5699f + (5.89912f - 12.5699f) * 0.632743f) * 1.5f, k_Epsilon);
	EXPECT_FLOAT_EQ(tornado::SpinOutside(10.0f, 5.0f, 4.0f), 10.0f);
	EXPECT_NEAR(tornado::SpinOutside(10.0f, 5.0f, 9.9f), 5.0f, k_Epsilon);
}

TEST(TornadoMaths, ThingsArePulledToTheWallAndRise)
{
	EXPECT_NEAR(tornado::RadialRate(10.0f, 20.0f, k_Step), -4.875f, k_Epsilon);
	EXPECT_NEAR(tornado::RadialRate(10.0f, 10.0f, k_Step), 0.0f, k_Epsilon);
	EXPECT_NEAR(tornado::RiseSpeed(0.0f, 0.0f, 100.0f, 0.5f, 0.1f), 5.0f, k_Epsilon);
	EXPECT_NEAR(tornado::RiseSpeed(80.0f, 0.0f, 100.0f, 0.5f, 0.3f), -9.0f, k_Epsilon);
	EXPECT_NEAR(tornado::HeightShare(50.0f, 0.0f, 100.0f), 0.5f, k_Epsilon);
	EXPECT_FLOAT_EQ(tornado::HeightShare(-5.0f, 0.0f, 100.0f), 0.0f);
	EXPECT_FLOAT_EQ(tornado::HeightShare(5.0f, 3.0f, 3.0f), 0.0f);
}

TEST(TornadoMaths, TheFootWandersByTheNoise)
{
	const auto wander = tornado::FootWander(0.5f, -0.5f, 1.0f, 0.2f, 30.0f, 2.0f);
	EXPECT_NEAR(wander.x, (1.0f * 0.5f + 0.5f) * 60.0f, k_Epsilon);
	EXPECT_NEAR(wander.y, (0.2f * 0.5f - 0.5f) * 60.0f, k_Epsilon);
	const auto times = tornado::FootWanderTimes(4.0f, 0.25f);
	EXPECT_FLOAT_EQ(times.a, 1.0f);
	EXPECT_FLOAT_EQ(times.b, 1.3f);
	EXPECT_FLOAT_EQ(times.c, 2.0f);
	EXPECT_FLOAT_EQ(times.d, 2.6f);
}

TEST(TornadoMaths, ItFadesInAndOut)
{
	EXPECT_FLOAT_EQ(tornado::CloseFade(-1.0f, 3.0f), 1.0f);
	EXPECT_NEAR(tornado::CloseFade(1.5f, 3.0f), 0.5f, k_Epsilon);
	EXPECT_FLOAT_EQ(tornado::CloseFade(4.0f, 3.0f), 0.0f);
	EXPECT_EQ(tornado::FadeAlpha(4.0f, 8.0f, 1.0f), 127);
	EXPECT_EQ(tornado::FadeAlpha(20.0f, 8.0f, 1.0f), 255);
	EXPECT_EQ(tornado::FadeAlpha(20.0f, 8.0f, 0.2f), 51);
}

TEST(TornadoMaths, ThrownUpFromTheFoot)
{
	const auto straight = tornado::Launch(0.0f, 1.0f, 0.5f, 20.0f, 2.0f, {1.0f, 2.0f, 3.0f});
	EXPECT_NEAR(straight.x, 1.0f, k_Epsilon);
	EXPECT_NEAR(straight.y, 0.5f * 20.0f * 2.0f + 2.0f, k_Epsilon);
	EXPECT_NEAR(straight.z, 3.0f, k_Epsilon);
	const auto level = tornado::Launch(std::numbers::pi_v<float> * 0.5f, 0.0f, 1.0f, 10.0f, 1.0f, glm::vec3(0.0f));
	EXPECT_NEAR(level.x, 10.0f, k_Epsilon);
	EXPECT_NEAR(level.y, 0.0f, k_Epsilon);
}

TEST(TornadoMaths, ItReachesWiderForAMightierTribeAndPicksUpWhatFits)
{
	const tornado::Funnel funnel;
	const float plain = 4.38053f + 36.7624f;
	EXPECT_NEAR(tornado::Reach(funnel, 0.5f), plain, k_Epsilon);
	EXPECT_NEAR(tornado::Reach(funnel, 2.0f), plain * 2.0f, k_Epsilon);
	EXPECT_NEAR(tornado::Reach(funnel, 9.0f), plain * 5.0f, k_Epsilon);
	EXPECT_TRUE(tornado::Fits(funnel, 8.0f));
	EXPECT_FALSE(tornado::Fits(funnel, 9.0f));
	EXPECT_NEAR(tornado::PileTake(0.5f, 150.0f, 650.0f), 400.0f, k_Epsilon);
	EXPECT_NEAR(tornado::PileTake(3.0f, 150.0f, 650.0f), 650.0f, k_Epsilon);
	EXPECT_NEAR(tornado::PotScale(0.1f, 1.0f), 0.2f, k_Epsilon);
	EXPECT_NEAR(tornado::PotScale(0.5f, 1.2f), 0.6f, k_Epsilon);
	EXPECT_NEAR(tornado::PotScale(4.0f, 0.7f), 0.7f, k_Epsilon);
}

TEST(TornadoMaths, TheLineNoiseRunsSmoothlyThroughItsLattice)
{
	const maths::ValueNoise noise;
	for (float x = -3.0f; x < 3.0f; x += 0.25f)
	{
		const float value = noise.Smooth(x);
		EXPECT_LT(std::abs(value), 2.0f);
		// No jump across a whole number
		EXPECT_NEAR(noise.Smooth(std::floor(x) + 1.0f - 1e-4f), noise.Smooth(std::floor(x) + 1.0f), 1e-2f);
	}
}

// The rule

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

/// Flat dry land with things near the middle for the tornado to act on
class FakeWorld final: public ParticleWorldInterface
{
public:
	[[nodiscard]] float LandHeight(glm::vec2 /*xz*/) const override { return 0.0f; }
	[[nodiscard]] uint32_t PlayerColour(int /*player*/) const override { return 0xFF0000u; }
	[[nodiscard]] glm::vec3 CameraRight() const override { return {1.0f, 0.0f, 0.0f}; }
	[[nodiscard]] glm::vec3 CameraUp() const override { return {0.0f, 1.0f, 0.0f}; }
	void StartSound(const Effect& /*effect*/, const std::shared_ptr<ParticleSoundLink>& sound) override
	{
		sounds.push_back(sound);
	}
	[[nodiscard]] uint32_t GameTurn() const override { return turn; }
	[[nodiscard]] glm::u8vec3 LandColour(glm::vec2 /*xz*/) const override { return {128, 64, 0}; }
	[[nodiscard]] std::vector<TornadoCandidate> TornadoCandidates(glm::vec3 /*foot*/, float reach) const override
	{
		lastReach = reach;
		return candidates;
	}
	void CatchCreature(entt::entity creature) override { caught.push_back(creature); }
	entt::entity TakeFromPile(entt::entity /*pile*/, uint32_t amount, float /*sizeShare*/) override
	{
		taken.push_back(amount);
		return pot;
	}
	std::optional<glm::mat3> Carry(const std::shared_ptr<CarriedObject>& carried) override
	{
		this->carried.push_back(carried);
		return glm::mat3(1.0f);
	}

	uint32_t turn {0};
	std::vector<TornadoCandidate> candidates;
	mutable float lastReach {0.0f};
	std::vector<entt::entity> caught;
	std::vector<uint32_t> taken;
	entt::entity pot {entt::null};
	std::vector<std::shared_ptr<CarriedObject>> carried;
	std::vector<std::shared_ptr<ParticleSoundLink>> sounds;
};

/// A miracle that lets the tornado destroy anything, with a tribal power
class FakeSpell final: public SpellSink
{
public:
	bool SpellEvent(const SpellEventInfo& event) override
	{
		events.push_back(event);
		return true;
	}
	[[nodiscard]] int PowerUpLevel() const override { return 1; }
	[[nodiscard]] float TribalPower() const override { return tribalPower; }

	[[nodiscard]] size_t Count(SpellEventInfo::Type type) const
	{
		return static_cast<size_t>(std::ranges::count(events, type, &SpellEventInfo::type));
	}

	std::vector<SpellEventInfo> events;
	float tribalPower {2.0f};
};

std::string Object(std::string_view className, std::string_view name, std::string_view properties)
{
	return "BEGINCLASS " + std::string(className) + " " + std::string(name) + "\nBEGINPROPERTIES\n" + std::string(properties) +
	       "ENDPROPERTIES\nENDCLASS\n";
}

/// The tornado's groups of the storm's file: the root (10, made at the start here rather than by the clouds) that makes
/// the tornado (11), the flung (12), the flying (13), the shells (14) and the dust (15)
std::string TornadoFile()
{
	std::string text = "BEGINPROPERTIES\nPROPERTY DeleteOnCloseDown BOOL 0\nPROPERTY Hierarchies ARRAY SIZE 16 0 0 0 0 0 0 0 "
	                   "0 0 0 0 0 0 0 0 0\nPROPERTY InitiallyCreated ARRAY SIZE 16 0 0 0 0 0 0 0 0 0 0 1 0 1 0 0 "
	                   "0\nPROPERTY MaxSpellAge FLOAT -1\nENDPROPERTIES\n";
	text += Object("ParticlePointCreator", "Point", "PROPERTY ColorA INTEGER 255\n");
	text += Object("ParticleSpriteCreator", "Dust",
	               "PROPERTY ColorA INTEGER 100\nPROPERTY ColorR INTEGER 255\nPROPERTY ColorG INTEGER 255\n"
	               "PROPERTY ColorB INTEGER 255\nPROPERTY InitialScale FLOAT 15\n"
	               "PROPERTY TextureFileName STRING .\\Data\\Textures\\S_SpriteSheet3.raw\n");
	text += Object("CreateRuleAnAtom", "Root",
	               "PROPERTY Group INTEGER 10\nPROPERTY NextGroups ARRAY SIZE 1 11\nPROPERTY PCreator PERSIS_PNTR Point\n");
	text += Object("CreateRuleSphere", "Shells",
	               "PROPERTY Group INTEGER 14\nPROPERTY NumAtoms INTEGER 2\nPROPERTY Radius FLOAT 0\n"
	               "PROPERTY PCreator PERSIS_PNTR Point\n");
	text += Object("UR_Tornado", "Tornado",
	               "PROPERTY Group INTEGER 11\nPROPERTY GroupFlying INTEGER 13\nPROPERTY GroupDebris INTEGER 15\n"
	               "PROPERTY GroupMesh INTEGER 14\nPROPERTY GroupToMoveToOnCloseDown INTEGER 12\n"
	               "PROPERTY GroupToMoveToOnceDone INTEGER 12\nPROPERTY PCreator PERSIS_PNTR Point\n"
	               "PROPERTY DebrisSpriteCreator PERSIS_PNTR Dust\n"
	               "PROPERTY SoundTornado SOUND_ACTION SOUND_SPELL_TORNADO LOOPING 1 ONLYONE 0 SOFTRELEASE 1 "
	               "USESURFACE 0\n");
	return text;
}

class TornadoRuleTest: public ::testing::Test
{
protected:
	std::unique_ptr<Effect> Make()
	{
		auto file = psys::ParticleFile::Parse(TornadoFile());
		EXPECT_TRUE(file.has_value());
		auto effect = std::make_unique<Effect>(std::make_shared<const psys::ParticleFile>(std::move(*file)),
		                                       EffectServices {classes, world, random, noise}, glm::vec3(100.0f, 0.0f, 100.0f),
		                                       40.0f, false);
		effect->SetSink(&spell);
		return effect;
	}

	void Run(Effect& effect, int steps)
	{
		for (int i = 0; i < steps; ++i)
		{
			effect.Step(k_Step);
			++world.turn;
		}
	}

	ParticleClassRegistry classes {ParticleClassRegistry::WithAllClasses()};
	FakeWorld world;
	FakeRandom random;
	maths::ValueNoise noise;
	FakeSpell spell;
};

TEST_F(TornadoRuleTest, ItStandsOnTheLandRoaringAndStrikesItsFootEveryStep)
{
	auto effect = Make();
	Run(*effect, 10);
	EXPECT_EQ(spell.Count(SpellEventInfo::Type::Point), 10u);
	ASSERT_EQ(world.sounds.size(), 1u);
	EXPECT_EQ(world.sounds.front()->sound.action.sound, "SOUND_SPELL_TORNADO");
	// The foot wanders near where it was cast, on the land
	const auto foot = effect->GetOrigin();
	EXPECT_FLOAT_EQ(foot.y, 0.0f);
	EXPECT_LT(glm::distance(glm::vec2(foot.x, foot.z), glm::vec2(100.0f, 100.0f)), 46.0f);
	// Dust is thrown up, coloured by the land
	std::vector<Effect::DrawAtom> dust;
	effect->Collect(0.0f, dust);
	ASSERT_FALSE(dust.empty());
	EXPECT_EQ(dust.front().rgb[0], (255 * 128) >> 8);
	EXPECT_EQ(dust.front().rgb[2], 0);
	// Closed down, it stops striking
	effect->CloseDown();
	const auto struck = spell.Count(SpellEventInfo::Type::Point);
	Run(*effect, 5);
	EXPECT_EQ(spell.Count(SpellEventInfo::Type::Point), struck);
}

TEST_F(TornadoRuleTest, OnceFadedInItPicksUpWhatItReachesEveryThirdTurnAndFlingsItWhenClosed)
{
	const auto villager = static_cast<entt::entity>(7);
	const auto creature = static_cast<entt::entity>(8);
	world.candidates = {
	    {.object = creature, .kind = TornadoCandidate::Kind::Creature, .position = {100.0f, 0.0f, 100.0f}, .radius = 9.0f},
	    {.object = villager, .kind = TornadoCandidate::Kind::Liftable, .position = {101.0f, 0.0f, 100.0f}, .radius = 0.5f},
	};
	auto effect = Make();
	// Not before it has faded in
	Run(*effect, 79);
	EXPECT_EQ(spell.Count(SpellEventInfo::Type::Capture), 0u);
	Run(*effect, 6);
	ASSERT_GE(spell.Count(SpellEventInfo::Type::Capture), 1u);
	// Reached as far as the funnel's radii times the tribal power
	EXPECT_NEAR(world.lastReach, (4.38053f + 36.7624f) * 2.0f, k_Epsilon);
	// The creature is caught, the villager carried
	EXPECT_FALSE(world.caught.empty());
	EXPECT_EQ(world.caught.front(), creature);
	ASSERT_FALSE(world.carried.empty());
	const auto carried = world.carried.front();
	EXPECT_EQ(carried->object, villager);
	ASSERT_NE(carried->atom, nullptr);
	// Closing down flings it: the particle lives on in the flung group
	effect->CloseDown();
	Run(*effect, 2);
	EXPECT_NE(carried->atom, nullptr);
	effect.reset();
	EXPECT_EQ(carried->atom, nullptr);
}

TEST_F(TornadoRuleTest, APileGivesUpAPotOfWhatItHolds)
{
	const auto pile = static_cast<entt::entity>(5);
	world.pot = static_cast<entt::entity>(6);
	world.candidates = {
	    {.object = pile, .kind = TornadoCandidate::Kind::Pile, .position = {100.0f, 0.0f, 100.0f}, .radius = 3.0f}};
	auto effect = Make();
	Run(*effect, 85);
	ASSERT_FALSE(world.taken.empty());
	// Without a size for it, the tornado is of size 1 and takes the most
	EXPECT_EQ(world.taken.front(), 650u);
	ASSERT_FALSE(world.carried.empty());
	EXPECT_EQ(world.carried.front()->object, world.pot);
	// Taken up as it is made, without asking the miracle
	EXPECT_EQ(spell.Count(SpellEventInfo::Type::Capture), 0u);
}

TEST_F(TornadoRuleTest, AHandDroppedPileThatFitsIsLiftedWhole)
{
	const auto pile = static_cast<entt::entity>(5);
	world.pot = static_cast<entt::entity>(6);
	world.candidates = {{.object = pile,
	                     .kind = TornadoCandidate::Kind::Liftable,
	                     .position = {100.0f, 0.0f, 100.0f},
	                     .radius = 0.5f,
	                     .pile = true}};
	auto effect = Make();
	Run(*effect, 85);
	ASSERT_FALSE(world.carried.empty());
	EXPECT_EQ(world.carried.front()->object, pile);
	EXPECT_TRUE(world.taken.empty());
}

TEST_F(TornadoRuleTest, AHandDroppedPileTooBigForTheFunnelGivesUpAPot)
{
	const auto pile = static_cast<entt::entity>(5);
	world.pot = static_cast<entt::entity>(6);
	world.candidates = {{.object = pile,
	                     .kind = TornadoCandidate::Kind::Liftable,
	                     .position = {100.0f, 0.0f, 100.0f},
	                     .radius = 1000.0f,
	                     .pile = true}};
	auto effect = Make();
	Run(*effect, 85);
	ASSERT_FALSE(world.taken.empty());
	EXPECT_EQ(world.taken.front(), 650u);
	ASSERT_FALSE(world.carried.empty());
	EXPECT_EQ(world.carried.front()->object, world.pot);
}
} // namespace

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
#include <set>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "3D/TreeGrowth.h"
#include "Common/GameRandom.h"
#include "Particles/ParticleClassRegistry.h"
#include "Particles/ParticleEffect.h"
#include "Particles/StormMaths.h"

using namespace openblack;
using namespace openblack::particles;

namespace
{
constexpr float k_Epsilon = 1e-4f;
constexpr float k_Step = 0.1f;

// The rain storm

TEST(StormMaths, WindAndRainAreRoundedIntoTheWeathersBytes)
{
	EXPECT_EQ(storm::WeatherByte(12.4f), 12);
	EXPECT_EQ(storm::WeatherByte(-12.6f), -13);
	// A half goes to the even number
	EXPECT_EQ(storm::WeatherByte(12.5f), 12);
	EXPECT_EQ(storm::WeatherByte(13.5f), 14);
	EXPECT_EQ(storm::WeatherByte(-12.5f), -12);
	EXPECT_EQ(storm::WeatherByte(127.0f), 127);
	// Held at 128 first, which comes round to -128
	EXPECT_EQ(storm::WeatherByte(500.0f), -128);
	EXPECT_EQ(storm::WeatherByte(-500.0f), -128);
}

TEST(StormMaths, ItRainsAtLeastAHundredButOnlyTheLowByteCounts)
{
	EXPECT_EQ(storm::RainByte(true, 50.0f, 1.0f), 100);
	EXPECT_EQ(storm::RainByte(true, 100.0f, 1.0f), 100);
	EXPECT_EQ(storm::RainByte(true, 0.0f, 1.0f), 100);
	EXPECT_EQ(storm::RainByte(true, 120.0f, 1.0f), 120);
	// 150 is -106 as the weather reads it: no rain at all
	EXPECT_EQ(storm::RainByte(true, 150.0f, 1.0f), -106);
	EXPECT_EQ(storm::RainByte(true, std::nullopt, 1.0f), 100);
	EXPECT_EQ(storm::RainByte(false, 100.0f, 1.0f), 0);
}

TEST(StormMaths, TheStormsWindIsFasterTheWiderItIs)
{
	const storm::StormWind wind;
	const glm::vec3 east {1.0f, 0.0f, 0.0f};
	EXPECT_EQ(storm::WindBytes(east, 20.0f, 1.0f, wind), glm::ivec2(40, 0));
	EXPECT_EQ(storm::WindBytes(east, 60.0f, 1.0f, wind), glm::ivec2(70, 0));
	EXPECT_EQ(storm::WindBytes(east, 1000.0f, 1.0f, wind), glm::ivec2(100, 0));
	EXPECT_EQ(storm::WindBytes(east, 1000.0f, 0.5f, wind), glm::ivec2(50, 0));
	// Not made a unit: a hard throw blows harder, up to the byte's limit
	EXPECT_EQ(storm::WindBytes({0.0f, 0.0f, 2.0f}, 1000.0f, 1.0f, wind), glm::ivec2(0, -128));
	EXPECT_EQ(storm::WindBytes(glm::vec3(0.0f), 60.0f, 1.0f, wind), glm::ivec2(0, 0));
}

TEST(StormMaths, TheRainStormCoversTheCloudsAndMore)
{
	const auto small = storm::RainStormFor(glm::vec3(1.0f), 24.0f, 30.0f, 8.0f, {3, 4}, 100);
	EXPECT_FLOAT_EQ(small.innerRadius, 60.0f);
	EXPECT_FLOAT_EQ(small.outerRadius, 80.0f);
	EXPECT_FLOAT_EQ(small.fadeInSeconds, 4.0f);
	EXPECT_EQ(small.temperature, 20);
	EXPECT_EQ(small.overcast, 80);
	EXPECT_EQ(small.rain, 100);
	EXPECT_EQ(small.snow, 0);
	EXPECT_EQ(small.windX, 3);
	EXPECT_EQ(small.windZ, 4);
	const auto wide = storm::RainStormFor(glm::vec3(0.0f), 144.0f, 180.0f, 8.0f, {0, 0}, 100);
	EXPECT_FLOAT_EQ(wide.innerRadius, 144.0f);
	EXPECT_FLOAT_EQ(wide.outerRadius, 360.0f);
	EXPECT_FLOAT_EQ(wide.cloudHeight, 180.0f);
}

// The clouds

TEST(StormMaths, TheCloudsGatherInOverTenSeconds)
{
	EXPECT_FLOAT_EQ(storm::GatherScale(2.0f, 0.0f), 2.0f);
	EXPECT_FLOAT_EQ(storm::GatherScale(2.0f, 5.0f), 1.5f);
	EXPECT_FLOAT_EQ(storm::GatherScale(2.0f, 10.0f), 1.0f);
	EXPECT_FLOAT_EQ(storm::GatherScale(2.0f, 100.0f), 1.0f);
	EXPECT_FLOAT_EQ(storm::GatherScale(1.0f, 3.0f), 1.0f);
	EXPECT_FLOAT_EQ(storm::NewCloudRadius(0.7f, 48.0f), 1.4f * 48.0f);
	EXPECT_FLOAT_EQ(storm::NewCloudRadius(1.0f, 48.0f), 1.7f * 48.0f);
}

TEST(StormMaths, ACloudTurnsFastestHalfwayThroughItsLife)
{
	EXPECT_FLOAT_EQ(storm::CloudAngularSpeed(0.0f, 0.2f, 1.0f), 0.0f);
	EXPECT_FLOAT_EQ(storm::CloudAngularSpeed(0.5f, 0.2f, -1.0f), -0.2f);
	EXPECT_FLOAT_EQ(storm::CloudAngularSpeed(1.0f, 0.2f, 1.0f), 0.0f);
	// It closes in on the middle as it ages
	const float start = storm::CloudDistance(0.0f, 60.0f, 0.0f, 0.0f, 1.0f);
	EXPECT_FLOAT_EQ(start, 60.0f * 1.3f);
	EXPECT_FLOAT_EQ(storm::CloudDistance(1.0f, 60.0f, 0.0f, 0.0f, 1.0f), 0.0f);
	EXPECT_FLOAT_EQ(storm::CloudDistance(0.0f, 60.0f, 0.0f, std::numbers::pi_v<float>, 2.0f), 60.0f * 0.7f * 2.0f);
}

TEST(StormMaths, ACloudThickensThenThinsAndDarkens)
{
	const storm::CloudShades shades;
	const auto born = storm::LookOf(0.0f, 0.3f, 1.0f, shades);
	EXPECT_EQ(born.grey, 120);
	EXPECT_EQ(born.alpha, 0);
	EXPECT_FLOAT_EQ(born.ratio, 10.0f);
	EXPECT_FLOAT_EQ(born.scale, 0.0f);
	const auto fullest = storm::LookOf(shades.fractionToMaxSize, 0.3f, 1.0f, shades);
	EXPECT_EQ(fullest.alpha, 180);
	EXPECT_NEAR(fullest.scale, 0.3f, k_Epsilon);
	const auto old = storm::LookOf(1.0f, 0.3f, 1.0f, shades);
	EXPECT_EQ(old.grey, 90);
	EXPECT_EQ(old.alpha, 0);
	EXPECT_FLOAT_EQ(old.ratio, 1.0f);
}

TEST(StormMaths, AStruckCloudFlashesWhiteBlueAndFades)
{
	EXPECT_EQ(storm::FlashLight(0.0f, 0.5f), glm::u8vec3(199, 199, 254));
	EXPECT_EQ(storm::FlashLight(0.25f, 0.5f), glm::u8vec3(99, 99, 126));
	EXPECT_EQ(storm::FlashLight(0.6f, 0.5f), glm::u8vec3(0));
}

TEST(StormMaths, ThunderSizesAndTheWaitForTheNextBolt)
{
	EXPECT_EQ(storm::ThunderSize(0.1f), 3);
	EXPECT_EQ(storm::ThunderSize(0.5f), 2);
	EXPECT_EQ(storm::ThunderSize(0.9f), 1);
	EXPECT_FLOAT_EQ(storm::NextStrikeWait(0.5f, 3.0f, std::nullopt), 1.5f);
	EXPECT_FLOAT_EQ(storm::NextStrikeWait(1.0f, 3.0f, 2.0f), 1.5f);
	// A tribal power below 1 doesn't slow it
	EXPECT_FLOAT_EQ(storm::NextStrikeWait(1.0f, 3.0f, 0.5f), 3.0f);
}

// The drift

TEST(StormMaths, TheStormDriftsTowardsThreeTimesTheWind)
{
	glm::vec3 velocity(0.0f);
	const glm::vec3 wind {10.0f, 0.0f, 0.0f};
	for (int i = 0; i < 2000; ++i)
	{
		velocity = storm::DriftVelocity(velocity, wind, 30.0f, 0.06f, k_Step);
	}
	EXPECT_NEAR(velocity.x, 30.0f, 0.1f);
	// One time constant in, it has most of the way to go still
	velocity = glm::vec3(0.0f);
	for (int i = 0; i < 167; ++i)
	{
		velocity = storm::DriftVelocity(velocity, wind, 30.0f, 0.06f, k_Step);
	}
	EXPECT_NEAR(velocity.x, 30.0f * (1.0f - std::exp(-1.0f)), 0.3f);
}

TEST(StormMaths, AStormAScriptCastStaysPut)
{
	const glm::vec3 wind {10.0f, 0.0f, 0.0f};
	EXPECT_EQ(storm::DriftWind(wind, false), wind);
	glm::vec3 velocity(0.0f);
	for (int i = 0; i < 2000; ++i)
	{
		velocity = storm::DriftVelocity(velocity, storm::DriftWind(wind, true), 30.0f, 0.06f, k_Step);
	}
	EXPECT_EQ(velocity, glm::vec3(0.0f));
}

TEST(StormMaths, AStormInTheHandFlashesAQuarterToAWholeGapApart)
{
	maths::EmitterClock clock;
	const maths::EmitterLimits limits {.frequency = 2.0f, .maxAlive = 4, .maxTotal = -1, .randomise = true};
	// The first flash at once, the next a gap of half a second times a quarter plus the random number later
	EXPECT_TRUE(storm::ShouldFlash(clock, limits, 0.0f, 0.1f, 0, [] { return 0.5f; }));
	EXPECT_NEAR(clock.next, 0.375f, k_Epsilon);
	EXPECT_FALSE(storm::ShouldFlash(clock, limits, 0.2f, 0.1f, 0, [] { return 0.0f; }));
	EXPECT_TRUE(storm::ShouldFlash(clock, limits, 0.3f, 0.1f, 0, [] { return 0.0f; }));
	EXPECT_NEAR(clock.next, 0.375f + 0.125f, k_Epsilon);
	// More alive than its most: none
	EXPECT_FALSE(storm::ShouldFlash(clock, limits, 1.0f, 0.1f, 5, [] { return 0.0f; }));
}

// The swirl

TEST(StormMaths, TheSwirlTightensThenDisperses)
{
	const storm::SwirlParams params;
	float radius = params.maxRadius;
	float age = 0.0f;
	while (age < params.dispersalAge)
	{
		radius = storm::SwirlRadius(radius, age, k_Step, params);
		age += k_Step;
	}
	EXPECT_FLOAT_EQ(radius, params.minRadius);
	EXPECT_FLOAT_EQ(storm::SwirlTurning(params.minRadius, params), 4.0f);
	EXPECT_FLOAT_EQ(storm::SwirlTurning(params.maxRadius, params), 0.6f);
	EXPECT_GT(storm::SwirlRadius(radius, params.dispersalAge, k_Step, params), radius);
	EXPECT_FLOAT_EQ(storm::SwirlAcceleration(0.5f, params), 0.0f);
	EXPECT_FLOAT_EQ(storm::SwirlAcceleration(2.0f, params), 0.5f);
	EXPECT_FLOAT_EQ(storm::SwirlAcceleration(5.0f, params), 1.0f);
	EXPECT_FLOAT_EQ(*storm::SwirlAlpha(1.0f, params), 255.0f);
	// Linear over the fade, in whole steps of the byte, truncated
	EXPECT_FLOAT_EQ(*storm::SwirlAlpha(3.4f, params), 127.0f);
	EXPECT_FLOAT_EQ(*storm::SwirlAlpha(2.4f, params), 255.0f);
	EXPECT_FLOAT_EQ(*storm::SwirlAlpha(4.39f, params), 1.0f);
	EXPECT_FALSE(storm::SwirlAlpha(4.5f, params).has_value());
}

TEST(StormMaths, AWiderStormCostsTheSquareOfItsWidth)
{
	EXPECT_FLOAT_EQ(storm::CostToMaintain(20.0f, 40.0f, 40.0f), 20.0f);
	EXPECT_FLOAT_EQ(storm::CostToMaintain(20.0f, 120.0f, 40.0f), 180.0f);
	EXPECT_FLOAT_EQ(storm::CostToMaintain(25.0f, 1000.0f, 40.0f), 15625.0f);
}

// What the rain does

TEST(TreeGrowth, RainAndGoodLandMakeATreeGrowFaster)
{
	const tree_growth::Type type {.turnsBetween = 100, .amount = 0.01f, .rainAccelerator = 2.0f};
	EXPECT_FLOAT_EQ(tree_growth::Growth(type, 0, 0.0f), 0.01f);
	// A hundred points of rain at an accelerator of 2 trebles it
	EXPECT_FLOAT_EQ(tree_growth::Growth(type, 100, 0.0f), 0.03f);
	EXPECT_FLOAT_EQ(tree_growth::Growth(type, -50, 0.0f), 0.01f);
	EXPECT_FLOAT_EQ(tree_growth::Growth(type, 0, 1.0f), 0.015f);
	EXPECT_FLOAT_EQ(tree_growth::Growth(type, 0, -1.0f), 0.005f);
	EXPECT_FLOAT_EQ(tree_growth::Grown(0.5f, 0.1f, 1.0f), 0.6f);
	EXPECT_FLOAT_EQ(tree_growth::Grown(0.95f, 0.1f, 1.0f), 1.0f);
}

// The rules on a fake world

class FakeWorld final: public ParticleWorldInterface
{
public:
	[[nodiscard]] float LandHeight(glm::vec2 /*xz*/) const override { return 5.0f; }
	[[nodiscard]] uint32_t PlayerColour(int /*player*/) const override { return 0xFF0000u; }
	[[nodiscard]] glm::vec3 CameraRight() const override { return {1.0f, 0.0f, 0.0f}; }
	[[nodiscard]] glm::vec3 CameraUp() const override { return {0.0f, 1.0f, 0.0f}; }
	void StartSound(const Effect& /*effect*/, const std::shared_ptr<ParticleSoundLink>& sound) override
	{
		sounds.push_back(sound);
	}
	[[nodiscard]] glm::vec3 WindAt(glm::vec3 /*point*/) const override { return wind; }
	uint32_t AddRainStorm(const storm::RainStorm& storm) override
	{
		storms.push_back(storm);
		alive.push_back(true);
		return static_cast<uint32_t>(storms.size());
	}
	bool MoveRainStorm(uint32_t storm, glm::vec3 centre) override
	{
		storms.at(storm - 1).centre = centre;
		if (ended.contains(storm))
		{
			alive.at(storm - 1) = false;
			return false;
		}
		return true;
	}
	void RemoveRainStorm(uint32_t storm) override { alive.at(storm - 1) = false; }

	glm::vec3 wind {0.0f};
	std::vector<storm::RainStorm> storms;
	std::vector<bool> alive;
	/// Storms a script has ended
	std::set<uint32_t> ended;
	std::vector<std::shared_ptr<ParticleSoundLink>> sounds;
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

/// A storm miracle, plain or powered up, that records where it has moved
class FakeStorm final: public SpellSink
{
public:
	bool SpellEvent(const SpellEventInfo& event) override
	{
		events.push_back(event);
		return true;
	}
	[[nodiscard]] int PowerUpLevel() const override { return level; }
	[[nodiscard]] std::optional<float> RainAmount() const override { return 50.0f; }
	void MoveTo(glm::vec3 position) override { moved = position; }

	int level {-1};
	std::vector<SpellEventInfo> events;
	std::optional<glm::vec3> moved;
};

/// The storm's file, trimmed to what the rules here read: five cores with their clouds, the bolt's group and the drift
std::string StormFile()
{
	return R"(BEGINPROPERTIES
PROPERTY DeleteOnCloseDown BOOL 0
PROPERTY Hierarchies ARRAY SIZE 5 0 0 0 0 0
PROPERTY InitiallyCreated ARRAY SIZE 5 1 0 0 0 0
PROPERTY MaxSpellAge FLOAT -1
ENDPROPERTIES
BEGINCLASS ParticlePointCreator ParticlePointCreator0
BEGINPROPERTIES
PROPERTY ColorA INTEGER 255
ENDPROPERTIES
ENDCLASS
BEGINCLASS ParticleMistCreator ParticleMistCreator0
BEGINPROPERTIES
PROPERTY InitialScale FLOAT 12
PROPERTY InitialScaleMin FLOAT 8
PROPERTY RandomiseScale BOOL 1
PROPERTY TakeRatioFromMatrix BOOL 1
ENDPROPERTIES
ENDCLASS
BEGINCLASS CreateRuleSphere CreateRuleSphere0
BEGINPROPERTIES
PROPERTY Group INTEGER 0
PROPERTY NextGroups ARRAY SIZE 1 2
PROPERTY NumAtoms INTEGER 5
PROPERTY PCreator PERSIS_PNTR ParticlePointCreator0
PROPERTY Radius FLOAT 0
ENDPROPERTIES
ENDCLASS
BEGINCLASS UR_CloudMoverNew UR_CloudMoverNew0
BEGINPROPERTIES
PROPERTY DelayBeforeMove FLOAT 5
PROPERTY Group INTEGER 0
PROPERTY WindDamping FLOAT 0.06
PROPERTY WindMagnification FLOAT 30
ENDPROPERTIES
ENDCLASS
BEGINCLASS UR_CloudGather UR_CloudGather0
BEGINPROPERTIES
PROPERTY CloudHeight PERSIS_PNTR FP_Height
PROPERTY Group INTEGER 2
PROPERTY LightningGroup INTEGER 4
PROPERTY PCreator PERSIS_PNTR ParticleMistCreator0
PROPERTY RadiusFloatProvider PERSIS_PNTR FP_Radius
PROPERTY ScaleFloatProvider PERSIS_PNTR FP_Scale
PROPERTY SoundLightning SOUND_ACTION SOUND_SPELL_LIGHTNING LOOPING 0 ONLYONE 0 SOFTRELEASE 1 USESURFACE 0
ENDPROPERTIES
ENDCLASS
BEGINCLASS MagnitudeFloatProvider FP_Radius
BEGINPROPERTIES
PROPERTY ScaleBy FLOAT 1.2
ENDPROPERTIES
ENDCLASS
BEGINCLASS MagnitudeFloatProvider FP_Scale
BEGINPROPERTIES
PROPERTY ScaleBy FLOAT 0.0075
ENDPROPERTIES
ENDCLASS
BEGINCLASS MagnitudeFloatProvider FP_Height
BEGINPROPERTIES
PROPERTY ScaleBy FLOAT 1.5
ENDPROPERTIES
ENDCLASS
)";
}

class StormRulesTest: public ::testing::Test
{
protected:
	std::unique_ptr<Effect> Make(float magnitude, glm::vec3 direction = glm::vec3(0.0f))
	{
		auto file = psys::ParticleFile::Parse(StormFile());
		EXPECT_TRUE(file.has_value());
		auto effect = std::make_unique<Effect>(std::make_shared<const psys::ParticleFile>(std::move(*file)),
		                                       EffectServices {classes, world, random, noise}, origin, magnitude, false);
		effect->SetProcessInfo({.cameraForward = direction});
		effect->SetSink(&spell);
		return effect;
	}
	void Run(Effect& effect, float seconds)
	{
		const auto steps = std::lround(seconds / k_Step);
		for (long i = 0; i < steps; ++i)
		{
			effect.Step(k_Step);
		}
	}
	[[nodiscard]] size_t Clouds(const Effect& effect) const { return effect.AtomCount() - 5; }

	glm::vec3 origin {100.0f, 5.0f, 200.0f};
	ParticleClassRegistry classes {ParticleClassRegistry::WithAllClasses()};
	FakeWorld world;
	FakeRandom random;
	maths::ValueNoise noise;
	FakeStorm spell;
};
} // namespace

TEST_F(StormRulesTest, FiveCoresGatherTenCloudsEachAtACloudAndAHalfOfTheirRadiusUp)
{
	auto effect = Make(40.0f);
	Run(*effect, 4.0f);
	// A cloud and a quarter a second for each core
	EXPECT_EQ(Clouds(*effect), 25u);
	Run(*effect, 6.0f);
	EXPECT_EQ(Clouds(*effect), 50u);
	std::vector<Effect::DrawAtom> mists;
	effect->Collect(1.0f, mists, Creator::Kind::Mist);
	ASSERT_FALSE(mists.empty());
	for (const auto& mist : mists)
	{
		// 60 above the land, plus a little for each cloud
		EXPECT_GE(mist.position.y, 5.0f + 60.0f - k_Epsilon);
		EXPECT_LT(glm::distance(glm::vec2(mist.position.x, mist.position.z), glm::vec2(origin.x, origin.z)),
		          1.7f * 48.0f * 1.3f * 2.0f);
	}
}

TEST_F(StormRulesTest, OnlyTheFirstCoresCloudsLayTheRainStormAndItGoesAtCloseDown)
{
	auto effect = Make(40.0f, {1.0f, 0.0f, 0.0f});
	Run(*effect, 1.0f);
	ASSERT_EQ(world.storms.size(), 1u);
	const auto& rain = world.storms.front();
	EXPECT_FLOAT_EQ(rain.innerRadius, 60.0f);
	EXPECT_FLOAT_EQ(rain.outerRadius, 120.0f);
	EXPECT_EQ(rain.rain, 100);
	EXPECT_EQ(rain.overcast, 80);
	EXPECT_EQ(rain.windX, 55);
	EXPECT_EQ(rain.windZ, 0);
	effect->CloseDown();
	effect->Step(k_Step);
	EXPECT_FALSE(world.alive.front());
}

TEST_F(StormRulesTest, ThePlainStormNeverStrikesThePoweredUpOneDoes)
{
	auto plain = Make(40.0f);
	Run(*plain, 20.0f);
	EXPECT_TRUE(world.sounds.empty());
	spell.level = 0;
	auto powered = Make(40.0f);
	Run(*powered, 9.4f);
	EXPECT_TRUE(world.sounds.empty());
	Run(*powered, 3.0f);
	ASSERT_FALSE(world.sounds.empty());
	EXPECT_EQ(world.sounds.front()->sound.action.sound, "SOUND_SPELL_LIGHTNING");
	EXPECT_TRUE(world.sounds.front()->sound.travelsAtSoundSpeed);
	// Thunder comes from the land under its cloud, not from the cloud
	EXPECT_TRUE(world.sounds.front()->sound.onLand);
	EXPECT_FLOAT_EQ(world.sounds.front()->position.y, 5.0f);
}

TEST_F(StormRulesTest, AStormAScriptEndsIsLaidAgainAtTheNextStep)
{
	auto effect = Make(40.0f, {1.0f, 0.0f, 0.0f});
	Run(*effect, 1.0f);
	ASSERT_EQ(world.storms.size(), 1u);
	world.ended.insert(1);
	effect->Step(k_Step);
	EXPECT_EQ(world.storms.size(), 1u);
	effect->Step(k_Step);
	ASSERT_EQ(world.storms.size(), 2u);
	EXPECT_TRUE(world.alive.back());
	// Fading in again over half the clouds' forming time
	EXPECT_FLOAT_EQ(world.storms.back().fadeInSeconds, world.storms.front().fadeInSeconds);
}

TEST_F(StormRulesTest, CastLookingStraightDownTheStormBringsNoWind)
{
	auto effect = Make(40.0f, {0.0f, -1.0f, 0.0f});
	Run(*effect, 1.0f);
	ASSERT_EQ(world.storms.size(), 1u);
	EXPECT_EQ(world.storms.front().windX, 0);
	EXPECT_EQ(world.storms.front().windZ, 0);
}

TEST_F(StormRulesTest, InTheHandFlashesOfLightningLightTheCloudTheyComeFrom)
{
	const std::string text = R"(BEGINPROPERTIES
PROPERTY DeleteOnCloseDown BOOL 0
PROPERTY Hierarchies ARRAY SIZE 3 0 0 0
PROPERTY InitiallyCreated ARRAY SIZE 3 1 0 0
PROPERTY MaxSpellAge FLOAT -1
ENDPROPERTIES
BEGINCLASS ParticlePointCreator ParticlePointCreator0
BEGINPROPERTIES
PROPERTY ColorA INTEGER 255
ENDPROPERTIES
ENDCLASS
BEGINCLASS ParticleSpriteCreator Cloud
BEGINPROPERTIES
PROPERTY ColorR INTEGER 0
PROPERTY ColorG INTEGER 0
PROPERTY ColorB INTEGER 0
PROPERTY ColorA INTEGER 255
ENDPROPERTIES
ENDCLASS
BEGINCLASS CreateRuleAnAtom CreateRuleAnAtom0
BEGINPROPERTIES
PROPERTY Group INTEGER 0
PROPERTY NextGroups ARRAY SIZE 1 1
PROPERTY PCreator PERSIS_PNTR Cloud
ENDPROPERTIES
ENDCLASS
BEGINCLASS EmitterRuleLightningSprite EmitterRuleSimple_Lightning
BEGINPROPERTIES
PROPERTY EmissionFreq FLOAT 0.8
PROPERTY Group INTEGER 1
PROPERTY MaxAtoms INTEGER 4
PROPERTY PCreator PERSIS_PNTR ParticlePointCreator0
PROPERTY Randomise BOOL 0
ENDPROPERTIES
ENDCLASS
)";
	auto file = psys::ParticleFile::Parse(text);
	ASSERT_TRUE(file.has_value());
	Effect effect(std::make_shared<const psys::ParticleFile>(std::move(*file)), EffectServices {classes, world, random, noise},
	              origin, 1.0f, false);
	const auto cloudLight = [&effect] {
		std::vector<Effect::DrawAtom> sprites;
		effect.Collect(1.0f, sprites);
		return sprites.empty() ? glm::u8vec3(0)
		                       : glm::u8vec3(sprites.front().rgb[0], sprites.front().rgb[1], sprites.front().rgb[2]);
	};
	effect.Step(k_Step);
	effect.Step(k_Step);
	// The flash fades white blue from its brightest as the sprite is made
	const auto first = cloudLight();
	EXPECT_GT(first.z, 200);
	EXPECT_LT(first.x, first.z);
	// It lasts half a second, however often the flashes come
	Run(effect, 0.5f);
	EXPECT_EQ(cloudLight(), glm::u8vec3(0));
	Run(effect, 0.3f);
	EXPECT_EQ(cloudLight(), glm::u8vec3(0));
}

TEST_F(StormRulesTest, AfterFiveSecondsTheStormDriftsWithTheWindAndTakesTheMiracleWithIt)
{
	world.wind = {10.0f, 0.0f, 0.0f};
	auto effect = Make(40.0f);
	Run(*effect, 4.9f);
	EXPECT_FALSE(spell.moved.has_value());
	Run(*effect, 10.0f);
	ASSERT_TRUE(spell.moved.has_value());
	EXPECT_GT(spell.moved->x, origin.x + 1.0f);
	EXPECT_NEAR(spell.moved->z, origin.z, k_Epsilon);
	EXPECT_NEAR(world.storms.front().centre.x, spell.moved->x, 2.0f);
}

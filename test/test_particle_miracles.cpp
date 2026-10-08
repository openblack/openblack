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
#include <map>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Common/GameRandom.h"
#include "Magic/MapSpiral.h"
#include "Particles/ParticleClassRegistry.h"
#include "Particles/ParticleEffect.h"
#include "Particles/ParticleMiracleMaths.h"
#include "Particles/ParticleSounds.h"

using namespace openblack;
using namespace openblack::particles;

namespace
{
constexpr float k_Step = 0.1f;
constexpr float k_Epsilon = 1e-3f;

/// Flat land at height 0, with things to strike, shields and the sounds started
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
	[[nodiscard]] std::optional<TargetInfo> Target(entt::entity target, bool centre) const override
	{
		for (const auto& candidate : candidates)
		{
			if (candidate.object == target)
			{
				return TargetInfo {.position =
				                       candidate.position + glm::vec3(0.0f, centre ? candidate.height * 0.5f : 0.0f, 0.0f),
				                   .radius = 1.0f,
				                   .height = candidate.height};
			}
		}
		return std::nullopt;
	}
	[[nodiscard]] std::vector<StrikeCandidate> StrikeCandidates(glm::vec3 centre, size_t cells) const override
	{
		// Those standing in the cells searched, cell by cell
		std::vector<StrikeCandidate> found;
		const auto cellOf = [](glm::vec3 point) {
			return glm::ivec2(static_cast<int>(std::floor(point.x / 10.0f)), static_cast<int>(std::floor(point.z / 10.0f)));
		};
		for (const auto cell : openblack::magic::SpiralCells(cellOf(centre), cells))
		{
			for (const auto& candidate : candidates)
			{
				if (cellOf(candidate.position) == cell)
				{
					found.push_back(candidate);
				}
			}
		}
		return found;
	}
	void QueueArcs(entt::entity object) override { arcsQueued.push_back(object); }
	void AddShield(const std::shared_ptr<ShieldSphere>& shield) override { shields.push_back(shield); }
	[[nodiscard]] std::shared_ptr<ShieldSphere> FindShield(glm::vec3 point, float margin) const override
	{
		for (const auto& weak : shields)
		{
			if (auto shield = weak.lock(); shield && glm::distance(point, shield->centre) < shield->radius + margin)
			{
				return shield;
			}
		}
		return nullptr;
	}
	[[nodiscard]] std::shared_ptr<ShieldSphere> ShieldOf(const Effect& effect) const override
	{
		for (const auto& weak : shields)
		{
			if (auto shield = weak.lock(); shield && shield->owner == &effect)
			{
				return shield;
			}
		}
		return nullptr;
	}

	[[nodiscard]] std::optional<glm::vec3> ObjectPosition(entt::entity object) const override
	{
		const auto found = objects.find(object);
		return found != objects.end() ? std::optional(found->second) : std::nullopt;
	}
	[[nodiscard]] SurfacePoint RandomSurfacePoint(entt::entity object, Effect& effect) const override
	{
		const auto found = objects.find(object);
		if (found == objects.end())
		{
			return {};
		}
		// A model one high over its place, a point picked on its top
		return {.kind = SurfacePoint::Kind::Point,
		        .position = found->second + glm::vec3(effect.Random(1.0f), 1.0f, effect.Random(1.0f))};
	}

	std::map<entt::entity, glm::vec3> objects;
	std::vector<StrikeCandidate> candidates;
	std::vector<entt::entity> arcsQueued;
	std::vector<std::weak_ptr<ShieldSphere>> shields;
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

/// A miracle that records its events, lets shield strikes through or not, and is cast by a human player
class FakeSpell final: public SpellSink
{
public:
	bool SpellEvent(const SpellEventInfo& event) override
	{
		events.push_back(event);
		return event.type != SpellEventInfo::Type::HitSpell || getsThroughShields;
	}
	[[nodiscard]] int PowerUpLevel() const override { return -1; }
	[[nodiscard]] bool IsHumanPlayerCasting() const override { return true; }
	[[nodiscard]] entt::entity Spell() const override { return self; }

	[[nodiscard]] size_t Count(SpellEventInfo::Type type) const
	{
		return static_cast<size_t>(std::ranges::count(events, type, &SpellEventInfo::type));
	}

	std::vector<SpellEventInfo> events;
	bool getsThroughShields {false};
	entt::entity self {entt::null};
};

std::string Header(std::string_view initially = "1 0 0")
{
	return "BEGINPROPERTIES\nPROPERTY DeleteOnCloseDown BOOL 0\nPROPERTY Hierarchies ARRAY SIZE 3 0 0 0\n"
	       "PROPERTY InitiallyCreated ARRAY SIZE 3 " +
	       std::string(initially) + "\nPROPERTY MaxSpellAge FLOAT -1\nENDPROPERTIES\n";
}

std::string Object(std::string_view className, std::string_view name, std::string_view properties)
{
	return "BEGINCLASS " + std::string(className) + " " + std::string(name) + "\nBEGINPROPERTIES\n" + std::string(properties) +
	       "ENDPROPERTIES\nENDCLASS\n";
}

constexpr std::string_view k_Point = "PROPERTY ColorA INTEGER 255\n";
constexpr std::string_view k_Chain = "PROPERTY ColorA INTEGER 255\nPROPERTY InitialScale FLOAT 1\n"
                                     "PROPERTY TextureFileName STRING .\\Data\\Textures\\S_Lightning.raw\n";

class ParticleMiracleTest: public ::testing::Test
{
protected:
	std::unique_ptr<Effect> Make(const std::string& text, glm::vec3 origin = glm::vec3(0.0f))
	{
		auto file = psys::ParticleFile::Parse(text);
		EXPECT_TRUE(file.has_value()) << text;
		auto effect = std::make_unique<Effect>(std::make_shared<const psys::ParticleFile>(std::move(*file)),
		                                       EffectServices {classes, world, random, noise}, origin, 1.0f, false);
		effect->SetSink(&spell);
		effect->SetPlayer(0);
		return effect;
	}

	ParticleClassRegistry classes {ParticleClassRegistry::WithAllClasses()};
	FakeWorld world;
	FakeRandom random;
	maths::ValueNoise noise;
	FakeSpell spell;
};
} // namespace

// The maths

TEST(ParticleMiracleMaths, TheHandsSpeedIsEasedIntoAThrow)
{
	const maths::ThrowSpeeds speeds;
	EXPECT_FLOAT_EQ(maths::ThrowSpeedFromHand(0.0f, speeds), 0.0f);
	EXPECT_FLOAT_EQ(maths::ThrowSpeedFromHand(25.0f, speeds), 25.0f);
	EXPECT_FLOAT_EQ(maths::ThrowSpeedFromHand(250.0f, speeds), 125.0f);
	EXPECT_FLOAT_EQ(maths::ThrowSpeedFromHand(450.0f, speeds), 200.0f);
	EXPECT_FLOAT_EQ(maths::ThrowSpeedFromHand(9999.0f, speeds), 200.0f);
}

TEST(ParticleMiracleMaths, AThrowGoesALittleAboveTheHandsMovement)
{
	const auto launch = maths::HandThrow({0.0f, 0.0f, 30.0f}, 0.3f);
	EXPECT_NEAR(launch.speed, 30.0f, k_Epsilon);
	EXPECT_NEAR(std::asin(launch.direction.y), 0.3f, k_Epsilon);
	EXPECT_NEAR(launch.direction.z, std::cos(0.3f), k_Epsilon);
	// Straight up stays straight up
	EXPECT_NEAR(maths::LiftDirection({0.0f, 1.0f, 0.0f}, 0.3f).y, 1.0f, k_Epsilon);
	EXPECT_FLOAT_EQ(maths::HandThrow(glm::vec3(0.0f)).speed, 0.0f);
}

TEST(ParticleMiracleMaths, ALobArcsToMostOfTheWayToItsTargetNoSteeperThanItsLimit)
{
	constexpr float k_Gravity = 30.0f;
	const glm::vec3 from {0.0f, 10.0f, 0.0f};
	const glm::vec3 target {0.0f, 0.0f, 100.0f};
	const auto launch = maths::Lob(from, target, k_Gravity);
	ASSERT_GT(launch.speed, 0.0f);
	const float steepness =
	    launch.direction.y / std::sqrt(launch.direction.x * launch.direction.x + launch.direction.z * launch.direction.z);
	EXPECT_LE(steepness, std::tan(maths::k_LobSteepest) + k_Epsilon);
	// Flown under gravity, it comes down to the aim's height about 80 of the way along
	glm::vec3 p = from;
	glm::vec3 v = launch.direction * launch.speed;
	const float aimY = from.y + (target.y - from.y) * maths::k_LobAimShare;
	for (int i = 0; i < 100000 && !(v.y < 0.0f && p.y <= aimY); ++i)
	{
		p += v * 0.001f;
		v.y -= k_Gravity * 0.001f;
	}
	EXPECT_NEAR(p.z, 100.0f * maths::k_LobAimShare, 2.0f);
}

TEST(ParticleMiracleMaths, ABounceKeepsTheSlideAndTurnsBackTheFall)
{
	const auto v = maths::BounceOffSlope({10.0f, -20.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 0.0f, k_Step, 0.9f, 0.1f);
	EXPECT_NEAR(v.x, 9.0f, k_Epsilon);
	EXPECT_NEAR(v.y, 2.0f, k_Epsilon);
	// Drag takes off up to the slide
	const auto dragged = maths::BounceOffSlope({10.0f, -20.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 40.0f, k_Step, 1.0f, 0.0f);
	EXPECT_NEAR(dragged.x, 6.0f, k_Epsilon);
	// Leaving the slope, nothing happens
	EXPECT_EQ(maths::BounceOffSlope({1.0f, 5.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 40.0f, k_Step, 0.5f, 0.5f),
	          glm::vec3(1.0f, 5.0f, 0.0f));
}

TEST(ParticleMiracleMaths, ABoltLooksForTargetsInACone)
{
	const glm::vec3 origin {0.0f, 10.0f, 0.0f};
	const float north = std::numbers::pi_v<float> / 2.0f;
	const float cosHalf = std::cos(std::numbers::pi_v<float> / 4.0f);
	EXPECT_TRUE(maths::InStrikeCone(origin, north, cosHalf, {1.0f, 0.0f, 20.0f}));
	EXPECT_FALSE(maths::InStrikeCone(origin, north, cosHalf, {20.0f, 0.0f, 1.0f}));
	EXPECT_FALSE(maths::InStrikeCone(origin, north, cosHalf, {0.0f, 0.0f, -20.0f}));
	EXPECT_EQ(maths::StrikesAtOnce(10, 3), 2);
	EXPECT_EQ(maths::StrikesAtOnce(10, 50), 10);
	EXPECT_EQ(maths::StrikesAtOnce(10, 0), 0);
}

TEST(ParticleMiracleMaths, AForkSplitsItsTargetsByWhichSideOfTheSplitTheyLie)
{
	const std::vector<glm::vec3> tips {{0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 30.0f}, {5.0f, 0.0f, 25.0f}};
	const auto split = maths::SplitTargets(tips, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 20.0f}, {0.0f, 0.0f, 15.0f});
	EXPECT_EQ(split.ahead, (std::vector<size_t> {1, 2}));
	EXPECT_EQ(split.behind, (std::vector<size_t> {0}));
	// Neither side is left empty
	const std::vector<glm::vec3> beyond {{0.0f, 0.0f, 30.0f}, {1.0f, 0.0f, 31.0f}};
	const auto lopsided = maths::SplitTargets(beyond, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 30.0f}, {0.0f, 0.0f, 10.0f});
	EXPECT_EQ(lopsided.ahead.size(), 1u);
	EXPECT_EQ(lopsided.behind.size(), 1u);
	EXPECT_NEAR(maths::ForkJointScale(2.0f, 0, 0.0f), 2.0f, k_Epsilon);
	EXPECT_NEAR(maths::ForkJointScale(2.0f, 0, 1.0f), 1.0f, k_Epsilon);
	EXPECT_NEAR(maths::ForkJointScale(2.0f, 1, 0.0f), 1.0f, k_Epsilon);
}

TEST(ParticleMiracleMaths, AShieldIsEnteredAtItsSurfaceAndBouncesWhatStrikesIt)
{
	const glm::vec3 centre {0.0f, 0.0f, 0.0f};
	EXPECT_TRUE(maths::InsideSphere({0.0f, 9.0f, 0.0f}, centre, 10.0f, 0.0f));
	EXPECT_FALSE(maths::InsideSphere({0.0f, 11.0f, 0.0f}, centre, 10.0f, 0.0f));
	EXPECT_TRUE(maths::InsideSphere({0.0f, 11.0f, 0.0f}, centre, 10.0f, 2.0f));
	const auto entry = maths::SphereEntry({0.0f, 0.0f, -30.0f}, {0.0f, 0.0f, 30.0f}, centre, 10.0f, 0.0f);
	EXPECT_NEAR(entry.z, -10.0f, k_Epsilon);
	// Starting inside, at the start; missing it, the end
	EXPECT_EQ(maths::SphereEntry({0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 30.0f}, centre, 10.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	EXPECT_EQ(maths::SphereEntry({20.0f, 0.0f, -30.0f}, {20.0f, 0.0f, 30.0f}, centre, 10.0f, 0.0f),
	          glm::vec3(20.0f, 0.0f, 30.0f));
	const auto bounced = maths::DeflectOffSphere({0.0f, 0.0f, -10.0f}, centre, {1.0f, 0.0f, 5.0f});
	EXPECT_NEAR(bounced.z, -5.0f, k_Epsilon);
	EXPECT_NEAR(bounced.x, 1.0f, k_Epsilon);
}

// The rules

TEST_F(ParticleMiracleTest, AFireballIsThrownFromTheHandFliesBouncesAndSaysWhereItIsEachStep)
{
	auto effect =
	    Make(Header() + Object("ParticleSpriteCreator", "Ball", k_Point) +
	         Object("CreateWithInitialDirection", "Throw", "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Ball\n") +
	         Object("UpdateRuleGravityWithFloor", "Flight",
	                "PROPERTY Group INTEGER 0\nPROPERTY Gravity FLOAT 30\nPROPERTY UseWind BOOL 0\n"
	                "PROPERTY DampingVerticalBounce FLOAT 0.5\n") +
	         Object("EventAlways", "Event", "PROPERTY Group INTEGER 0\n"));
	effect->SetDirection({0.0f, 0.0f, 30.0f});
	effect->SetProcessInfo({.handPosition = {0.0f, 5.0f, 0.0f}});
	// The game draws the ball from its second step
	effect->Step(k_Step);
	effect->Step(k_Step);
	std::vector<Effect::DrawAtom> atoms;
	effect->Collect(1.0f, atoms);
	ASSERT_EQ(atoms.size(), 1u);
	// From the hand, northwards and up
	EXPECT_GT(atoms[0].position.z, 0.0f);
	bool landed = false;
	float lastZ = atoms[0].position.z;
	for (int step = 0; step < 60; ++step)
	{
		effect->Step(k_Step);
		atoms.clear();
		effect->Collect(1.0f, atoms);
		ASSERT_EQ(atoms.size(), 1u);
		EXPECT_GE(atoms[0].position.y, -k_Epsilon);
		EXPECT_GE(atoms[0].position.z, lastZ - k_Epsilon);
		lastZ = atoms[0].position.z;
		landed = landed || atoms[0].position.y < k_Epsilon;
	}
	EXPECT_TRUE(landed);
	EXPECT_EQ(spell.Count(SpellEventInfo::Type::Point), 62u);
}

TEST_F(ParticleMiracleTest, AFireballBouncesOffAShieldItIsNotLetThrough)
{
	// The shield's own effect raises it
	FakeSpell shieldSpell;
	shieldSpell.self = static_cast<entt::entity>(5);
	auto shieldFile = psys::ParticleFile::Parse(Header() +
	                                            Object("UR_AddDefensiveSphere", "Sphere",
	                                                   "PROPERTY Group INTEGER 0\nPROPERTY SphereRadius PERSIS_PNTR "
	                                                   "Radius\n") +
	                                            Object("ConstFloatProvider", "Radius", "PROPERTY ConstValue FLOAT 10\n"));
	ASSERT_TRUE(shieldFile.has_value());
	Effect shield(std::make_shared<const psys::ParticleFile>(std::move(*shieldFile)),
	              EffectServices {classes, world, random, noise}, {0.0f, 0.0f, 30.0f}, 1.0f, false);
	shield.SetSink(&shieldSpell);
	shield.Step(k_Step);
	ASSERT_EQ(world.shields.size(), 1u);
	EXPECT_FALSE(shield.Finished());

	// Thrown flat, straight at it: the hand moves down by the lift every hand's throw has, so the ball leaves level
	auto ball =
	    Make(Header() + Object("ParticleSpriteCreator", "Ball", k_Point) +
	         Object("CreateWithInitialDirection", "Throw", "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Ball\n") +
	         Object("UpdateRuleGravityWithFloor", "Flight",
	                "PROPERTY Group INTEGER 0\nPROPERTY Gravity FLOAT 0\nPROPERTY UseWind BOOL 0\n"
	                "PROPERTY CheckShieldDeflections BOOL 1\n"));
	ball->SetDirection(glm::vec3(0.0f, -std::sin(maths::k_ThrowLift), std::cos(maths::k_ThrowLift)) * 40.0f);
	ball->SetProcessInfo({.handPosition = {0.0f, 2.0f, 0.0f}});
	float furthest = 0.0f;
	for (int step = 0; step < 30; ++step)
	{
		ball->Step(k_Step);
		std::vector<Effect::DrawAtom> atoms;
		ball->Collect(1.0f, atoms);
		// The game draws the ball from its second step
		if (!atoms.empty())
		{
			furthest = std::max(furthest, atoms.at(0).position.z);
		}
	}
	// It struck the shield, which sparked, and never got inside it
	ASSERT_GE(spell.Count(SpellEventInfo::Type::HitSpell), 1u);
	const auto hit = std::ranges::find(spell.events, SpellEventInfo::Type::HitSpell, &SpellEventInfo::type);
	EXPECT_EQ(hit->target, shieldSpell.self);
	EXPECT_LT(furthest, 30.0f - 10.0f + 1.0f);
	EXPECT_FALSE(world.shields.front().lock()->impacts.empty());
}

TEST_F(ParticleMiracleTest, ABoltStrikesWhatIsInFrontOfTheHandAndNotWhatIsBehind)
{
	const auto ahead = static_cast<entt::entity>(1);
	const auto behind = static_cast<entt::entity>(2);
	world.candidates = {{.object = ahead, .position = {0.0f, 0.0f, 20.0f}, .height = 10.0f},
	                    {.object = behind, .position = {0.0f, 0.0f, -20.0f}, .height = 10.0f}};
	auto bolt = Make(Header("1 0 0") + Object("ParticleChainCreator", "Joint", k_Chain) +
	                 Object("UR_Lightning", "Bolt",
	                        "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Joint\nPROPERTY ForkGroup INTEGER 1\n"
	                        "PROPERTY MinLightningObjects INTEGER 1\nPROPERTY MaxJointsPerFork INTEGER 6\n"
	                        "PROPERTY DefaultSearchRadius FLOAT 50\nPROPERTY SplitAngle FLOAT 0.785\n"));
	bolt->SetProcessInfo({.handPosition = {0.0f, 15.0f, 0.0f}, .cameraForward = {0.0f, 0.0f, 1.0f}, .enabled = true});
	for (int step = 0; step < 20; ++step)
	{
		bolt->Step(k_Step);
	}
	ASSERT_GE(spell.Count(SpellEventInfo::Type::Landed), 1u);
	for (const auto& event : spell.events)
	{
		if (event.type == SpellEventInfo::Type::Landed)
		{
			// At the top of what it strikes, and everything within reach of it takes the strike, not only the target
			EXPECT_TRUE(event.target == entt::null);
			EXPECT_NEAR(event.position.z, 20.0f, k_Epsilon);
			EXPECT_NEAR(event.position.y, 10.0f, k_Epsilon);
		}
	}
	// Its forks are drawn as ribbons from the hand
	Effect::DrawWalk walk;
	bolt->Walk(1.0f, walk);
	ASSERT_FALSE(walk.chains.empty());
	EXPECT_NEAR(walk.joints.at(walk.chains.front().firstJoint).position.y, 15.0f, k_Epsilon);
}

TEST_F(ParticleMiracleTest, ACreatureInTheConeTakesEveryFork)
{
	world.candidates = {
	    {.object = static_cast<entt::entity>(1), .position = {-3.0f, 0.0f, 20.0f}, .height = 2.0f},
	    {.object = static_cast<entt::entity>(2), .position = {3.0f, 0.0f, 25.0f}, .height = 10.0f, .drawsBolt = true}};
	auto bolt = Make(Header("1 0 0") + Object("ParticleChainCreator", "Joint", k_Chain) +
	                 Object("UR_Lightning", "Bolt",
	                        "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Joint\nPROPERTY ForkGroup INTEGER 1\n"
	                        "PROPERTY MinLightningObjects INTEGER 4\nPROPERTY MaxJointsPerFork INTEGER 6\n"
	                        "PROPERTY DefaultSearchRadius FLOAT 50\nPROPERTY SplitAngle FLOAT 0.785\n"));
	bolt->SetProcessInfo({.handPosition = {0.0f, 15.0f, 0.0f}, .cameraForward = {0.0f, 0.0f, 1.0f}, .enabled = true});
	for (int step = 0; step < 20; ++step)
	{
		bolt->Step(k_Step);
	}
	ASSERT_GE(spell.Count(SpellEventInfo::Type::Landed), 1u);
	for (const auto& event : spell.events)
	{
		if (event.type == SpellEventInfo::Type::Landed)
		{
			// Every strike is on the creature, none on the villager or the ground
			EXPECT_NEAR(event.position.x, 3.0f, k_Epsilon);
			EXPECT_NEAR(event.position.z, 25.0f, k_Epsilon);
		}
	}
}

TEST_F(ParticleMiracleTest, ABoltFromACloudStrikesWhatIsWithinItsRadiusInItsCellsAndIsNotDrawnToCreatures)
{
	const auto villager = static_cast<entt::entity>(1);
	const auto creature = static_cast<entt::entity>(2);
	world.candidates = {{.object = villager, .position = {50.0f, 0.0f, 50.0f}},
	                    {.object = creature, .position = {45.0f, 0.0f, 52.0f}, .drawsBolt = true},
	                    // In the cells searched but beyond the radius
	                    {.object = static_cast<entt::entity>(3), .position = {41.0f, 0.0f, 41.0f}},
	                    // Within the radius, but in a cell only a bolt from a hand searches
	                    {.object = static_cast<entt::entity>(4), .position = {65.0f, 0.0f, 55.0f}}};
	auto bolt = Make(Header("1 0 0") + Object("ParticleChainCreator", "Joint", k_Chain) +
	                     Object("UR_Lightning", "Bolt",
	                            "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Joint\nPROPERTY ForkGroup INTEGER 1\n"
	                            "PROPERTY MinLightningObjects INTEGER 1\nPROPERTY MaxJointsPerFork INTEGER 6\n"
	                            "PROPERTY DefaultSearchRadius FLOAT 15\nPROPERTY CastingFromHand BOOL 0\n"),
	                 {55.0f, 0.0f, 55.0f});
	bolt->SetProcessInfo({.enabled = true});
	for (int step = 0; step < 30; ++step)
	{
		bolt->Step(k_Step);
	}
	ASSERT_GE(spell.Count(SpellEventInfo::Type::Landed), 1u);
	size_t onVillager = 0;
	for (const auto& event : spell.events)
	{
		if (event.type == SpellEventInfo::Type::Landed)
		{
			const glm::vec2 at(event.position.x, event.position.z);
			const bool onOne = glm::distance(at, glm::vec2(50.0f, 50.0f)) < k_Epsilon;
			EXPECT_TRUE(onOne || glm::distance(at, glm::vec2(45.0f, 52.0f)) < k_Epsilon) << at.x << "," << at.y;
			onVillager += onOne ? 1 : 0;
		}
	}
	// The creature doesn't take every fork
	EXPECT_GE(onVillager, 1u);
}

TEST_F(ParticleMiracleTest, ABoltGivenTargetsStrikesWhereTheyStandWithinItsRadius)
{
	const auto near = static_cast<entt::entity>(1);
	const auto far = static_cast<entt::entity>(2);
	world.candidates = {{.object = near, .position = {60.0f, 3.0f, 60.0f}, .height = 10.0f},
	                    {.object = far, .position = {100.0f, 0.0f, 100.0f}, .height = 10.0f}};
	auto bolt = Make(Header("1 0 0") + Object("ParticleChainCreator", "Joint", k_Chain) +
	                     Object("UR_Lightning", "Bolt",
	                            "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Joint\nPROPERTY ForkGroup INTEGER 1\n"
	                            "PROPERTY MinLightningObjects INTEGER 1\nPROPERTY MaxJointsPerFork INTEGER 6\n"
	                            "PROPERTY DefaultSearchRadius FLOAT 15\nPROPERTY CastingFromHand BOOL 0\n"
	                            "PROPERTY TakeTargetsFromManager BOOL 1\nPROPERTY RenewSearchEvery FLOAT 0\n"),
	                 {55.0f, 0.0f, 55.0f});
	bolt->AddTarget(near);
	bolt->AddTarget(far);
	bolt->SetProcessInfo({.enabled = true});
	for (int step = 0; step < 5; ++step)
	{
		bolt->Step(k_Step);
	}
	ASSERT_GE(spell.Count(SpellEventInfo::Type::Landed), 1u);
	for (const auto& event : spell.events)
	{
		if (event.type == SpellEventInfo::Type::Landed)
		{
			// At the foot of the near one, a point rather than the object's top; the far one is beyond the radius
			EXPECT_NEAR(event.position.x, 60.0f, k_Epsilon);
			EXPECT_NEAR(event.position.y, 3.0f, k_Epsilon);
			EXPECT_NEAR(event.position.z, 60.0f, k_Epsilon);
		}
	}
	EXPECT_TRUE(world.arcsQueued.empty());
}

TEST_F(ParticleMiracleTest, ArcsCrawlOverWhatABoltStrikesAgainAfterEachSearch)
{
	const auto tree = static_cast<entt::entity>(1);
	world.candidates = {{.object = tree, .position = {0.0f, 0.0f, 20.0f}, .height = 10.0f, .arcs = true}};
	auto bolt = Make(Header("1 0 0") + Object("ParticleChainCreator", "Joint", k_Chain) +
	                 Object("UR_Lightning", "Bolt",
	                        "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Joint\nPROPERTY ForkGroup INTEGER 1\n"
	                        "PROPERTY MinLightningObjects INTEGER 1\nPROPERTY MaxJointsPerFork INTEGER 6\n"
	                        "PROPERTY DefaultSearchRadius FLOAT 50\nPROPERTY SplitAngle FLOAT 0.785\n"));
	bolt->SetProcessInfo({.handPosition = {0.0f, 15.0f, 0.0f}, .cameraForward = {0.0f, 0.0f, 1.0f}, .enabled = true});
	// Not in the first 0.2 seconds after a search
	for (int step = 0; step < 2; ++step)
	{
		bolt->Step(k_Step);
	}
	EXPECT_TRUE(world.arcsQueued.empty());
	// Struck every step, it is queued once for each search, about every second
	for (int step = 2; step < 32; ++step)
	{
		bolt->Step(k_Step);
	}
	EXPECT_GE(world.arcsQueued.size(), 2u);
	EXPECT_LE(world.arcsQueued.size(), 3u);
	EXPECT_TRUE(std::ranges::all_of(world.arcsQueued, [tree](entt::entity object) { return object == tree; }));
}

TEST_F(ParticleMiracleTest, ABoltThatIsNoLongerCastStrikesNothing)
{
	world.candidates = {{.object = static_cast<entt::entity>(1), .position = {0.0f, 0.0f, 20.0f}, .height = 10.0f}};
	auto bolt = Make(Header("1 0 0") + Object("ParticleChainCreator", "Joint", k_Chain) +
	                 Object("UR_Lightning", "Bolt",
	                        "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Joint\nPROPERTY ForkGroup INTEGER 1\n"
	                        "PROPERTY MinLightningObjects INTEGER 1\n"));
	bolt->SetProcessInfo({.handPosition = {0.0f, 15.0f, 0.0f}, .cameraForward = {0.0f, 0.0f, 1.0f}, .enabled = false});
	for (int step = 0; step < 20; ++step)
	{
		bolt->Step(k_Step);
	}
	EXPECT_EQ(spell.Count(SpellEventInfo::Type::Landed), 0u);
}

TEST_F(ParticleMiracleTest, TheMiracleInTheHandFollowsTheHand)
{
	auto effect = Make(Header() + Object("ParticleSpriteCreator", "Glow", k_Point) +
	                   Object("CreateRuleAnAtom", "One", "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Glow\n") +
	                   Object("UR_FollowLocalHand", "Follow", "PROPERTY Group INTEGER 0\n"));
	effect->SetProcessInfo({.handPosition = {3.0f, 4.0f, 5.0f}});
	effect->Step(k_Step);
	effect->SetProcessInfo({.handPosition = {13.0f, 4.0f, 5.0f}});
	effect->Step(k_Step);
	std::vector<Effect::DrawAtom> atoms;
	effect->Collect(1.0f, atoms);
	ASSERT_EQ(atoms.size(), 1u);
	EXPECT_NEAR(atoms[0].position.x, 13.0f, k_Epsilon);
}

TEST_F(ParticleMiracleTest, TheSprinklingSourceFollowsTheHandNoHigherThanALimit)
{
	auto effect =
	    Make(Header() + Object("ParticlePointCreator", "Source", k_Point) +
	         Object("UR_HandSprinkle", "Sprinkle", "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Source\n"));
	effect->SetProcessInfo({.handPosition = {1.0f, 500.0f, 2.0f}});
	effect->Step(k_Step);
	ASSERT_EQ(effect->AtomCount(), 1u);
	Effect::DrawWalk walk;
	effect->Walk(1.0f, walk);
	// A point is not drawn, but its place is where the grains fall from: the land plus 58
	EXPECT_TRUE(walk.atoms.empty());
}

TEST_F(ParticleMiracleTest, AnAtomsSoundIsLetGoWhenTheAtomGoes)
{
	auto effect = Make(Header() + Object("ParticleSpriteCreator", "Glow", k_Point) +
	                   Object("CreateRuleAnAtom", "One", "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Glow\n") +
	                   Object("StartStopSoundOnCondition", "Hum",
	                          "PROPERTY Group INTEGER 0\nPROPERTY Sound SOUND_ACTION SOUND_SPELL_HEAL LOOPING 1 ONLYONE 0 "
	                          "SOFTRELEASE 0 USESURFACE 0\n") +
	                   Object("RemoveRuleOldAgeOnly", "Old", "PROPERTY Group INTEGER 0\nPROPERTY DieAge FLOAT 0.35\n"));
	effect->Step(k_Step);
	ASSERT_EQ(world.sounds.size(), 1u);
	EXPECT_EQ(world.sounds[0]->sound.action.sound, "SOUND_SPELL_HEAL");
	EXPECT_NE(world.sounds[0]->atom, nullptr);
	// Started once, kept while the atom lives
	effect->Step(k_Step);
	EXPECT_EQ(world.sounds.size(), 1u);
	for (int step = 0; step < 5; ++step)
	{
		effect->Step(k_Step);
	}
	EXPECT_EQ(world.sounds[0]->atom, nullptr);
}

TEST_F(ParticleMiracleTest, SparklesComeOffTheModelOfTheObjectTheEffectIsGiven)
{
	const auto pile = static_cast<entt::entity>(7);
	world.objects[pile] = {10.0f, 0.0f, 20.0f};
	auto effect =
	    Make(Header() + Object("ParticleSpriteCreator", "Grain", k_Point) +
	         Object("CreateRule_GameObjectRef", "Anchor", "PROPERTY Group INTEGER 0\nPROPERTY NextGroups ARRAY SIZE 1 1\n") +
	         Object("ER_EmitFromParentAtom", "Sparkles",
	                "PROPERTY Group INTEGER 1\nPROPERTY PCreator PERSIS_PNTR Grain\nPROPERTY MaxAtoms INTEGER 15\n"
	                "PROPERTY AtomAgeZeroSize FLOAT 1.7\nPROPERTY DoScaling BOOL 0\nPROPERTY EmitOnlyAboveLandscape BOOL 1\n"));
	effect->AddTarget(pile);
	for (int i = 0; i < 10; ++i)
	{
		effect->Step(k_Step);
	}
	// The unseen anchor and, at fifteen over 1.7 seconds, nearly nine owed after a second, let out until none is owed
	EXPECT_EQ(effect->AtomCount(), 1u + 9u);
	std::vector<Effect::DrawAtom> drawn;
	effect->Collect(1.0f, drawn);
	ASSERT_FALSE(drawn.empty());
	for (const auto& sparkle : drawn)
	{
		EXPECT_NEAR(sparkle.position.y, 1.0f, k_Epsilon);
		EXPECT_GE(sparkle.position.x, 10.0f);
		EXPECT_LE(sparkle.position.x, 11.0f);
	}
	// Once the object goes the anchor stays where it was, and points at it on the ground are not above the land: the
	// sparkles die out and no more come
	world.objects.clear();
	for (int i = 0; i < 25; ++i)
	{
		effect->Step(k_Step);
	}
	EXPECT_EQ(effect->AtomCount(), 1u);
}

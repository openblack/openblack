/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdlib>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <gtest/gtest.h>

#include "Common/GameRandom.h"
#include "Common/Zip.h"
#include "Particles/ParticleClassRegistry.h"
#include "Particles/ParticleCreators.h"
#include "Particles/ParticleDrawFrame.h"
#include "Particles/ParticleEffect.h"

using namespace openblack;
using namespace openblack::particles;

namespace
{
constexpr float k_Step = 0.1f;
constexpr float k_Epsilon = 1e-4f;

/// Flat land at a height, red players and a camera looking north
class FakeWorld final: public ParticleWorldInterface
{
public:
	[[nodiscard]] float LandHeight(glm::vec2 /*xz*/) const override { return landHeight; }
	[[nodiscard]] uint32_t PlayerColour(int /*player*/) const override { return 0xFF0000u; }
	[[nodiscard]] glm::vec3 CameraRight() const override { return {1.0f, 0.0f, 0.0f}; }
	[[nodiscard]] glm::vec3 CameraUp() const override { return {0.0f, 1.0f, 0.0f}; }
	void StartSound(const Effect& /*effect*/, const std::shared_ptr<ParticleSoundLink>& sound) override
	{
		sounds.push_back(sound->sound.action.sound);
	}

	[[nodiscard]] std::optional<TargetInfo> Target(entt::entity target, bool centre) const override
	{
		const auto found = std::ranges::find(targets, target, &std::pair<entt::entity, glm::vec3>::first);
		if (found == targets.end())
		{
			return std::nullopt;
		}
		return TargetInfo {
		    .position = found->second + glm::vec3(0.0f, centre ? 1.0f : 0.0f, 0.0f), .radius = 3.0f, .height = 2.0f};
	}
	[[nodiscard]] bool IsTargetClaimed(entt::entity target) const override { return claimed.contains(target); }
	void ClaimTarget(entt::entity target, bool claim) override
	{
		if (claim)
		{
			claimed.insert(target);
		}
		else
		{
			claimed.erase(target);
		}
	}
	void SetTargetGlow(entt::entity target, glm::u8vec3 rgb) override { glows[target] = rgb; }

	float landHeight {0.0f};
	std::vector<std::string> sounds;
	std::vector<std::pair<entt::entity, glm::vec3>> targets;
	std::set<entt::entity> claimed;
	std::map<entt::entity, glm::u8vec3> glows;
};

/// The game's generator on seeds of its own
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

/// Records the events a miracle is sent
class FakeSpell final: public SpellSink
{
public:
	bool SpellEvent(const SpellEventInfo& event) override
	{
		events.push_back(event);
		return true;
	}
	[[nodiscard]] int PowerUpLevel() const override { return 0; }

	std::vector<SpellEventInfo> events;
};

std::string Header(std::string_view hierarchies = "0 0", float maxAge = -1.0f, bool deleteOnCloseDown = false)
{
	return "BEGINPROPERTIES\n"
	       "PROPERTY DeleteOnCloseDown BOOL " +
	       std::string(deleteOnCloseDown ? "1" : "0") + "\nPROPERTY Hierarchies ARRAY SIZE 2 " + std::string(hierarchies) +
	       "\nPROPERTY InitiallyCreated ARRAY SIZE 2 1 0\nPROPERTY MaxSpellAge FLOAT " + std::to_string(maxAge) +
	       "\nENDPROPERTIES\n";
}

std::string Object(std::string_view className, std::string_view name, std::string_view properties)
{
	return "BEGINCLASS " + std::string(className) + " " + std::string(name) + "\nBEGINPROPERTIES\n" + std::string(properties) +
	       "ENDPROPERTIES\nENDCLASS\n";
}

constexpr std::string_view k_Sprite = "PROPERTY ColorA INTEGER 200\nPROPERTY InitialScale FLOAT 2\n"
                                      "PROPERTY TextureFileName STRING .\\Data\\Textures\\S_SpriteSheet1.raw\n";

class ParticleEffectTest: public ::testing::Test
{
protected:
	std::unique_ptr<Effect> Make(const std::string& text, glm::vec3 origin = glm::vec3(0.0f), float magnitude = 1.0f)
	{
		auto file = psys::ParticleFile::Parse(text);
		EXPECT_TRUE(file.has_value()) << text;
		return std::make_unique<Effect>(std::make_shared<const psys::ParticleFile>(std::move(*file)),
		                                EffectServices {classes, world, random, noise}, origin, magnitude, false);
	}

	ParticleClassRegistry classes {ParticleClassRegistry::WithAllClasses()};
	FakeWorld world;
	FakeRandom random;
	maths::ValueNoise noise;
};
} // namespace

TEST_F(ParticleEffectTest, AnEmitterFillsItsCollectionUpToItsLimit)
{
	auto effect = Make(Header() + Object("ParticleSpriteCreator", "Sprite0", k_Sprite) +
	                       Object("DiskEmitter", "Emitter0",
	                              "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Sprite0\n"
	                              "PROPERTY EmissionFreq FLOAT 10\nPROPERTY Randomise BOOL 0\nPROPERTY MaxAtoms INTEGER 4\n"
	                              "PROPERTY Radius FLOAT 3\nPROPERTY Height FLOAT 1\n"),
	                   {100.0f, 0.0f, 200.0f});
	effect->Step(k_Step);
	EXPECT_EQ(effect->AtomCount(), 1u);
	for (int i = 0; i < 20; ++i)
	{
		effect->Step(k_Step);
	}
	// One a step while no more than four are alive
	EXPECT_EQ(effect->AtomCount(), 5u);
	std::vector<Effect::DrawAtom> atoms;
	effect->Collect(1.0f, atoms);
	ASSERT_EQ(atoms.size(), 5u);
	for (const auto& atom : atoms)
	{
		// On the disk round the origin, a unit up, with the creator's look
		EXPECT_LE(glm::length(glm::vec2(atom.position.x - 100.0f, atom.position.z - 200.0f)), 3.0f + k_Epsilon);
		EXPECT_NEAR(atom.position.y, 1.0f, k_Epsilon);
		EXPECT_NEAR(atom.scale, 2.0f, k_Epsilon);
		EXPECT_NEAR(atom.alpha, 200.0f, k_Epsilon);
	}
	EXPECT_FALSE(effect->Finished());
}

TEST_F(ParticleEffectTest, AtomsDieOfOldAgeAndTheEffectEndsAfterClosingDown)
{
	auto effect = Make(Header() + Object("ParticleSpriteCreator", "Sprite0", k_Sprite) +
	                   Object("EmitterRuleSimple", "Emitter0",
	                          "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Sprite0\n"
	                          "PROPERTY EmissionFreq FLOAT 10\nPROPERTY Randomise BOOL 0\nPROPERTY Speed FLOAT 0\n"
	                          "PROPERTY RemoveOnCloseDown BOOL 1\n") +
	                   Object("RemoveRuleOldAgeOnly", "Remove0", "PROPERTY Group INTEGER 0\nPROPERTY DieAge FLOAT 0.45\n"));
	for (int i = 0; i < 20; ++i)
	{
		effect->Step(k_Step);
	}
	// Each lives five steps
	EXPECT_EQ(effect->AtomCount(), 5u);
	effect->CloseDown();
	EXPECT_TRUE(effect->Closing());
	EXPECT_FALSE(effect->DeleteOnCloseDown());
	for (int i = 0; i < 4; ++i)
	{
		effect->Step(k_Step);
		EXPECT_FALSE(effect->Finished());
	}
	effect->Step(k_Step);
	effect->Step(k_Step);
	EXPECT_EQ(effect->AtomCount(), 0u);
	EXPECT_TRUE(effect->Finished());
}

TEST_F(ParticleEffectTest, ItEndsAtItsFilesAge)
{
	auto effect =
	    Make(Header("0 0", 0.25f) + Object("ParticleSpriteCreator", "Sprite0", k_Sprite) +
	         Object("CreateRuleAnAtom", "Create0", "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Sprite0\n"));
	effect->Step(k_Step);
	EXPECT_FALSE(effect->Finished());
	effect->Step(k_Step);
	effect->Step(k_Step);
	EXPECT_TRUE(effect->Finished());
}

TEST_F(ParticleEffectTest, RulesFadeScaleAndMoveAtoms)
{
	auto effect = Make(Header() + Object("ParticleSpriteCreator", "Sprite0", k_Sprite) +
	                   Object("CreateRuleAnAtom", "Create0",
	                          "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Sprite0\nPROPERTY OffsetY FLOAT 10\n") +
	                   Object("AR_FadeAlpha", "Fade0",
	                          "PROPERTY Group INTEGER 0\nPROPERTY StartTime FLOAT 0\nPROPERTY StopTime FLOAT 1\n"
	                          "PROPERTY StartAlpha INTEGER 200\nPROPERTY StopAlpha INTEGER 0\n") +
	                   Object("UR_ChangeScale", "Scale0",
	                          "PROPERTY Group INTEGER 0\nPROPERTY StartTime FLOAT 0\nPROPERTY StopTime FLOAT 1\n"
	                          "PROPERTY StartScale FLOAT 1\nPROPERTY StopScale FLOAT 3\n") +
	                   Object("UpdateRuleGravity", "Gravity0",
	                          "PROPERTY Group INTEGER 0\nPROPERTY Gravity FLOAT 10\nPROPERTY MaxSpeed FLOAT 100\n"));
	effect->Step(k_Step); // made
	for (int i = 0; i < 5; ++i)
	{
		effect->Step(k_Step);
	}
	std::vector<Effect::DrawAtom> atoms;
	effect->Collect(1.0f, atoms);
	ASSERT_EQ(atoms.size(), 1u);
	// Half a second old by the last step's rules: half faded, scaled half way, fallen
	EXPECT_NEAR(atoms[0].alpha, 100.0f, 1.0f);
	EXPECT_NEAR(atoms[0].scale, 2.0f * 2.0f, 1e-3f);
	EXPECT_LT(atoms[0].position.y, 10.0f);
	// Drawn half way between the last two steps
	std::vector<Effect::DrawAtom> between;
	effect->Collect(0.5f, between);
	ASSERT_EQ(between.size(), 1u);
	EXPECT_GT(between[0].position.y, atoms[0].position.y);
}

TEST_F(ParticleEffectTest, AnEffectToldSoStepsTwiceTheFirstTime)
{
	const auto file = Header() + Object("ParticleSpriteCreator", "Sprite0", k_Sprite) +
	                  Object("CreateRuleAnAtom", "Create0",
	                         "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Sprite0\nPROPERTY OffsetY FLOAT 10\n");
	auto once = Make(file);
	auto twice = Make(file);
	twice->StepTwiceFirstTime();
	once->Step(k_Step);
	twice->Step(k_Step);
	EXPECT_NEAR(once->GetAge(), k_Step, 1e-6f);
	EXPECT_NEAR(twice->GetAge(), 2.0f * k_Step, 1e-6f);
	// Then one step at a time, a step ahead
	once->Step(k_Step);
	twice->Step(k_Step);
	EXPECT_NEAR(once->GetAge(), 2.0f * k_Step, 1e-6f);
	EXPECT_NEAR(twice->GetAge(), 3.0f * k_Step, 1e-6f);
}

TEST_F(ParticleEffectTest, HierarchiesCarryTheirChildren)
{
	// Group 0's atom is a hierarchy: group 1's atoms live in its frame, two up and scaled by its scale of 3
	auto effect = Make(
	    Header("1 0") + Object("ParticlePointCreator", "Point0", "PROPERTY InitialScale FLOAT 3\n") +
	        Object("ParticleSpriteCreator", "Sprite0", k_Sprite) +
	        Object("CreateRuleAnAtom", "Create0",
	               "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Point0\nPROPERTY NextGroups ARRAY SIZE 1 1\n") +
	        Object("CreateRuleAnAtom", "Create1",
	               "PROPERTY Group INTEGER 1\nPROPERTY PCreator PERSIS_PNTR Sprite0\nPROPERTY OffsetY FLOAT 2\n"),
	    {50.0f, 0.0f, 0.0f});
	effect->Step(k_Step);
	effect->Step(k_Step);
	std::vector<Effect::DrawAtom> atoms;
	effect->Collect(1.0f, atoms);
	ASSERT_EQ(atoms.size(), 1u);
	EXPECT_NEAR(atoms[0].position.x, 50.0f, k_Epsilon);
	EXPECT_NEAR(atoms[0].position.y, 6.0f, k_Epsilon);
	// Drawn at its scale times its parent's
	EXPECT_NEAR(atoms[0].scale, 6.0f, k_Epsilon);
	EXPECT_EQ(effect->CollectionCount(), 2u);
}

TEST_F(ParticleEffectTest, FloatProvidersFollowTheMagnitude)
{
	auto effect = Make(Header() + Object("ParticleSpriteCreator", "Sprite0", k_Sprite) +
	                       Object("MagnitudeFloatProvider", "Magnitude0",
	                              "PROPERTY ScaleBy FLOAT 0.5\nPROPERTY Minimum FLOAT 0\nPROPERTY Maximum FLOAT 10\n") +
	                       Object("CreateRuleAnAtom", "Create0",
	                              "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Sprite0\n"
	                              "PROPERTY InitScaleFP PERSIS_PNTR Magnitude0\n"),
	                   glm::vec3(0.0f), 4.0f);
	effect->Step(k_Step);
	effect->Step(k_Step);
	std::vector<Effect::DrawAtom> atoms;
	effect->Collect(1.0f, atoms);
	ASSERT_EQ(atoms.size(), 1u);
	// The creator's 2 times 4 * 0.5
	EXPECT_NEAR(atoms[0].scale, 4.0f, k_Epsilon);
}

TEST_F(ParticleEffectTest, ConditionsGateRules)
{
	// The emitter only runs once the effect closes down
	auto effect = Make(Header() + Object("ParticleSpriteCreator", "Sprite0", k_Sprite) +
	                   Object("EventConditionTrueOnCloseDown", "OnClose", "") +
	                   Object("EmitterRuleSimple", "Emitter0",
	                          "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Sprite0\n"
	                          "PROPERTY Condition PERSIS_PNTR OnClose\nPROPERTY EmissionFreq FLOAT 10\n"
	                          "PROPERTY Randomise BOOL 0\n"));
	effect->Step(k_Step);
	effect->Step(k_Step);
	EXPECT_EQ(effect->AtomCount(), 0u);
	effect->CloseDown();
	effect->Step(k_Step);
	EXPECT_EQ(effect->AtomCount(), 1u);
}

TEST_F(ParticleEffectTest, LandingTellsTheMiracle)
{
	world.landHeight = 5.0f;
	auto effect = Make(Header() + Object("ParticleSpriteCreator", "Sprite0", k_Sprite) +
	                   Object("CreateRuleAnAtom", "Create0",
	                          "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Sprite0\nPROPERTY OffsetY FLOAT 8\n") +
	                   Object("UpdateRuleGravity", "Gravity0", "PROPERTY Group INTEGER 0\nPROPERTY Gravity FLOAT 50\n") +
	                   Object("LandscapeCollide", "Land0", "PROPERTY Group INTEGER 0\nPROPERTY SendEvent BOOL 1\n"));
	FakeSpell spell;
	effect->SetSink(&spell);
	ASSERT_EQ(spell.events.size(), 1u);
	EXPECT_EQ(spell.events[0].type, SpellEventInfo::Type::Started);
	for (int i = 0; i < 30 && effect->AtomCount() <= 1; ++i)
	{
		effect->Step(k_Step);
		if (spell.events.size() > 1)
		{
			break;
		}
	}
	ASSERT_EQ(spell.events.size(), 2u);
	EXPECT_EQ(spell.events[1].type, SpellEventInfo::Type::Landed);
	EXPECT_LT(spell.events[1].position.y, 5.0f);
	EXPECT_EQ(effect->AtomCount(), 0u);
}

TEST_F(ParticleEffectTest, UnportedClassesAreNamedAndKeepAMiraclesEffectAlive)
{
	auto effect = Make(Header() + Object("UR_SomethingNew", "New0", "PROPERTY Group INTEGER 0\n"));
	ASSERT_EQ(effect->UnportedClasses().size(), 1u);
	EXPECT_EQ(effect->UnportedClasses()[0], "UR_SomethingNew");
	effect->Step(k_Step);
	// Without a miracle nothing keeps it
	EXPECT_TRUE(effect->Finished());
	FakeSpell spell;
	effect->SetSink(&spell);
	EXPECT_FALSE(effect->Finished());
	effect->CloseDown();
	EXPECT_TRUE(effect->Finished());
}

TEST_F(ParticleEffectTest, SpritesTakeThePlayersColour)
{
	auto effect =
	    Make(Header() + Object("ParticleSpriteCreator", "Sprite0", std::string(k_Sprite) + "PROPERTY UsePlayerColor BOOL 1\n") +
	         Object("CreateRuleAnAtom", "Create0",
	                "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Sprite0\n"
	                "PROPERTY SoundOfCreate SOUND_ACTION SOUND_POP LOOPING 0 ONLYONE 0 SOFTRELEASE 0 "
	                "USESURFACE 0\n"));
	effect->SetPlayer(0);
	effect->Step(k_Step);
	effect->Step(k_Step);
	std::vector<Effect::DrawAtom> atoms;
	effect->Collect(1.0f, atoms);
	ASSERT_EQ(atoms.size(), 1u);
	EXPECT_EQ(atoms[0].rgb, (std::array<uint8_t, 3> {254, 0, 0}));
	// And its creation's sound was asked for
	EXPECT_EQ(world.sounds, (std::vector<std::string> {"SOUND_POP"}));
}

TEST_F(ParticleEffectTest, ItIsWalkedNewestFirstWithRibbonsAfterTheirAtomsAndChildrenLast)
{
	// Group 0 makes three sprites, each carrying a group 1 collection of one sprite; its collection also holds a ribbon
	// of two joints
	auto effect =
	    Make(Header() + Object("ParticleSpriteCreator", "Sprite0", k_Sprite) +
	         Object("ParticleSpriteCreator", "Sprite1", "PROPERTY InitialScale FLOAT 7\n") +
	         Object("ParticleChainCreator", "Chain0", "PROPERTY TextureFileName STRING S_Lightning.raw\n") +
	         Object("EmitterRuleSimple", "Emitter0",
	                "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Sprite0\nPROPERTY EmissionFreq FLOAT 10\n"
	                "PROPERTY Randomise BOOL 0\nPROPERTY Speed FLOAT 0\nPROPERTY MaxTotalAtomsToEmit INTEGER 3\n"
	                "PROPERTY NextGroups ARRAY SIZE 1 1\n") +
	         Object("CreateRuleSphere", "Joints0",
	                "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Chain0\nPROPERTY NumAtoms INTEGER 2\n"
	                "PROPERTY Radius FLOAT 5\n") +
	         Object("CreateRuleAnAtom", "Child0", "PROPERTY Group INTEGER 1\nPROPERTY PCreator PERSIS_PNTR Sprite1\n"));
	for (int i = 0; i < 5; ++i)
	{
		effect->Step(k_Step);
	}
	Effect::DrawWalk walk;
	effect->Walk(1.0f, walk);
	ASSERT_EQ(walk.atoms.size(), 6u);
	ASSERT_EQ(walk.chains.size(), 1u);
	EXPECT_EQ(walk.chains[0].jointCount, 2u);
	ASSERT_EQ(walk.steps.size(), 7u);
	// The three parents, the newest first by age, then the ribbon, then the children
	for (size_t i = 0; i < 3; ++i)
	{
		EXPECT_FALSE(walk.steps[i].chain);
		EXPECT_NEAR(walk.atoms[walk.steps[i].index].scale, 2.0f, k_Epsilon);
	}
	EXPECT_LT(walk.atoms[walk.steps[0].index].age, walk.atoms[walk.steps[1].index].age);
	EXPECT_LT(walk.atoms[walk.steps[1].index].age, walk.atoms[walk.steps[2].index].age);
	EXPECT_TRUE(walk.steps[3].chain);
	for (size_t i = 4; i < 7; ++i)
	{
		EXPECT_NEAR(walk.atoms[walk.steps[i].index].scale, 7.0f, k_Epsilon);
	}
	// The children follow their parents' order: the newest parent's first
	EXPECT_LT(walk.atoms[walk.steps[4].index].age, walk.atoms[walk.steps[6].index].age);
}

TEST_F(ParticleEffectTest, ABallThisComputersPlayerThrowsIsDrawnInTheHandAtFirst)
{
	class ThrowingSpell final: public SpellSink
	{
	public:
		bool SpellEvent(const SpellEventInfo& /*event*/) override { return true; }
		[[nodiscard]] int PowerUpLevel() const override { return 0; }
		[[nodiscard]] bool IsMyInterfaceCasting() const override { return true; }
		[[nodiscard]] bool IsHumanPlayerCasting() const override { return true; }
	} spell;
	auto effect = Make(Header() + Object("ParticleSpriteCreator", "Sprite0", k_Sprite) +
	                   Object("CreateWithInitialDirection", "Throw0",
	                          "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Sprite0\nPROPERTY NumAtoms INTEGER 1\n"
	                          "PROPERTY PredictStartPos BOOL 1\nPROPERTY PredictFraction FLOAT 1\n"));
	effect->SetSink(&spell);
	const glm::vec3 hand(0.0f, 10.0f, 0.0f);
	effect->SetProcessInfo({.handPosition = hand});
	effect->SetDirection({20.0f, 0.0f, 0.0f});
	// An atom is drawn from its second step
	effect->Step(k_Step);
	effect->Step(k_Step);
	Effect::DrawWalk walk;
	effect->Walk(1.0f, walk);
	ASSERT_EQ(walk.atoms.size(), 1u);
	// It starts a step's throw ahead of the hand, but is drawn back towards it, by all but the share of the step it has
	// lived of two seconds
	const float age = walk.atoms[0].age;
	const glm::vec3 start = hand + glm::vec3(20.0f, 0.0f, 0.0f) * k_Step;
	const glm::vec3 drawn = walk.atoms[0].position;
	const glm::vec3 flown = drawn - (hand - start) * std::clamp(1.0f - age * 0.5f, 0.0f, 1.0f);
	EXPECT_LT(drawn.x, flown.x);
	// Two seconds on, it is drawn where it is
	for (int i = 0; i < 21; ++i)
	{
		effect->Step(k_Step);
	}
	walk.Clear();
	effect->Walk(1.0f, walk);
	ASSERT_EQ(walk.atoms.size(), 1u);
	EXPECT_GE(walk.atoms[0].age, 2.0f);
}

TEST_F(ParticleEffectTest, ABurstFliesOutOnceItsFuseHasBurnt)
{
	auto effect = Make(Header() + Object("ParticleSpriteCreator", "Sprite0", k_Sprite) +
	                   Object("ConstFloatProvider", "Slow", "PROPERTY ConstValue FLOAT 4\n") +
	                   Object("ConstFloatProvider", "Fast", "PROPERTY ConstValue FLOAT 6\n") +
	                   Object("CreateRuleFusedSphericalExplode", "Burst0",
	                          "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Sprite0\nPROPERTY NumAtoms INTEGER 30\n"
	                          "PROPERTY MinSpeed PERSIS_PNTR Slow\nPROPERTY MaxSpeed PERSIS_PNTR Fast\n"
	                          "PROPERTY FuseTime FLOAT 0.25\nPROPERTY ScaleYSpeed FLOAT 2\nPROPERTY OnlyHemisphere BOOL 1\n"));
	effect->Step(k_Step);
	effect->Step(k_Step);
	EXPECT_EQ(effect->AtomCount(), 0u);
	effect->Step(k_Step);
	effect->Step(k_Step);
	ASSERT_EQ(effect->AtomCount(), 30u);
	effect->Step(k_Step);
	// Only once, all upwards, at one speed between the two with the height doubled
	EXPECT_EQ(effect->AtomCount(), 30u);
}

TEST_F(ParticleEffectTest, AHealChakraFollowsEachTargetAndLightsIt)
{
	constexpr auto k_Villager = static_cast<entt::entity>(5);
	constexpr auto k_Gone = static_cast<entt::entity>(6);
	world.targets = {{k_Villager, {10.0f, 0.0f, 20.0f}}};
	FakeSpell spell;
	auto effect = Make(
	    Header() + Object("ParticlePointCreator", "Point0", "") + Object("ParticleSpriteCreator", "Sprite0", k_Sprite) +
	    Object(
	        "UR_HealSpellChakra", "Heal0",
	        "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Point0\nPROPERTY NextGroups ARRAY SIZE 1 1\n"
	        "PROPERTY TakeCentrePos BOOL 1\nPROPERTY ScalePropObjectSize BOOL 1\nPROPERTY MaxAlpha FLOAT 200\n"
	        "PROPERTY AtomAgeMaxAlpha FLOAT 0.5\nPROPERTY AtomAgeZeroAlpha FLOAT 1.5\nPROPERTY SpecularColorR INTEGER 100\n") +
	    Object("CreateRuleSphere", "Burst0",
	           "PROPERTY Group INTEGER 1\nPROPERTY PCreator PERSIS_PNTR Sprite0\nPROPERTY NumAtoms INTEGER 4\n"));
	effect->SetSink(&spell);
	effect->AddTarget(k_Gone);
	effect->AddTarget(k_Villager);
	effect->Step(k_Step);
	// The one still there is chakraed and healed, at its centre and its size
	EXPECT_TRUE(world.claimed.contains(k_Villager));
	ASSERT_EQ(spell.events.size(), 2u);
	EXPECT_EQ(spell.events[1].type, SpellEventInfo::Type::Object);
	EXPECT_EQ(spell.events[1].target, k_Villager);
	EXPECT_EQ(effect->TargetCount(), 0u);
	for (int i = 0; i < 5; ++i)
	{
		effect->Step(k_Step);
	}
	// Rising to full at half a second of the burst, lighting its target
	ASSERT_TRUE(world.glows.contains(k_Villager));
	EXPECT_GT(world.glows[k_Villager].r, 80);
	// It moves with its target
	world.targets[0].second = {30.0f, 0.0f, 20.0f};
	effect->Step(k_Step);
	std::vector<Effect::DrawAtom> sparks;
	effect->Collect(1.0f, sparks);
	ASSERT_EQ(sparks.size(), 4u);
	// Gone: the chakra ends, lets the target go, and the effect waits for more
	world.targets.clear();
	effect->Step(k_Step);
	EXPECT_FALSE(world.claimed.contains(k_Villager));
	EXPECT_EQ(world.glows[k_Villager], glm::u8vec3(0));
	EXPECT_FALSE(effect->Finished());
	effect->CloseDown();
	for (int i = 0; i < 3; ++i)
	{
		effect->Step(k_Step);
	}
}

TEST_F(ParticleEffectTest, DrawnCreatorsMakeTheirAtoms)
{
	auto effect =
	    Make(Header() + Object("ParticleMistCreator", "Mist0", "PROPERTY Ratio FLOAT 3\nPROPERTY InitialScale FLOAT 4\n") +
	         Object("ParticleSymbolSpriteCreator", "Symbol0", "PROPERTY InitialScale FLOAT 2\n") +
	         Object("ParticleLightMapCreator", "Light0",
	                "PROPERTY NumFramesInFile INTEGER 4\n"
	                "PROPERTY NumFramesInUse INTEGER 2\n") +
	         Object("CreateRuleAnAtom", "A", "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Mist0\n") +
	         Object("CreateRuleAnAtom", "B", "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Symbol0\n") +
	         Object("CreateRuleAnAtom", "C", "PROPERTY Group INTEGER 0\nPROPERTY PCreator PERSIS_PNTR Light0\n"));
	effect->Step(k_Step);
	effect->Step(k_Step);
	EXPECT_TRUE(effect->UnportedClasses().empty());
	Effect::DrawWalk walk;
	effect->Walk(1.0f, walk);
	ASSERT_EQ(walk.atoms.size(), 3u);
	const auto find = [&walk](Creator::Kind kind) {
		return *std::ranges::find_if(walk.atoms, [kind](const auto& atom) { return atom.creator->kind == kind; });
	};
	const auto mist = find(Creator::Kind::Mist);
	EXPECT_NEAR(mist.scale, 4.0f, k_Epsilon);
	EXPECT_NEAR(mist.creatorValue.x, 3.0f, k_Epsilon);
	EXPECT_GE(mist.creatorValue.y, 0.0f);
	EXPECT_LT(mist.creatorValue.y, 16.0f);
	EXPECT_NEAR(find(Creator::Kind::Symbol).scale, 2.0f, k_Epsilon);
	// Without resources the light map has no bitmap, so it stamps nothing
	EXPECT_EQ(static_cast<const LightMapCreator*>(find(Creator::Kind::LightMap).creator)->numFrames, 2);
	draw::Frame frame;
	draw::AddEffect(frame, walk, draw::DrawPath::Sorted, glm::vec3(0.0f), -1,
	                {.textures = [](std::string_view) { return std::optional(std::pair<entt::id_type, entt::id_type>(1, 2)); },
	                 .playerColour = [](int) { return 0x00FF00u; },
	                 .random = {}});
	// The mist, and the symbol's three sprites
	EXPECT_EQ(frame.mists.size(), 1u);
	EXPECT_EQ(frame.sprites.size(), 3u);
	EXPECT_TRUE(frame.lightStamps.empty());
	ASSERT_EQ(frame.groups.size(), 1u);
	EXPECT_EQ(frame.groups[0].itemCount, 4u);
}

TEST_F(ParticleEffectTest, RandomNumbersAreDrawnOnlyInASteps)
{
	auto effect = Make(Header());
	EXPECT_FLOAT_EQ(effect->Random(10.0f), 0.0f);
	EXPECT_EQ(random.GetParticleStream(), ParticleRandomStream::None);
}

TEST_F(ParticleEffectTest, RunsEveryFileOfTheGame)
{
	const char* gamePath = std::getenv("OPENBLACK_GAME_PATH");
	if (gamePath == nullptr)
	{
		GTEST_SKIP() << "OPENBLACK_GAME_PATH is not set";
	}
	const auto directory = std::filesystem::path(gamePath) / "Data" / "Spells" / "ZSpellFiles";
	if (!std::filesystem::is_directory(directory))
	{
		GTEST_SKIP() << "No particle files in the game folder";
	}
	size_t count = 0;
	for (const auto& entry : std::filesystem::directory_iterator(directory))
	{
		const auto name = entry.path().filename().string();
		if (!name.ends_with("_txt.zzz"))
		{
			continue;
		}
		std::ifstream stream(entry.path(), std::ios::binary);
		const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
		const auto compressed = psys::SplitCompressed(bytes);
		ASSERT_TRUE(compressed.has_value()) << name;
		const auto text =
		    zip::Inflate(std::vector<uint8_t>(compressed->deflated.begin(), compressed->deflated.end()), compressed->textSize);
		auto effect = Make(std::string(text.begin(), text.end()), glm::vec3(0.0f), 2.0f);
		// Ten seconds of turns, then closing down for another ten
		size_t most = 0;
		size_t sprites = 0;
		size_t drawn = 0;
		Effect::DrawWalk walk;
		draw::Frame frame;
		const draw::Sources sources {
		    .textures = [](std::string_view) { return std::optional(std::pair<entt::id_type, entt::id_type>(1, 2)); },
		    .playerColour = [](int) { return 0xFFFFFFu; },
		    .random = [](float) { return 0.0f; },
		};
		for (int step = 0; step < 200; ++step)
		{
			if (step == 100)
			{
				effect->CloseDown();
			}
			effect->Step(k_Step);
			most = std::max(most, effect->AtomCount());
			std::vector<Effect::DrawAtom> atoms;
			effect->Collect(1.0f, atoms);
			sprites = std::max(sprites, atoms.size());
			walk.Clear();
			effect->Walk(1.0f, walk);
			frame.Clear();
			draw::AddEffect(frame, walk, draw::DrawPath::Sorted, glm::vec3(0.0f), 0, sources);
			drawn = std::max(drawn, frame.items.size());
			if (effect->Finished())
			{
				break;
			}
		}
		// The spot visuals' smoke, steam and bonfire run entirely on the classes ported so far
		if (name == "SF_Smoke_txt.zzz" || name == "SF_Steam_txt.zzz" || name == "SF_Bonfire_txt.zzz")
		{
			EXPECT_TRUE(effect->UnportedClasses().empty()) << name;
			EXPECT_GT(sprites, 10u) << name;
		}
		// The player icon fountain's symbols and the heal chakra's sparks are run in full now
		if (name == "SF_PlayerIconFountain_txt.zzz" || name == "SF_HealChakra_txt.zzz")
		{
			EXPECT_TRUE(effect->UnportedClasses().empty()) << name;
		}
		if (name == "SF_PlayerIconFountain_txt.zzz")
		{
			EXPECT_GT(drawn, 10u) << name;
		}
		++count;
	}
	EXPECT_GT(count, 100u);
}

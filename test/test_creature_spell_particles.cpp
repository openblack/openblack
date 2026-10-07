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
#include "Creature/CreatureRig.h"
#include "Particles/CreatureSpellMaths.h"
#include "Particles/ParticleClassRegistry.h"
#include "Particles/ParticleEffect.h"

using namespace openblack;
using namespace openblack::particles;

namespace
{
constexpr float k_Step = 0.1f;
constexpr float k_Epsilon = 1e-3f;
const auto k_Creature = static_cast<entt::entity>(7);

/// Flat land with one creature standing on it, its bones a box, and the sounds started
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
	[[nodiscard]] std::optional<CreatureSpellBody> CreatureBody(entt::entity creature) const override
	{
		if (creature != k_Creature || gone)
		{
			return std::nullopt;
		}
		return body;
	}

	CreatureSpellBody body {
	    .origin = {100.0f, 0.0f, 100.0f},
	    .size = 2.0f,
	    // The feet, the eyes and the top of the head
	    .bones = {{98.0f, 0.0f, 99.0f},
	              {102.0f, 0.0f, 99.0f},
	              {99.0f, 20.0f, 97.0f},
	              {101.0f, 20.0f, 97.0f},
	              {100.0f, 24.0f, 101.0f}},
	    .mirror = {1, 0, 3, 2, 4},
	    .rightFoot = 0,
	    .rightEye = 2,
	};
	bool gone {false};
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

/// A creature spell, cast by a player's hand unless said otherwise
class FakeSpell final: public SpellSink
{
public:
	bool SpellEvent(const SpellEventInfo& /*event*/) override { return true; }
	[[nodiscard]] int PowerUpLevel() const override { return -1; }
	[[nodiscard]] bool IsCreatureCasting() const override { return creatureCasting; }
	[[nodiscard]] bool IsScriptCasting() const override { return scriptCasting; }
	[[nodiscard]] int CreatureSpellKind() const override { return kind; }

	bool creatureCasting {false};
	bool scriptCasting {false};
	int kind {1};
};

std::string Header()
{
	return "BEGINPROPERTIES\nPROPERTY DeleteOnCloseDown BOOL 0\nPROPERTY Hierarchies ARRAY SIZE 6 0 0 0 0 0 0\n"
	       "PROPERTY InitiallyCreated ARRAY SIZE 6 1 0 0 0 0 0\nPROPERTY MaxSpellAge FLOAT -1\nENDPROPERTIES\n";
}

std::string Object(std::string_view className, std::string_view name, std::string_view properties)
{
	return "BEGINCLASS " + std::string(className) + " " + std::string(name) + "\nBEGINPROPERTIES\n" + std::string(properties) +
	       "ENDPROPERTIES\nENDCLASS\n";
}

/// The creature spell's first rule, with the given groups under its atom
std::string First(std::string_view nextGroups)
{
	return Object("UR_CreatureSpell", "UR_CreatureSpell0",
	              "PROPERTY Group INTEGER 0\nPROPERTY NextGroups ARRAY SIZE " + std::string(nextGroups) +
	                  "\nPROPERTY PCreator PERSIS_PNTR NULL_STRING\n"
	                  "PROPERTY SoundCreatureSpell SOUND_ACTION SOUND_SPELL_CREATURE_WHISPS LOOPING 1 ONLYONE 0 "
	                  "SOFTRELEASE 1 USESURFACE 0\n"
	                  "PROPERTY SoundCreatureSpellCast SOUND_ACTION SOUND_SPELL_CREATURE_SPELL_CAST LOOPING 0 ONLYONE 0 "
	                  "SOFTRELEASE 1 USESURFACE 0\n");
}

const std::string k_Point = Object("ParticleSpriteCreator", "Point",
                                   "PROPERTY ColorA INTEGER 50\nPROPERTY InitialScale FLOAT 1\nPROPERTY NumFrames INTEGER 1\n"
                                   "PROPERTY TextureFileName STRING .\\Data\\Textures\\S_SpriteSheet3.raw\n");

const std::string k_Wisps = Object("UR_CreatureSpellGeneric", "Wisps",
                                   "PROPERTY DelayBeforeEmit FLOAT 1\nPROPERTY Group INTEGER 1\nPROPERTY NumAtoms INTEGER 3\n"
                                   "PROPERTY PCreator PERSIS_PNTR Point\nPROPERTY PhiSpeed FLOAT 1.5\n"
                                   "PROPERTY TaperFrac FLOAT 0.5\nPROPERTY ThetaSpeed FLOAT 3.5\n");

class CreatureSpellParticles: public ::testing::Test
{
protected:
	std::unique_ptr<Effect> Make(const std::string& body)
	{
		auto file = psys::ParticleFile::Parse(Header() + body);
		EXPECT_TRUE(file.has_value());
		auto effect = std::make_unique<Effect>(std::make_shared<const psys::ParticleFile>(std::move(*file)),
		                                       EffectServices {classes, world, random, noise}, world.body.origin, 1.0f, false);
		effect->SetSink(&spell);
		effect->SetPlayer(0);
		ProcessInfo info;
		info.handPosition = k_Hand;
		effect->SetProcessInfo(info);
		effect->AddTarget(k_Creature);
		return effect;
	}

	/// The atoms of a kind of creator, as drawn after the last step
	static std::vector<Effect::DrawAtom> Atoms(const Effect& effect, Creator::Kind kind)
	{
		std::vector<Effect::DrawAtom> atoms;
		effect.Collect(1.0f, atoms, kind);
		return atoms;
	}

	static constexpr glm::vec3 k_Hand {60.0f, 40.0f, 60.0f};
	ParticleClassRegistry classes {ParticleClassRegistry::WithAllClasses()};
	FakeWorld world;
	FakeRandom random;
	maths::ValueNoise noise;
	FakeSpell spell;
};
} // namespace

TEST(CreatureSpellMaths, WispsWindRoundTheBoxOfTheBones)
{
	const std::vector<glm::vec3> bones {{-2.0f, 0.0f, -1.0f}, {2.0f, 10.0f, 2.0f}};
	const auto box = maths::WispBoxOf(bones);
	EXPECT_FLOAT_EQ(box.centreX, 0.0f);
	EXPECT_FLOAT_EQ(box.centreZ, 0.5f);
	EXPECT_FLOAT_EQ(box.bottom, 0.0f);
	EXPECT_FLOAT_EQ(box.height, 10.0f);
	EXPECT_FLOAT_EQ(box.radius, 2.5f);
	// At the top, narrowed by the taper; with no wobble yet
	const auto top = maths::WispOrbit(box, 0.0f, 0.0f, 0.5f, 0.0f);
	EXPECT_NEAR(top.y, 12.0f, k_Epsilon);
	EXPECT_NEAR(top.x, (1.0f - 0.5f * std::pow(0.3f * 3.3333333f, 2.0f)) * 2.5f * 1.2f, k_Epsilon);
	// At the bottom, full width
	const auto bottom = maths::WispOrbit(box, 0.0f, std::numbers::pi_v<float>, 0.5f, 0.0f);
	EXPECT_NEAR(bottom.y, 0.0f, k_Epsilon);
	EXPECT_NEAR(bottom.x, 3.0f, k_Epsilon);
}

TEST(CreatureSpellMaths, WispsFlyFromTheHandOverTwoSecondsAndFadeIn)
{
	const glm::vec3 hand(0.0f);
	const glm::vec3 orbit(10.0f, 0.0f, 0.0f);
	EXPECT_FLOAT_EQ(maths::WispPosition(hand, orbit, 0.0f).x, 0.0f);
	EXPECT_FLOAT_EQ(maths::WispPosition(hand, orbit, 1.0f).x, 5.0f);
	EXPECT_FLOAT_EQ(maths::WispPosition(hand, orbit, 3.0f).x, 10.0f);
	EXPECT_EQ(maths::WispAlpha(0.0f), 20);
	EXPECT_EQ(maths::WispAlpha(0.25f), 35);
	EXPECT_EQ(maths::WispAlpha(2.0f), 50);
	// Three over two seconds, no more
	EXPECT_FLOAT_EQ(maths::WispsDue(0.0f, 3, 0.1f, 2.0f), 0.15f);
	EXPECT_FLOAT_EQ(maths::WispsDue(2.95f, 3, 0.1f, 2.0f), 3.0f);
}

TEST(CreatureSpellMaths, TheFliesCircleTheHeadHalfAsWideAgainAsTheEyes)
{
	const glm::vec3 right(-1.0f, 10.0f, 0.0f);
	const glm::vec3 left(1.0f, 10.0f, 0.0f);
	const auto start = maths::ItchOrbit(right, left, 0.0f, 1.0f);
	EXPECT_NEAR(start.x, -1.5f, k_Epsilon);
	const auto quarter = maths::ItchOrbit(right, left, std::numbers::pi_v<float> / 2.0f, 1.0f);
	EXPECT_NEAR(glm::length(quarter - glm::vec3(0.0f, 10.0f, 0.0f)), 1.5f, k_Epsilon);
	EXPECT_NEAR(quarter.y, 10.0f, k_Epsilon);
}

TEST(CreatureRigMirror, TheMirrorTableEndsTheFile)
{
	creature::CreatureRig rig;
	rig.fileTail = {9, 9, 0, 0, 2, 1, 3};
	rig.creatureVersion = 21;
	const auto mirror = rig.MirrorBones(4);
	ASSERT_TRUE(mirror.has_value());
	EXPECT_EQ(*mirror, (std::vector<uint32_t> {0, 2, 1, 3}));
	// Too many bones for the file, or a bone out of range: none
	EXPECT_FALSE(rig.MirrorBones(8).has_value());
	EXPECT_FALSE(rig.MirrorBones(6).has_value());
	// A version 20 file's first entry belongs to the second bone
	rig.fileTail = {1, 0, 2};
	rig.creatureVersion = 20;
	EXPECT_EQ(*rig.MirrorBones(3), (std::vector<uint32_t> {0, 1, 2}));
}

TEST_F(CreatureSpellParticles, AnAtomStaysAtTheCreaturesFeetAndHumsWithTheCastingSound)
{
	auto effect = Make(First("0") + k_Point);
	effect->Step(k_Step);
	ASSERT_EQ(world.sounds.size(), 2u);
	EXPECT_EQ(world.sounds[0]->sound.action.sound, "SOUND_SPELL_CREATURE_WHISPS");
	EXPECT_EQ(world.sounds[1]->sound.action.sound, "SOUND_SPELL_CREATURE_SPELL_CAST");
	world.body.origin = {110.0f, 0.0f, 100.0f};
	effect->Step(k_Step);
	ASSERT_EQ(effect->AtomCount(), 1u);
	EXPECT_NEAR(world.sounds[0]->atom->position.x, 110.0f, k_Epsilon);
}

TEST_F(CreatureSpellParticles, ACreaturesCastSoundsCastByACreatureAndAScriptsMakesNone)
{
	spell.creatureCasting = true;
	auto byCreature = Make(First("0"));
	byCreature->Step(k_Step);
	ASSERT_EQ(world.sounds.size(), 2u);
	EXPECT_EQ(world.sounds[1]->sound.action.sound, "SOUND_SPELL_CREATURE_SPELL_CAST_BY_OTHER_CREATURE");
	world.sounds.clear();
	spell.scriptCasting = true;
	auto byScript = Make(First("0"));
	byScript->Step(k_Step);
	EXPECT_EQ(world.sounds.size(), 1u);
}

TEST_F(CreatureSpellParticles, WispsComeAfterASecondOneByOneFromTheHand)
{
	auto effect = Make(First("1 1") + k_Wisps + k_Point);
	const auto count = [&effect]() { return effect->AtomCount() - 1; };
	for (int i = 0; i < 10; ++i)
	{
		effect->Step(k_Step);
	}
	EXPECT_EQ(count(), 0u);
	for (int i = 0; i < 3; ++i)
	{
		effect->Step(k_Step);
	}
	EXPECT_EQ(count(), 1u);
	for (int i = 0; i < 20; ++i)
	{
		effect->Step(k_Step);
	}
	EXPECT_EQ(count(), 3u);
}

TEST_F(CreatureSpellParticles, AnInvisibleCreaturesWispsFadeWithIt)
{
	spell.kind = 7;
	world.body.invisible = 0.5f;
	auto effect = Make(First("1 1") + k_Wisps + k_Point);
	for (int i = 0; i < 15; ++i)
	{
		effect->Step(k_Step);
	}
	const auto atoms = Atoms(*effect, Creator::Kind::Sprite);
	ASSERT_FALSE(atoms.empty());
	// The collection at half, the wisp itself still fading in
	for (const auto& atom : atoms)
	{
		EXPECT_LT(atom.alpha, 50.0f * 0.5f + 1.0f);
	}
}

TEST_F(CreatureSpellParticles, TheFliesWaitAtTheHandThenCircleTheHead)
{
	const auto itch = Object("UR_CreatureSpellItch", "Itch",
	                         "PROPERTY Group INTEGER 4\nPROPERTY NumAtoms INTEGER 1\nPROPERTY PCreator PERSIS_PNTR Point\n"
	                         "PROPERTY PauseBeforeGotoCreature FLOAT 0.2\n");
	auto effect = Make(First("1 4") + itch + k_Point);
	// A new atom is drawn from its second step
	effect->Step(k_Step);
	effect->Step(k_Step);
	auto atoms = Atoms(*effect, Creator::Kind::Sprite);
	ASSERT_EQ(atoms.size(), 1u);
	EXPECT_NEAR(glm::distance(atoms[0].position, k_Hand), 0.0f, k_Epsilon);
	for (int i = 0; i < 5; ++i)
	{
		effect->Step(k_Step);
	}
	atoms = Atoms(*effect, Creator::Kind::Sprite);
	ASSERT_EQ(atoms.size(), 1u);
	// Round the middle of the eyes, half as far again as each eye is from it
	const glm::vec3 middle(100.0f, 20.0f, 97.0f);
	EXPECT_NEAR(glm::distance(atoms[0].position, middle), 1.5f, k_Epsilon);
}

TEST_F(CreatureSpellParticles, HeartsRiseFromAllOverANiceCreatureFourASecond)
{
	const auto hearts = Object("UR_CreatureSpellCompassion", "Hearts",
	                           "PROPERTY DieAge FLOAT 5\nPROPERTY Group INTEGER 4\nPROPERTY InitSpeed FLOAT 6\n"
	                           "PROPERTY MaxAtoms INTEGER 20\nPROPERTY PCreator PERSIS_PNTR Point\n");
	auto effect = Make(First("1 4") + hearts + k_Point);
	for (int i = 0; i < 10; ++i)
	{
		effect->Step(k_Step);
	}
	EXPECT_NEAR(static_cast<float>(effect->AtomCount() - 1), 4.0f, 1.0f);
	for (const auto& atom : Atoms(*effect, Creator::Kind::Sprite))
	{
		EXPECT_FLOAT_EQ(atom.scale, 2.0f);
	}
	for (int i = 0; i < 100; ++i)
	{
		effect->Step(k_Step);
	}
	EXPECT_LE(effect->AtomCount() - 1, 20u);
}

TEST(CreatureSpellMaths, ACreaturesCastGivesUpOnceItSwingsTooFar)
{
	// The game takes 40 as radians: a cosine of about -0.667, so only a swing past about 132 degrees ends it
	EXPECT_FALSE(maths::SwungTooFar({1.0f, 0.0f}, {0.0f, 1.0f}, 40.0f));
	EXPECT_TRUE(maths::SwungTooFar({1.0f, 0.0f}, {-1.0f, 0.1f}, 40.0f));
	EXPECT_FALSE(maths::SwungTooFar({0.0f, 0.0f}, {-1.0f, 0.0f}, 40.0f));
}

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
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Common/GameRandom.h"
#include "Particles/BeamMaths.h"
#include "Particles/ParticleClassRegistry.h"
#include "Particles/ParticleCreators.h"
#include "Particles/ParticleEffect.h"

using namespace openblack;
using namespace openblack::particles;

namespace
{
constexpr float k_Epsilon = 1e-4f;
constexpr float k_Step = 0.1f;

const auto k_FlatLand = [](glm::vec2 /*xz*/) { return 0.0f; };

/// Flat land at a height, and objects standing where they are put
class FakeWorld final: public ParticleWorldInterface
{
public:
	[[nodiscard]] float LandHeight(glm::vec2 /*xz*/) const override { return landHeight; }
	[[nodiscard]] uint32_t PlayerColour(int /*player*/) const override { return 0xFF0000u; }
	[[nodiscard]] glm::vec3 CameraRight() const override { return {1.0f, 0.0f, 0.0f}; }
	[[nodiscard]] glm::vec3 CameraUp() const override { return {0.0f, 1.0f, 0.0f}; }
	[[nodiscard]] std::optional<TargetInfo> Target(entt::entity target, bool centre) const override
	{
		for (const auto& [entity, position] : targets)
		{
			if (entity == target)
			{
				return TargetInfo {
				    .position = position + glm::vec3(0.0f, centre ? 1.0f : 0.0f, 0.0f), .radius = 1.0f, .height = 2.0f};
			}
		}
		return std::nullopt;
	}

	float landHeight {0.0f};
	std::vector<std::pair<entt::entity, glm::vec3>> targets;
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

/// The simple beam's file as the game has it for a creature's casting, with the minimum height given
std::string BeamFile(float minHeight = -4.31602e+08f)
{
	return "BEGINPROPERTIES\nPROPERTY DeleteOnCloseDown BOOL 0\nPROPERTY Hierarchies ARRAY SIZE 2 0 0\n"
	       "PROPERTY InitiallyCreated ARRAY SIZE 2 1 0\nPROPERTY MaxSpellAge FLOAT -1\nENDPROPERTIES\n"
	       "BEGINCLASS UR_SimpleBeam UR_SimpleBeam0\nBEGINPROPERTIES\n"
	       "PROPERTY Condition PERSIS_PNTR NULL_STRING\nPROPERTY BeamGroup INTEGER 1\nPROPERTY ForkScaleMax FLOAT 0.8\n"
	       "PROPERTY ForkScaleMin FLOAT 0.1\nPROPERTY Group INTEGER 0\nPROPERTY MaxJointsPerFork INTEGER 20\n"
	       "PROPERTY MinHeight FLOAT " +
	       std::to_string(minHeight) +
	       "\nPROPERTY NextGroups ARRAY SIZE 0\nPROPERTY NumBeams INTEGER 3\nPROPERTY NumSplinePoints INTEGER 6\n"
	       "PROPERTY PCreator PERSIS_PNTR ParticleChainCreator0\nPROPERTY RandomFrac FLOAT 3\n"
	       "PROPERTY RemoveOnCloseDown BOOL 0\nPROPERTY SpeedV FLOAT 1\nPROPERTY WiggleFreq FLOAT 3\n"
	       "PROPERTY WiggleSpeed FLOAT -4\nENDPROPERTIES\nENDCLASS\n"
	       "BEGINCLASS ParticleChainCreator ParticleChainCreator0\nBEGINPROPERTIES\n"
	       "PROPERTY ColorA INTEGER 33\nPROPERTY FrameHeight INTEGER 256\nPROPERTY FrameWidth INTEGER 64\n"
	       "PROPERTY InitialScale FLOAT 1\nPROPERTY NumTexturesForWholeChain INTEGER 4\n"
	       "PROPERTY TextureFileName STRING .\\Data\\Textures\\S_Beam.raw\nENDPROPERTIES\nENDCLASS\n";
}

class ParticleBeamTest: public ::testing::Test
{
protected:
	std::unique_ptr<Effect> Make(const std::string& text, glm::vec3 origin, float magnitude = 1.0f)
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

TEST(BeamMaths, TheBulgeIsNoneAtTheEndsAndAllAtTheMiddle)
{
	EXPECT_FLOAT_EQ(maths::BeamBulge(0.0f), 0.0f);
	EXPECT_FLOAT_EQ(maths::BeamBulge(1.0f), 0.0f);
	EXPECT_FLOAT_EQ(maths::BeamBulge(0.5f), 1.0f);
	EXPECT_FLOAT_EQ(maths::BeamBulge(0.25f), 0.75f);
}

TEST(BeamMaths, KeyPointsRunStraightWithoutWiggle)
{
	const maths::BeamWiggle still {.frequency = 3.0f, .speed = -4.0f, .amount = 0.0f, .minHeight = -1000.0f};
	const auto keys = maths::BeamKeyPoints(
	    {0.0f, 10.0f, 0.0f}, {10.0f, 0.0f, 20.0f}, 6, still, 1.0f, 0, [](float) { return 0.5f; }, k_FlatLand);
	ASSERT_EQ(keys.size(), 6u);
	for (size_t i = 0; i < keys.size(); ++i)
	{
		const float t = static_cast<float>(i) / 5.0f;
		EXPECT_NEAR(keys[i].x, 10.0f * t, k_Epsilon);
		EXPECT_NEAR(keys[i].y, 10.0f - (10.0f * t), k_Epsilon);
		EXPECT_NEAR(keys[i].z, 20.0f * t, k_Epsilon);
	}
}

TEST(BeamMaths, KeyPointsBetweenTheEndsArePushedByTheNoise)
{
	const maths::BeamWiggle wiggle {.frequency = 3.0f, .speed = -4.0f, .amount = 3.0f, .minHeight = -1000.0f};
	std::vector<float> asked;
	const auto noise = [&asked](float x) {
		asked.push_back(x);
		return 0.5f;
	};
	const glm::vec3 start(0.0f, 50.0f, 0.0f);
	const glm::vec3 end(0.0f, 50.0f, 50.0f);
	const float age = 0.3f;
	const int beam = 2;
	const auto keys = maths::BeamKeyPoints(start, end, 6, wiggle, age, beam, noise, k_FlatLand);
	ASSERT_EQ(keys.size(), 6u);
	// The ends are left where they are
	EXPECT_EQ(keys.front(), start);
	EXPECT_NEAR(glm::distance(keys.back(), end), 0.0f, k_Epsilon);
	// Each point between asks the noise across, along and up, drifting at its own share of the speed
	ASSERT_EQ(asked.size(), 12u);
	for (int i = 1; i < 5; ++i)
	{
		const float t = static_cast<float>(i) / 5.0f;
		const float bulge = 1.0f - ((2.0f * t - 1.0f) * (2.0f * t - 1.0f));
		const auto k = static_cast<size_t>(i - 1) * 3;
		EXPECT_NEAR(asked[k], (age * -4.0f) + (t * 3.0f) + 2.0f, k_Epsilon);
		EXPECT_NEAR(asked[k + 1], (age * -4.0f * 0.7f) + (t * 3.0f) + 2.0f, k_Epsilon);
		EXPECT_NEAR(asked[k + 2], (age * -4.0f * 1.3f) + (t * 3.0f) + 2.0f, k_Epsilon);
		const auto& key = keys[static_cast<size_t>(i)];
		EXPECT_NEAR(key.x, 0.5f * 3.0f * bulge, k_Epsilon);
		EXPECT_NEAR(key.z, (50.0f * t) + (0.5f * 3.0f * bulge), k_Epsilon);
		// Upwards only, half as far
		EXPECT_NEAR(key.y, 50.0f + ((0.5f + 1.0f) * 3.0f * bulge * 0.5f), k_Epsilon);
	}
}

TEST(BeamMaths, KeyPointsKeepAboveTheLand)
{
	const maths::BeamWiggle wiggle {.frequency = 3.0f, .speed = 1.0f, .amount = 1.0f, .minHeight = 2.0f};
	const auto hill = [](glm::vec2 /*xz*/) { return 5.0f; };
	// Pushed as far down as the noise goes, the points between would be under the hill
	const auto keys = maths::BeamKeyPoints(
	    {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 30.0f}, 4, wiggle, 0.0f, 0, [](float) { return -1.0f; }, hill);
	ASSERT_EQ(keys.size(), 4u);
	EXPECT_FLOAT_EQ(keys[1].y, 7.0f);
	EXPECT_FLOAT_EQ(keys[2].y, 7.0f);
	// But not the ends
	EXPECT_FLOAT_EQ(keys.front().y, 1.0f);
	EXPECT_FLOAT_EQ(keys.back().y, 1.0f);
}

TEST(BeamMaths, JointsFollowACurveThroughTheKeyPointsThickestAtTheMiddle)
{
	const std::vector<glm::vec3> keys {
	    {0.0f, 0.0f, 0.0f},  {1.0f, 2.0f, 10.0f}, {-1.0f, 3.0f, 20.0f},
	    {2.0f, 1.0f, 30.0f}, {0.0f, 0.0f, 40.0f}, {0.0f, 0.0f, 50.0f},
	};
	// Eleven joints put every other one on a key point
	const auto joints = maths::BeamJoints(keys, 11, 0.1f, 0.8f);
	ASSERT_EQ(joints.size(), 11u);
	for (size_t i = 0; i < keys.size(); ++i)
	{
		EXPECT_NEAR(glm::distance(joints[2 * i].position, keys[i]), 0.0f, 1e-3f) << i;
	}
	EXPECT_NEAR(joints.front().scale, 0.1f, k_Epsilon);
	EXPECT_NEAR(joints.back().scale, 0.1f, k_Epsilon);
	EXPECT_NEAR(joints[5].scale, 0.8f, k_Epsilon);
	// A tenth of the way: 1 - 0.8^2 of the way from the smallest to the largest
	EXPECT_NEAR(joints[1].scale, 0.1f + (0.36f * 0.7f), k_Epsilon);
}

TEST(BeamMaths, TheCurveLeavesAndArrivesFlat)
{
	// Evenly along a straight line the curve still bends at the ends, where it leaves and arrives flat
	const std::vector<glm::vec3> keys {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 2.0f}, {0.0f, 0.0f, 3.0f}};
	const auto joints = maths::BeamJoints(keys, 31, 1.0f, 1.0f);
	ASSERT_EQ(joints.size(), 31u);
	const float first = joints[1].position.z - joints[0].position.z;
	const float middle = joints[16].position.z - joints[15].position.z;
	EXPECT_LT(first, middle * 0.5f);
	EXPECT_NEAR(joints[15].position.z, 1.5f, k_Epsilon);
}

TEST(ParticleDraw, RibbonTextureSlidesAndWrapsRoundAtOneRepeat)
{
	ChainCreator creator;
	creator.frameWidth = 64;
	creator.frameHeight = 128;
	creator.texturesForWholeChain = 1;
	const auto still = creator.SegmentUv(1, 4);
	const auto slid = creator.SegmentUv(1, 4, -1, 0.25f);
	EXPECT_FLOAT_EQ(slid[0].y, still[0].y + 0.25f);
	EXPECT_FLOAT_EQ(slid[3].y, still[3].y + 0.25f);
	// One repeat is half a sheet: 0.75 wraps round to 0.25, and backwards slides stay forwards of the frame's start
	EXPECT_FLOAT_EQ(creator.SegmentUv(1, 4, -1, 0.75f)[0].y, slid[0].y);
	EXPECT_FLOAT_EQ(creator.SegmentUv(1, 4, -1, -0.25f)[0].y, slid[0].y);
}

TEST_F(ParticleBeamTest, EachTargetPositionGetsThreeBeamsFromTheOrigin)
{
	const glm::vec3 origin(100.0f, 30.0f, 100.0f);
	const glm::vec3 target(110.0f, 5.0f, 120.0f);
	auto effect = Make(BeamFile(), origin);
	EXPECT_TRUE(effect->UnportedClasses().empty());
	effect->AddTargetPosition(target);
	for (int i = 0; i < 3; ++i)
	{
		effect->Step(k_Step);
	}
	EXPECT_EQ(effect->TargetPositionCount(), 0u);
	// The beam's own atom and the joints of its three beams
	EXPECT_EQ(effect->AtomCount(), 1u + (3u * 20u));
	Effect::DrawWalk walk;
	effect->Walk(1.0f, walk);
	ASSERT_EQ(walk.chains.size(), 3u);
	for (const auto& chain : walk.chains)
	{
		ASSERT_EQ(chain.jointCount, 20u);
		const auto first = walk.joints[chain.firstJoint];
		const auto last = walk.joints[chain.firstJoint + chain.jointCount - 1];
		// The first joint made ends at the target and the newest starts at the origin, both at the smallest scale
		EXPECT_NEAR(glm::distance(first.position, target), 0.0f, 1e-3f);
		EXPECT_NEAR(glm::distance(last.position, origin), 0.0f, 1e-3f);
		EXPECT_NEAR(first.scale, 0.1f, k_Epsilon);
		EXPECT_NEAR(last.scale, 0.1f, k_Epsilon);
		EXPECT_NEAR(first.alpha, 33.0f, k_Epsilon);
		// Its texture slides at a sheet a second over its life
		EXPECT_NEAR(chain.textureScroll, effect->GetAge(), k_Epsilon);
	}
	// No two beams wiggle alike
	const auto middle = [&walk](size_t chain) { return walk.joints[walk.chains[chain].firstJoint + 10].position; };
	EXPECT_GT(glm::distance(middle(0), middle(1)), k_Epsilon);
	EXPECT_GT(glm::distance(middle(1), middle(2)), k_Epsilon);
}

TEST_F(ParticleBeamTest, TheBeamsFollowTheOriginAndAMovingObject)
{
	auto effect = Make(BeamFile(), {0.0f, 20.0f, 0.0f}, 2.0f);
	const auto object = static_cast<entt::entity>(7);
	world.targets.emplace_back(object, glm::vec3(0.0f, 0.0f, 40.0f));
	effect->AddTarget(object);
	effect->Step(k_Step);
	world.targets[0].second = glm::vec3(5.0f, 0.0f, 40.0f);
	effect->SetOrigin({1.0f, 20.0f, 0.0f});
	effect->Step(k_Step);
	effect->Step(k_Step);
	Effect::DrawWalk walk;
	effect->Walk(1.0f, walk);
	ASSERT_EQ(walk.chains.size(), 3u);
	const auto& chain = walk.chains[0];
	// To the object's middle, from where the origin is now; the effect's magnitude scales the joints
	EXPECT_NEAR(glm::distance(walk.joints[chain.firstJoint].position, glm::vec3(5.0f, 1.0f, 40.0f)), 0.0f, 1e-3f);
	EXPECT_NEAR(glm::distance(walk.joints[chain.firstJoint + 19].position, glm::vec3(1.0f, 20.0f, 0.0f)), 0.0f, 1e-3f);
	EXPECT_NEAR(walk.joints[chain.firstJoint].scale, 0.2f, k_Epsilon);
	// Once the object has gone the beams keep to where it last was
	world.targets.clear();
	effect->Step(k_Step);
	effect->Step(k_Step);
	walk.Clear();
	effect->Walk(1.0f, walk);
	ASSERT_EQ(walk.chains.size(), 3u);
	EXPECT_NEAR(glm::distance(walk.joints[walk.chains[0].firstJoint].position, glm::vec3(5.0f, 1.0f, 40.0f)), 0.0f, 1e-3f);
}

TEST_F(ParticleBeamTest, NoBeamsAreMadeOnceClosingDown)
{
	auto effect = Make(BeamFile(), {0.0f, 20.0f, 0.0f});
	effect->Step(k_Step);
	// It waits for targets
	EXPECT_FALSE(effect->Finished());
	effect->CloseDown();
	effect->AddTargetPosition({0.0f, 0.0f, 10.0f});
	effect->Step(k_Step);
	EXPECT_EQ(effect->AtomCount(), 0u);
	EXPECT_EQ(effect->TargetPositionCount(), 1u);
	EXPECT_TRUE(effect->Finished());
}

TEST_F(ParticleBeamTest, BeamsKeepAboveTheLandBetweenTheirEnds)
{
	world.landHeight = 50.0f;
	auto effect = Make(BeamFile(2.0f), {0.0f, 20.0f, 0.0f});
	effect->AddTargetPosition({0.0f, 20.0f, 30.0f});
	effect->Step(k_Step);
	effect->Step(k_Step);
	Effect::DrawWalk walk;
	effect->Walk(1.0f, walk);
	ASSERT_EQ(walk.chains.size(), 3u);
	// The ends stay under the land, but the curve rises over it through the key points between
	const auto& chain = walk.chains[0];
	EXPECT_NEAR(walk.joints[chain.firstJoint].position.y, 20.0f, k_Epsilon);
	float highest = 0.0f;
	for (uint32_t j = 0; j < chain.jointCount; ++j)
	{
		highest = std::max(highest, walk.joints[chain.firstJoint + j].position.y);
	}
	EXPECT_GT(highest, 52.0f);
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <memory>

#include <entt/entity/entity.hpp>
#include <gtest/gtest.h>

#include "3D/AllMeshes.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/AnimatedStatic.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/PhysicsClasses.h"
#include "ECS/Registry.h"
#include "InfoConstants.h"
#include "Physics/TurnRules.h"
#include "Resources/ResourceManager.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using physics::MaterialRow;
using physics_classes::BodyKind;
namespace turn = openblack::physics::turn;

namespace
{
class PhysicsClassesTest: public ::testing::Test
{
protected:
	void SetUp() override
	{
		_info = std::make_unique<InfoConstants>();
		auto& rock = _info->mobileStatic.at(static_cast<size_t>(MobileStaticInfo::RockChalk));
		rock.mobileType = MobileStaticInfo::Rock;
		rock.meshId = MeshId::Dummy;
		auto& fence = _info->mobileStatic.at(static_cast<size_t>(MobileStaticInfo::CeltFenceShort));
		fence.mobileType = MobileStaticInfo::None;
		fence.meshId = MeshId::BuildingCelticFenceShort;
		auto& ball = _info->mobileStatic.at(static_cast<size_t>(MobileStaticInfo::ToyBall));
		ball.meshId = MeshId::ObjectToyBall;
		auto& bowling = _info->mobileStatic.at(static_cast<size_t>(MobileStaticInfo::ToyBowlingBall));
		bowling.meshId = MeshId::ObjectToyBowlingBall;
		auto& lantern = _info->mobileStatic.at(static_cast<size_t>(MobileStaticInfo::StreetLantern));
		lantern.mobileType = MobileStaticInfo::None;
		lantern.meshId = MeshId::Dummy;
		auto& altar = _info->mobileStatic.at(static_cast<size_t>(MobileStaticInfo::StandaloneAltar));
		altar.mobileType = MobileStaticInfo::None;
		altar.meshId = MeshId::Dummy;
		auto& offering = _info->pot.at(0);
		offering.meshId = MeshId::I_OfferingFood;
		offering.canBecomeAPhysicsObject = 1;
		auto& pile = _info->pot.at(1);
		pile.meshId = MeshId::Dummy;
		pile.canBecomeAPhysicsObject = 0;
	}

	physics_classes::ClassFacts Classify(entt::entity entity, physics_classes::ClassInputs inputs = {})
	{
		return physics_classes::Classify(_registry, entity, *_info, inputs);
	}

	entt::entity Static(MobileStaticInfo type)
	{
		const auto entity = _registry.Create();
		_registry.Assign<MobileStatic>(entity, type);
		return entity;
	}

	Registry _registry;
	std::unique_ptr<InfoConstants> _info;
};
} // namespace

TEST_F(PhysicsClassesTest, RocksAreDenseObstaclesThatBreakBuildings)
{
	const auto facts = Classify(Static(MobileStaticInfo::RockChalk));
	EXPECT_EQ(facts.row, MaterialRow::Rock);
	EXPECT_EQ(facts.body, BodyKind::Model);
	EXPECT_TRUE(facts.canBecomePhysicsObject);
	EXPECT_TRUE(facts.interacts);
	EXPECT_TRUE(facts.physicallyDestroysAbodes);
}

TEST_F(PhysicsClassesTest, HeavyStaticsAreMadeOfRock)
{
	for (const auto type : {MobileStaticInfo::GateTotemApe, MobileStaticInfo::GateTotemTiger, MobileStaticInfo::WeepingStone,
	                        MobileStaticInfo::WeepingStoneReward, MobileStaticInfo::SingingStone_1})
	{
		EXPECT_EQ(Classify(Static(type)).row, MaterialRow::Rock);
	}
}

TEST_F(PhysicsClassesTest, FencesAndToysHaveTheirOwnMaterials)
{
	EXPECT_EQ(Classify(Static(MobileStaticInfo::CeltFenceShort)).row, MaterialRow::Fence);
	const auto ball = Classify(Static(MobileStaticInfo::ToyBall));
	EXPECT_EQ(ball.row, MaterialRow::ToyBall);
	EXPECT_FALSE(ball.physicallyDestroysAbodes);
	const auto bowling = Classify(Static(MobileStaticInfo::ToyBowlingBall));
	EXPECT_EQ(bowling.row, MaterialRow::BowlingBall);
	EXPECT_TRUE(bowling.physicallyDestroysAbodes);
	// A toy whose model the game doesn't list is as heavy as a rock
	EXPECT_EQ(Classify(Static(MobileStaticInfo::ToyDie)).row, MaterialRow::Rock);
}

TEST_F(PhysicsClassesTest, OtherStaticsStandAsBuildingsDo)
{
	const auto altar = Static(MobileStaticInfo::StandaloneAltar);
	EXPECT_EQ(Classify(altar).row, MaterialRow::DefaultMovable);
	EXPECT_TRUE(Classify(altar).interacts);
	EXPECT_FALSE(Classify(altar, {.life = 0.005f}).interacts);
	EXPECT_FALSE(Classify(altar, {.percentBuilt = 0.1f}).interacts);
	EXPECT_FALSE(Classify(Static(MobileStaticInfo::StreetLantern)).interacts);
	const auto bonfire = Classify(Static(MobileStaticInfo::Bonfire));
	EXPECT_FALSE(bonfire.interacts);
	EXPECT_FALSE(bonfire.canBecomePhysicsObject);
}

TEST_F(PhysicsClassesTest, PotsFlyByTheirInfoAndAreHitUnlessInAStoragePit)
{
	const auto offering = _registry.Create();
	_registry.Assign<Pot>(offering, Pot {.amount = 1, .maxAmount = 1, .type = static_cast<PotInfo>(0)});
	const auto facts = Classify(offering);
	EXPECT_EQ(facts.row, MaterialRow::Hay);
	EXPECT_TRUE(facts.canBecomePhysicsObject);
	EXPECT_TRUE(facts.interacts);
	EXPECT_FALSE(Classify(offering, {.partOfStoragePit = true}).interacts);

	const auto pile = _registry.Create();
	_registry.Assign<Pot>(pile, Pot {.amount = 1, .maxAmount = 1, .type = static_cast<PotInfo>(1)});
	EXPECT_EQ(Classify(pile).row, MaterialRow::Pot);
	EXPECT_FALSE(Classify(pile).canBecomePhysicsObject);
}

TEST_F(PhysicsClassesTest, MobileObjects)
{
	const auto make = [this](MobileObjectInfo type) {
		const auto entity = _registry.Create();
		_registry.Assign<MobileObject>(entity, type);
		return Classify(entity);
	};
	EXPECT_EQ(make(MobileObjectInfo::Champi).row, MaterialRow::Champignon);
	EXPECT_EQ(make(MobileObjectInfo::MagicMushroom).row, MaterialRow::MagicMushroom);
	EXPECT_EQ(make(MobileObjectInfo::Toadstool).row, MaterialRow::Toadstool);
	EXPECT_EQ(make(MobileObjectInfo::Ball).row, MaterialRow::Football);
	EXPECT_EQ(make(MobileObjectInfo::EgyptBarrel).row, MaterialRow::DefaultMovable);
	const auto whale = make(MobileObjectInfo::Whale);
	EXPECT_FALSE(whale.canBecomePhysicsObject);
	EXPECT_TRUE(whale.interacts);
	EXPECT_FALSE(make(MobileObjectInfo::Creed).interacts);
}

TEST_F(PhysicsClassesTest, LivingThings)
{
	const auto villager = _registry.Create();
	_registry.Assign<Villager>(villager);
	const auto facts = Classify(villager);
	EXPECT_EQ(facts.body, BodyKind::VillagerBox);
	EXPECT_EQ(facts.row, MaterialRow::Villager);
	EXPECT_TRUE(facts.canBecomePhysicsObject);
	EXPECT_TRUE(facts.animated);
	EXPECT_TRUE(facts.upright);
	EXPECT_FALSE(Classify(villager, {.villagerReachable = false}).canBecomePhysicsObject);

	const auto animal = _registry.Create();
	_registry.Assign<Animal>(animal);
	EXPECT_EQ(Classify(animal).body, BodyKind::AnimalBox);
	EXPECT_EQ(Classify(animal).row, MaterialRow::Animal);

	const auto creature = _registry.Create();
	_registry.Assign<Creature>(creature);
	const auto big = Classify(creature);
	EXPECT_EQ(big.body, BodyKind::Creature);
	EXPECT_FALSE(big.canBecomePhysicsObject);
	EXPECT_TRUE(big.interacts);
	EXPECT_FLOAT_EQ(big.fixedMass.value_or(0.0f), 1000.0f);
}

TEST_F(PhysicsClassesTest, TreesFlyButAreNotHitWhileDeadTreesAre)
{
	const auto tree = _registry.Create();
	_registry.Assign<Tree>(tree);
	const auto standing = Classify(tree);
	EXPECT_EQ(standing.body, BodyKind::Tree);
	EXPECT_TRUE(standing.rooted);
	EXPECT_FALSE(standing.interacts);
	EXPECT_TRUE(standing.canBecomePhysicsObject);

	const auto dead = _registry.Create();
	_registry.Assign<DeadTree>(dead);
	EXPECT_EQ(Classify(dead).row, MaterialRow::Tree);
	EXPECT_FALSE(Classify(dead).rooted);
	EXPECT_TRUE(Classify(dead).interacts);

	// The wood the hand carries is its own model, of the movable default
	const auto log = _registry.Create();
	_registry.Assign<DeadTree>(log);
	_registry.Assign<Mesh>(log, resources::HashIdentifier(MeshId::ObjectWoodInHand), static_cast<int8_t>(0),
	                       static_cast<int8_t>(0));
	EXPECT_EQ(Classify(log).row, MaterialRow::DefaultMovable);
	EXPECT_EQ(Classify(log).body, BodyKind::Model);
}

TEST_F(PhysicsClassesTest, BuildingsAreHeavyStaticObstacles)
{
	const auto make = [this](AbodeNumber type) {
		const auto entity = _registry.Create();
		_registry.Assign<Abode>(entity, Abode {.type = type});
		return entity;
	};
	const auto house = make(AbodeNumber::A);
	const auto facts = Classify(house);
	EXPECT_FALSE(facts.canBecomePhysicsObject);
	EXPECT_FALSE(facts.checksPoints);
	EXPECT_TRUE(facts.interacts);
	EXPECT_FLOAT_EQ(facts.fixedMass.value_or(0.0f), 2000.0f);
	EXPECT_EQ(facts.row, MaterialRow::DefaultUnmovable);
	EXPECT_FALSE(Classify(house, {.life = 0.01f}).interacts);
	EXPECT_FALSE(Classify(house, {.percentBuilt = 0.05f}).interacts);
	// The village centre is always in the way; graveyards, pitches and totems never
	EXPECT_TRUE(Classify(make(AbodeNumber::TownCentre), {.life = 0.0f}).interacts);
	EXPECT_FALSE(Classify(make(AbodeNumber::Graveyard)).interacts);
	EXPECT_FALSE(Classify(make(AbodeNumber::FootballPitch)).interacts);
	EXPECT_FALSE(Classify(make(AbodeNumber::Totem)).interacts);

	const auto temple = _registry.Create();
	_registry.Assign<Temple>(temple);
	EXPECT_FLOAT_EQ(Classify(temple).fixedMass.value_or(0.0f), 10000.0f);
	EXPECT_FALSE(Classify(temple).checksPoints);

	const auto feature = _registry.Create();
	_registry.Assign<Feature>(feature);
	EXPECT_TRUE(Classify(feature).interacts);
	EXPECT_FALSE(Classify(feature).canBecomePhysicsObject);
}

TEST_F(PhysicsClassesTest, OnlyThePhysicalShieldIsAnObstacle)
{
	const auto physical = _registry.Create();
	_registry.Assign<MagicShield>(physical, MagicShield {.kind = MagicShield::Kind::Physical});
	_registry.Assign<ShieldDome>(physical);
	const auto facts = Classify(physical);
	EXPECT_EQ(facts.row, MaterialRow::PhysicalShield);
	EXPECT_TRUE(facts.interacts);
	EXPECT_TRUE(facts.alwaysStays);
	EXPECT_FALSE(facts.canBecomePhysicsObject);

	const auto spiritual = _registry.Create();
	_registry.Assign<MagicShield>(spiritual, MagicShield {.kind = MagicShield::Kind::Spiritual});
	_registry.Assign<ShieldDome>(spiritual);
	EXPECT_FALSE(Classify(spiritual).interacts);
}

TEST_F(PhysicsClassesTest, FieldsAreNeverInThePhysics)
{
	const auto field = _registry.Create();
	_registry.Assign<Field>(field);
	EXPECT_EQ(Classify(field).body, BodyKind::None);
	EXPECT_FALSE(Classify(field).interacts);
}

TEST(PhysicsClasses, WeightGrowsWithTheCubeOfTheScaleAndNeverReachesNothing)
{
	EXPECT_FLOAT_EQ(physics_classes::Weight(82.5f, 1.0f), 82.5f);
	EXPECT_FLOAT_EQ(physics_classes::Weight(10.0f, 2.0f), 80.0f);
	EXPECT_FLOAT_EQ(physics_classes::BodyMass(0.0f), 0.01f);
	EXPECT_FLOAT_EQ(physics_classes::BodyMass(5.0f), 5.0f);
}

TEST(PhysicsTurn, ImpactIsTheMeanForceAndIgnoresTheSmallest)
{
	EXPECT_FALSE(turn::Impact(glm::vec3(0.01f, 0.0f, 0.0f)).has_value());
	EXPECT_FLOAT_EQ(turn::Impact(glm::vec3(0.0f, 200.0f, 0.0f)).value_or(0.0f), 10.0f);
	EXPECT_FLOAT_EQ(turn::GLoad(98.1f, 10.0f), 1.0f);
}

TEST(PhysicsTurn, KnocksSoundWhenHitOrHardEnough)
{
	EXPECT_FALSE(turn::WantsCollisionSound(true, true, 1000.0f, 1.0f));
	EXPECT_TRUE(turn::WantsCollisionSound(false, true, 0.0f, 1.0f));
	EXPECT_FALSE(turn::WantsCollisionSound(false, false, 4.9f, 1.0f));
	EXPECT_TRUE(turn::WantsCollisionSound(false, false, 4.91f, 1.0f));
}

TEST(PhysicsTurn, LoudnessByHowManyWeightsTheKnockWas)
{
	const float weight = 1.0f / 9.81f;
	EXPECT_EQ(turn::CollisionLevel(1.0f, weight, false), turn::SoundLevel::Soft);
	EXPECT_EQ(turn::CollisionLevel(2.0f, weight, false), turn::SoundLevel::Medium);
	EXPECT_EQ(turn::CollisionLevel(3.5f, weight, false), turn::SoundLevel::Hard);
	EXPECT_EQ(turn::CollisionLevel(3.5f, weight, true), turn::SoundLevel::Medium);
	const auto keys = turn::CollisionKeys(turn::SoundLevel::Hard, static_cast<int32_t>(SoundCollisionType::SolidStone),
	                                      static_cast<int32_t>(SoundCollisionType::Ground));
	EXPECT_EQ(keys, (std::array<int32_t, 5> {1, 0, 22, 16, 75}));
}

TEST(PhysicsTurn, APairStaysQuietForTwoTurns)
{
	turn::SoundPairs pairs;
	EXPECT_TRUE(pairs.MaySound(1, 2));
	pairs.Add(1, 2);
	EXPECT_FALSE(pairs.MaySound(1, 2));
	EXPECT_FALSE(pairs.MaySound(2, 1));
	EXPECT_TRUE(pairs.MaySound(1, 3));
	pairs.EndTurn();
	EXPECT_FALSE(pairs.MaySound(1, 2));
	pairs.EndTurn();
	EXPECT_TRUE(pairs.MaySound(1, 2));
	for (uint32_t i = 0; i < turn::SoundPairs::k_MostPairs; ++i)
	{
		pairs.Add(100 + i, 0);
	}
	EXPECT_FALSE(pairs.MaySound(5, 6));
}

TEST(PhysicsTurn, MovingBodiesWakeTheCellsTheyWillReach)
{
	const auto still = turn::WakeCells(glm::vec3(55.0f, 0.0f, 55.0f), glm::vec3(0.0f), 2.0f);
	EXPECT_EQ(still.low, glm::ivec2(5, 5));
	EXPECT_EQ(still.high, glm::ivec2(5, 5));
	const auto moving = turn::WakeCells(glm::vec3(55.0f, 0.0f, 55.0f), glm::vec3(100.0f, -50.0f, 0.0f), 2.0f);
	EXPECT_EQ(moving.low, glm::ivec2(4, 4));
	EXPECT_EQ(moving.high, glm::ivec2(6, 6));
	// Off the map, onto its edge cells
	const auto edge = turn::SquareCells(glm::vec3(-30.0f, 0.0f, 5200.0f), 5.0f);
	EXPECT_EQ(edge.low, glm::ivec2(0, 511));
	EXPECT_EQ(edge.high, glm::ivec2(0, 511));
}

TEST(PhysicsTurn, DustPuffsGrowThenShrinkOverASecond)
{
	turn::DustPuff puff {.velocity = glm::vec3(1.0f, 0.0f, 0.0f), .size = 4.0f, .variant = 15};
	EXPECT_FLOAT_EQ(turn::DustPuffSize(puff), 0.0f);
	puff.age = 0.0625f;
	EXPECT_NEAR(turn::DustPuffSize(puff), 4.0f * 0.9375f * 0.5f, 1e-5f);
	puff.age = 0.5f;
	EXPECT_NEAR(turn::DustPuffSize(puff), 2.0f, 1e-5f);
	EXPECT_EQ(turn::DustPuffFrame(puff), 16 + ((15 + 1) & 15));
	EXPECT_TRUE(turn::AdvanceDustPuff(puff, 0.25f));
	EXPECT_FLOAT_EQ(puff.position.x, 0.25f);
	EXPECT_FALSE(turn::AdvanceDustPuff(puff, 0.25f));
	EXPECT_FLOAT_EQ(turn::LandingPuffSize(1.0f), 2.0f);
	EXPECT_FLOAT_EQ(turn::LandingPuffSize(10.0f), 5.0f);
}

TEST(PhysicsTurn, ColoursBlendTowardsAnotherKeepingTheirAlpha)
{
	EXPECT_EQ(turn::BlendColour(0x50806040u, 0xFFFFFFFFu, 0), 0x50806040u);
	EXPECT_EQ(turn::BlendColour(0x50000000u, 0x00FF8000u, 128), 0x507F4000u);
	EXPECT_EQ(turn::BlendColour(0x50FF0000u, 0x00000000u, 128), 0x507F0000u);
}

TEST(PhysicsTurn, RingsOnTheWater)
{
	const auto hit = turn::ImpactRing(glm::vec3(10.0f, -3.0f, 20.0f), 2.0f);
	EXPECT_EQ(hit.position, glm::vec3(10.0f, 0.1f, 20.0f));
	EXPECT_FLOAT_EQ(hit.growth, 4.0f);
	EXPECT_FLOAT_EQ(hit.rate, 0.5f);
	EXPECT_EQ(hit.cell, 0x3F);
	EXPECT_EQ(turn::BobRing(glm::vec3(0.0f), 1.0f).cell, 0x30);
}

TEST(PhysicsTurn, FastThingsWhooshPastTheCamera)
{
	const glm::vec3 camera(0.0f);
	EXPECT_TRUE(turn::PassesCamera(glm::vec3(11.0f, 0, 0), glm::vec3(9.0f, 0, 0), camera, glm::vec3(-21.0f, 0, 0)));
	EXPECT_FALSE(turn::PassesCamera(glm::vec3(11.0f, 0, 0), glm::vec3(9.0f, 0, 0), camera, glm::vec3(-19.0f, 0, 0)));
	EXPECT_FALSE(turn::PassesCamera(glm::vec3(9.5f, 0, 0), glm::vec3(9.0f, 0, 0), camera, glm::vec3(-30.0f, 0, 0)));
}

TEST(PhysicsTurn, SinkingAndBobbing)
{
	EXPECT_TRUE(turn::NearSeaLevel(false, 0.4f, 1.0f));
	EXPECT_FALSE(turn::NearSeaLevel(false, 0.6f, 1.0f));
	EXPECT_FALSE(turn::NearSeaLevel(true, -5.0f, 1.0f));
	EXPECT_TRUE(turn::Bobbed(-1.0f, 0.5f));
	EXPECT_TRUE(turn::Bobbed(1.0f, -0.5f));
	EXPECT_FALSE(turn::Bobbed(0.0f, 0.5f));
	EXPECT_FLOAT_EQ(turn::PushForce(10.0f), 98.1f);
}

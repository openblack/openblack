/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "PhysicsClasses.h"

#include <algorithm>

#include "3D/AllMeshes.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/AnimatedStatic.h"
#include "ECS/Components/BuildingDamage.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Flowers.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/Reward.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "InfoConstants.h"
#include "Physics/BodyShapes.h"
#include "Resources/ResourceManager.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using physics::MaterialRow;

namespace
{
/// A building is hit while more than this much of it stands built
constexpr float k_LeastBuiltToHit = 0.1f;
/// ...and while it has more life than this
constexpr float k_LeastLifeToHit = 0.01f;

/// The ordinary rule for buildings and the things that stand like them: hit while built and standing
bool StandingBuilding(const physics_classes::ClassInputs& inputs)
{
	return inputs.percentBuilt > k_LeastBuiltToHit && inputs.life > k_LeastLifeToHit;
}

template <typename Info, typename Index>
const Info* Row(const auto& rows, Index index)
{
	const auto i = static_cast<size_t>(index);
	return i < rows.size() ? &rows[i] : nullptr;
}

/// The things made of a model the hand's toys are: each its own material
std::optional<MaterialRow> ToyRow(MeshId mesh)
{
	switch (mesh)
	{
	case MeshId::ObjectToyBall:
		return MaterialRow::ToyBall;
	case MeshId::ObjectToyBowlingBall:
		return MaterialRow::BowlingBall;
	case MeshId::ObjectToyCuddly:
		return MaterialRow::ToyCuddly;
	case MeshId::ObjectToyDice:
		return MaterialRow::ToyDie;
	case MeshId::ObjectToySkittle:
		return MaterialRow::Skittle;
	default:
		return std::nullopt;
	}
}

/// The statics as heavy as rocks: the gate totems, the weeping stones and the singing stone
bool HeavyStatic(MobileStaticInfo type)
{
	return (type >= MobileStaticInfo::GateTotemApe && type <= MobileStaticInfo::GateTotemTiger) ||
	       type == MobileStaticInfo::WeepingStone || type == MobileStaticInfo::WeepingStoneReward ||
	       type == MobileStaticInfo::SingingStone_1;
}

physics_classes::ClassFacts MobileStaticFacts(MobileStaticInfo type, const InfoConstants& info,
                                              const physics_classes::ClassInputs& inputs)
{
	// The lanterns and the singing stone's base aren't statics to the physics but plain objects: they never fly and are
	// made of the unmovable material; the lanterns are never hit
	if (type == MobileStaticInfo::StreetLantern || type == MobileStaticInfo::CountryLantern)
	{
		return {.body = physics_classes::BodyKind::Model, .row = MaterialRow::DefaultUnmovable};
	}
	if (type == MobileStaticInfo::SingingStoneBase)
	{
		return {.body = physics_classes::BodyKind::Model, .row = MaterialRow::DefaultUnmovable, .interacts = true};
	}
	// A bonfire neither flies nor is hit
	if (type == MobileStaticInfo::Bonfire)
	{
		return {.body = physics_classes::BodyKind::Model, .row = MaterialRow::DefaultUnmovable};
	}
	physics_classes::ClassFacts facts {
	    .body = physics_classes::BodyKind::Model, .row = MaterialRow::DefaultMovable, .canBecomePhysicsObject = true};
	const auto* row = Row<GMobileStaticInfo>(info.mobileStatic, type);
	const auto mobileType = row != nullptr ? row->mobileType : MobileStaticInfo::None;
	const auto mesh = row != nullptr ? row->meshId : MeshId::Dummy;
	const bool toy = physics_classes::IsToyModel(mesh);
	const bool fence = physics_classes::IsFenceModel(mesh);
	const bool rockLike = HeavyStatic(type) || mobileType == MobileStaticInfo::Rock;
	// The heavy statics and rocks first, then fences, then toys each of their own material (an unknown toy is as heavy
	// as a rock)
	if (rockLike)
	{
		facts.row = MaterialRow::Rock;
	}
	else if (fence)
	{
		facts.row = MaterialRow::Fence;
	}
	else if (toy)
	{
		facts.row = ToyRow(mesh).value_or(MaterialRow::Rock);
	}
	// Toys, rocks and their like, fences and idols are always hit; anything else as a building is
	facts.interacts = toy || rockLike || fence || mobileType == MobileStaticInfo::Idol || StandingBuilding(inputs);
	facts.physicallyDestroysAbodes = facts.row == MaterialRow::Rock || facts.row == MaterialRow::BowlingBall;
	return facts;
}

physics_classes::ClassFacts MobileObjectFacts(MobileObjectInfo type)
{
	physics_classes::ClassFacts facts {.body = physics_classes::BodyKind::Model,
	                                   .row = MaterialRow::DefaultMovable,
	                                   .canBecomePhysicsObject = true,
	                                   .interacts = true};
	switch (type)
	{
	case MobileObjectInfo::Champi:
		facts.row = MaterialRow::Champignon;
		break;
	case MobileObjectInfo::MagicMushroom:
		facts.row = MaterialRow::MagicMushroom;
		break;
	case MobileObjectInfo::Toadstool:
		facts.row = MaterialRow::Toadstool;
		break;
	case MobileObjectInfo::Ball:
		facts.row = MaterialRow::Football;
		break;
	case MobileObjectInfo::LumpOfPoo:
		facts.row = MaterialRow::Poo;
		break;
	case MobileObjectInfo::OneOffSpellSeed:
		facts.row = MaterialRow::OneOffSpell;
		break;
	case MobileObjectInfo::Whale:
		// Never thrown, always in the way
		facts.canBecomePhysicsObject = false;
		break;
	case MobileObjectInfo::Creed:
		facts.canBecomePhysicsObject = false;
		facts.interacts = false;
		break;
	case MobileObjectInfo::HanoiPuzzleBase:
		// The puzzle's base never moves
		facts.canBecomePhysicsObject = false;
		break;
	case MobileObjectInfo::HanoiPuzzlePart1:
	case MobileObjectInfo::HanoiPuzzlePart2:
	case MobileObjectInfo::HanoiPuzzlePart3:
	case MobileObjectInfo::HanoiPuzzlePart4:
		// The puzzle's blocks move only while they aren't set in place; the puzzle isn't ported, so none is set
		break;
	default:
		break;
	}
	// Anything that can't fly is made of the unmovable material
	if (!facts.canBecomePhysicsObject && facts.row == MaterialRow::DefaultMovable)
	{
		facts.row = MaterialRow::DefaultUnmovable;
	}
	return facts;
}

/// A gate, the piper's cave or the phone box: hit by what is thrown, with a collision model of its own picked by its
/// state; chess pieces and the others are never hit
physics_classes::ClassFacts AnimatedStaticFacts(const AnimatedStatic& still)
{
	physics_classes::ClassFacts facts {.body = physics_classes::BodyKind::CollisionModel,
	                                   .row = MaterialRow::DefaultUnmovable,
	                                   .checksPoints = false,
	                                   .fixedMass = physics::shapes::k_TempleHeartMass};
	switch (still.type)
	{
	case AnimatedStaticInfo::NorseGate:
		facts.interacts = true;
		facts.collisionMesh = static_cast<uint32_t>(still.openState == 1 ? MeshId::NorseGatePhys2 : MeshId::NorseGatePhys1);
		break;
	case AnimatedStaticInfo::GateStonePlinth:
		facts.interacts = true;
		if (still.openState != 1 && still.plinthState != 0)
		{
			facts.collisionMesh =
			    static_cast<uint32_t>(still.plinthFull != 0 ? MeshId::GateTotemPlinthePhys3 : MeshId::GateTotemPlinthePhys2);
		}
		else
		{
			facts.collisionMesh = static_cast<uint32_t>(MeshId::GateTotemPlinthePhys1);
		}
		break;
	case AnimatedStaticInfo::PhoneBox:
		facts.interacts = true;
		facts.collisionMesh = static_cast<uint32_t>(MeshId::GateTotemPlinthePhys1);
		break;
	case AnimatedStaticInfo::PiperCaveEntrance:
		facts.interacts = true;
		facts.collisionMesh = static_cast<uint32_t>(MeshId::PiperEntrancePhys1);
		break;
	default:
		break;
	}
	return facts;
}
} // namespace

bool physics_classes::IsToyModel(MeshId mesh)
{
	return mesh >= MeshId::ObjectToyBall && mesh <= MeshId::ObjectToySkittle;
}

bool physics_classes::IsFenceModel(MeshId mesh)
{
	return mesh == MeshId::BuildingAmericanFence || mesh == MeshId::BuildingCelticFenceShort ||
	       mesh == MeshId::BuildingCelticFenceTall;
}

float physics_classes::Weight(float infoWeight, float scale)
{
	return scale * scale * scale * infoWeight;
}

float physics_classes::BodyMass(float weight)
{
	return std::max(weight, physics::shapes::k_MinMass);
}

namespace
{
physics_classes::ClassFacts ClassifyKind(const Registry& registry, entt::entity entity, const InfoConstants& info,
                                         const physics_classes::ClassInputs& inputs)
{
	using physics_classes::BodyKind;
	using physics_classes::ClassFacts;
	// A reward chest can be knocked about only once it is on the land; it weighs the same whatever its size
	if (const auto* chest = registry.TryGet<const Reward>(entity))
	{
		return {.body = BodyKind::Model,
		        .row = MaterialRow::DefaultMovable,
		        .canBecomePhysicsObject = chest->state == Reward::State::Landed,
		        .interacts = true,
		        .fixedMass = reward::k_Weight};
	}
	// A piece broken off a building flies and lands, but nothing flying hits it
	if (registry.AllOf<BuildingPiece>(entity))
	{
		return {.body = BodyKind::BuildingPiece, .row = MaterialRow::Fragment, .canBecomePhysicsObject = true, .dynamic = true};
	}
	if (registry.AllOf<Creature>(entity))
	{
		return {.body = BodyKind::Creature,
		        .row = MaterialRow::DefaultUnmovable,
		        .interacts = true,
		        .fixedMass = physics::shapes::k_CreatureMass};
	}
	if (registry.AllOf<Villager>(entity))
	{
		return {.body = BodyKind::VillagerBox,
		        .row = MaterialRow::Villager,
		        .canBecomePhysicsObject = inputs.villagerReachable,
		        .interacts = true,
		        .animated = true,
		        .upright = true,
		        .dynamic = true};
	}
	if (registry.AllOf<Animal>(entity))
	{
		return {.body = BodyKind::AnimalBox,
		        .row = MaterialRow::Animal,
		        .canBecomePhysicsObject = true,
		        .interacts = true,
		        .animated = true,
		        .upright = true,
		        .dynamic = true};
	}
	if (registry.AllOf<Tree>(entity))
	{
		// Standing trees are never hit: thrown things pass through them
		return {.body = BodyKind::Tree,
		        .row = MaterialRow::Tree,
		        .canBecomePhysicsObject = true,
		        .rooted = true,
		        .upright = true,
		        .dynamic = true,
		        .unclampedMass = true};
	}
	if (registry.AllOf<DeadTree>(entity))
	{
		// The wood the hand carries is a dead tree drawn as a log, a movable thing of its own model
		const auto* mesh = registry.TryGet<const Mesh>(entity);
		const bool log = mesh != nullptr && mesh->id == resources::HashIdentifier(MeshId::ObjectWoodInHand) &&
		                 !registry.Get<const DeadTree>(entity).felled;
		if (log)
		{
			return {
			    .body = BodyKind::Model, .row = MaterialRow::DefaultMovable, .canBecomePhysicsObject = true, .interacts = true};
		}
		return {.body = BodyKind::Tree,
		        .row = MaterialRow::Tree,
		        .canBecomePhysicsObject = true,
		        .interacts = true,
		        .dynamic = true,
		        .unclampedMass = true};
	}
	if (const auto* pot = registry.TryGet<const Pot>(entity))
	{
		const auto* row = Row<GPotInfo>(info.pot, pot->type);
		const bool offering = row != nullptr && row->meshId == MeshId::I_OfferingFood;
		return {.body = BodyKind::Model,
		        .row = offering ? MaterialRow::Hay : MaterialRow::Pot,
		        .canBecomePhysicsObject = row != nullptr && row->canBecomeAPhysicsObject != 0,
		        .interacts = !inputs.partOfStoragePit};
	}
	if (registry.AllOf<OneOffSpellSeed>(entity))
	{
		return MobileObjectFacts(MobileObjectInfo::OneOffSpellSeed);
	}
	if (const auto* still = registry.TryGet<const MobileStatic>(entity))
	{
		return MobileStaticFacts(still->type, info, inputs);
	}
	if (const auto* mobile = registry.TryGet<const MobileObject>(entity))
	{
		return MobileObjectFacts(mobile->type);
	}
	if (const auto* shield = registry.TryGet<const MagicShield>(entity))
	{
		// The spiritual shield lets everything through; the physical one is a heavy dome that never moves
		if (shield->kind != MagicShield::Kind::Physical || !registry.AllOf<ShieldDome>(entity))
		{
			return {};
		}
		return {.body = BodyKind::ShieldDome, .row = MaterialRow::PhysicalShield, .interacts = true, .alwaysStays = true};
	}
	if (registry.AllOf<Temple>(entity))
	{
		// The temple's heart is hit while more than a tenth of it is built, the tenth taken at the double precision its
		// test is made in
		constexpr double k_LeastHeartBuilt = 0.1;
		return {.body = BodyKind::Model,
		        .row = MaterialRow::DefaultUnmovable,
		        .interacts = static_cast<double>(inputs.percentBuilt) > k_LeastHeartBuilt,
		        .checksPoints = false,
		        .fixedMass = physics::shapes::k_TempleHeartMass};
	}
	if (const auto* abode = registry.TryGet<const Abode>(entity))
	{
		ClassFacts facts {.body = BodyKind::Model,
		                  .row = MaterialRow::DefaultUnmovable,
		                  .checksPoints = false,
		                  .fixedMass = physics::shapes::k_BuildingMass};
		switch (abode->type)
		{
		case AbodeNumber::TownCentre:
			facts.interacts = true;
			break;
		case AbodeNumber::Graveyard:
		case AbodeNumber::FootballPitch:
		case AbodeNumber::Field:
			facts.interacts = false;
			break;
		default:
			// The town's totem and the spell dispensers too stand as buildings do
			facts.interacts = StandingBuilding(inputs);
			break;
		}
		return facts;
	}
	if (const auto* animated = registry.TryGet<const AnimatedStatic>(entity))
	{
		return AnimatedStaticFacts(*animated);
	}
	if (registry.AnyOf<Feature, Flowers>(entity))
	{
		// Features stand as buildings do, and never move
		return {.body = BodyKind::Model, .row = MaterialRow::DefaultUnmovable, .interacts = StandingBuilding(inputs)};
	}
	// Fields, forests, teleport stones and anything else the physics doesn't know
	return {};
}
} // namespace

physics_classes::ClassFacts physics_classes::Classify(const Registry& registry, entt::entity entity, const InfoConstants& info,
                                                      const ClassInputs& inputs)
{
	auto facts = ClassifyKind(registry, entity, info, inputs);
	facts.immovable = inputs.immovable;
	// A model's body moves only when its object can fly and isn't held immovable; the hand-built bodies always move
	if (facts.body == BodyKind::Model)
	{
		facts.dynamic = facts.canBecomePhysicsObject && !inputs.immovable;
	}
	return facts;
}

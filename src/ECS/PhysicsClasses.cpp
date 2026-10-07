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

bool IsToy(MobileStaticInfo type)
{
	return type >= MobileStaticInfo::ToyBall && type <= MobileStaticInfo::ToyBowlingBall;
}

bool IsFence(MeshId mesh)
{
	return mesh == MeshId::BuildingAmericanFence || mesh == MeshId::BuildingCelticFenceShort ||
	       mesh == MeshId::BuildingCelticFenceTall;
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
	physics_classes::ClassFacts facts {
	    .body = physics_classes::BodyKind::Model, .row = MaterialRow::DefaultMovable, .canBecomePhysicsObject = true};
	const auto* row = Row<GMobileStaticInfo>(info.mobileStatic, type);
	const auto mobileType = row != nullptr ? row->mobileType : MobileStaticInfo::None;
	const auto mesh = row != nullptr ? row->meshId : MeshId::Dummy;
	const bool toy = IsToy(type);
	const bool fence = !toy && IsFence(mesh);
	const bool rockLike = HeavyStatic(type) || mobileType == MobileStaticInfo::Rock;
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
	// Street lanterns and bonfires are never hit; toys, rocks and their like, fences and idols always; anything else as
	// a building is
	if (type == MobileStaticInfo::StreetLantern || type == MobileStaticInfo::CountryLantern)
	{
		facts.interacts = false;
	}
	else if (type == MobileStaticInfo::Bonfire)
	{
		facts.interacts = false;
		facts.canBecomePhysicsObject = false;
	}
	else
	{
		facts.interacts = toy || rockLike || fence || mobileType == MobileStaticInfo::Idol || StandingBuilding(inputs);
	}
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
	default:
		break;
	}
	return facts;
}
} // namespace

float physics_classes::Weight(float infoWeight, float scale)
{
	return scale * scale * scale * infoWeight;
}

float physics_classes::BodyMass(float weight)
{
	return std::max(weight, physics::shapes::k_MinMass);
}

physics_classes::ClassFacts physics_classes::Classify(const Registry& registry, entt::entity entity, const InfoConstants& info,
                                                      const ClassInputs& inputs)
{
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
		        .upright = true};
	}
	if (registry.AllOf<Animal>(entity))
	{
		return {.body = BodyKind::AnimalBox,
		        .row = MaterialRow::Animal,
		        .canBecomePhysicsObject = true,
		        .interacts = true,
		        .animated = true,
		        .upright = true};
	}
	if (registry.AllOf<Tree>(entity))
	{
		// Standing trees are never hit: thrown things pass through them
		return {
		    .body = BodyKind::Tree, .row = MaterialRow::Tree, .canBecomePhysicsObject = true, .rooted = true, .upright = true};
	}
	if (registry.AllOf<DeadTree>(entity))
	{
		// The wood the hand carries is a dead tree drawn as a log, a movable thing of its own model
		const auto* mesh = registry.TryGet<const Mesh>(entity);
		const bool log = mesh != nullptr && mesh->id == resources::HashIdentifier(MeshId::ObjectWoodInHand);
		return {.body = log ? BodyKind::Model : BodyKind::Tree,
		        .row = log ? MaterialRow::DefaultMovable : MaterialRow::Tree,
		        .canBecomePhysicsObject = true,
		        .interacts = true};
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
		return {.body = BodyKind::Model,
		        .row = MaterialRow::DefaultUnmovable,
		        .interacts = inputs.percentBuilt > k_LeastBuiltToHit,
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
		case AbodeNumber::Totem:
		case AbodeNumber::Graveyard:
		case AbodeNumber::FootballPitch:
		case AbodeNumber::SpellDispenser:
		case AbodeNumber::Field:
			facts.interacts = false;
			break;
		default:
			facts.interacts = StandingBuilding(inputs);
			break;
		}
		return facts;
	}
	if (registry.AnyOf<Feature, AnimatedStatic, Flowers>(entity))
	{
		// Features stand as buildings do, and never move
		return {.body = BodyKind::Model, .row = MaterialRow::DefaultUnmovable, .interacts = StandingBuilding(inputs)};
	}
	// Fields, forests, the spell dispensers, teleport stones and anything else the physics doesn't know
	return {};
}

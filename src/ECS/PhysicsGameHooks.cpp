/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "PhysicsGameHooks.h"

#include <cmath>

#include <chrono>

#include <glm/geometric.hpp>

#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Common/GameRandom.h"
#include "Creature/CreatureCatch.h"
#include "Creature/CreatureDesires.h"
#include "Creature/CreatureRig.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/AtHome.h"
#include "ECS/Components/BuildingDamage.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureFight.h"
#include "ECS/Components/CreatureObjectAction.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/Indestructible.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/LivingPhysics.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/ScriptControl.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/StoragePit.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/ObjectPhysics.h"
#include "ECS/PhysicsClasses.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AnimalSystemInterface.h"
#include "ECS/Systems/BuildingDamageSystemInterface.h"
#include "ECS/Systems/CreatureAnimationSystemInterface.h"
#include "ECS/Systems/CreatureHandSystemInterface.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/CreatureObjectActionSystemInterface.h"
#include "ECS/Systems/ForestSystemInterface.h"
#include "ECS/Systems/Implementations/VillagerPhysics.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/MagicShieldSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/PlayerSystemInterface.h"
#include "ECS/Systems/ResourceStoreSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/MagicWorldInterface.h"
#include "Magic/SpellRules.h"
#include "Physics/Body.h"
#include "Physics/LivingRules.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace living = openblack::physics::living;

namespace
{
Registry& Entities()
{
	return Locator::entitiesRegistry::value();
}

/// The game's crush, scaled by how much of it a blow does
magic::EffectValues CrushOf(float share)
{
	constexpr size_t k_CrushPreset = 3;
	auto values = magic::EffectValues::From(Locator::infoConstants::value().effect.at(k_CrushPreset));
	values.Scale(share);
	return values;
}

/// Whether a blow can hurt a villager: not one hidden in its home or a building, nor one in a hand
bool TakesBlows(entt::entity villager)
{
	auto& registry = Entities();
	if (registry.AnyOf<AtHome, InHand>(villager))
	{
		return false;
	}
	const auto* action = registry.TryGet<const LivingAction>(villager);
	return action == nullptr || !Locator::livingActionSystem::has_value() ||
	       Locator::livingActionSystem::value().VillagerGetState(*action, LivingAction::Index::Top) !=
	           VillagerStates::GoAndHideInNearbyBuilding;
}

/// Where a blow's crush comes from: the thing that struck (none for a fall onto the land), and the player whose throw
/// it was, if any. A creature that struck takes the blow's alignment as its own.
magic::EffectSource BlowSource(entt::entity hitter, std::optional<PlayerNames> player)
{
	auto& registry = Entities();
	const bool validHitter = hitter != entt::null && registry.Valid(hitter);
	// The blow comes from the centre of the striking thing's body
	std::optional<glm::vec3> point;
	if (validHitter && Locator::dynamicsSystem::has_value())
	{
		if (const auto* entry = Locator::dynamicsSystem::value().Find(hitter); entry != nullptr && entry->body != nullptr)
		{
			point = entry->body->Centre();
		}
	}
	return {
	    .player = player.value_or(PlayerNames::NEUTRAL),
	    .casterCreature = validHitter && registry.AllOf<Creature>(hitter) ? hitter : entt::null,
	    .appliedBy = validHitter ? hitter : entt::null,
	    .playerless = !player.has_value(),
	    .blow = true,
	    .point = point,
	};
}

/// A villager or animal knocked harder than twice its weight is crushed by the blow, through its defences, applied by
/// what struck it and put down to the thrower's player
void HurtLiving(entt::entity living, const ImpactInfo& impact)
{
	const auto crush = living::LivingCrush(impact.g);
	if (!crush.has_value() || !Locator::magicSystem::has_value() || !Locator::infoConstants::has_value())
	{
		return;
	}
	if (Entities().AllOf<Villager>(living) && !TakesBlows(living))
	{
		return;
	}
	Locator::magicSystem::value().ApplyEffectToObject(living, CrushOf(*crush), BlowSource(impact.hitBy, impact.player));
}

/// Whether a thing is a toy, which strikes a creature without hurting it
bool IsToy(entt::entity object)
{
	return Locator::infoConstants::has_value() && physics_classes::IsToy(Entities(), object, Locator::infoConstants::value());
}

/// The bones a creature acts with and the moments of its object animations, if its species has them
const creature::CreatureRig::ActionPoints* RigPointsOf(const Creature& creature)
{
	const auto& rigs = Locator::resources::value().GetCreatureRigs();
	const auto id = creature::GetRigId(creature.species);
	return rigs.Contains(id) && rigs.Handle(id)->actionPoints.has_value() ? &*rigs.Handle(id)->actionPoints : nullptr;
}

/// A creature struck by a thrown thing (not a toy) is hurt a hundredth of how hard it was struck for its weight, its
/// own player's blow makes it think less of the player, and every blow makes it angry and afraid
void HurtCreature(DynamicsSystemInterface& dynamics, const PhysicsEntry& entry, const ImpactInfo& impact)
{
	auto& registry = Entities();
	const auto creature = entry.entity;
	if (impact.hitBy == entt::null || !registry.Valid(impact.hitBy) || IsToy(impact.hitBy))
	{
		return;
	}
	// The turn's force on its body sways it where the striking thing is
	if (const auto* hitter = dynamics.Find(impact.hitBy);
	    hitter != nullptr && hitter->body != nullptr && Locator::creatureAnimationSystem::has_value())
	{
		Locator::creatureAnimationSystem::value().KickSway(creature, entry.forceSum, hitter->body->Centre());
	}
	const auto& body = registry.Get<const Creature>(creature);
	// The creature of the player at this computer is not hurt at all while a script controls it
	if (registry.AllOf<ScriptControlled>(creature) && Locator::playerSystem::has_value() &&
	    body.owner == Locator::playerSystem::value().GetLocalPlayer())
	{
		return;
	}
	const auto* morph = registry.TryGet<const CreatureMorph>(creature);
	const float mass = living::CreatureMass(body.size, morph != nullptr ? morph->drawn.thinFat : 0.0f,
	                                        morph != nullptr ? morph->drawn.weakStrong : 0.0f);
	const auto crush = living::CreatureCrush(impact.impact, mass);
	if (!crush.has_value())
	{
		return;
	}
	// The crush is the striking thing's, put down to the player whose throw it was; the creature's own credit only
	// decides whether its player struck it
	if (Locator::magicSystem::has_value() && Locator::infoConstants::has_value())
	{
		const auto* hitter = dynamics.Find(impact.hitBy);
		const auto hitterPlayer = hitter != nullptr ? hitter->player : std::nullopt;
		Locator::magicSystem::value().ApplyEffectToObject(creature, CrushOf(*crush), BlowSource(impact.hitBy, hitterPlayer));
	}
	if (!Locator::creatureMindSystem::has_value())
	{
		return;
	}
	auto& minds = Locator::creatureMindSystem::value();
	if (impact.player.has_value() && *impact.player == body.owner)
	{
		minds.UpdateAttitudeFromFeedback(creature, -*crush);
	}
	minds.ChangeDesireSource(creature, creature_desires::sources::k_FearFromDamage, *crush);
	minds.ChangeDesireSource(creature, creature_desires::sources::k_AngerFromDamage, *crush);
}

/// A physical shield struck by a thing that breaks buildings pays for the blow by its momentum
void StrikeShield(DynamicsSystemInterface& dynamics, entt::entity shield, const ImpactInfo& impact)
{
	if (impact.hitBy == entt::null || !Locator::magicShieldSystem::has_value() ||
	    !dynamics.PhysicallyDestroysAbodes(impact.hitBy))
	{
		return;
	}
	const auto* hitter = dynamics.Find(impact.hitBy);
	if (hitter == nullptr || hitter->body == nullptr)
	{
		return;
	}
	const float momentum = glm::length(hitter->body->velocity) * hitter->body->Mass();
	Locator::magicShieldSystem::value().Impact(shield, impact.hitBy, momentum, hitter->player);
}

/// An animal coming down stands facing opposite its body's forward axis, plays its kind's landing as it lies, and its
/// flock's home moves to it; one killed in the air dies where it lands
void LandAnimal(PhysicsEntry* entry, entt::entity entity)
{
	auto& registry = Entities();
	auto* animal = registry.TryGet<Animal>(entity);
	auto* transform = registry.TryGet<Transform>(entity);
	if (animal == nullptr || transform == nullptr)
	{
		return;
	}
	auto pose = living::LandingPose::None;
	if (entry != nullptr && entry->body != nullptr)
	{
		pose = living::AnimalLandingPose(entry->body->TurnStartAxes()[0].y);
		const float heading = living::AnimalLandingHeading(entry->body->Axes());
		transform->rotation = physics::QuarterTurned(living::HeadingAxes(heading));
		// Its heading across the land as the animals keep it, from the way it is drawn
		const auto side = transform->rotation[0];
		animal->heading = std::atan2(-side.x, side.z);
	}
	// It goes on from where it came down, on the land
	if (Locator::terrainSystem::has_value())
	{
		transform->position.y =
		    Locator::terrainSystem::value().GetHeightAt(glm::vec2(transform->position.x, transform->position.z));
	}
	const auto position = transform->position;
	animal->position = position;
	animal->previousPosition = position;
	animal->previousHeading = animal->heading;
	animal->height = 0.0f;
	animal->move.position = {map_coords::ToFixed(position.x), map_coords::ToFixed(position.z)};
	animal->move.goal = animal->move.position;
	if (animal->Dead())
	{
		// Killed in the air it starts dying where it lands; one already dying lies dead
		const bool wasDying = animal->state == AnimalState::Dying || animal->state == AnimalState::Dead;
		if (Locator::animalSystem::has_value())
		{
			Locator::animalSystem::value().SetDying(entity);
		}
		if (wasDying)
		{
			animal->state = AnimalState::Dead;
			animal->turnsInState = 0;
		}
		return;
	}
	// Its flock's home moves by its kind's rule
	if (registry.Valid(animal->flock) && Locator::animalSystem::has_value())
	{
		auto& animals = Locator::animalSystem::value();
		switch (living::LairOnLanding(animal->type, animals.LeaderOf(animal->flock) == entity))
		{
		case living::LandedLair::WhereItLanded:
			animals.SetFlockCentre(animal->flock, glm::vec2(position.x, position.z));
			break;
		case living::LandedLair::ForestOfItsKind:
		{
			// Tigers and wolves choose a forest; a wolf only when it leads its flock, keeping to where it is while a
			// script holds it. (A flag of the wolf's flock that stops the choice altogether isn't known in openblack.)
			const bool leader = animals.LeaderOf(animal->flock) == entity;
			if (animal->type == AnimalInfo::Wolf && !leader)
			{
				break;
			}
			std::optional<glm::vec3> lair;
			if (animal->type == AnimalInfo::Wolf && registry.AllOf<InScript>(entity))
			{
				lair = position;
			}
			else if (Locator::forestSystem::has_value())
			{
				lair = Locator::forestSystem::value().ForestLair(animal->type, position);
			}
			if (lair.has_value())
			{
				animals.SetFlockCentre(animal->flock, glm::vec2(lair->x, lair->z));
			}
			break;
		}
		case living::LandedLair::Unchanged:
			break;
		}
	}
	// It plays its landing clip through, kept still, then decides what to do; a kind with none waits on its current clip
	if (const auto clip = living::AnimalLandedClip(animal->type, pose))
	{
		animal->animation = *clip;
		animal->clipPlace = 0;
	}
	animal->afterClip = AnimalState::DecideWhatToDo;
	animal->state = AnimalState::WaitForClip;
	animal->turnsInState = 0;
}

void StartFlying(entt::entity object)
{
	auto& registry = Entities();
	if (registry.AllOf<Villager>(object))
	{
		villager_physics::StartFlying(object);
		return;
	}
	if (auto* animal = registry.TryGet<Animal>(object))
	{
		if (const auto clip = living::ClipsOf(animal->type).thrown)
		{
			animal->animation = *clip;
			animal->clipPlace = 0;
		}
	}
}

} // namespace

PhysicsStarted PhysicsGameHooks::InitialisePhysics(DynamicsSystemInterface& dynamics, entt::entity object,
                                                   const PhysicsStart& start)
{
	// A villager drops what it carries before it starts to fly
	if (Entities().AllOf<Villager>(object))
	{
		DropCarriedResource(dynamics, object, start.velocity);
	}
	// A villager takes to the air first; a state it can't leave keeps it on the ground, and its body is not started
	if (Entities().AllOf<Villager>(object) && !villager_physics::StartFlying(object))
	{
		return {};
	}
	const auto started = PhysicsClassHooks::InitialisePhysics(dynamics, object, start);
	if (started.started && !Entities().AllOf<Villager>(object))
	{
		StartFlying(object);
	}
	return started;
}

entt::entity PhysicsGameHooks::EndPhysics(DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity object,
                                          bool insert)
{
	auto& registry = Entities();
	if (registry.AllOf<Tree>(object))
	{
		return object_physics::EndTree(dynamics, entry, object, insert);
	}
	// A big piece of a building that comes to rest goes back into it as rubble
	if (registry.AllOf<BuildingPiece>(object) && Locator::buildingDamageSystem::has_value())
	{
		return Locator::buildingDamageSystem::value().PieceAtRest(dynamics, entry, object, insert);
	}
	if (registry.AllOf<DeadTree>(object))
	{
		return object_physics::EndDeadTree(dynamics, entry, object, insert);
	}
	if (registry.AllOf<Pot>(object))
	{
		return object_physics::EndPot(dynamics, entry, object, insert);
	}
	// A rock or other static put down gently by a town becomes its artefact
	if (registry.AllOf<MobileStatic>(object))
	{
		object_physics::ConsiderArtefact(entry, object, insert);
	}
	const auto kept = PhysicsClassHooks::EndPhysics(dynamics, entry, object, insert);
	if (kept == entt::null || !registry.Valid(kept))
	{
		return kept;
	}
	if (registry.AllOf<Villager>(kept))
	{
		villager_physics::Land(entry, kept);
	}
	else if (registry.AllOf<Animal>(kept))
	{
		LandAnimal(entry, kept);
	}
	return kept;
}

bool PhysicsGameHooks::HasSunk(DynamicsSystemInterface& dynamics, PhysicsEntry& entry)
{
	auto& registry = Entities();
	if (registry.AllOf<Villager>(entry.entity))
	{
		return villager_physics::Sink(entry);
	}
	if (registry.AllOf<Animal>(entry.entity))
	{
		// Something already going from the world hasn't sunk
		if (!registry.Valid(entry.entity))
		{
			return false;
		}
		// The player who dropped it may teach their creature to throw things in the sea; it dies and is gone
		if (const auto dropper = villager_physics::DropperOf(entry.entity);
		    dropper.has_value() && Locator::creatureMindSystem::has_value())
		{
			const auto& at = registry.Get<const Transform>(entry.entity).position;
			Locator::creatureMindSystem::value().PlayerDid(living::k_DeedThrowInTheSea, at, entry.entity, *dropper);
		}
		world_objects::Remove(entry.entity);
		return true;
	}
	return PhysicsClassHooks::HasSunk(dynamics, entry);
}

void PhysicsGameHooks::DropSound(entt::entity object)
{
	const auto ticks =
	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	object_physics::TreeDropSound(object, static_cast<uint64_t>(ticks));
}

void PhysicsGameHooks::OfferToCatchingCreatures(entt::entity object, PhysicsEntry& entry)
{
	auto& registry = Entities();
	if (entry.body == nullptr || !Locator::creatureObjectActionSystem::has_value() ||
	    !Locator::creatureAnimationSystem::has_value() || !Locator::gameRandom::has_value())
	{
		return;
	}
	auto& actions = Locator::creatureObjectActionSystem::value();
	auto& animations = Locator::creatureAnimationSystem::value();
	std::vector<entt::entity> creatures;
	registry.Each<const Creature, const Transform>(
	    [&creatures](entt::entity creature, const Creature&, const Transform&) { creatures.push_back(creature); });
	const auto handHeld =
	    Locator::creatureHandSystem::has_value() ? Locator::creatureHandSystem::value().GetCreature() : std::nullopt;
	const float objectWeight = Locator::dynamicsSystem::has_value() ? Locator::dynamicsSystem::value().WeightOf(object) : 0.0f;
	for (const auto creature : creatures)
	{
		// Not one the hand is holding, one fighting, one a script controls, nor one already catching. (A creature under a
		// script's only desire must also be free to react; openblack's scripts set no only desire.)
		const auto* acting = registry.TryGet<const CreatureObjectAction>(creature);
		if (handHeld == creature || registry.AllOf<CreatureFighting>(creature) || registry.AllOf<ScriptControlled>(creature) ||
		    (acting != nullptr && acting->kind == creature_object_actions::Kind::Catch &&
		     acting->status != creature_object_actions::Status::Done &&
		     acting->status != creature_object_actions::Status::Failed))
		{
			continue;
		}
		const auto& body = registry.Get<const Creature>(creature);
		const auto& transform = registry.Get<const Transform>(creature);
		const auto* morph = registry.TryGet<const CreatureMorph>(creature);
		const float weight = living::CreatureMass(body.size, morph != nullptr ? morph->drawn.thinFat : 0.0f,
		                                          morph != nullptr ? morph->drawn.weakStrong : 0.0f);
		// The thing's own weight, not kept from nothing as its body's mass is
		if (!(objectWeight < creature_catch::k_MostWeightShare * weight) || !actions.CanPickUp(object))
		{
			continue;
		}
		// Not its own throw; another player's (none are allied in openblack) only now and then
		if (entry.thrower == creature)
		{
			continue;
		}
		if (entry.player.has_value() && *entry.player != body.owner &&
		    Locator::gameRandom::value().GameRand(100) > creature_catch::k_OtherPlayersChance)
		{
			continue;
		}
		const auto* points = RigPointsOf(body);
		const auto stepMs = animations.AnimationDuration(creature, creature_catch::k_CatchStep);
		const auto stepTravel = animations.AnimationTravel(creature, creature_catch::k_CatchStep);
		if (points == nullptr || !stepMs.has_value() || !stepTravel.has_value())
		{
			continue;
		}
		const creature_catch::Approach approach {.thing = entry.body->Centre(),
		                                         .velocity = entry.body->velocity,
		                                         .creature = transform.position,
		                                         .size = body.size,
		                                         .modelScale = transform.scale.x,
		                                         .catchMs = points->catchMs,
		                                         .stepMs = *stepMs,
		                                         .stepTravel = stepTravel->x};
		if (!creature_catch::Reaches(approach))
		{
			continue;
		}
		// It stops where it is, and is made to play by catching the thing: what it was doing fails
		if (Locator::creatureLocomotionSystem::has_value())
		{
			Locator::creatureLocomotionSystem::value().Stop(creature);
		}
		if (Locator::creatureMindSystem::has_value())
		{
			Locator::creatureMindSystem::value().ForceCatch(creature, object);
		}
		else
		{
			actions.Catch(creature, object);
		}
	}
}

void PhysicsGameHooks::StartFlyingFromHand([[maybe_unused]] DynamicsSystemInterface& dynamics, PhysicsEntry& entry)
{
	StartFlying(entry.entity);
}

void PhysicsGameHooks::ReactToImpact(DynamicsSystemInterface& dynamics, PhysicsEntry& entry, const ImpactInfo& impact)
{
	auto& registry = Entities();
	const auto object = entry.entity;
	const auto hitter = impact.hitBy;
	if (!registry.Valid(object))
	{
		return;
	}
	if (registry.AllOf<Creature>(object))
	{
		HurtCreature(dynamics, entry, impact);
		return;
	}
	if (const auto* shield = registry.TryGet<const MagicShield>(object);
	    shield != nullptr && shield->kind == MagicShield::Kind::Physical)
	{
		StrikeShield(dynamics, object, impact);
		return;
	}
	// Rocks break buildings, the village centre, storage pits and spell dispensers among them
	if (registry.AnyOf<Abode, StoragePit, SpellDispenser>(object))
	{
		if (Locator::buildingDamageSystem::has_value())
		{
			Locator::buildingDamageSystem::value().ReactToImpact(dynamics, entry, impact);
		}
		return;
	}
	// A thing that is a resource and meets a store of it goes into the store: trees, dead trees and fences as wood,
	// mushrooms and animals as food, a pot or pile as what it holds, which a pile of the same also takes
	if (hitter != entt::null && registry.Valid(hitter) && Locator::resourceStoreSystem::has_value())
	{
		auto& stores = Locator::resourceStoreSystem::value();
		const auto resource = stores.ResourceOf(object);
		// A fence a script made indestructible stays out of stores
		if (resource.type != ResourceType::None && !registry.AllOf<MobileStatic, Indestructible>(object))
		{
			// An animal the store won't take is hurt by the blow as any other
			if (stores.IsStore(hitter, resource.type) &&
			    (stores.TakeObject(hitter, object, impact.player) || !registry.AllOf<Animal>(object)))
			{
				return;
			}
			if (registry.AllOf<Pot>(object))
			{
				if (const auto* other = registry.TryGet<const Pot>(hitter);
				    other != nullptr && stores.ResourceOf(hitter).type == resource.type)
				{
					stores.AddToPile(hitter, resource.type, resource.amount, resource.poisoned);
					world_objects::LeaveGhost(object);
					world_objects::Remove(object);
					return;
				}
			}
		}
	}
	// People and animals are hurt by hard knocks, their own falls included
	if (registry.AnyOf<Villager, Animal>(object))
	{
		HurtLiving(object, impact);
		return;
	}
	// Rocks wear away under hard knocks, and break in two once worn out
	if (object_physics::IsRock(object))
	{
		object_physics::KnockRock(dynamics, object, impact);
	}
}

void PhysicsGameHooks::ImpactFeedback([[maybe_unused]] DynamicsSystemInterface& dynamics, PhysicsEntry& entry, bool hit)
{
	// A person, an animal or a rock the player's hand threw may teach the player's creature to do harm by throwing,
	// whenever it is knocked, and to throw things at what it struck
	auto& registry = Entities();
	if (!entry.Has(PhysicsEntry::k_FromHand) || !entry.player.has_value() || !Locator::creatureMindSystem::has_value() ||
	    !registry.Valid(entry.entity))
	{
		return;
	}
	const bool alive = registry.AnyOf<Villager, Animal, Creature>(entry.entity);
	if ((!alive && !object_physics::IsRock(entry.entity)) || registry.AllOf<Tree>(entry.entity))
	{
		return;
	}
	const auto& at = registry.Get<const Transform>(entry.entity).position;
	Locator::creatureMindSystem::value().PlayerDid(hit ? living::k_DeedDamageByThrowingAt : living::k_DeedDamageByThrowing, at,
	                                               entry.entity, *entry.player);
}

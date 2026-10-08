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

#include <glm/geometric.hpp>

#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Creature/CreatureDesires.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/AtHome.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/LivingPhysics.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/PhysicsClasses.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AnimalSystemInterface.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/Implementations/VillagerPhysics.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/MagicShieldSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/ResourceStoreSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/SpellRules.h"
#include "Physics/Body.h"
#include "Physics/LivingRules.h"

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

/// A villager or animal knocked harder than twice its weight is crushed by the blow, through its defences, put down
/// to the thrower's player
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
	Locator::magicSystem::value().ApplyEffectToObject(living, CrushOf(*crush), impact.player.value_or(PlayerNames::NEUTRAL));
}

/// Whether a thing is a toy, which strikes a creature without hurting it
bool IsToy(entt::entity object)
{
	const auto* mobile = Entities().TryGet<const MobileStatic>(object);
	return mobile != nullptr && Locator::infoConstants::has_value() &&
	       physics_classes::IsToyModel(
	           Locator::infoConstants::value().mobileStatic.at(static_cast<size_t>(mobile->type)).meshId);
}

/// A creature struck by a thrown thing (not a toy) is hurt a hundredth of how hard it was struck for its weight, its
/// own player's blow makes it think less of the player, and every blow makes it angry and afraid
void HurtCreature(entt::entity creature, const ImpactInfo& impact)
{
	auto& registry = Entities();
	if (impact.hitBy == entt::null || !registry.Valid(impact.hitBy) || IsToy(impact.hitBy))
	{
		return;
	}
	// TODO(physics): a blow sways the creature's upper or lower body; openblack's creature keeps no body sway yet. A
	// creature a script controls isn't hurt by its own player's blows; openblack's scripts don't control creatures yet
	const auto& body = registry.Get<const Creature>(creature);
	const auto* morph = registry.TryGet<const CreatureMorph>(creature);
	const float mass = living::CreatureMass(body.size, morph != nullptr ? morph->drawn.thinFat : 0.0f,
	                                        morph != nullptr ? morph->drawn.weakStrong : 0.0f);
	const auto crush = living::CreatureCrush(impact.impact, mass);
	if (!crush.has_value())
	{
		return;
	}
	if (Locator::magicSystem::has_value() && Locator::infoConstants::has_value())
	{
		Locator::magicSystem::value().ApplyEffectToObject(creature, CrushOf(*crush),
		                                                  impact.player.value_or(PlayerNames::NEUTRAL));
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
		const glm::mat3 axes(glm::vec3(std::cos(heading), 0.0f, std::sin(heading)), glm::vec3(0.0f, 1.0f, 0.0f),
		                     glm::vec3(-std::sin(heading), 0.0f, std::cos(heading)));
		transform->rotation = physics::QuarterTurned(axes);
		// Its heading across the land as the animals keep it, from the way it is drawn
		const auto side = transform->rotation[0];
		animal->heading = std::atan2(-side.x, side.z);
	}
	registry.AssignOrReplace<LivingLanding>(entity, LivingLanding {.pose = pose});
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
	if (registry.Valid(animal->flock) && Locator::animalSystem::has_value())
	{
		Locator::animalSystem::value().SetFlockCentre(animal->flock, glm::vec2(position.x, position.z));
	}
	if (const auto clip = living::AnimalLandedClip(animal->type, pose))
	{
		animal->animation = *clip;
		animal->clipPlace = 0;
	}
	animal->state = AnimalState::DecideWhatToDo;
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

bool IsRock(entt::entity object)
{
	const auto* mobile = Entities().TryGet<const MobileStatic>(object);
	return mobile != nullptr && Locator::infoConstants::has_value() &&
	       Locator::infoConstants::value().mobileStatic.at(static_cast<size_t>(mobile->type)).mobileType ==
	           MobileStaticInfo::Rock;
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
	const auto started = PhysicsClassHooks::InitialisePhysics(dynamics, object, start);
	if (started.started)
	{
		StartFlying(object);
	}
	return started;
}

entt::entity PhysicsGameHooks::EndPhysics(DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity object,
                                          bool insert)
{
	const auto kept = PhysicsClassHooks::EndPhysics(dynamics, entry, object, insert);
	auto& registry = Entities();
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
		HurtCreature(object, impact);
		return;
	}
	if (const auto* shield = registry.TryGet<const MagicShield>(object);
	    shield != nullptr && shield->kind == MagicShield::Kind::Physical)
	{
		StrikeShield(dynamics, object, impact);
		return;
	}
	// A thing that is a resource and meets a store of it goes into the store: trees, dead trees and fences as wood,
	// mushrooms and animals as food, a pot or pile as what it holds, which a pile of the same also takes
	if (hitter != entt::null && registry.Valid(hitter) && Locator::resourceStoreSystem::has_value())
	{
		auto& stores = Locator::resourceStoreSystem::value();
		const auto resource = stores.ResourceOf(object);
		if (resource.type != ResourceType::None)
		{
			// TODO(stores): a fence a script made indestructible stays out of stores; openblack has no indestructible
			// flag yet
			if (stores.IsStore(hitter, resource.type))
			{
				stores.TakeObject(hitter, object, impact.player);
				return;
			}
			if (registry.AllOf<Pot>(object))
			{
				if (const auto* other = registry.TryGet<const Pot>(hitter);
				    other != nullptr && stores.ResourceOf(hitter).type == resource.type)
				{
					stores.AddToPile(hitter, resource.type, resource.amount);
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
	if ((!alive && !IsRock(entry.entity)) || registry.AllOf<Tree>(entry.entity))
	{
		return;
	}
	const auto& at = registry.Get<const Transform>(entry.entity).position;
	Locator::creatureMindSystem::value().PlayerDid(hit ? living::k_DeedDamageByThrowingAt : living::k_DeedDamageByThrowing, at,
	                                               entry.entity, *entry.player);
}

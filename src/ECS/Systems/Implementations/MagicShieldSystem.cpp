/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "MagicShieldSystem.h"

#include <cmath>

#include <algorithm>
#include <chrono>
#include <limits>
#include <numbers>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <spdlog/spdlog.h>

#include "3D/AllMeshes.h"
#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Common/GUtilsDistance.h"
#include "Common/GameRandom.h"
#include "ECS/Components/AtHome.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureObjectAction.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Spell.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/TownAggression.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/Systems/TownDesireSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/MagicTables.h"
#include "Magic/ShieldRules.h"
#include "Particles/ParticleEffect.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"
#include "VillagerShieldShelter.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace shield = openblack::magic::shield;

namespace
{
constexpr float k_TurnSeconds = std::chrono::duration<float>(TimeSystemInterface::k_TurnDuration).count();
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// What a thrown thing of no known weight weighs
constexpr float k_DefaultMass = 1.0f;

ecs::Registry& EntityRegistry()
{
	return Locator::entitiesRegistry::value();
}

float LandHeight(glm::vec3 point)
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(glm::xz(point)) : 0.0f;
}

const InfoConstants& Info()
{
	return Locator::infoConstants::value();
}

uint32_t GameTurn()
{
	return Locator::time::has_value() ? static_cast<uint32_t>(Locator::time::value().GetTurn()) : 0;
}

/// What a thrown thing weighs, by its kind's table and its size; whether it is a rock, which the game's physics counts as
/// striking hard enough to cost a dome: the stones and gate totems among the mobile statics
std::pair<float, bool> WeightOf(const ecs::Registry& registry, entt::entity entity)
{
	const auto& info = Info();
	const float scale = registry.AllOf<Transform>(entity) ? registry.Get<const Transform>(entity).scale.x : 1.0f;
	if (const auto* feature = registry.TryGet<const Feature>(entity))
	{
		return {shield::ThrownMass(info.feature.at(static_cast<size_t>(feature->type)).weight, scale), false};
	}
	if (const auto* mobile = registry.TryGet<const MobileStatic>(entity))
	{
		const auto type = mobile->type;
		const bool rock = type == MobileStaticInfo::SingingStone_1 || type == MobileStaticInfo::WeepingStone ||
		                  type == MobileStaticInfo::WeepingStoneReward || type == MobileStaticInfo::GateTotemApe ||
		                  type == MobileStaticInfo::GateTotemBlank || type == MobileStaticInfo::GateTotemCow ||
		                  type == MobileStaticInfo::GateTotemTiger;
		return {shield::ThrownMass(info.mobileStatic.at(static_cast<size_t>(type)).weight, scale), rock};
	}
	if (const auto* mobile = registry.TryGet<const MobileObject>(entity))
	{
		return {shield::ThrownMass(info.mobileObject.at(static_cast<size_t>(mobile->type)).weight, scale), false};
	}
	return {shield::ThrownMass(0.0f, scale), false};
}

/// The town nearest to a point, within the reach a shield protects
entt::entity NearestTown(glm::vec3 point)
{
	auto& registry = EntityRegistry();
	entt::entity nearest = entt::null;
	float best = shield::k_NearestTownDistance;
	registry.Each<const Town, const Transform>([&](entt::entity entity, const Town&, const Transform& transform) {
		const float distance = glm::distance(glm::xz(transform.position), glm::xz(point));
		if (distance < best)
		{
			best = distance;
			nearest = entity;
		}
	});
	return nearest;
}

} // namespace

void MagicShieldSystem::Raise(entt::entity spell, const Spell& miracle)
{
	auto& registry = EntityRegistry();
	const bool physical = miracle.magicType == MagicType::PhysicalShield;
	const auto object = registry.Create();
	auto position = miracle.castPosition;
	position.y = LandHeight(position);
	registry.Assign<Transform>(object, position, glm::mat3(1.0f), glm::vec3(1.0f));
	auto& shieldObject = registry.Assign<MagicShield>(
	    object, MagicShield {.kind = physical ? MagicShield::Kind::Physical : MagicShield::Kind::Spiritual,
	                         .spell = spell,
	                         .player = miracle.caster.player,
	                         .radius = miracle.magnitude,
	                         .town = NearestTown(position),
	                         .reaction = 0,
	                         .struckReaction = 0});
	// No other player has influence under it
	registry.Assign<AntiInfluence>(object, AntiInfluence {.owner = miracle.caster.player, .radius = miracle.magnitude});
	// Its player's people react to it as far as a little beyond its edge
	shieldObject.reaction = CreateReaction(object, Reaction::ReactToMagicShield, position, 1.0f);
	// Spread at once out to its table's reach, it reaches a little beyond the shield's edge from then on
	_reactions.at(shieldObject.reaction).radius = shield::ReactionReach(miracle.magnitude);
	_objects.insert_or_assign(spell, object);

	if (!physical)
	{
		return;
	}
	const auto* info = magic::GetMagicInfoAs<GMagicShieldInfo>(Info(), miracle.magicType);
	if (info == nullptr)
	{
		return;
	}
	auto& dome = registry.Assign<ShieldDome>(object);
	dome.shape = shield::MakeDomeShape(*info, miracle.magnitude, miracle.processInfo.spin);
	dome.mesh = resources::HashIdentifier(MeshId::SpellSolidShield);
	if (Locator::resources::has_value())
	{
		const auto& meshes = Locator::resources::value().GetMeshes();
		if (meshes.Contains(dome.mesh))
		{
			const auto mesh = meshes.Handle(dome.mesh);
			dome.halfExtent = mesh->GetBoundingBox().Size() * 0.5f;
			dome.hull = shield::DomeHull(mesh->GetPhysicsTriangles(), position, dome.shape);
		}
	}
	// It takes its first two turns at once, so it starts as it would have been made
	for (int i = 0; i < 2; ++i)
	{
		dome.pose = shield::StepDome(dome.shape, dome.state, dome.age, k_TurnSeconds);
	}
	dome.previous = dome.pose;
	dome.solidScale = dome.pose.scale;
	auto& transform = registry.Get<Transform>(object);
	transform.position.y = position.y + dome.pose.height;
	transform.scale = glm::vec3(dome.pose.scale);
	// Its sparkling trail and hum, on the dome
	if (Locator::particleSystem::has_value())
	{
		auto& particles = Locator::particleSystem::value();
		dome.effect = particles.Start(ParticleType::PhysicalShieldFx, transform.position, miracle.magnitude, true);
		if (dome.effect != ParticleSystemInterface::k_NoEffect)
		{
			particles.SetPlayer(dome.effect, static_cast<int>(miracle.caster.player));
			particles.AddTarget(dome.effect, object);
		}
	}
}

void MagicShieldSystem::Strip(entt::entity object, MagicShield& shieldObject)
{
	auto& registry = EntityRegistry();
	registry.Remove<AntiInfluence>(object);
	std::vector<uint32_t> ids;
	for (const auto& [id, reaction] : _reactions)
	{
		if (reaction.object == object)
		{
			ids.push_back(id);
		}
	}
	for (const auto id : ids)
	{
		RemoveReaction(id, true);
	}
	if (Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().RemoveFrom(object);
	}
	shieldObject.reaction = 0;
	shieldObject.struckReaction = 0;
}

void MagicShieldSystem::LetGo(entt::entity object, MagicShield& shieldObject)
{
	auto& registry = EntityRegistry();
	_objects.erase(shieldObject.spell);
	shieldObject.spell = entt::null;
	Strip(object, shieldObject);
	if (auto* dome = registry.TryGet<ShieldDome>(object))
	{
		// The dome fades away by itself
		dome->state.dying = true;
		return;
	}
	registry.Destroy(object);
}

bool MagicShieldSystem::StepDome(entt::entity object)
{
	auto& registry = EntityRegistry();
	auto& dome = registry.Get<ShieldDome>(object);
	auto& transform = registry.Get<Transform>(object);
	const auto& shieldObject = registry.Get<const MagicShield>(object);
	dome.age += k_TurnSeconds;
	dome.previous = dome.pose;
	dome.pose = shield::StepDome(dome.shape, dome.state, dome.age, k_TurnSeconds);
	if (dome.pose.gone)
	{
		if (dome.effect != ParticleSystemInterface::k_NoEffect && Locator::particleSystem::has_value())
		{
			Locator::particleSystem::value().Delete(dome.effect);
		}
		return false;
	}
	// The solid shape follows the drawn dome only in steps
	if (shield::NeedsRescale(dome.pose.scale, dome.solidScale))
	{
		dome.solidScale = dome.pose.scale;
	}
	const float strength = shieldObject.spell != entt::null && Locator::magicSystem::has_value()
	                           ? Locator::magicSystem::value().SpellStrength(shieldObject.spell)
	                           : 1.0f;
	if (!dome.state.dying)
	{
		dome.alpha = shield::DomeAlpha(strength);
	}
	transform.position.y = LandHeight(transform.position) + dome.pose.height;
	transform.rotation = glm::eulerAngleY(dome.pose.angle);
	transform.scale = glm::vec3(dome.pose.scale);
	if (dome.effect != ParticleSystemInterface::k_NoEffect && Locator::particleSystem::has_value())
	{
		auto& particles = Locator::particleSystem::value();
		particles.SetOrigin(dome.effect, transform.position);
		if (auto* effect = particles.Find(dome.effect))
		{
			effect->SetMagnitude(shieldObject.radius);
			const float alpha = dome.state.dying ? shield::DyingEffectAlpha(dome.alpha, dome.state.dieTime) : dome.alpha;
			effect->SetGlobalAlpha(alpha);
		}
	}
	return true;
}

void MagicShieldSystem::ProcessTurn()
{
	if (!Locator::infoConstants::has_value() || !Locator::entitiesRegistry::has_value())
	{
		return;
	}
	auto& registry = EntityRegistry();

	// Shield miracles without an object yet raise theirs
	std::vector<std::pair<entt::entity, const Spell*>> raise;
	registry.Each<const Spell>([&](entt::entity entity, const Spell& spell) {
		if (spell.spellClass == magic::SpellClass::Shield && !spell.closedDown && !_objects.contains(entity))
		{
			raise.emplace_back(entity, &spell);
		}
	});
	for (const auto& [entity, spell] : raise)
	{
		Raise(entity, *spell);
	}

	std::vector<entt::entity> letGo;
	std::vector<entt::entity> gone;
	registry.Each<MagicShield>([&](entt::entity object, MagicShield& shieldObject) {
		if (shieldObject.spell != entt::null)
		{
			const auto* spell = registry.Valid(shieldObject.spell) ? registry.TryGet<const Spell>(shieldObject.spell) : nullptr;
			if (spell == nullptr || spell->closedDown)
			{
				letGo.push_back(object);
			}
		}
		// The struck reaction is let go of once it has gone
		if (shieldObject.struckReaction != 0 && !_reactions.contains(shieldObject.struckReaction))
		{
			shieldObject.struckReaction = 0;
		}
	});
	for (const auto object : letGo)
	{
		LetGo(object, registry.Get<MagicShield>(object));
	}
	registry.Each<const ShieldDome>([&](entt::entity object, const ShieldDome&) {
		if (!StepDome(object))
		{
			gone.push_back(object);
		}
	});
	for (const auto object : gone)
	{
		auto& shieldObject = registry.Get<MagicShield>(object);
		if (shieldObject.spell != entt::null)
		{
			_objects.erase(shieldObject.spell);
		}
		Strip(object, shieldObject);
		registry.Destroy(object);
	}
	ProcessReactions();
	ProcessVillagers();
	registry.SetDirty();
}

void MagicShieldSystem::Update(float seconds)
{
	if (!Locator::entitiesRegistry::has_value() || !Locator::infoConstants::has_value() || seconds <= 0.0f)
	{
		return;
	}
	auto& registry = EntityRegistry();
	// What flies against a dome's solid shape, from its raising until it is gone, bounces off it; the blow may cost it.
	// The game's physics pushes a thing out by springs over the steps of each turn, and that physics isn't here yet: until
	// it is, a move into the face is turned back out of it.
	std::vector<std::tuple<entt::entity, entt::entity, float>> blows;
	registry.Each<const ShieldDome, const MagicShield>([&](entt::entity object, const ShieldDome& dome, const MagicShield&) {
		if (dome.hull.empty())
		{
			return;
		}
		registry.Each<Thrown, Transform>([&](entt::entity thrown, Thrown& flight, Transform& at) {
			const auto from = at.position - (flight.velocity * seconds);
			const auto hit = shield::CrossHull(dome.hull, from, at.position);
			if (!hit.has_value())
			{
				return;
			}
			flight.velocity -= hit->normal * (2.0f * glm::dot(flight.velocity, hit->normal));
			at.position = hit->point + (hit->normal * 0.01f);
			blows.emplace_back(object, thrown, glm::length(flight.velocity));
		});
	});
	for (const auto& [object, thrown, speed] : blows)
	{
		Impact(object, thrown, speed);
	}
	if (!blows.empty())
	{
		registry.SetDirty();
	}
}

void MagicShieldSystem::Impact(entt::entity object, entt::entity thrown, float speed)
{
	auto& registry = EntityRegistry();
	auto& shieldObject = registry.Get<MagicShield>(object);
	if (shieldObject.spell == entt::null || !Locator::magicSystem::has_value())
	{
		return;
	}
	auto& magic = Locator::magicSystem::value();
	if (!(magic.SpellStrength(shieldObject.spell) > 0.0f))
	{
		return;
	}
	// Only a rock strikes hard enough to count; anything else just bounces off
	const auto [mass, rock] = WeightOf(registry, thrown);
	if (!rock)
	{
		return;
	}
	const auto& at = registry.Get<const Transform>(thrown);
	const auto* flight = registry.TryGet<const Thrown>(thrown);
	// The blow moves the alignment of the one who threw it, the shield's effect taken at the rock's place on the map
	// rather than by the rock, and costs the shield by its momentum
	magic.SendSpellEvent(shieldObject.spell,
	                     {.type = particles::SpellEventInfo::Type::Object,
	                      .position = {map_coords::Quantise(at.position.x), at.position.y, map_coords::Quantise(at.position.z)},
	                      .velocity = glm::vec3(0.0f),
	                      .strength = 1.0f,
	                      .checkShields = false,
	                      .target = entt::null});
	const auto* info = magic::GetMagicInfoAs<GMagicShieldInfo>(Info(), MagicType::PhysicalShield);
	const float perMomentum = info != nullptr ? info->chantCostPerImpactMomentum : 0.0f;
	const float cost = shield::ImpactCost(perMomentum, speed, mass);
	const float strength = magic.ForcePayForSpell(shieldObject.spell, cost);
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Magic: a dome was struck at speed {:.1f}, costing it {:.1f}, strength {:.2f}",
	                    speed, cost, strength);
	// The town it protects counts the thrower's player as having attacked it, with no harm done
	if (shieldObject.town != entt::null && registry.Valid(shieldObject.town))
	{
		auto aggressor = PlayerNames::NEUTRAL;
		if (flight != nullptr && flight->thrower != entt::null && registry.Valid(flight->thrower))
		{
			if (const auto* creature = registry.TryGet<const Creature>(flight->thrower))
			{
				aggressor = creature->owner;
			}
		}
		auto* aggression = registry.TryGet<TownAggression>(shieldObject.town);
		if (aggression == nullptr)
		{
			aggression = &registry.Assign<TownAggression>(shieldObject.town);
		}
		const bool owner = registry.Get<const Town>(shieldObject.town).owner == aggressor;
		ecs::town_aggression::Attacked(aggression->record, aggressor, owner, 0.0f, Info().town.firstTimeDamageDoneAddition,
		                               GameTurn());
	}
	ShieldStruck(shieldObject.spell, !(strength > 0.0f));
}

void MagicShieldSystem::ShieldStruck(entt::entity spell, bool destroyed)
{
	const auto found = _objects.find(spell);
	if (found == _objects.end())
	{
		return;
	}
	auto& registry = EntityRegistry();
	const auto object = found->second;
	auto& shieldObject = registry.Get<MagicShield>(object);
	const auto position = registry.Get<const Transform>(object).position;
	const bool physical = shieldObject.kind == MagicShield::Kind::Physical;
	if (destroyed)
	{
		// Its people no longer react to it standing, but to its fall
		RemoveReactions(object, Reaction::ReactToMagicShield);
		shieldObject.reaction = 0;
		CreateReaction(object, Reaction::ReactToMagicShieldDestroyed, position,
		               shield::ImpressiveMultiplier(Reaction::ReactToMagicShieldDestroyed, false, physical));
		return;
	}
	if (shieldObject.struckReaction == 0 || !_reactions.contains(shieldObject.struckReaction))
	{
		shieldObject.struckReaction =
		    CreateReaction(object, Reaction::ReactToMagicShieldStruck, position,
		                   shield::ImpressiveMultiplier(Reaction::ReactToMagicShieldStruck, false, physical));
		return;
	}
	// Struck again, its time runs from now
	_reactions.at(shieldObject.struckReaction).firstReacted = GameTurn();
}

bool MagicShieldSystem::HasObject(entt::entity spell) const
{
	return _objects.contains(spell);
}

bool MagicShieldSystem::KeepsReactionOff(glm::vec3 watcher, glm::vec3 initiator) const
{
	auto& registry = EntityRegistry();
	if (!Locator::resources::has_value())
	{
		return false;
	}
	// Every shield's object is drawn from the solid shield's model, the spiritual one at its radius's scale
	const auto meshId = resources::HashIdentifier(MeshId::SpellSolidShield);
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(meshId))
	{
		return false;
	}
	const auto halfExtent = meshes.Handle(meshId)->GetBoundingBox().Size() * 0.5f;
	// What made the reaction is judged where it is now, at its height above the land
	const float initiatorAbove = initiator.y - LandHeight(initiator);
	bool kept = false;
	registry.Each<const MagicShield, const Transform>(
	    [&](entt::entity entity, const MagicShield& shield, const Transform& transform) {
		    if (kept)
		    {
			    return;
		    }
		    const auto* dome = registry.TryGet<const ShieldDome>(entity);
		    const float scale = dome != nullptr ? dome->solidScale : shield.radius * shield::k_DomeScalePerRadius;
		    const auto volume = shield::VolumeOf(halfExtent, scale);
		    const float across = glm::distance(glm::xz(transform.position), glm::xz(watcher));
		    if (!(volume.radius > across))
		    {
			    return;
		    }
		    const glm::vec3 shieldOnLand(transform.position.x, LandHeight(transform.position), transform.position.z);
		    const bool within =
		        dome != nullptr
		            ? shield::WithinPhysicalShield(volume, glm::distance(glm::xz(initiator), glm::xz(transform.position)),
		                                           initiatorAbove)
		            : shield.spell != entt::null && shield::WithinSpiritualShield(shieldOnLand, shield.radius, initiator);
		    kept = !within;
	    });
	return kept;
}

std::optional<entt::entity> MagicShieldSystem::ShieldAt(glm::vec3 point) const
{
	if (!Locator::entitiesRegistry::has_value() || !Locator::resources::has_value())
	{
		return std::nullopt;
	}
	// A shield's reach is its object's, drawn from the solid shield's model, the spiritual one at its radius's scale
	const auto meshId = resources::HashIdentifier(MeshId::SpellSolidShield);
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(meshId))
	{
		return std::nullopt;
	}
	auto& registry = EntityRegistry();
	const auto halfExtent = meshes.Handle(meshId)->GetBoundingBox().Size() * 0.5f;
	std::optional<entt::entity> found;
	registry.Each<const MagicShield, const Transform>(
	    [&](entt::entity entity, const MagicShield& shield, const Transform& transform) {
		    if (found.has_value())
		    {
			    return;
		    }
		    const auto* dome = registry.TryGet<const ShieldDome>(entity);
		    const float scale = dome != nullptr ? dome->solidScale : shield.radius * shield::k_DomeScalePerRadius;
		    if (gutils::GetDistanceInMetres(transform.position, point) < shield::VolumeOf(halfExtent, scale).radius)
		    {
			    found = entity;
		    }
	    });
	return found;
}

std::vector<MagicShieldSystemInterface::Avoid> MagicShieldSystem::CreatureAvoids(PlayerNames creaturePlayer,
                                                                                 bool scripted) const
{
	std::vector<Avoid> avoid;
	if (!Locator::entitiesRegistry::has_value())
	{
		return avoid;
	}
	EntityRegistry().Each<const MagicShield, const Transform>([&](const MagicShield& shieldObject, const Transform& transform) {
		if (shield::CreatureMustAvoid(creaturePlayer, shieldObject.player, scripted))
		{
			avoid.push_back({.centre = glm::xz(transform.position), .radius = shieldObject.radius});
		}
	});
	return avoid;
}

std::vector<MagicShieldSystemInterface::DomeDraw> MagicShieldSystem::GetDomes(float fraction) const
{
	std::vector<DomeDraw> domes;
	if (!Locator::entitiesRegistry::has_value())
	{
		return domes;
	}
	EntityRegistry().Each<const ShieldDome, const Transform>([&](const ShieldDome& dome, const Transform& transform) {
		if (!dome.pose.drawn)
		{
			return;
		}
		const auto& from = dome.previous.drawn ? dome.previous : dome.pose;
		// The angle turns the short way round between turns
		float turn = dome.pose.angle - from.angle;
		if (turn > std::numbers::pi_v<float>)
		{
			turn -= k_TwoPi;
		}
		else if (turn < -std::numbers::pi_v<float>)
		{
			turn += k_TwoPi;
		}
		const float land = LandHeight(transform.position);
		domes.push_back({.mesh = dome.mesh,
		                 .position = {transform.position.x, land + from.height + ((dome.pose.height - from.height) * fraction),
		                              transform.position.z},
		                 .angle = from.angle + (turn * fraction),
		                 .scale = from.scale + ((dome.pose.scale - from.scale) * fraction)});
	});
	return domes;
}

void MagicShieldSystem::Reset()
{
	_objects.clear();
	_reactions.clear();
	_cursor = 0;
}

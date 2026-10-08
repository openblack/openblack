/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FlockMiracle.h"

#include <cmath>

#include <chrono>

#include "Common/GUtilsAngle.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/FlockSpell.h"
#include "ECS/Components/Spell.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/AnimalSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "FlockMiracleRules.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "MagicTables.h"
#include "SpellBehaviours.h"

using namespace openblack;
using namespace openblack::magic;
using openblack::ecs::components::FlockSpell;
using openblack::ecs::components::Spell;
using openblack::particles::SpellEventInfo;

namespace
{
constexpr float k_TurnSeconds = std::chrono::duration<float>(ecs::systems::TimeSystemInterface::k_TurnDuration).count();
/// A new flock's leader roams this far, and its followers keep this close to it
constexpr float k_FlockDomainRadius = 80.0f;

/// The numbers of the flock's tables
struct FlockTables
{
	uint32_t numberToCreate {0};
	float alignmentSwitch {0.0f};
	float distanceToTravel {0.0f};
	float huntingRadius {0.0f};
	bool ground {false};
};

FlockTables TablesOf(const InfoConstants& info, MagicType type)
{
	if (const auto* flying = GetMagicInfoAs<GMagicFlockFlyingInfo>(info, type))
	{
		return {flying->numberToCreate, flying->alignmentSwitch, flying->distanceToTravel, 0.0f, false};
	}
	if (const auto* ground = GetMagicInfoAs<GMagicFlockGroundInfo>(info, type))
	{
		return {ground->numberToCreate, ground->alignmentSwitch, ground->distanceToTravel, ground->huntingRadius, true};
	}
	return {};
}

float AlignmentOf(PlayerNames player)
{
	return Locator::alignmentSystem::has_value() ? Locator::alignmentSystem::value().GetPlayerAlignment(player) : 0.0f;
}

ecs::Registry& EntityRegistry()
{
	return Locator::entitiesRegistry::value();
}

FlockSpell* DataOf(const Spell& spell)
{
	auto& registry = EntityRegistry();
	return registry.TryGet<FlockSpell>(registry.ToEntity(spell));
}

glm::vec2 Xz(const glm::vec3& point)
{
	return {point.x, point.z};
}
} // namespace

ParticleType FlockMiracle::ParticleTypeOf(const Spell& spell) const
{
	if (spell.spellClass == SpellClass::FlockGround)
	{
		return ParticleType::FlockGroundDust;
	}
	const auto tables = TablesOf(Locator::infoConstants::value(), spell.magicType);
	return flock::IsEvil(AlignmentOf(spell.caster.player), tables.alignmentSwitch) ? ParticleType::FlockFlyingRainEvil
	                                                                               : ParticleType::FlockFlyingRainGood;
}

void FlockMiracle::Start(SpellServicesInterface& services, Spell& spell)
{
	auto& registry = EntityRegistry();
	const auto entity = registry.ToEntity(spell);
	const auto tables = TablesOf(services.Info(), spell.magicType);
	auto& data = registry.AssignOrReplace<FlockSpell>(entity);
	// The sweep starts where the hand is
	const auto hand = spell.processInfo.handPosition;
	data.lastSpawn = Xz(hand);
	data.lastSpawnHeight = hand.y - services.World().LandHeight(Xz(hand));
	data.evil = !tables.ground && flock::IsEvil(AlignmentOf(spell.caster.player), tables.alignmentSwitch);
	if (Locator::animalSystem::has_value())
	{
		// The flock is made at the cast point; its domain and how far its followers range are the same
		data.flock =
		    Locator::animalSystem::value().CreateFlock(Xz(spell.castPosition), k_FlockDomainRadius, k_FlockDomainRadius);
	}
}

bool FlockMiracle::AnimalsLeft(const Spell& spell) const
{
	const auto* data = DataOf(spell);
	return data != nullptr && Locator::animalSystem::has_value() &&
	       !Locator::animalSystem::value().MembersOf(data->flock).empty();
}

bool FlockMiracle::FollowsHand(SpellServicesInterface& services, const Spell& spell) const
{
	const auto* data = DataOf(spell);
	if (data == nullptr || spell.closedDown || !spell.humanCasting || !spell.fromLocalHand)
	{
		return false;
	}
	const auto tables = TablesOf(services.Info(), spell.magicType);
	return data->created < flock::NumberToCreate(tables.numberToCreate, services.TribalPower(spell));
}

void FlockMiracle::ProcessTurn(SpellServicesInterface& services, Spell& spell)
{
	auto* data = DataOf(spell);
	if (data == nullptr || !Locator::animalSystem::has_value())
	{
		return;
	}
	auto& animals = Locator::animalSystem::value();
	auto& registry = EntityRegistry();
	// The miracle is where its leader is
	if (const auto leader = animals.LeaderOf(data->flock); leader != entt::null)
	{
		spell.position = registry.Get<const ecs::components::Transform>(leader).position;
	}
	// The animals are made along the sweep until they all are, even once the miracle is over
	Emit(services, spell);
	if (!spell.closedDown)
	{
		CheckShields(services, spell);
	}
	else if (!data->closedDown)
	{
		// Over: every animal there fades out
		data->closedDown = true;
		for (const auto animal : animals.MembersOf(data->flock))
		{
			animals.StartFading(animal);
		}
	}
	// The hand's trail follows the hand while the animals are being made, and closes once they all are
	if (data->castEffect != 0 && Locator::particleSystem::has_value())
	{
		auto& particles = Locator::particleSystem::value();
		if (auto* effect = particles.Find(data->castEffect))
		{
			effect->SetProcessInfo(spell.processInfo);
		}
		if (!FollowsHand(services, spell))
		{
			particles.CloseDown(data->castEffect);
			data->castEffect = 0;
		}
	}
	// The leader wanders about wherever it is now
	if (const auto leader = animals.LeaderOf(data->flock); leader != entt::null)
	{
		animals.SetFlockCentre(data->flock, Xz(registry.Get<const ecs::components::Transform>(leader).position));
	}
}

void FlockMiracle::Emit(SpellServicesInterface& services, Spell& spell)
{
	auto& registry = EntityRegistry();
	const auto entity = registry.ToEntity(spell);
	auto& data = registry.Get<FlockSpell>(entity);
	auto& animals = Locator::animalSystem::value();
	auto& world = services.World();
	const auto& info = services.Info();
	const auto tables = TablesOf(info, spell.magicType);
	const int number = flock::NumberToCreate(tables.numberToCreate, services.TribalPower(spell));

	// This computer's caster's hand leaves a trail of sparkles or smoke while the doves or bats are made
	if (!data.castEffectStarted && !tables.ground && spell.fromLocalHand && Locator::particleSystem::has_value())
	{
		data.castEffectStarted = true;
		auto& particles = Locator::particleSystem::value();
		const auto trail = data.evil ? ParticleType::FlockFlyingCastEvil : ParticleType::FlockFlyingCastGood;
		data.castEffect = particles.Start(trail, spell.processInfo.handPosition, 1.0f);
		particles.SetPlayer(data.castEffect, static_cast<int>(spell.caster.player));
		if (auto* effect = particles.Find(data.castEffect))
		{
			effect->SetProcessInfo(spell.processInfo);
		}
	}
	if (data.created >= number)
	{
		return;
	}

	// The turn's piece of the sweep, from where the hand was to where it is
	const float before = data.emitted;
	data.emitted = flock::EmitAfterTurn(data.emitted, k_TurnSeconds);
	const auto from = data.lastSpawn;
	const float fromHeight = data.lastSpawnHeight;
	const auto hand = spell.processInfo.handPosition;
	data.lastSpawn = Xz(hand);
	data.lastSpawnHeight = hand.y - world.LandHeight(Xz(hand));
	const auto onMap = [&world](glm::vec2 point) { return world.InBounds({point.x, 0.0f, point.y}); };
	const bool ignoreInfluence = Locator::magicSystem::has_value() && Locator::magicSystem::value().IsIgnoringInfluence();

	while (static_cast<float>(data.created) < data.emitted)
	{
		++data.created;
		// The species by the caster's alignment now, for each animal
		const auto type = tables.ground                                                             ? AnimalInfo::SpellWolf
		                  : flock::IsEvil(AlignmentOf(spell.caster.player), tables.alignmentSwitch) ? AnimalInfo::SpellBat
		                                                                                            : AnimalInfo::SpellDove;
		const float f = flock::SpawnFraction(data.created, before, data.emitted);
		const auto spawn = flock::SpawnPoint(from, data.lastSpawn, f);
		const float height = tables.ground ? 0.0f : fromHeight + ((data.lastSpawnHeight - fromHeight) * f);
		const float jitterX = services.GameRandom(2.0f * flock::k_SpawnJitter);
		const float jitterZ = services.GameRandom(2.0f * flock::k_SpawnJitter);
		const auto created = flock::Jitter(spawn, jitterX, jitterZ);
		// Off the map, or outside a human caster's influence, nothing appears
		if (!onMap(created))
		{
			continue;
		}
		if (spell.humanCasting && !ignoreInfluence && !world.InInfluence(spell.caster.player, spell.position))
		{
			continue;
		}
		const auto direction = flock::Direction(spell.humanCasting, spell.processInfo.cameraForward, spell.castPosition,
		                                        spell.processInfo.handPosition);
		const float side = flock::Side(spell.humanCasting, direction, spawn, Xz(spell.castPosition), data.created);
		const auto heading = flock::Rotate(direction, flock::FanAngle(data.created, side, number));
		const auto target = flock::Destination(spawn, heading, tables.distanceToTravel, onMap);
		if (!target.has_value())
		{
			continue;
		}
		const bool wolf = type == AnimalInfo::SpellWolf;
		const auto& animalInfo = info.animal.at(static_cast<size_t>(type));
		const float altitude = wolf ? 0.0f : animalInfo.altitudeNormal;
		const float maxExtra =
		    wolf ? flock::k_WolfScaleMax - flock::k_WolfScaleMin : flock::k_BirdScaleMax - flock::k_BirdScaleMin;
		// Facing where it is going from where it appears, and then sized by the miracle whatever its size as it was born
		const auto facing = gutils::GetAngleFromXZ(created, *target);
		const auto animal = animals.CreateSpellAnimal(type, created, height, facing, spell.caster.player, data.flock, entity);
		animals.SetScale(animal, flock::SpawnScale(wolf, services.GameRandom(maxExtra)));
		services.AddEffectTarget(spell, animal);
		if (wolf && Locator::particleSystem::has_value())
		{
			// Each wolf appears with a puff
			const glm::vec3 at(created.x, world.LandHeight(created), created.y);
			Locator::particleSystem::value().StartSpotVisual(SpotVisualType::MagicObjectCreated, at, std::nullopt, entt::null);
		}
		const auto leader = animals.LeaderOf(data.flock);
		if (animal == leader)
		{
			// The leader's appearance is what the living about react to, moving as it moves
			const auto& transform = registry.Get<const ecs::components::Transform>(animal);
			spells::ApplyDefaultEffect(services, spell,
			                           {.type = SpellEventInfo::Type::Point,
			                            .position = transform.position,
			                            .velocity = animals.MovementOf(animal),
			                            .strength = 1.0f,
			                            .checkShields = false,
			                            .target = entt::null});
			if (wolf)
			{
				animals.SendWolf(animal, spawn, *target, tables.huntingRadius);
			}
			else
			{
				animals.SendBird(animal, *target, altitude, true);
			}
		}
		else
		{
			// The others go where the leader is heading as they appear: a wolf pack's, its leader's prey if it is
			// hunting by then
			const auto goal = leader != entt::null ? animals.GoalOf(leader) : *target;
			if (wolf)
			{
				animals.SendWolf(animal, spawn, goal, tables.huntingRadius);
			}
			else
			{
				animals.SendBird(animal, goal, leader != entt::null ? animals.GoalHeightOf(leader) : altitude, false);
			}
		}
	}
}

void FlockMiracle::CheckShields(SpellServicesInterface& services, Spell& spell)
{
	auto& registry = EntityRegistry();
	const auto* data = DataOf(spell);
	auto& animals = Locator::animalSystem::value();
	for (const auto animal : animals.MembersOf(data->flock))
	{
		auto* spellAnimal = registry.TryGet<ecs::components::SpellAnimal>(animal);
		if (spellAnimal == nullptr)
		{
			continue;
		}
		// Inside a shield now and not at the last turn, within its own radius; one fading away still strikes it
		const auto position = registry.Get<const ecs::components::Transform>(animal).position;
		const float radius = animals.RadiusOf(animal);
		const auto shield = services.ShieldAt(position, radius);
		const auto before = services.ShieldAt(spellAnimal->previous, radius);
		spellAnimal->previous = position;
		if (!shield.has_value() || before == shield)
		{
			continue;
		}
		// It strikes the shield; held, the animal fades away
		const bool through = spells::ApplyDefaultEffect(services, spell,
		                                                {.type = SpellEventInfo::Type::HitSpell,
		                                                 .position = position,
		                                                 .velocity = glm::vec3(0.0f),
		                                                 .strength = 1.0f,
		                                                 .checkShields = false,
		                                                 .target = *shield});
		services.StrikeShield(*shield, position);
		if (!through)
		{
			animals.StartFading(animal);
		}
	}
}

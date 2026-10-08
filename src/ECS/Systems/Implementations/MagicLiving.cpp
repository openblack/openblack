/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "MagicLiving.h"

#include <cmath>

#include <algorithm>

#include <entt/entity/entity.hpp>

#include "3D/CreatureBody.h"
#include "3D/MapCoords.h"
#include "Creature/CreatureDesires.h"
#include "Creature/CreatureMarks.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureFight.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/Fire.h"
#include "ECS/Components/MiracleImpression.h"
#include "ECS/Components/Poisoned.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureFightSystemInterface.h"
#include "ECS/Systems/CreatureSkinSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/HealTargets.h"
#include "Magic/MapSpiral.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// A villager's health out of this is its life
constexpr float k_VillagerHealthScale = 100.0f;
/// A creature changes its mind about another creature for its miracles no more often than this many turns
constexpr uint32_t k_OpinionTurns = 600;
/// Blocking in a fight, a creature takes this share of a miracle
constexpr float k_BlockedEffectShare = 0.1f;

ecs::Registry& EntityRegistry()
{
	return Locator::entitiesRegistry::value();
}

/// The kinds of effect whose weights against a creature change how nice it finds the caster, those it is given at all
constexpr std::array<magic::EffectKind, 4> k_OpinionKinds {magic::EffectKind::Burn, magic::EffectKind::Crush,
                                                           magic::EffectKind::Hit, magic::EffectKind::Heal};
} // namespace

const GObjectInfo* magic_living::InfoOf(entt::entity entity)
{
	const auto& registry = EntityRegistry();
	if (!Locator::infoConstants::has_value() || !registry.Valid(entity))
	{
		return nullptr;
	}
	const auto& info = Locator::infoConstants::value();
	if (const auto* creature = registry.TryGet<const Creature>(entity))
	{
		const auto row = creature::InfoRow(creature->species);
		return row < info.creature.size() ? &info.creature.at(row) : nullptr;
	}
	if (const auto* villager = registry.TryGet<const Villager>(entity))
	{
		const auto kind = static_cast<size_t>(GVillagerInfo::Find(villager->tribe, villager->number));
		return kind < info.villager.size() ? &info.villager.at(kind) : nullptr;
	}
	if (const auto* animal = registry.TryGet<const Animal>(entity))
	{
		const auto kind = static_cast<size_t>(animal->type);
		return kind < info.animal.size() ? &info.animal.at(kind) : nullptr;
	}
	return nullptr;
}

std::optional<float> magic_living::LifeOf(entt::entity entity)
{
	const auto& registry = EntityRegistry();
	if (!registry.Valid(entity))
	{
		return std::nullopt;
	}
	if (const auto* needs = registry.TryGet<const CreatureNeeds>(entity))
	{
		return needs->needs.life;
	}
	if (const auto* villager = registry.TryGet<const Villager>(entity))
	{
		// Its life is kept finer than its health, so that small hurts add up, unless its health was set since
		const auto* life = registry.TryGet<const ObjectLife>(entity);
		const bool current =
		    life != nullptr && static_cast<uint32_t>(std::ceil(life->life * k_VillagerHealthScale)) == villager->health;
		return current ? life->life : static_cast<float>(villager->health) / k_VillagerHealthScale;
	}
	if (const auto* animal = registry.TryGet<const Animal>(entity))
	{
		return animal->life;
	}
	return std::nullopt;
}

void magic_living::SetLife(entt::entity entity, float life)
{
	auto& registry = EntityRegistry();
	life = std::clamp(life, 0.0f, 1.0f);
	if (auto* needs = registry.TryGet<CreatureNeeds>(entity))
	{
		needs->needs.life = life;
	}
	else if (auto* villager = registry.TryGet<Villager>(entity))
	{
		const float before = ecs::world_objects::LifeOf(entity);
		auto* kept = registry.TryGet<ObjectLife>(entity);
		(kept != nullptr ? *kept : registry.Assign<ObjectLife>(entity)).life = life;
		villager->health = static_cast<uint32_t>(std::ceil(life * k_VillagerHealthScale));
		ecs::world_objects::CountInjury(entity, before, life);
	}
	else if (auto* animal = registry.TryGet<Animal>(entity))
	{
		animal->life = life;
	}
}

bool magic_living::IsDead(entt::entity entity)
{
	const auto& registry = EntityRegistry();
	if (registry.AnyOf<Creature>(entity))
	{
		return false;
	}
	const auto life = LifeOf(entity);
	return life.has_value() && !(*life > 0.0f);
}

bool magic_living::IsDove(AnimalInfo type)
{
	return type == AnimalInfo::Dove || type == AnimalInfo::CitadelDove || type == AnimalInfo::SpellDove;
}

bool magic_living::CanBeHealedByHeal(entt::entity entity)
{
	const auto& registry = EntityRegistry();
	if (!registry.Valid(entity))
	{
		return false;
	}
	if (registry.AnyOf<Creature>(entity))
	{
		return true;
	}
	if (const auto* animal = registry.TryGet<const Animal>(entity); animal != nullptr && IsDove(animal->type))
	{
		return false;
	}
	return registry.AnyOf<Villager, Animal>(entity) && !IsDead(entity);
}

std::vector<entt::entity> magic_living::HealTargets(glm::vec3 point, float radius, size_t maximum)
{
	if (!Locator::entitiesMap::has_value())
	{
		return {};
	}
	const auto& registry = EntityRegistry();
	const auto& map = Locator::entitiesMap::value();
	// The things that move in each cell, as the map keeps them: whoever came into the cell last first
	std::vector<magic::HealCandidate> inCell;
	return magic::FindHealTargets(point, radius, maximum, [&](glm::ivec2 cell) -> std::span<const magic::HealCandidate> {
		inCell.clear();
		if (!map_coords::InBounds(cell))
		{
			return {};
		}
		for (const auto entity : map.GetMobileInGridCell(ecs::MapInterface::CellId(cell)))
		{
			if (!registry.Valid(entity))
			{
				continue;
			}
			const auto* transform = registry.TryGet<const Transform>(entity);
			if (transform == nullptr)
			{
				continue;
			}
			inCell.push_back({.entity = entity,
			                  .position = transform->position,
			                  .living = registry.AnyOf<Villager, Animal, Creature>(entity),
			                  .healable = CanBeHealedByHeal(entity)});
		}
		return inCell;
	});
}

magic::EffectDefence magic_living::DefenceOf(entt::entity entity)
{
	const auto* info = InfoOf(entity);
	if (info == nullptr)
	{
		return {};
	}
	const auto defence = magic::EffectDefence::From(*info);
	const auto* creature = EntityRegistry().TryGet<const Creature>(entity);
	return creature != nullptr ? magic::CreatureDefence(defence, creature->size) : defence;
}

namespace
{
/// A heal mends a creature's cuts and scars as though so much time had passed
void HealMarks(entt::entity entity, float heal)
{
	if (heal > 0.0f && Locator::creatureSkinSystem::has_value())
	{
		Locator::creatureSkinSystem::value().Heal(
		    entity, static_cast<uint32_t>(heal * static_cast<float>(creature_marks::k_HealEffectCounts)));
	}
}

/// How nice a creature finds a creature whose miracle reached it (itself too) changes, by how its burn, crush, hit and heal
/// weigh against a creature, no more often than every minute
void ChangeOpinion(entt::entity entity, const magic::EffectValues& values, const magic::EffectSource& source)
{
	auto& registry = EntityRegistry();
	if (source.casterCreature == entt::null || !registry.Valid(source.casterCreature) ||
	    !registry.AllOf<Creature>(source.casterCreature) || !Locator::infoConstants::has_value())
	{
		return;
	}
	const uint32_t turn = Locator::time::has_value() ? Locator::time::value().GetTurn() : 0;
	auto* clock = registry.TryGet<CreatureMiracleOpinion>(entity);
	if (clock == nullptr)
	{
		clock = &registry.Assign<CreatureMiracleOpinion>(entity);
	}
	if (clock->changed && turn - clock->lastTurn <= k_OpinionTurns)
	{
		return;
	}
	clock->changed = true;
	clock->lastTurn = turn;
	const auto& table = Locator::infoConstants::value().alignment;
	float change = 0.0f;
	for (const auto kind : k_OpinionKinds)
	{
		const auto row = static_cast<size_t>(kind);
		if (row < table.size() && values[kind] > 0.0f)
		{
			change += values[kind] * table.at(row).creature;
		}
	}
	if (auto* mind = registry.TryGet<CreatureMindState>(entity))
	{
		mind->leash.attitudes.push_back({.creature = static_cast<uint32_t>(source.casterCreature), .change = change});
	}
}

/// Hurt by a miracle out of a fight, a creature is frightened and angered by it, unless its own player cast it
void FrightenedAndAngered(entt::entity entity, float damage, const magic::EffectSource& source)
{
	auto& registry = EntityRegistry();
	const auto* creature = registry.TryGet<const Creature>(entity);
	auto* mind = registry.TryGet<CreatureMindState>(entity);
	if (creature == nullptr || mind == nullptr || !mind->desires.has_value() || source.player == creature->owner)
	{
		return;
	}
	const float amount = std::clamp(damage, 0.0f, 1.0f);
	creature_desires::ChangeSource(*mind->desires, creature_desires::sources::k_FearFromDamage, amount);
	creature_desires::ChangeSource(*mind->desires, creature_desires::sources::k_AngerFromDamage, amount);
	// TODO(raffclar): the cut or scar it leaves where it struck needs the point on the body's skin a point in the world
	// falls on
}
} // namespace

bool magic_living::TakesEffectItsOwnWay(entt::entity entity, const magic::EffectValues& values,
                                        const magic::EffectSource& source)
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(entity) || !registry.AnyOf<Creature>(entity))
	{
		return false;
	}
	const auto defence = DefenceOf(entity);
	const float damage = magic::DamageFrom(values, defence);
	const bool self = source.casterCreature == entity;
	// A creature's own miracle never harms it
	if (damage > 0.0f && self)
	{
		return true;
	}
	auto* fighting = registry.TryGet<CreatureFighting>(entity);
	const bool inDuel = fighting != nullptr && fighting->stage == CreatureFighting::Stage::Duel;
	// In a fight a miracle that does no harm, such as a heal, reaches it only from itself or its own player's hand
	const bool ownPlayers = source.casterCreature == entt::null && source.player == registry.Get<const Creature>(entity).owner;
	if (damage <= 0.0f && !self && inDuel && !ownPlayers)
	{
		return true;
	}
	if (!inDuel)
	{
		FrightenedAndAngered(entity, damage, source);
		return false;
	}
	// In a fight it takes the miracle as a blow: blocking, a tenth of it; it reels, a heal gives back the health it fights
	// with, and harm takes it, which can knock it out. Its life is untouched.
	auto taken = values;
	const bool blocking = Locator::creatureFightSystem::has_value() && Locator::creatureFightSystem::value().IsBlocking(entity);
	if (blocking)
	{
		std::ranges::for_each(taken.numbers, [](float& number) { number *= k_BlockedEffectShare; });
	}
	if (Locator::creatureFightSystem::has_value())
	{
		Locator::creatureFightSystem::value().Recoil(entity);
	}
	const float heal = magic::HealFrom(taken, defence);
	if (heal > 0.0f)
	{
		HealMarks(entity, heal);
		fighting->fighter.health = std::clamp(fighting->fighter.health + heal, 0.0f, 1.0f);
	}
	if (damage > 0.0f)
	{
		fighting->fighter.health -= damage;
		if (fighting->fighter.health <= 0.0f && Locator::creatureFightSystem::has_value())
		{
			Locator::creatureFightSystem::value().KnockOut(entity);
		}
	}
	// As out of a fight, the heal mends its marks, and its opinion of a creature that cast it changes
	HealMarks(entity, heal);
	ChangeOpinion(entity, taken, source);
	return true;
}

void magic_living::AfterEffect(entt::entity entity, const magic::EffectValues& values, const magic::EffectSource& source)
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(entity) || !registry.AnyOf<Creature>(entity))
	{
		return;
	}
	HealMarks(entity, magic::HealFrom(values, DefenceOf(entity)));
	ChangeOpinion(entity, values, source);
}

void magic_living::CurePoison(entt::entity entity)
{
	auto& registry = EntityRegistry();
	if (registry.Valid(entity))
	{
		registry.Remove<Poisoned>(entity);
	}
}

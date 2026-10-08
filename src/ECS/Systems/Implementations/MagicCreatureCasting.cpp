/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// Creatures casting miracles. A creature casts at an object as big as the object: a little bigger than its radius on the
// ground, a creature by its height, a fire seed's miracle by the caster's own height. The miracle takes its prayer power
// to start with and the creatures' timer from its tables, and the creature pays what it costs to make with its body; it
// can't cast while too tired to, nor at something the miracle can't be cast at. It is cast at the object's feet, or on
// the object for a seed that casts on objects. Each turn its effect flows from between the creature's hands, towards
// what it was cast at. Letting go of it stops a miracle that lasts only while held, such as a lightning bolt.

#define LOCATOR_IMPLEMENTATIONS

#include <algorithm>

#include <glm/geometric.hpp>

#include "3D/CreatureBody.h"
#include "Creature/CreatureRig.h"
#include "Creature/CreatureSpellCasting.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/Spell.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/MagicTables.h"
#include "Magic/SpellBehaviours.h"
#include "MagicSystem.h"
#include "ObjectMeasures.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
ecs::Registry& Entities()
{
	return Locator::entitiesRegistry::value();
}

/// Where a creature's left and right hands are, as it is posed now; none when it has no hands
std::optional<std::pair<glm::vec3, glm::vec3>> HandBonesOf(entt::entity creature)
{
	const auto& registry = Entities();
	const auto* body = registry.TryGet<const Creature>(creature);
	const auto* transform = registry.TryGet<const Transform>(creature);
	const auto* animation = registry.TryGet<const CreatureAnimation>(creature);
	if (body == nullptr || transform == nullptr || animation == nullptr || animation->boneMatrices.empty())
	{
		return std::nullopt;
	}
	const auto& rigs = Locator::resources::value().GetCreatureRigs();
	const auto rigId = creature::GetRigId(body->species);
	if (!rigs.Contains(rigId) || !rigs.Handle(rigId)->actionPoints.has_value())
	{
		return std::nullopt;
	}
	const auto right = rigs.Handle(rigId)->actionPoints->rightHand;
	const auto left = right < animation->mirror.size() ? animation->mirror[right] : right;
	const auto placement = creature::PlacementMatrix(transform->position, transform->rotation, transform->scale);
	return std::pair {glm::vec3(creature::PosedBone(left, animation->boneMatrices, placement)[3]),
	                  glm::vec3(creature::PosedBone(right, animation->boneMatrices, placement)[3])};
}

/// Where a creature holds its hands: between its right hand and the left, as it is posed now; where it stands when it
/// has no hands to hold
glm::vec3 HandsOf(entt::entity creature)
{
	const auto& registry = Entities();
	const auto* body = registry.TryGet<const Creature>(creature);
	const auto* transform = registry.TryGet<const Transform>(creature);
	const auto* animation = registry.TryGet<const CreatureAnimation>(creature);
	if (body == nullptr || transform == nullptr)
	{
		return glm::vec3(0.0f);
	}
	const auto& rigs = Locator::resources::value().GetCreatureRigs();
	const auto rigId = creature::GetRigId(body->species);
	if (animation == nullptr || animation->boneMatrices.empty() || !rigs.Contains(rigId) ||
	    !rigs.Handle(rigId)->actionPoints.has_value())
	{
		return transform->position;
	}
	const auto right = rigs.Handle(rigId)->actionPoints->rightHand;
	const auto left = right < animation->mirror.size() ? animation->mirror[right] : right;
	const auto placement = creature::PlacementMatrix(transform->position, transform->rotation, transform->scale);
	const auto rightAt = glm::vec3(creature::PosedBone(right, animation->boneMatrices, placement)[3]);
	const auto leftAt = glm::vec3(creature::PosedBone(left, animation->boneMatrices, placement)[3]);
	return (rightAt + leftAt) * 0.5f;
}

/// The creature's body as it pays for miracles, and its species' casting values
std::optional<std::pair<creature_spell_casting::Body, creature_spell_casting::Rates>> CasterBodyOf(entt::entity creature)
{
	const auto& registry = Entities();
	const auto* body = registry.TryGet<const Creature>(creature);
	const auto* needs = registry.TryGet<const CreatureNeeds>(creature);
	const auto& info = Locator::infoConstants::value();
	if (body == nullptr || needs == nullptr)
	{
		return std::nullopt;
	}
	const auto row = creature::InfoRow(body->species);
	if (row >= info.creature.size())
	{
		return std::nullopt;
	}
	const auto& species = info.creature.at(row);
	return std::pair {creature_spell_casting::Body {.size = body->size,
	                                                .strength = body->strength,
	                                                .energy = needs->needs.energy,
	                                                .exhaustion = needs->needs.exhaustion},
	                  creature_spell_casting::Rates {.chantsPerEnergy = species.chantsPerEnergy,
	                                                 .energyFloor = species.spellEnergyFloor,
	                                                 .sizeFactor = species.spellSizeFactor}};
}
} // namespace

namespace
{
/// Casting from above, the creature's effect flows from at least this far above what it is cast at, or this share of its
/// height above it
constexpr float k_CastFromAboveLeast = 10.0f;
constexpr float k_CastFromAboveShare = 1.3f;
/// The beams of a cast from above are this big, and this much bigger for each of the creature's size
constexpr float k_BeamSizeLeast = 0.4f;
constexpr float k_BeamSizePerSize = 0.3f;
} // namespace

bool MagicSystem::CanCreatureCastAt(MagicType type, entt::entity target)
{
	if (magic::ClassOf(type) == magic::SpellClass::Creature)
	{
		return CanCastOn(type, target);
	}
	const auto position = object_measures::PositionOf(Entities(), target);
	return position.has_value() && magic::spells::CanCastAtPointItself(*this, type, *position);
}

bool MagicSystem::CastByCreature(entt::entity creature, MagicType type, entt::entity target)
{
	ReleaseCreatureCast(creature);
	auto& registry = Entities();
	const auto& info = Info();
	const auto* body = registry.TryGet<const Creature>(creature);
	const auto at = object_measures::PositionOf(registry, target);
	if (body == nullptr || !at.has_value() || static_cast<size_t>(type) >= magic::k_MagicTypeCount || type == MagicType::None)
	{
		return false;
	}
	// As big as what it is cast at
	const auto* targetCreature = registry.TryGet<const Creature>(target);
	const auto seed = magic::FindFirstSpellSeedForMagicType(info, type);
	const float magnitude =
	    creature_spell_casting::CastMagnitude(object_measures::TwoDRadius(registry, target),
	                                          targetCreature != nullptr ? std::optional(targetCreature->size) : std::nullopt,
	                                          seed == SpellSeedType::Fire, object_measures::Height(registry, creature));
	const auto& effect = magic::GetMagicEffectInfo(info, type);
	const magic::SpellCastData cast {
	    .magnitude = magnitude,
	    .chants = effect.initialChants,
	    .duration = magic::GetTimerWhenCreatureCasting(info, type),
	    .maxObjectsToCreate = -1,
	};
	const auto paying = CasterBodyOf(creature);
	if (!paying.has_value() || !creature_spell_casting::CanCast(paying->first, paying->second, effect.costToCreate) ||
	    !CanCreatureCastAt(type, target))
	{
		return false;
	}
	const SpellCaster caster {.kind = SpellCaster::Kind::Creature, .player = body->owner, .entity = creature};
	// Its effect flows from between its hands towards what it is cast at; food, wood and water it casts from a point
	// above what it is cast at, which stays there
	const auto hands =
	    magic::IsCreatureCastFromAbove(info, type)
	        ? *at + glm::vec3(0.0f,
	                          std::max(k_CastFromAboveLeast, k_CastFromAboveShare * object_measures::Height(registry, target)),
	                          0.0f)
	        : HandsOf(creature);
	const auto towards = *at - hands;
	const particles::ProcessInfo process {
	    .handPosition = hands,
	    .cameraForward = glm::length(towards) > 0.0f ? glm::normalize(towards) : glm::vec3(0.0f),
	};
	const bool onObject = seed.has_value() && magic::GetSpellSeedInfo(info, *seed).castOnObject != 0;
	const auto spell = onObject ? CastOn(type, caster, target, cast, process) : Cast(type, caster, *at, cast, process);
	// Past its body's and the miracle's checks, the cast counts as made even when no miracle comes of it
	if (spell == entt::null)
	{
		return true;
	}
	_creatureCasts.insert_or_assign(creature, spell);
	// Casting from above, a beam reaches from each of its hands to the point it casts from
	if (magic::IsCreatureCastFromAbove(info, type) && Locator::particleSystem::has_value())
	{
		if (const auto bones = HandBonesOf(creature))
		{
			auto& particles = Locator::particleSystem::value();
			const float size = k_BeamSizeLeast + (k_BeamSizePerSize * body->size);
			auto& beams = _creatureBeams[creature];
			beams = {particles.StartSpotVisual(SpotVisualType::MagicBeam, bones->first, std::nullopt, creature, size),
			         particles.StartSpotVisual(SpotVisualType::MagicBeam, bones->second, std::nullopt, creature, size)};
			for (const auto beam : beams)
			{
				particles.AddTargetPosition(beam, hands);
			}
		}
	}
	// The creature pays for making it with its body
	if (auto* made = FindSpell(spell))
	{
		if (auto* payer = CasterOf(*made))
		{
			payer->MaintainSpell(effect.costToCreate);
		}
	}
	return true;
}

void MagicSystem::ReleaseCreatureCast(entt::entity creature)
{
	// Its beams from casting from above go
	if (const auto beams = _creatureBeams.find(creature); beams != _creatureBeams.end())
	{
		if (Locator::particleSystem::has_value())
		{
			for (const auto beam : beams->second)
			{
				Locator::particleSystem::value().CloseDown(beam);
			}
		}
		_creatureBeams.erase(beams);
	}
	const auto found = _creatureCasts.find(creature);
	if (found == _creatureCasts.end())
	{
		return;
	}
	// One that lasts only while held stops; any other runs its time
	if (const auto* spell = FindSpell(found->second); spell != nullptr && spell->duration < 0.0f)
	{
		CloseDown(found->second);
	}
	_creatureCasts.erase(found);
}

void MagicSystem::UpdateCreatureCast(Spell& spell)
{
	auto& registry = Entities();
	if (!registry.Valid(spell.caster.entity) || magic::IsCreatureCastFromAbove(Info(), spell.magicType))
	{
		return;
	}
	spell.processInfo.handPosition = HandsOf(spell.caster.entity);
	const auto towards = spell.castPosition - spell.processInfo.handPosition;
	spell.processInfo.cameraForward = glm::length(towards) > 0.0f ? glm::normalize(towards) : glm::vec3(0.0f);
}

void MagicSystem::KeepCreatureBeams()
{
	if (!Locator::particleSystem::has_value())
	{
		_creatureBeams.clear();
		return;
	}
	auto& particles = Locator::particleSystem::value();
	// Each turn the beams are moved back onto the creature's hands, until they have gone
	std::erase_if(_creatureBeams, [&](auto& entry) {
		const bool running = std::ranges::any_of(entry.second, [&](auto beam) { return particles.IsRunning(beam); });
		if (!Entities().Valid(entry.first) || !running)
		{
			return true;
		}
		const auto bones = HandBonesOf(entry.first);
		if (!bones.has_value())
		{
			return false;
		}
		particles.SetOrigin(entry.second[0], bones->first);
		particles.SetOrigin(entry.second[1], bones->second);
		return false;
	});
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The creature spells in the game: who they may be cast on, how a creature takes one, and what each does to it turn by
// turn. A creature takes a spell two seconds after it is cast; the spell eases over it, holds, and eases off again,
// putting the creature back as it was unless a script has turned that off.

#define LOCATOR_IMPLEMENTATIONS

#include <algorithm>
#include <array>
#include <chrono>
#include <string>

#include <entt/entity/entity.hpp>
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Camera/Camera.h"
#include "Creature/CreatureAudio.h"
#include "Creature/CreatureSpellCasting.h"
#include "Creature/CreatureSpellMind.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureFight.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/CreatureSpells.h"
#include "ECS/Components/Spell.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/LeashSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/HandMotion.h"
#include "Magic/MagicTables.h"
#include "MagicSystem.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
constexpr float k_TurnSeconds = std::chrono::duration<float>(TimeSystemInterface::k_TurnDuration).count();
constexpr float k_TurnsPerSecond = 1.0f / k_TurnSeconds;
/// The spells' sound bank, which a creature spell's own sound is played from
constexpr std::string_view k_SpellSoundBank = "spells.sad";

/// The hand pours a creature spell onto its creature for this long, rising by this share of the creature's height and
/// tipping by a radian
constexpr float k_GrainSeconds = 4.0f;
constexpr float k_GrainRaiseShare = 0.4f;
constexpr float k_GrainTilt = 1.0f;
/// A creature of size 1 is about this tall
constexpr float k_HeightOfSizeOne = 15.0f;

ecs::Registry& EntityRegistry()
{
	return Locator::entitiesRegistry::value();
}

/// The moods' cheat is cleared as they wear off: on a creature of no player at once, but on a player's creature only
/// while it is on the learning leash, so that one led on the compassion or aggression leash keeps it until its time is up
void ClearCheat(entt::entity entity, CreatureSpells& component, creature_desires::Desires& desires)
{
	const auto* creature = EntityRegistry().TryGet<const Creature>(entity);
	if (creature != nullptr && creature->owner != PlayerNames::NEUTRAL)
	{
		if (!Locator::leashSystem::has_value() || Locator::leashSystem::value().TypeOf(entity) != LeashType::Rope)
		{
			return;
		}
	}
	if (component.cheat.has_value())
	{
		creature_spell_mind::ClearCheatDominance(desires);
	}
	component.cheat.reset();
}
} // namespace

float CreatureSpellCaster::MaintainSpell(float amount)
{
	auto& registry = EntityRegistry();
	auto* creature = registry.Valid(_creature) ? registry.TryGet<Creature>(_creature) : nullptr;
	auto* needs = creature != nullptr ? registry.TryGet<CreatureNeeds>(_creature) : nullptr;
	if (needs == nullptr || !Locator::infoConstants::has_value())
	{
		return 0.0f;
	}
	const auto& info = Locator::infoConstants::value();
	const auto row = creature::InfoRow(creature->species);
	if (row >= info.creature.size())
	{
		return 0.0f;
	}
	const auto& species = info.creature.at(row);
	const creature_spell_casting::Rates rates {.chantsPerEnergy = species.chantsPerEnergy,
	                                           .energyFloor = species.spellEnergyFloor,
	                                           .sizeFactor = species.spellSizeFactor};
	creature_spell_casting::Body body {.size = creature->size,
	                                   .strength = creature->strength,
	                                   .energy = needs->needs.energy,
	                                   .exhaustion = needs->needs.exhaustion};
	// Each magic type's share of the energy it costs
	const auto type = static_cast<size_t>(_type);
	const float share = type < info.creatureMagicActionKnownAboutEntry.size()
	                        ? info.creatureMagicActionKnownAboutEntry.at(type).field0x5c
	                        : 0.0f;
	const float paid = creature_spell_casting::MaintainSpell(body, rates, amount, share);
	needs->needs.energy = body.energy;
	needs->needs.exhaustion = body.exhaustion;
	return paid;
}

bool MagicSystem::CanCastOn(MagicType type, entt::entity target) const
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(target))
	{
		return false;
	}
	// The creature spells are cast on creatures only, and not on one with too many spells waiting
	if (magic::ClassOf(type) == magic::SpellClass::Creature)
	{
		if (!registry.AllOf<Creature>(target) || !creature_spells::SpellOf(type).has_value())
		{
			return false;
		}
		const auto* spells = registry.TryGet<const CreatureSpells>(target);
		return spells == nullptr || !spells->spells.QueueFull();
	}
	return true;
}

void MagicSystem::ReceiveCreatureSpell(entt::entity creature, entt::entity miracle, Spell& spell)
{
	auto& registry = EntityRegistry();
	const auto which = creature_spells::SpellOf(spell.magicType);
	if (!which.has_value() || !registry.AllOf<Creature>(creature))
	{
		return;
	}
	auto* component = registry.TryGet<CreatureSpells>(creature);
	if (component == nullptr)
	{
		component = &registry.Assign<CreatureSpells>(creature);
	}
	component->spells.startDelayTurns = creature_spells::TurnsOf(creature_spells::k_StartDelaySeconds, k_TurnsPerSecond);
	// The creature holds the spell for the miracle's time, made longer by the caster's tribal power; the miracle itself
	// then runs until the creature lets it go
	const float seconds = spell.duration > 0.0f ? spell.duration * TribalPower(spell) : spell.duration;
	const auto result =
	    creature_spells::Receive(component->spells, *which, creature_spells::TurnsOf(seconds, k_TurnsPerSecond), miracle);
	spell.duration = magic::k_NoTimeLimit;
	if (result.replaced != entt::null && result.replaced != miracle)
	{
		CloseDown(result.replaced);
	}
	StartHandGrain(spell, creature);
}

void MagicSystem::StartHandGrain(const Spell& spell, entt::entity creature)
{
	if (!spell.fromLocalHand)
	{
		return;
	}
	const auto* body = EntityRegistry().TryGet<const Creature>(creature);
	const float height = body != nullptr ? body->size * k_HeightOfSizeOne : k_HeightOfSizeOne;
	magic::StartPour(_pour, {.totalTime = k_GrainSeconds,
	                         .heightToRaise = height * k_GrainRaiseShare,
	                         .angleToRaise = k_GrainTilt,
	                         .loops = false});
}

void MagicSystem::StopHandGrain(const Spell& spell)
{
	// Any miracle the local hand cast stops the hand's pour as it closes down, whatever is pouring
	if (spell.fromLocalHand && _pour.active)
	{
		magic::StopPour(_pour);
	}
}

void MagicSystem::ProcessCreatureSpells()
{
	auto& registry = EntityRegistry();
	std::array<creature_spells::Timing, creature_spells::k_SpellCount> timings {};
	for (size_t i = 0; i < timings.size(); ++i)
	{
		const auto type = static_cast<MagicType>(static_cast<size_t>(MagicType::CreatureSpellFreeze) + i);
		if (const auto* info = magic::GetMagicInfoAs<GMagicCreatureSpellInfo>(Info(), type))
		{
			timings.at(i) = {.startSeconds = info->startTransitionDuration, .finishSeconds = info->finishTransitionDuration};
		}
	}
	std::vector<entt::entity> creatures;
	registry.Each<const CreatureSpells>([&](entt::entity entity, const CreatureSpells&) { creatures.push_back(entity); });
	for (const auto entity : creatures)
	{
		auto& component = registry.Get<CreatureSpells>(entity);
		// A spell whose miracle has gone wears off early
		for (auto& slot : component.spells.slots)
		{
			if (slot.phase != creature_spells::Phase::Off && slot.miracle != entt::null && FindSpell(slot.miracle) == nullptr)
			{
				creature_spells::FinishEarly(slot);
				slot.miracle = entt::null;
			}
		}
		for (auto& waiting : component.spells.waiting)
		{
			if (waiting.miracle != entt::null && FindSpell(waiting.miracle) == nullptr)
			{
				waiting.miracle = entt::null;
			}
		}
		// A desire a spell made dominant lasts its time
		if (component.cheat.has_value())
		{
			auto* mind = registry.TryGet<CreatureMindState>(entity);
			if (mind == nullptr || !mind->desires.has_value() ||
			    !creature_spell_mind::StepCheat(*mind->desires, *component.cheat, k_TurnsPerSecond))
			{
				component.cheat.reset();
			}
		}
		const auto turn = creature_spells::Step(component.spells, timings, k_TurnsPerSecond);
		for (const auto& event : turn.events)
		{
			ApplyCreatureSpell(entity, event);
		}
		for (const auto miracle : turn.ended)
		{
			CloseDown(miracle);
		}
	}
}

void MagicSystem::ApplyCreatureSpell(entt::entity entity, const creature_spells::TurnEvent& event)
{
	using creature_desires::Desire;
	using creature_spells::Event;
	using creature_spells::Spell;
	auto& registry = EntityRegistry();
	auto* creature = registry.TryGet<Creature>(entity);
	if (creature == nullptr)
	{
		return;
	}
	auto& component = registry.Get<CreatureSpells>(entity);
	auto& slot = component.spells[event.spell];
	const auto effect = creature_spells::EffectOf(event.spell);
	// The body value a spell pulls, if it pulls one
	float* value = nullptr;
	switch (event.spell)
	{
	case Spell::Small:
	case Spell::Big:
		value = &creature->size;
		break;
	case Spell::Weak:
	case Spell::Strong:
		value = &creature->strength;
		break;
	case Spell::Fat:
	case Spell::Thin:
		value = &creature->fatness;
		break;
	case Spell::Nice:
	case Spell::Nasty:
		value = &creature->alignment;
		break;
	default:
		break;
	}
	auto* mind = registry.TryGet<CreatureMindState>(entity);
	auto* desires = mind != nullptr && mind->desires.has_value() ? &*mind->desires : nullptr;
	auto* animation = registry.TryGet<CreatureAnimation>(entity);
	const auto desire = effect.desire.has_value() ? std::optional(static_cast<Desire>(*effect.desire)) : std::nullopt;
	auto* minds = Locator::creatureMindSystem::has_value() ? &Locator::creatureMindSystem::value() : nullptr;
	auto* leash = Locator::leashSystem::has_value() ? &Locator::leashSystem::value() : nullptr;
	const bool mood = event.spell == Spell::Nice || event.spell == Spell::Nasty || event.spell == Spell::Itchy;
	switch (event.event)
	{
	case Event::Start:
		if (value != nullptr)
		{
			slot.before = *value;
		}
		// Freezing it, a mood taking it, or making it itch stops what it was doing
		if ((event.spell == Spell::Freeze || mood) && minds != nullptr)
		{
			minds->AbandonAction(entity);
		}
		if (event.spell == Spell::Freeze)
		{
			// Frozen where it stands, its mind still: no plans, no learning
			if (mind != nullptr && !mind->paused)
			{
				mind->paused = true;
				component.pausedMind = true;
			}
			if (Locator::creatureLocomotionSystem::has_value())
			{
				Locator::creatureLocomotionSystem::value().Stop(entity);
			}
			component.freeze = 0.0f;
		}
		if (event.spell == Spell::Invisible)
		{
			component.invisible = true;
			component.fizz = 0.0f;
		}
		if (desire.has_value() && desires != nullptr)
		{
			// It wants this above all else, and most else is held down
			component.cheat = creature_spell_mind::SetCheatDominant(*desires, *desire, true, k_TurnsPerSecond);
		}
		// Nice puts it on the compassion leash, nasty on the aggression leash, if it is on one
		if ((event.spell == Spell::Nice || event.spell == Spell::Nasty) && leash != nullptr && leash->IsLeashed(entity))
		{
			leash->ChangeType(entity, event.spell == Spell::Nice ? LeashType::Good : LeashType::Evil);
		}
		// The spell's own sound starts on the creature as the spell takes hold, from the spells' bank: the growing
		// creature's as it grows, the shrinking one's as it shrinks
		if (effect.soundAction != 0 && Locator::audio::has_value())
		{
			const std::array<int32_t, 5> keys {0, 0, 0, 0, effect.soundAction};
			if (const auto* transform = registry.TryGet<const Transform>(entity))
			{
				const auto played =
				    Locator::audio::value().PlayAnimEffect(std::string(k_SpellSoundBank), keys, entity, transform->position);
				SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Creature spell {} on creature {}: sound {} sample {} outcome {}",
				                    static_cast<int>(event.spell), entt::to_integral(entity), effect.soundAction,
				                    played.sample.value_or(-1), static_cast<int>(played.outcome));
				if (played.emitter != entt::null && played.sample.has_value())
				{
					_creatureSpellSounds.push_back({.emitter = played.emitter, .creature = entity, .sample = *played.sample});
				}
			}
		}
		break;
	case Event::Ease:
		if (value != nullptr)
		{
			float target = slot.before;
			if (event.spell == Spell::Small || event.spell == Spell::Big)
			{
				// In a fight the size spells go by its size now rather than all the way
				const auto inFight = registry.AllOf<CreatureFighting>(entity) ? std::optional(creature->size) : std::nullopt;
				target = creature_spells::SizeTarget(event.spell, slot.before, component.smallestSize, component.largestSize,
				                                     inFight);
			}
			else
			{
				target = creature_spells::Target(event.spell).value_or(slot.before);
			}
			*value = creature_spells::Ease(slot.before, target, event.ratio);
		}
		if (event.spell == Spell::Freeze)
		{
			component.freeze = std::clamp(event.ratio, 0.0f, 1.0f);
			if (animation != nullptr)
			{
				animation->playbackScale = 1.0f - component.freeze;
			}
		}
		if (event.spell == Spell::Invisible)
		{
			component.fizz = creature_spells::k_InvisibleFizz * std::clamp(event.ratio, 0.0f, 1.0f);
		}
		break;
	case Event::Hold:
		// An itchy creature won't be led: the leash comes off, and it wants to scratch over and over
		if (event.spell == Spell::Itchy)
		{
			if (leash != nullptr && leash->IsLeashed(entity))
			{
				leash->TakeOff(entity);
			}
			if (desires != nullptr)
			{
				component.cheat = creature_spell_mind::SetCheatDominant(*desires, Desire::Scratch, true, k_TurnsPerSecond);
			}
		}
		break;
	case Event::BeginFinish:
		break;
	case Event::Finish:
		if (value != nullptr)
		{
			*value = slot.before;
		}
		if (event.spell == Spell::Freeze)
		{
			component.freeze = 0.0f;
			if (animation != nullptr)
			{
				animation->playbackScale = 1.0f;
			}
			if (mind != nullptr && component.pausedMind)
			{
				mind->paused = false;
			}
			component.pausedMind = false;
		}
		if (event.spell == Spell::Invisible)
		{
			component.invisible = false;
			component.fizz = 0.0f;
		}
		if (desire.has_value() && desires != nullptr)
		{
			// What it wanted most it now wants least of all; the moods let the other desires go, the needs don't
			creature_spell_mind::MakeLeastDominant(*desires, *desire);
			if (mood)
			{
				ClearCheat(entity, component, *desires);
			}
		}
		break;
	}
}

void MagicSystem::KeepCreatureSpellSounds()
{
	if (!Locator::audio::has_value() || !Locator::camera::has_value() || !Locator::resources::has_value())
	{
		_creatureSpellSounds.clear();
		return;
	}
	auto& audio = Locator::audio::value();
	auto& registry = EntityRegistry();
	auto& sounds = Locator::resources::value().GetSounds();
	const auto eye = Locator::camera::value().GetOrigin();
	// A creature spell's sound, the itch's going round and round, stops once its creature has gone or the camera is as
	// far from it as the sound carries
	std::erase_if(_creatureSpellSounds, [&](const CreatureSpellSound& playing) {
		if (!audio.EmitterExists(playing.emitter))
		{
			return true;
		}
		const auto* at = registry.Valid(playing.creature) ? registry.TryGet<const Transform>(playing.creature) : nullptr;
		const auto id = entt::hashed_string(fmt::format("{}/{}", k_SpellSoundBank, playing.sample).c_str()).value();
		if (at == nullptr || (sounds.Contains(id) && glm::distance(eye, at->position) >= sounds.Handle(id)->maxDistance))
		{
			audio.StopEmitter(playing.emitter);
			return true;
		}
		return false;
	});
}

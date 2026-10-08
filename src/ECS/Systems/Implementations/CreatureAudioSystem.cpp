/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CreatureAudioSystem.h"

#include <cmath>

#include <algorithm>
#include <string>
#include <vector>

#include <LNDFile.h>
#include <fmt/format.h>
#include <glm/geometric.hpp>
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "3D/LandIslandInterface.h"
#include "3D/TempleInteriorInterface.h"
#include "Audio/AudioManagerInterface.h"
#include "Camera/Camera.h"
#include "Creature/CreatureAudio.h"
#include "Creature/CreatureLayers.h"
#include "Creature/CreatureRig.h"
#include "ECS/Components/AudioEmitter.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureAudio.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/SoundGround.h"
#include "ECS/Systems/CinematicDirectorSystemInterface.h"
#include "ECS/Systems/FootprintSystemInterface.h"
#include "ECS/Systems/SnowSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;
using openblack::audio::AnimEffectPlay;
using openblack::creature::CreatureRig;

namespace
{
/// The local player, whose creature is always heard in its own voice
constexpr PlayerNames k_LocalPlayer = PlayerNames::PLAYER_ONE;

const CreatureRig* RigOf(const Creature& creature)
{
	auto& rigs = Locator::resources::value().GetCreatureRigs();
	const auto id = creature::GetRigId(creature.species);
	return rigs.Contains(id) ? &*rigs.Handle(id) : nullptr;
}

/// Plays an event's sound from the creature and notes it among the creature's last sounds
void Sound(entt::entity entity, const Creature& creature, const Transform& transform, const CreatureRig& rig,
           CreatureAudio& heard, const creature_audio::SoundEvent& event, const std::string& silence)
{
	const auto bank = event.kind == creature_audio::EventKind::Generic
	                      ? std::string(creature_audio::k_GenericBank)
	                      : creature_audio::VoiceBank(rig.soundBankName, creature.species);
	const auto keys = creature_audio::Keys(creature.size, creature.alignment, rig.soundObject,
	                                       creature_audio::SurfaceKey(ecs::sound_ground::At(transform.position)), event.action);
	CreatureAudio::Heard note {
	    .atMs = heard.clockMs,
	    .kind = event.kind,
	    .keys = keys,
	    .bank = bank,
	    .sample = std::nullopt,
	    .played = false,
	    .note = silence,
	};
	if (silence.empty())
	{
		if (event.mode != 0)
		{
			// Stopping a sound or letting its loop run out, which no shipped animation does
			note.note = "stop and release are not supported";
		}
		else if (Locator::audio::has_value())
		{
			const auto result = Locator::audio::value().PlayAnimEffect(bank, keys.ToArray(), entity, transform.position);
			note.sample = result.sample;
			note.played = result.outcome == AnimEffectPlay::Outcome::Played;
			switch (result.outcome)
			{
			case AnimEffectPlay::Outcome::Played:
				break;
			case AnimEffectPlay::Outcome::NoEffect:
				note.note = "no effect for the keys";
				break;
			case AnimEffectPlay::Outcome::NotLoaded:
				note.note = "sample not loaded";
				break;
			case AnimEffectPlay::Outcome::TooFar:
				note.note =
				    fmt::format("too far ({:.0f})", glm::distance(Locator::camera::value().GetOrigin(), transform.position));
				break;
			case AnimEffectPlay::Outcome::AlreadyPlaying:
				note.note = "already playing";
				break;
			}
		}
	}
	[[maybe_unused]] const auto array = keys.ToArray();
	SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Creature {} {} ({}) from {} keys [{} {} {} {} {}]: {}{}",
	                    static_cast<uint32_t>(entity), creature_audio::Name(event.action), static_cast<int32_t>(event.action),
	                    bank, array[0], array[1], array[2], array[3], array[4],
	                    note.sample.has_value() ? fmt::format("sample {}", *note.sample) : std::string("no sample"),
	                    note.played ? std::string(" played") : fmt::format(", silent: {}", note.note));
	heard.recent.push_back(std::move(note));
	while (heard.recent.size() > CreatureAudio::k_RecentCount)
	{
		heard.recent.pop_front();
	}
}

/// The layers of the body as they are posed this frame
creature_audio::Layers LayersOf(const CreatureAnimation& animation)
{
	creature_audio::Layers layers;
	// The animations played by weight stand in for the body's own action
	if (animation.slots.empty() && creature_layers::IsPlaying(animation.body))
	{
		layers.body = creature_audio::Played {creature_layers::CurrentAnimation(animation.body), animation.body.timeMs};
	}
	if (animation.gesture.animation.has_value())
	{
		layers.gesture = creature_audio::Played {*animation.gesture.animation, animation.gesture.timeMs};
	}
	// A face being run back to its start to change makes no sound
	if (animation.face.current.has_value() && animation.face.wanted == animation.face.current)
	{
		layers.face = creature_audio::Played {*animation.face.current, animation.face.timeMs};
	}
	std::vector<float> weights;
	for (const auto& slot : animation.slots)
	{
		layers.slots.push_back({slot.animation, slot.timeMs});
		weights.push_back(slot.weight);
	}
	layers.soundingSlot = creature_audio::SoundingSlot(weights);
	return layers;
}
} // namespace

void CreatureAudioSystem::Update(std::chrono::duration<float, std::milli> gameTime)
{
	auto& registry = Locator::entitiesRegistry::value();

	// Every creature remembers where its animations were
	std::vector<entt::entity> unheard;
	registry.Each<const Creature, const CreatureAnimation>(
	    [&unheard](entt::entity entity, const Creature&, const CreatureAnimation&) { unheard.push_back(entity); },
	    entt::exclude<CreatureAudio>);
	for (const auto entity : unheard)
	{
		registry.Assign<CreatureAudio>(entity);
	}

	const creature_audio::Gate gate {
	    .muted = _muted,
	    .insideTemple = Locator::temple::has_value() && Locator::temple::value().Active(),
	    .wideScreen =
	        Locator::cinematicDirectorSystem::has_value() && Locator::cinematicDirectorSystem::value().IsWideScreenOn(),
	    .localPlayersCreature = false,
	    .otherVoicesEnabled = _otherVoices,
	};

	registry.Each<const Creature, const CreatureAnimation, const Transform, CreatureAudio>(
	    [&](entt::entity entity, const Creature& creature, const CreatureAnimation& animation, const Transform& transform,
	        CreatureAudio& heard) {
		    heard.clockMs += gameTime.count();
		    const auto* rig = RigOf(creature);
		    const auto layers = LayersOf(animation);
		    if (rig == nullptr)
		    {
			    return;
		    }
		    const creature_audio::InfoOf infoOf = [rig](size_t index) -> std::optional<creature_audio::AnimationInfo> {
			    const auto* played = rig->GetAnimation(CreatureRig::Mesh::Base, index);
			    if (played == nullptr)
			    {
				    return std::nullopt;
			    }
			    return creature_audio::AnimationInfo {
			        .events = index < rig->soundEvents.size() ? std::span(rig->soundEvents[index])
			                                                  : std::span<const creature_audio::SoundEvent> {},
			        .durationMs = played->duration,
			        .looping = played->looping,
			    };
		    };

		    // Every layer's events since the last frame, queued in order of when they fall within the frame
		    const auto queue = creature_audio::FrameEvents(heard.last, layers, gameTime.count(), infoOf);

		    auto creatureGate = gate;
		    creatureGate.localPlayersCreature = creature.owner == k_LocalPlayer;
		    for (const auto& fired : queue)
		    {
			    // Hair groups shown and hidden by animations are not drawn differently yet
			    if (fired.event.kind == creature_audio::EventKind::HairGroup)
			    {
				    continue;
			    }
			    // A footstep leaves its print even when it isn't heard
			    if (creature_audio::IsFootstep(fired.event.action) && Locator::footprintSystem::has_value())
			    {
				    Locator::footprintSystem::value().Step(entity);
			    }
			    if (creature_audio::IsHeard(fired.event.kind, creatureGate))
			    {
				    Sound(entity, creature, transform, *rig, heard, fired.event, {});
			    }
			    else if (!creatureGate.muted)
			    {
				    Sound(entity, creature, transform, *rig, heard, fired.event,
				          creatureGate.insideTemple ? "inside the temple"
				          : creatureGate.wideScreen ? "cut scene"
				                                    : "another player's creature's voice");
			    }
		    }
	    });

	// The creatures' sounds follow them about
	registry.Each<AudioEmitter>([&registry](AudioEmitter& emitter) {
		if (emitter.owner == entt::null || !registry.Valid(emitter.owner))
		{
			return;
		}
		if (const auto* transform = registry.TryGet<const Transform>(emitter.owner);
		    transform != nullptr && registry.AnyOf<Creature>(emitter.owner))
		{
			emitter.position = transform->position;
		}
	});
}

void CreatureAudioSystem::Play(entt::entity creature, const creature_audio::SoundEvent& event)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(creature) || !registry.AllOf<Creature, Transform>(creature))
	{
		return;
	}
	const auto& component = registry.Get<const Creature>(creature);
	const auto* rig = RigOf(component);
	if (rig == nullptr || event.kind == creature_audio::EventKind::HairGroup)
	{
		return;
	}
	auto* heard = registry.TryGet<CreatureAudio>(creature);
	if (heard == nullptr)
	{
		heard = &registry.Assign<CreatureAudio>(creature);
	}
	Sound(creature, component, registry.Get<const Transform>(creature), *rig, *heard, event, {});
}

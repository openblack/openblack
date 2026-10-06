/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>

#include <entt/entity/fwd.hpp>

#include "Creature/CreatureAudio.h"

namespace openblack::ecs::systems
{

/// Sounds the creatures' animations as they play past the moments their sounds are placed on: footsteps and blows from
/// the bank all creatures share, roars and cries from the species' own, each played from the creature and following it
/// (see creature_audio and components::CreatureAudio)
class CreatureAudioSystemInterface
{
public:
	virtual ~CreatureAudioSystemInterface() = default;

	/// Once a frame, by the game time, after the bodies are posed
	virtual void Update(std::chrono::duration<float, std::milli> gameTime) = 0;

	/// Silences every creature, for trying things out
	[[nodiscard]] virtual bool IsMuted() const = 0;
	virtual void SetMuted(bool muted) = 0;
	/// Whether the creatures of players other than the local one are heard in their own voices, as a script can ask.
	/// Their footsteps and blows are always heard.
	[[nodiscard]] virtual bool AreOtherVoicesEnabled() const = 0;
	virtual void SetOtherVoicesEnabled(bool enabled) = 0;

	/// Sounds an event on a creature at once, as its animation would, whoever it belongs to and whether or not the
	/// creatures are muted, in a cut scene or the temple. For trying the sounds out.
	virtual void Play(entt::entity creature, const creature_audio::SoundEvent& event) = 0;
};

} // namespace openblack::ecs::systems

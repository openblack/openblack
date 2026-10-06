/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/CreatureAudioSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class CreatureAudioSystem final: public CreatureAudioSystemInterface
{
public:
	void Update(std::chrono::duration<float, std::milli> gameTime) override;
	[[nodiscard]] bool IsMuted() const override { return _muted; }
	void SetMuted(bool muted) override { _muted = muted; }
	[[nodiscard]] bool AreOtherVoicesEnabled() const override { return _otherVoices; }
	void SetOtherVoicesEnabled(bool enabled) override { _otherVoices = enabled; }
	void Play(entt::entity creature, const creature_audio::SoundEvent& event) override;

private:
	bool _muted {false};
	bool _otherVoices {false};
};

} // namespace openblack::ecs::systems

/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Audio/AudioManagerInterface.h"
#include "Window.h"

namespace openblack::debug::gui
{
class Audio final: public Window
{
public:
	Audio() noexcept;

protected:
	void Draw() noexcept override;
	void Update() noexcept override;
	void ProcessEventOpen(const SDL_Event& event) noexcept override;
	void ProcessEventAlways(const SDL_Event& event) noexcept override;

private:
	void Emitters() noexcept;
	void Music() noexcept;
	void AudioSettings() noexcept;
	audio::PlayType _playType {audio::PlayType::Once};
	entt::id_type _selectedSound;
	std::string _selectedSoundPack;
	std::string _selectedMusicPack;
	entt::entity _selectedEmitter;
};
} // namespace openblack::debug::gui

/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Audio/AudioManagerInterface.h"
#include "Game.h"
#include "Window.h"

namespace openblack::debug::gui
{
class Audio: public Window
{
public:
	Audio();

protected:
	void Draw(Game& game) override;
	void Update(Game& game, const Renderer& renderer) override;
	void ProcessEventOpen(const SDL_Event& event) override;
	void ProcessEventAlways(const SDL_Event& event) override;

private:
	void Emitters();
	void Music();
	void AudioSettings();
	PlayType _playType;
	entt::id_type _selectedSound;
	std::string _selectedSoundPack;
	std::string _selectedMusicPack;
	entt::entity _selectedEmitter;
};
} // namespace openblack::debug::gui

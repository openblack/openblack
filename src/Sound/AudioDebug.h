/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "AudioEmitter.h"
#include "Game.h"

namespace openblack::audio
{
class AudioDebug
{
public:
	static void ShowDebugGui(Game& game);
private:
	static void AudioPlayer(Game& game, const std::vector<std::shared_ptr<SoundPack>>& soundPacks);
	static void AudioSettings(Game& game);
	static std::string _selectedPack;
	static AudioEmitterId _selectedEmitter;
};
}
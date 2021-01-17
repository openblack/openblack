/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <SadFile.h>

#include "Sound.h"
#include <string>
#include <vector>

namespace openblack::audio
{

class SoundPack
{
public:
	void LoadFromFile(const std::string& filename);

	[[nodiscard]] const SoundMap& GetSounds() const { return _sounds; }
	std::unique_ptr<Sound>& GetSound(SoundId id) { return _sounds[id]; }
	[[nodiscard]] const std::string& GetName() const { return _name; }
	[[nodiscard]] const std::string& GetFileName() const { return _sadFile.GetFilename(); }
	[[nodiscard]] bool IsMusic() { return _isMusic; }
private:
	bool _isMusic;
	SoundMap _sounds;
	std::string _name;
	sad::SadFile _sadFile;
};

} // namespace openblack::audio

/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <PackFile.h>

#include "Sound.h"

#ifdef HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif // HAS_FILESYSTEM
#include <string>
#include <vector>

namespace openblack::audio
{

class SoundPack
{
public:
	bool LoadFromFile(const fs::path& path);

	[[nodiscard]] const SoundMap& GetSounds() const { return _sounds; }
	std::unique_ptr<Sound>& GetSoundId(std::string name) { return _sounds[_soundIdLookup[name]]; }
	std::unique_ptr<Sound>& GetSound(SoundId id) { return _sounds[id]; }
	[[nodiscard]] const std::string& GetName() const { return _name; }
	[[nodiscard]] const std::string& GetFileName() const { return _filename; }
	[[nodiscard]] bool IsMusic() { return _isMusic; }

private:
	bool _isMusic;
	SoundMap _sounds;
	SoundIdMap _soundIdLookup;
	std::string _filename;
	std::string _name;
};

} // namespace openblack::audio

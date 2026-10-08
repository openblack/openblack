/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SASFile.h"

#include <fstream>
#include <sstream>
#include <string_view>

using namespace openblack::sas;

std::optional<SASFile> openblack::sas::Parse(std::istream& stream)
{
	SASFile file;
	if (!(stream >> file.version))
	{
		return std::nullopt;
	}
	std::string name;
	while (stream >> name && name != "END")
	{
		ClipSounds clip {.clip = name, .soundType = 0, .sounds = {}};
		if (file.version != 0 && !(stream >> clip.soundType))
		{
			break;
		}
		int32_t more = 1;
		while (more != 0)
		{
			FrameSound sound;
			if (!(stream >> sound.time >> sound.action >> sound.mode >> more))
			{
				file.clips.push_back(std::move(clip));
				return file;
			}
			clip.sounds.push_back(sound);
		}
		file.clips.push_back(std::move(clip));
	}
	return file;
}

std::optional<SASFile> openblack::sas::Parse(std::span<const uint8_t> buffer)
{
	std::istringstream stream(std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size()));
	return Parse(stream);
}

std::optional<SASFile> openblack::sas::Open(const std::filesystem::path& path)
{
	std::ifstream stream(path);
	if (!stream)
	{
		return std::nullopt;
	}
	return Parse(stream);
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ClipSounds.h"

using namespace openblack;
using namespace openblack::audio;

clip_sounds::ClipSoundTable::ClipSoundTable(const sas::SASFile& file)
{
	for (const auto& clip : file.clips)
	{
		// A clip named twice keeps the sounds of both, as each record adds to it
		auto& sounds = _byClip[clip.clip];
		sounds.clip = clip.clip;
		sounds.soundType = clip.soundType;
		sounds.sounds.insert(sounds.sounds.end(), clip.sounds.begin(), clip.sounds.end());
	}
}

const sas::ClipSounds* clip_sounds::ClipSoundTable::Find(std::string_view clip) const
{
	const auto found = _byClip.find(std::string(clip));
	return found != _byClip.end() ? &found->second : nullptr;
}

std::vector<size_t> clip_sounds::Passed(std::span<const sas::FrameSound> sounds, uint32_t place, uint32_t played,
                                        uint32_t duration, bool looping)
{
	std::vector<size_t> passed;
	const auto between = [&sounds, &passed](uint32_t from, uint32_t to) {
		for (size_t i = 0; i < sounds.size(); ++i)
		{
			if (sounds[i].time >= 0 && static_cast<uint32_t>(sounds[i].time) >= from &&
			    static_cast<uint32_t>(sounds[i].time) < to)
			{
				passed.push_back(i);
			}
		}
	};
	const uint32_t end = place + played;
	if (end < duration || duration == 0)
	{
		between(place, end);
		return passed;
	}
	between(place, duration);
	between(0, looping ? end % duration : duration);
	return passed;
}

SoundSize clip_sounds::SizeOf(int32_t soundType, bool isVillager, bool child, bool woman)
{
	if (soundType != k_PeopleSounds)
	{
		return SoundSize::Medium;
	}
	if (!isVillager || child)
	{
		return SoundSize::Small;
	}
	return woman ? SoundSize::Medium : SoundSize::Large;
}

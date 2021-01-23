/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace openblack::audio
{

using SoundId = size_t;

SoundId CreateId();

enum class PlayType
{
	None,
	Repeat,
	Once,
	Overlap
};

enum class ChannelLayout
{
	Mono,
	Stereo
};

struct Sound
{
	SoundId id;
	std::string name;
	int priority;
	int sampleRate;
	int bitRate;
	float volume;
	int pitch;
	int pitchDeviation;
	std::vector<uint8_t> bytes;
	ChannelLayout channelLayout;
	PlayType playType;
	bool loaded;
	bool sectorMerged;
	int sectorCount;
	float lengthInSeconds;
};

using SoundMap = std::map<SoundId, std::unique_ptr<Sound>>;
using SoundIdMap = std::map<std::string, SoundId>;
} // namespace openblack::audio
